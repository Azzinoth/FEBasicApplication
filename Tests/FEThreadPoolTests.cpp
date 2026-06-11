#include "FEThreadPoolTests.h"

#include <atomic>
#include <chrono>
#include <cstdlib>
#include <memory>
#include <random>
#include <thread>

using namespace FocalEngine;

void FEThreadPoolTest::SetUp()
{
	DrainThreadPool();
}

void FEThreadPoolTest::TearDown()
{
	DrainThreadPool();
}

void FEThreadPoolTest::DrainThreadPool()
{
	auto Deadline = std::chrono::steady_clock::now() + std::chrono::seconds(5);
	while (THREAD_POOL.IsAnyThreadHaveActiveJob() && std::chrono::steady_clock::now() < Deadline)
	{
		THREAD_POOL.Update();
		std::this_thread::sleep_for(std::chrono::milliseconds(5));
	}
}

TEST_F(FEThreadPoolTest, ExecuteAndCallback_FastJob_FiresCallbackExactlyOnce)
{
	std::atomic<int> JobsExecuted{ 0 };
	std::atomic<int> CallbacksFired{ 0 };

	auto Job = [&JobsExecuted](void*, void*) {
		JobsExecuted.fetch_add(1, std::memory_order_acq_rel);
	};

	auto Callback = [&CallbacksFired](void*) {
		CallbacksFired.fetch_add(1, std::memory_order_acq_rel);
	};

	THREAD_POOL.Execute(Job, nullptr, nullptr, Callback);

	auto Deadline = std::chrono::steady_clock::now() + std::chrono::seconds(5);
	while (CallbacksFired.load(std::memory_order_acquire) == 0 && std::chrono::steady_clock::now() < Deadline)
	{
		THREAD_POOL.Update();
		std::this_thread::sleep_for(std::chrono::milliseconds(5));
	}

	ASSERT_EQ(JobsExecuted.load(), 1);
	ASSERT_EQ(CallbacksFired.load(), 1);
}

TEST_F(FEThreadPoolTest, AssignJobRaceRegression_ManyInstantJobs_AllCallbacksFire)
{
	constexpr int JobCount = 500;
	std::atomic<int> CallbacksFired{ 0 };

	auto Job = [](void*, void*) {
		// Do nothing.
	};

	auto Callback = [&CallbacksFired](void*) {
		CallbacksFired.fetch_add(1, std::memory_order_acq_rel);
	};

	for (int i = 0; i < JobCount; i++)
		THREAD_POOL.Execute(Job, nullptr, nullptr, Callback);
	
	auto Deadline = std::chrono::steady_clock::now() + std::chrono::seconds(15);
	while (CallbacksFired.load(std::memory_order_acquire) < JobCount && std::chrono::steady_clock::now() < Deadline)
	{
		THREAD_POOL.Update();
		std::this_thread::sleep_for(std::chrono::milliseconds(2));
	}

	EXPECT_EQ(CallbacksFired.load(), JobCount);
}

TEST_F(FEThreadPoolTest, BatchedSubmit_AllCallbacksFire)
{
	// If any state leaks between batches in the thread pool, this will hang or undercount.
	constexpr int BatchCount = 100;
	constexpr int JobsPerBatch = 8;
	std::atomic<int> CallbacksFired{ 0 };

	auto Job = [](void*, void*) {
		// Do nothing.
	};

	auto Callback = [&CallbacksFired](void*) {
		CallbacksFired.fetch_add(1, std::memory_order_acq_rel);
	};

	for (int Batch = 0; Batch < BatchCount; Batch++)
	{
		const int CallbacksBefore = CallbacksFired.load();
		for (int j = 0; j < JobsPerBatch; j++)
			THREAD_POOL.Execute(Job, nullptr, nullptr, Callback);

		// Wait for this batch to fully drain.
		auto Deadline = std::chrono::steady_clock::now() + std::chrono::seconds(10);
		while (CallbacksFired.load(std::memory_order_acquire) < CallbacksBefore + JobsPerBatch && std::chrono::steady_clock::now() < Deadline)
		{
			THREAD_POOL.Update();
			std::this_thread::sleep_for(std::chrono::milliseconds(2));
		}

		ASSERT_EQ(CallbacksFired.load(), CallbacksBefore + JobsPerBatch) << "Batch " << Batch << " did not fully drain";
	}

	EXPECT_EQ(CallbacksFired.load(), BatchCount * JobsPerBatch);
}

TEST_F(FEThreadPoolTest, MixedJobDurations_AllCallbacksFire)
{
	constexpr int JobCount = 100;
	std::atomic<int> CallbacksFired{ 0 };

	auto Callback = [&CallbacksFired](void*) {
		CallbacksFired.fetch_add(1, std::memory_order_acq_rel);
	};

	for (int i = 0; i < JobCount; i++)
	{
		// Mix instant, 1ms, and 5ms jobs.
		const int JobDurationMs = (i % 3 == 0) ? 0 : ((i % 3 == 1) ? 1 : 5);
		auto Job = [JobDurationMs](void*, void*) {
			if (JobDurationMs > 0)
				std::this_thread::sleep_for(std::chrono::milliseconds(JobDurationMs));
		};
		THREAD_POOL.Execute(Job, nullptr, nullptr, Callback);
	}

	auto Deadline = std::chrono::steady_clock::now() + std::chrono::seconds(30);
	while (CallbacksFired.load(std::memory_order_acquire) < JobCount && std::chrono::steady_clock::now() < Deadline)
	{
		THREAD_POOL.Update();
		std::this_thread::sleep_for(std::chrono::milliseconds(2));
	}

	EXPECT_EQ(CallbacksFired.load(), JobCount);
}

TEST_F(FEThreadPoolTest, ManyMoreJobsThanThreads_JobsListDrainsCompletely)
{
	const int JobCount = static_cast<int>(THREAD_POOL.GetThreadCount()) * 10 + 7;
	std::atomic<int> CallbacksFired{ 0 };

	auto Job = [](void*, void*) {
		std::this_thread::sleep_for(std::chrono::milliseconds(3));
	};

	auto Callback = [&CallbacksFired](void*) {
		CallbacksFired.fetch_add(1, std::memory_order_acq_rel);
	};

	for (int i = 0; i < JobCount; i++)
		THREAD_POOL.Execute(Job, nullptr, nullptr, Callback);

	auto Deadline = std::chrono::steady_clock::now() + std::chrono::seconds(30);
	while (CallbacksFired.load(std::memory_order_acquire) < JobCount && std::chrono::steady_clock::now() < Deadline)
	{
		THREAD_POOL.Update();
		std::this_thread::sleep_for(std::chrono::milliseconds(2));
	}

	EXPECT_EQ(CallbacksFired.load(), JobCount);
}

// Sanity check that the void* pointers reach the job and the callback unchanged.
TEST_F(FEThreadPoolTest, InputAndOutputPointers_RoundtripCorrectly)
{
	int InputValue = 42;
	int OutputValue = 0;

	std::atomic<bool> bJobReceivedInput{ false };
	std::atomic<bool> bCallbackReceivedOutput{ false };

	auto Job = [&bJobReceivedInput, &InputValue](void* InputData, void* OutputData) {
		if (InputData == &InputValue && *static_cast<int*>(InputData) == 42)
			bJobReceivedInput.store(true, std::memory_order_release);

		// Write a result through OutputData so the callback can verify.
		if (OutputData != nullptr)
			*static_cast<int*>(OutputData) = 1337;
	};

	auto Callback = [&bCallbackReceivedOutput, &OutputValue](void* OutputData) {
		if (OutputData == &OutputValue && *static_cast<int*>(OutputData) == 1337)
			bCallbackReceivedOutput.store(true, std::memory_order_release);
	};

	THREAD_POOL.Execute(Job, &InputValue, &OutputValue, Callback);

	auto Deadline = std::chrono::steady_clock::now() + std::chrono::seconds(5);
	while (!bCallbackReceivedOutput.load(std::memory_order_acquire) && std::chrono::steady_clock::now() < Deadline)
	{
		THREAD_POOL.Update();
		std::this_thread::sleep_for(std::chrono::milliseconds(5));
	}

	EXPECT_TRUE(bJobReceivedInput.load());
	EXPECT_TRUE(bCallbackReceivedOutput.load());
	EXPECT_EQ(OutputValue, 1337);
}

TEST_F(FEThreadPoolTest, CompletedJob_CapturedResources_AreReleasedAfterCollection)
{
	std::shared_ptr<int> JobResource = std::make_shared<int>(1);
	std::shared_ptr<int> CallbackResource = std::make_shared<int>(2);
	std::weak_ptr<int> JobResourceWatcher = JobResource;
	std::weak_ptr<int> CallbackResourceWatcher = CallbackResource;

	std::atomic<bool> bCallbackFired{ false };
	THREAD_POOL.Execute([JobResource](void*, void*) {}, nullptr, nullptr, [CallbackResource, &bCallbackFired](void*) {
		bCallbackFired.store(true, std::memory_order_release);
	});

	JobResource.reset();
	CallbackResource.reset();

	auto Deadline = std::chrono::steady_clock::now() + std::chrono::seconds(5);
	while (!bCallbackFired.load(std::memory_order_acquire) && std::chrono::steady_clock::now() < Deadline)
	{
		THREAD_POOL.Update();
		std::this_thread::sleep_for(std::chrono::milliseconds(5));
	}
	ASSERT_TRUE(bCallbackFired.load());

	EXPECT_TRUE(JobResourceWatcher.expired()) << "The pool still holds the job functor of a completed job";
	EXPECT_TRUE(CallbackResourceWatcher.expired()) << "The pool still holds the callback functor of a completed job";
}

// Each callback synchronously submits the next job via Execute. The chain is many steps deep, so the
// callback->Execute->AssignJob->worker->bJobFinished->collect->callback cycle runs back-to-back rapidly.
TEST_F(FEThreadPoolTest, ChainedCallbackSubmits_DeepChainAllElementsFire)
{
	static constexpr int ChainLength = 200;
	static std::atomic<int> ChainStep;
	static std::atomic<bool> bChainComplete;
	ChainStep.store(0);
	bChainComplete.store(false);

	static std::function<void(void*)> ChainCallback;
	ChainCallback = [](void*) {
		const int Current = ChainStep.fetch_add(1, std::memory_order_acq_rel) + 1;
		if (Current < ChainLength)
		{
			THREAD_POOL.Execute([](void*, void*) {}, nullptr, nullptr, ChainCallback);
		}
		else
		{
			bChainComplete.store(true, std::memory_order_release);
		}
	};

	THREAD_POOL.Execute([](void*, void*) {}, nullptr, nullptr, ChainCallback);

	auto Deadline = std::chrono::steady_clock::now() + std::chrono::seconds(15);
	while (!bChainComplete.load(std::memory_order_acquire) && std::chrono::steady_clock::now() < Deadline)
	{
		THREAD_POOL.Update();
		std::this_thread::sleep_for(std::chrono::milliseconds(2));
	}

	EXPECT_EQ(ChainStep.load(), ChainLength) << "Chain stalled after " << ChainStep.load() << " of " << ChainLength << " steps";
}

TEST_F(FEThreadPoolTest, SubmitFromInsideCallback_NestedJobAlsoFires)
{
	std::atomic<int> OuterCallbacksFired{ 0 };
	std::atomic<int> NestedCallbacksFired{ 0 };

	auto NestedJob = [](void*, void*) {};
	auto NestedCallback = [&NestedCallbacksFired](void*) {
		NestedCallbacksFired.fetch_add(1, std::memory_order_acq_rel);
	};

	auto OuterJob = [](void*, void*) {};
	auto OuterCallback = [&](void*) {
		OuterCallbacksFired.fetch_add(1, std::memory_order_acq_rel);
		THREAD_POOL.Execute(NestedJob, nullptr, nullptr, NestedCallback);
	};

	THREAD_POOL.Execute(OuterJob, nullptr, nullptr, OuterCallback);

	auto Deadline = std::chrono::steady_clock::now() + std::chrono::seconds(10);
	while ((OuterCallbacksFired.load() == 0 || NestedCallbacksFired.load() == 0) && std::chrono::steady_clock::now() < Deadline)
	{
		THREAD_POOL.Update();
		std::this_thread::sleep_for(std::chrono::milliseconds(5));
	}

	EXPECT_EQ(OuterCallbacksFired.load(), 1);
	EXPECT_EQ(NestedCallbacksFired.load(), 1);
}

TEST_F(FEThreadPoolTest, CallbackThatReentersUpdate_FiresExactlyOnce)
{
	std::atomic<int> CallbacksFired{ 0 };

	THREAD_POOL.Execute([](void*, void*) {}, nullptr, nullptr, [&CallbacksFired](void*) {
		const int Count = CallbacksFired.fetch_add(1, std::memory_order_acq_rel) + 1;
		if (Count == 1)
			THREAD_POOL.Update(); // Re-enter the pump exactly once, from inside the callback.
	});

	auto Deadline = std::chrono::steady_clock::now() + std::chrono::seconds(5);
	while (CallbacksFired.load(std::memory_order_acquire) == 0 && std::chrono::steady_clock::now() < Deadline)
	{
		THREAD_POOL.Update();
		std::this_thread::sleep_for(std::chrono::milliseconds(5));
	}

	std::this_thread::sleep_for(std::chrono::milliseconds(50));
	THREAD_POOL.Update();
	std::this_thread::sleep_for(std::chrono::milliseconds(50));

	EXPECT_EQ(CallbacksFired.load(), 1);
}

TEST_F(FEThreadPoolTest, ExecuteFromAnotherThread_IsNotBlockedWhileCallbackRuns)
{
	std::atomic<bool> bCallbackStarted{ false };
	std::atomic<bool> bHelperDone{ false };

	THREAD_POOL.Execute([](void*, void*) {}, nullptr, nullptr, [&](void*) {
		bCallbackStarted.store(true);

		// Wait for the helper's Execute() to go through.
		while (!bHelperDone.load())
			std::this_thread::sleep_for(std::chrono::milliseconds(5));
	});

	std::thread Helper([&]() {
		while (!bCallbackStarted.load())
			std::this_thread::sleep_for(std::chrono::milliseconds(1));

		THREAD_POOL.Execute([](void*, void*) {});
		bHelperDone.store(true);
	});

	auto Deadline = std::chrono::steady_clock::now() + std::chrono::seconds(10);
	while (!bCallbackStarted.load() && std::chrono::steady_clock::now() < Deadline)
	{
		THREAD_POOL.Update();
		std::this_thread::sleep_for(std::chrono::milliseconds(5));
	}

	Helper.join();
	EXPECT_TRUE(bHelperDone.load());
}

// Submit several jobs to the same dedicated thread, wait for the queue to drain, then shut it down.
TEST_F(FEThreadPoolTest, DedicatedThread_ExecuteAndWait_RunsAllJobs)
{
	const std::string ID = THREAD_POOL.CreateDedicatedThread();
	ASSERT_FALSE(ID.empty());

	constexpr int JobCount = 25;
	std::atomic<int> JobsRan{ 0 };
	std::atomic<int> CallbacksFired{ 0 };

	auto Job = [&JobsRan](void*, void*) {
		JobsRan.fetch_add(1, std::memory_order_acq_rel);
		std::this_thread::sleep_for(std::chrono::milliseconds(1));
	};

	auto Callback = [&CallbacksFired](void*) {
		CallbacksFired.fetch_add(1, std::memory_order_acq_rel);
	};

	for (int i = 0; i < JobCount; i++)
		THREAD_POOL.Execute(ID, Job, nullptr, nullptr, Callback);

	EXPECT_TRUE(THREAD_POOL.WaitForDedicatedThread(ID));
	EXPECT_EQ(JobsRan.load(), JobCount);
	EXPECT_EQ(CallbacksFired.load(), JobCount);

	EXPECT_TRUE(THREAD_POOL.ShutdownDedicatedThread(ID));

	// Allow Update() to actually free the dedicated thread.
	auto Deadline = std::chrono::steady_clock::now() + std::chrono::seconds(5);
	while (THREAD_POOL.IsAnyDedicatedThreadHaveActiveJob() && std::chrono::steady_clock::now() < Deadline)
	{
		THREAD_POOL.Update();
		std::this_thread::sleep_for(std::chrono::milliseconds(5));
	}

	for (int i = 0; i < 20; i++)
	{
		THREAD_POOL.Update();
		std::this_thread::sleep_for(std::chrono::milliseconds(5));
	}
}

TEST_F(FEThreadPoolTest, DedicatedThread_InvalidID_DoesNothingGracefully)
{
	EXPECT_FALSE(THREAD_POOL.WaitForDedicatedThread("does-not-exist"));
	EXPECT_FALSE(THREAD_POOL.ShutdownDedicatedThread("does-not-exist"));

	// Submitting to an unknown ID should not crash application.
	bool bRan = false;
	THREAD_POOL.Execute("does-not-exist", [&bRan](void*, void*) {
		bRan = true;
	});

	std::this_thread::sleep_for(std::chrono::milliseconds(50));
	EXPECT_FALSE(bRan);
}

TEST_F(FEThreadPoolTest, JobThatThrows_DoesNotTerminateProcess)
{
	std::atomic<int> CallbacksFired{ 0 };

	THREAD_POOL.Execute([](void*, void*) {
		throw std::runtime_error("job exception");
	}, nullptr, nullptr, [&CallbacksFired](void*) {
		CallbacksFired.fetch_add(1, std::memory_order_acq_rel);
	});

	auto Deadline = std::chrono::steady_clock::now() + std::chrono::seconds(5);
	while (CallbacksFired.load(std::memory_order_acquire) == 0 && std::chrono::steady_clock::now() < Deadline)
	{
		THREAD_POOL.Update();
		std::this_thread::sleep_for(std::chrono::milliseconds(5));
	}

	EXPECT_EQ(CallbacksFired.load(), 1);

	// The pool must still be functional after a job threw.
	std::atomic<int> FollowUpCallbacksFired{ 0 };
	THREAD_POOL.Execute([](void*, void*) {}, nullptr, nullptr, [&FollowUpCallbacksFired](void*) {
		FollowUpCallbacksFired.fetch_add(1, std::memory_order_acq_rel);
	});

	Deadline = std::chrono::steady_clock::now() + std::chrono::seconds(5);
	while (FollowUpCallbacksFired.load(std::memory_order_acquire) == 0 && std::chrono::steady_clock::now() < Deadline)
	{
		THREAD_POOL.Update();
		std::this_thread::sleep_for(std::chrono::milliseconds(5));
	}

	EXPECT_EQ(FollowUpCallbacksFired.load(), 1);
}

TEST_F(FEThreadPoolTest, DedicatedThread_SubmitWhileOlderJobIsQueued_PreservesSubmissionOrder)
{
	const std::string ID = THREAD_POOL.CreateDedicatedThread();
	ASSERT_FALSE(ID.empty());
	std::this_thread::sleep_for(std::chrono::milliseconds(100)); // Let the worker reach its idle state.

	std::mutex OrderMutex;
	std::vector<int> ExecutionOrder;
	auto Record = [&OrderMutex, &ExecutionOrder](int Value) {
		std::lock_guard<std::mutex> Lock(OrderMutex);
		ExecutionOrder.push_back(Value);
	};

	THREAD_POOL.Execute(ID, [&](void*, void*) { Record(1); std::this_thread::sleep_for(std::chrono::milliseconds(50)); }); // Started right away.
	THREAD_POOL.Execute(ID, [&](void*, void*) { Record(2); }); // Queued behind job 1.

	std::this_thread::sleep_for(std::chrono::milliseconds(150)); // Job 1 finished.
	THREAD_POOL.Update(); // Collect job 1; the worker can be idle while job 2 is still queued.

	THREAD_POOL.Execute(ID, [&](void*, void*) { Record(3); }); // Must run after job 2.

	EXPECT_TRUE(THREAD_POOL.WaitForDedicatedThread(ID));
	std::this_thread::sleep_for(std::chrono::milliseconds(50));
	THREAD_POOL.Update();

	{
		std::lock_guard<std::mutex> Lock(OrderMutex);
		ASSERT_EQ(ExecutionOrder.size(), 3);
		EXPECT_EQ(ExecutionOrder[0], 1);
		EXPECT_EQ(ExecutionOrder[1], 2);
		EXPECT_EQ(ExecutionOrder[2], 3);
	}

	EXPECT_TRUE(THREAD_POOL.ShutdownDedicatedThread(ID));
	for (int i = 0; i < 20; i++)
	{
		THREAD_POOL.Update();
		std::this_thread::sleep_for(std::chrono::milliseconds(5));
	}
}

TEST_F(FEThreadPoolTest, LightThread_RunsAndJoins)
{
	const std::string ID = THREAD_POOL.CreateLightThread();
	ASSERT_FALSE(ID.empty());

	std::atomic<int> JobsRan{ 0 };
	ASSERT_TRUE(THREAD_POOL.ExecuteLightThread(ID, [&JobsRan]() {
		std::this_thread::sleep_for(std::chrono::milliseconds(5));
		JobsRan.store(1, std::memory_order_release);
	}));

	EXPECT_TRUE(THREAD_POOL.WaitForLightThread(ID));
	EXPECT_EQ(JobsRan.load(), 1);

	// A second WaitForLightThread on the same ID should return false, rather than crashing or double joining.
	EXPECT_FALSE(THREAD_POOL.WaitForLightThread(ID));

	EXPECT_TRUE(THREAD_POOL.RemoveLightThread(ID));
	// Removed thread should no longer be reachable.
	EXPECT_FALSE(THREAD_POOL.WaitForLightThread(ID));
	EXPECT_FALSE(THREAD_POOL.RemoveLightThread(ID));
}

TEST_F(FEThreadPoolTest, LightThread_InvalidID_ReturnsFalse)
{
	EXPECT_FALSE(THREAD_POOL.WaitForLightThread("does-not-exist"));
	EXPECT_FALSE(THREAD_POOL.RemoveLightThread("does-not-exist"));

	const std::string ID = THREAD_POOL.CreateLightThread();
	ASSERT_FALSE(ID.empty());
	EXPECT_FALSE(THREAD_POOL.WaitForLightThread(ID));
	EXPECT_TRUE(THREAD_POOL.RemoveLightThread(ID));
}

TEST_F(FEThreadPoolTest, ExecuteLightThread_SecondCallBeforeJoin_ReturnsFalse)
{
	const std::string ID = THREAD_POOL.CreateLightThread();
	ASSERT_FALSE(ID.empty());

	std::atomic<int> JobsRan{ 0 };
	ASSERT_TRUE(THREAD_POOL.ExecuteLightThread(ID, [&JobsRan]() {
		std::this_thread::sleep_for(std::chrono::milliseconds(50));
		JobsRan.fetch_add(1, std::memory_order_acq_rel);
	}));

	// While the first function is still running.
	EXPECT_FALSE(THREAD_POOL.ExecuteLightThread(ID, [&JobsRan]() {
		JobsRan.fetch_add(1, std::memory_order_acq_rel);
	}));

	// After the first function finished but before anyone joined it, the handle is still joinable.
	std::this_thread::sleep_for(std::chrono::milliseconds(100));
	EXPECT_FALSE(THREAD_POOL.ExecuteLightThread(ID, [&JobsRan]() {
		JobsRan.fetch_add(1, std::memory_order_acq_rel);
	}));

	EXPECT_TRUE(THREAD_POOL.WaitForLightThread(ID));
	EXPECT_EQ(JobsRan.load(), 1);

	// After a join the same ID is reusable.
	EXPECT_TRUE(THREAD_POOL.ExecuteLightThread(ID, [&JobsRan]() {
		JobsRan.fetch_add(1, std::memory_order_acq_rel);
	}));
	EXPECT_TRUE(THREAD_POOL.WaitForLightThread(ID));
	EXPECT_EQ(JobsRan.load(), 2);

	EXPECT_TRUE(THREAD_POOL.RemoveLightThread(ID));
}

TEST_F(FEThreadPoolTest, WaitForLightThread_LightThreadBodyUsesLightThreadAPI_DoesNotDeadlock)
{
	const std::string ID = THREAD_POOL.CreateLightThread();
	ASSERT_FALSE(ID.empty());

	std::string SpawnedID;
	ASSERT_TRUE(THREAD_POOL.ExecuteLightThread(ID, [&SpawnedID]() {
		std::this_thread::sleep_for(std::chrono::milliseconds(60));
		SpawnedID = THREAD_POOL.CreateLightThread();
	}));

	EXPECT_TRUE(THREAD_POOL.WaitForLightThread(ID));

	EXPECT_TRUE(THREAD_POOL.RemoveLightThread(ID));
	EXPECT_TRUE(THREAD_POOL.RemoveLightThread(SpawnedID));
}

TEST_F(FEThreadPoolTest, SetConcurrentThreadCount_OnlyGrows)
{
	const unsigned int Before = THREAD_POOL.GetThreadCount();
	EXPECT_FALSE(THREAD_POOL.SetConcurrentThreadCount(Before));

	if (Before > 0)
		EXPECT_FALSE(THREAD_POOL.SetConcurrentThreadCount(Before - 1));

	EXPECT_TRUE(THREAD_POOL.SetConcurrentThreadCount(Before + 2));
	EXPECT_EQ(THREAD_POOL.GetThreadCount(), Before + 2);
}

TEST_F(FEThreadPoolTest, SubmitFromInsideJob_WorkerThreadSubmit)
{
	constexpr int OuterCount = 64;
	std::atomic<int> NestedScheduled{ 0 };
	std::atomic<int> NestedCallbacksFired{ 0 };
	std::atomic<int> OuterCallbacksFired{ 0 };

	auto NestedCallback = [&NestedCallbacksFired](void*) {
		NestedCallbacksFired.fetch_add(1, std::memory_order_acq_rel);
	};
	auto NestedJob = [](void*, void*) {};

	auto OuterCallback = [&OuterCallbacksFired](void*) {
		OuterCallbacksFired.fetch_add(1, std::memory_order_acq_rel);
	};
	auto OuterJob = [&NestedScheduled, NestedJob, NestedCallback](void*, void*) {
		// This Execute runs on a worker thread, NOT the main thread.
		THREAD_POOL.Execute(NestedJob, nullptr, nullptr, NestedCallback);
		NestedScheduled.fetch_add(1, std::memory_order_acq_rel);
	};

	for (int i = 0; i < OuterCount; i++)
		THREAD_POOL.Execute(OuterJob, nullptr, nullptr, OuterCallback);

	auto Deadline = std::chrono::steady_clock::now() + std::chrono::seconds(20);
	while ((OuterCallbacksFired.load() < OuterCount || NestedCallbacksFired.load() < OuterCount)
		&& std::chrono::steady_clock::now() < Deadline)
	{
		THREAD_POOL.Update();
		std::this_thread::sleep_for(std::chrono::milliseconds(2));
	}

	EXPECT_EQ(OuterCallbacksFired.load(), OuterCount);
	EXPECT_EQ(NestedScheduled.load(), OuterCount);
	EXPECT_EQ(NestedCallbacksFired.load(), OuterCount) << "Lost nested submissions, likely race on JobsList/Threads from worker thread Execute()";
}

TEST_F(FEThreadPoolTest, NullJob_FiresCallbackExactlyOnce)
{
	std::atomic<int> CallbacksFired{ 0 };
	auto Callback = [&CallbacksFired](void*) {
		CallbacksFired.fetch_add(1, std::memory_order_acq_rel);
	};

	FE_THREAD_JOB_FUNC EmptyJob;
	THREAD_POOL.Execute(EmptyJob, nullptr, nullptr, Callback);

	auto Deadline = std::chrono::steady_clock::now() + std::chrono::seconds(3);
	while (CallbacksFired.load() == 0 && std::chrono::steady_clock::now() < Deadline)
	{
		THREAD_POOL.Update();
		std::this_thread::sleep_for(std::chrono::milliseconds(5));
	}

	EXPECT_EQ(CallbacksFired.load(), 1);
}

// ShutdownDedicatedThread is a graceful shutdown: active jobs and any queued jobs must run to completion and fire their callbacks before the thread is destroyed.
TEST_F(FEThreadPoolTest, ShutdownDedicatedThread_DrainsQueueAndFiresAllCallbacks)
{
	const std::string ID = THREAD_POOL.CreateDedicatedThread();
	ASSERT_FALSE(ID.empty());

	std::atomic<int> CallbacksFired{ 0 };
	auto Callback = [&CallbacksFired](void*) {
		CallbacksFired.fetch_add(1, std::memory_order_acq_rel);
	};

	auto SlowJob = [](void*, void*) {
		std::this_thread::sleep_for(std::chrono::milliseconds(50));
	};

	constexpr int Submitted = 5;
	for (int i = 0; i < Submitted; i++)
		THREAD_POOL.Execute(ID, SlowJob, nullptr, nullptr, Callback);

	// Shut down while one job is active and the rest are still queued.
	EXPECT_TRUE(THREAD_POOL.ShutdownDedicatedThread(ID));

	auto Deadline = std::chrono::steady_clock::now() + std::chrono::seconds(3);
	while (THREAD_POOL.IsAnyDedicatedThreadHaveActiveJob() && std::chrono::steady_clock::now() < Deadline)
	{
		THREAD_POOL.Update();
		std::this_thread::sleep_for(std::chrono::milliseconds(5));
	}

	for (int i = 0; i < 10; i++)
	{
		THREAD_POOL.Update();
		std::this_thread::sleep_for(std::chrono::milliseconds(5));
	}

	EXPECT_EQ(CallbacksFired.load(), Submitted);
}

TEST_F(FEThreadPoolTest, ForceShutdownDedicatedThread_DropsQueueAndReturnsImmediately)
{
	const std::string ID = THREAD_POOL.CreateDedicatedThread();
	ASSERT_FALSE(ID.empty());

	std::atomic<int> CallbacksFired{ 0 };
	std::atomic<int> JobsRan{ 0 };
	auto Callback = [&CallbacksFired](void*) {
		CallbacksFired.fetch_add(1, std::memory_order_acq_rel);
	};

	auto SlowJob = [&JobsRan](void*, void*) {
		JobsRan.fetch_add(1, std::memory_order_acq_rel);
		std::this_thread::sleep_for(std::chrono::milliseconds(50));
	};

	constexpr int Submitted = 5;
	for (int i = 0; i < Submitted; i++)
		THREAD_POOL.Execute(ID, SlowJob, nullptr, nullptr, Callback);

	// Force shutdown should not block on the running job.
	const auto Before = std::chrono::steady_clock::now();
	EXPECT_TRUE(THREAD_POOL.ForceShutdownDedicatedThread(ID));
	const auto ElapsedMs = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - Before).count();
	EXPECT_LT(ElapsedMs, 25) << "ForceShutdownDedicatedThread blocked for " << ElapsedMs << " ms";

	// Pump Update so the worker can exit and the slot can be reaped.
	auto Deadline = std::chrono::steady_clock::now() + std::chrono::seconds(2);
	while (THREAD_POOL.IsAnyDedicatedThreadHaveActiveJob() && std::chrono::steady_clock::now() < Deadline)
	{
		THREAD_POOL.Update();
		std::this_thread::sleep_for(std::chrono::milliseconds(5));
	}

	for (int i = 0; i < 20; i++)
	{
		THREAD_POOL.Update();
		std::this_thread::sleep_for(std::chrono::milliseconds(5));
	}

	// At most one Job ran (the active one if any was assigned before the force-shutdown). Queued ones were dropped before they ever started.
	EXPECT_LE(JobsRan.load(), 1);
	EXPECT_EQ(CallbacksFired.load(), 0) << "Force shutdown must not fire any callback";
	EXPECT_FALSE(THREAD_POOL.IsAnyDedicatedThreadHaveActiveJob());

	// Subsequent Execute on the destroyed ID is a silent no-op, not a crash.
	std::atomic<bool> bLateRan{ false };
	THREAD_POOL.Execute(ID, [&bLateRan](void*, void*) {
		bLateRan.store(true, std::memory_order_release);
	});
	std::this_thread::sleep_for(std::chrono::milliseconds(50));
	EXPECT_FALSE(bLateRan.load());
}

TEST_F(FEThreadPoolTest, ForceShutdownThenWait_DoesNotFireSuppressedCallback)
{
	std::atomic<int> CallbacksFired{ 0 };

	const std::string ID = THREAD_POOL.CreateDedicatedThread();
	ASSERT_FALSE(ID.empty());
	std::this_thread::sleep_for(std::chrono::milliseconds(100));

	THREAD_POOL.Execute(ID, [](void*, void*) {
		std::this_thread::sleep_for(std::chrono::milliseconds(100));
	}, nullptr, nullptr, [&CallbacksFired](void*) {
		CallbacksFired.fetch_add(1, std::memory_order_acq_rel);
	});

	std::this_thread::sleep_for(std::chrono::milliseconds(30)); // The job is assigned and running.
	ASSERT_TRUE(THREAD_POOL.ForceShutdownDedicatedThread(ID));

	EXPECT_FALSE(THREAD_POOL.WaitForDedicatedThread(ID));

	std::this_thread::sleep_for(std::chrono::milliseconds(50));
	for (int i = 0; i < 20; i++)
	{
		THREAD_POOL.Update();
		std::this_thread::sleep_for(std::chrono::milliseconds(5));
	}

	EXPECT_EQ(CallbacksFired.load(), 0);
}

TEST_F(FEThreadPoolTest, DedicatedThread_WaitWithConcurrentUpdatePump_ReturnsOnlyAfterCallbackRan)
{
	const std::string ID = THREAD_POOL.CreateDedicatedThread();
	ASSERT_FALSE(ID.empty());

	std::atomic<bool> bStopPumping{ false };
	std::thread Pumper([&bStopPumping]() {
		while (!bStopPumping.load(std::memory_order_acquire))
			THREAD_POOL.Update();
	});

	constexpr int Iterations = 200;
	std::atomic<bool> bCallbackDone{ false };
	int FailedIteration = -1;
	bool bWaitFailed = false;

	for (int i = 0; i < Iterations; i++)
	{
		bCallbackDone.store(false, std::memory_order_release);
		THREAD_POOL.Execute(ID, [](void*, void*) {}, nullptr, nullptr, [&bCallbackDone](void*) {
			// Widen the window between the collection and the invocation of the callback.
			std::this_thread::sleep_for(std::chrono::milliseconds(1));
			bCallbackDone.store(true, std::memory_order_release);
		});

		const bool bWaitResult = THREAD_POOL.WaitForDedicatedThread(ID);
		if (!bWaitResult || !bCallbackDone.load(std::memory_order_acquire))
		{
			FailedIteration = i;
			bWaitFailed = !bWaitResult;
			break;
		}
	}

	bStopPumping.store(true, std::memory_order_release);
	Pumper.join();

	EXPECT_EQ(FailedIteration, -1) << (bWaitFailed ? "WaitForDedicatedThread returned false" : "WaitForDedicatedThread returned before the callback completed")
		<< " at iteration " << FailedIteration;

	EXPECT_TRUE(THREAD_POOL.ShutdownDedicatedThread(ID));
	for (int i = 0; i < 20; i++)
	{
		THREAD_POOL.Update();
		std::this_thread::sleep_for(std::chrono::milliseconds(5));
	}
}

TEST_F(FEThreadPoolTest, ForceShutdownSignal_ReachesWorkerPromptly)
{
	constexpr int ThreadCount = 64;
	constexpr int ExitDeadlineMs = 500;

	std::vector<std::string> IDs;
	IDs.reserve(ThreadCount);
	for (int i = 0; i < ThreadCount; i++)
	{
		const std::string ID = THREAD_POOL.CreateDedicatedThread();
		ASSERT_FALSE(ID.empty());
		IDs.push_back(ID);
	}

	// Submit one quick job each so each worker is actively running, then force-shutdown.
	// This maximises the chance of the worker being mid-loop when bNeedToExit lands.
	std::atomic<int> JobsStarted{ 0 };
	auto QuickJob = [&JobsStarted](void*, void*) {
		JobsStarted.fetch_add(1, std::memory_order_acq_rel);
	};

	for (const auto& CurrentID : IDs)
		THREAD_POOL.Execute(CurrentID, QuickJob);

	for (const auto& CurrentID : IDs)
		EXPECT_TRUE(THREAD_POOL.ForceShutdownDedicatedThread(CurrentID));

	const auto Deadline = std::chrono::steady_clock::now() + std::chrono::milliseconds(ExitDeadlineMs);
	while (THREAD_POOL.IsAnyDedicatedThreadHaveActiveJob() && std::chrono::steady_clock::now() < Deadline)
	{
		THREAD_POOL.Update();
		std::this_thread::sleep_for(std::chrono::milliseconds(2));
	}
	for (int i = 0; i < 5; i++)
	{
		THREAD_POOL.Update();
		std::this_thread::sleep_for(std::chrono::milliseconds(2));
	}

	EXPECT_FALSE(THREAD_POOL.IsAnyDedicatedThreadHaveActiveJob());
}

TEST_F(FEThreadPoolTest, SetConcurrentThreadCount_ClampsAtMaximum)
{
	const unsigned int Before = THREAD_POOL.GetThreadCount();
	EXPECT_TRUE(THREAD_POOL.SetConcurrentThreadCount(FE_MAX_CONCURRENT_THREADS + 50));
	EXPECT_EQ(THREAD_POOL.GetThreadCount(), FE_MAX_CONCURRENT_THREADS);
	EXPECT_FALSE(THREAD_POOL.SetConcurrentThreadCount(FE_MAX_CONCURRENT_THREADS));
	// Guard against an accidental shrink past Before.
	EXPECT_GE(THREAD_POOL.GetThreadCount(), Before);
}

TEST_F(FEThreadPoolTest, ExecuteLightThread_InvalidID_ReturnsFalse)
{
	bool bRan = false;
	const bool bResult = THREAD_POOL.ExecuteLightThread("does-not-exist", [&bRan]() {
		bRan = true;
	});
	EXPECT_FALSE(bResult);
	std::this_thread::sleep_for(std::chrono::milliseconds(20));
	EXPECT_FALSE(bRan);
}

TEST_F(FEThreadPoolTest, ConcurrentCreateDedicatedThread)
{
	constexpr int Submitters = 4;
	constexpr int PerSubmitter = 25;
	std::vector<std::vector<std::string>> IDs(Submitters);

	std::atomic<bool> bCanStart{ false };
	std::vector<std::thread> Workers;
	for (int i = 0; i < Submitters; i++)
	{
		Workers.emplace_back([&, i]() {
			while (!bCanStart.load())
				std::this_thread::yield();

			for (int j = 0; j < PerSubmitter; j++)
				IDs[i].push_back(THREAD_POOL.CreateDedicatedThread());
		});
	}
	bCanStart.store(true);

	for (auto& Worker : Workers)
		Worker.join();

	std::unordered_map<std::string, int> Counts;
	int Empty = 0;
	for (const auto& CurrentIDList : IDs)
	{
		for (const auto& CurrentID : CurrentIDList)
			CurrentID.empty() ? Empty++ : Counts[CurrentID]++;
	}

	EXPECT_EQ(Empty, 0);
	EXPECT_EQ(static_cast<int>(Counts.size()), Submitters * PerSubmitter) << "Concurrent CreateDedicatedThread either dropped entries or generated duplicates DedicatedThreads.push_back is unsynchronised.";

	for (const auto& CurrentIDList : IDs)
		for (const auto& CurrentID : CurrentIDList)
			THREAD_POOL.ShutdownDedicatedThread(CurrentID);

	for (int i = 0; i < 50; i++)
	{
		THREAD_POOL.Update();
		std::this_thread::sleep_for(std::chrono::milliseconds(2));
	}
}

TEST_F(FEThreadPoolTest, ConcurrentDedicatedAndLightThreadCreation_ProducesUniqueIDs)
{
	constexpr int ThreadsPerKind = 50;

	std::vector<std::string> DedicatedIDs;
	std::vector<std::string> LightIDs;
	DedicatedIDs.reserve(ThreadsPerKind);
	LightIDs.reserve(ThreadsPerKind);

	std::atomic<bool> bCanStart{ false };
	std::thread DedicatedCreator([&]() {
		while (!bCanStart.load())
			std::this_thread::yield();

		for (int i = 0; i < ThreadsPerKind; i++)
			DedicatedIDs.push_back(THREAD_POOL.CreateDedicatedThread());
	});
	std::thread LightCreator([&]() {
		while (!bCanStart.load())
			std::this_thread::yield();

		for (int i = 0; i < ThreadsPerKind; i++)
			LightIDs.push_back(THREAD_POOL.CreateLightThread());
	});
	bCanStart.store(true);

	DedicatedCreator.join();
	LightCreator.join();

	std::unordered_map<std::string, int> Counts;
	int Empty = 0;
	for (const auto& CurrentID : DedicatedIDs)
		CurrentID.empty() ? Empty++ : Counts[CurrentID]++;
	for (const auto& CurrentID : LightIDs)
		CurrentID.empty() ? Empty++ : Counts[CurrentID]++;

	EXPECT_EQ(Empty, 0);
	EXPECT_EQ(static_cast<int>(Counts.size()), ThreadsPerKind * 2) << "Concurrent creation produced duplicate thread IDs";

	for (const auto& CurrentID : DedicatedIDs)
		THREAD_POOL.ShutdownDedicatedThread(CurrentID);

	for (const auto& CurrentID : LightIDs)
		THREAD_POOL.RemoveLightThread(CurrentID);

	for (int i = 0; i < 50; i++)
	{
		THREAD_POOL.Update();
		std::this_thread::sleep_for(std::chrono::milliseconds(2));
	}
}

TEST_F(FEThreadPoolTest, ConcurrentSubmitFromManyThreads)
{
	constexpr int SubmitterCount = 4;
	constexpr int JobsPerSubmitter = 250;
	constexpr int TotalJobs = SubmitterCount * JobsPerSubmitter;

	std::atomic<int> JobsRan{ 0 };
	std::atomic<int> CallbacksFired{ 0 };

	auto Job = [&JobsRan](void*, void*) {
		JobsRan.fetch_add(1, std::memory_order_acq_rel);
	};

	auto Callback = [&CallbacksFired](void*) {
		CallbacksFired.fetch_add(1, std::memory_order_acq_rel);
	};

	std::atomic<bool> bCanStart{ false };
	std::vector<std::thread> Submitters;
	Submitters.reserve(SubmitterCount);
	for (int CurrentSubmitter = 0; CurrentSubmitter < SubmitterCount; CurrentSubmitter++)
	{
		Submitters.emplace_back([&, CurrentSubmitter]() {
			while (!bCanStart.load(std::memory_order_acquire))
				std::this_thread::yield();

			for (int i = 0; i < JobsPerSubmitter; i++)
				THREAD_POOL.Execute(Job, nullptr, nullptr, Callback);
		});
	}
	bCanStart.store(true, std::memory_order_release);

	auto Deadline = std::chrono::steady_clock::now() + std::chrono::seconds(30);
	while (CallbacksFired.load(std::memory_order_acquire) < TotalJobs && std::chrono::steady_clock::now() < Deadline)
	{
		THREAD_POOL.Update();
		std::this_thread::sleep_for(std::chrono::milliseconds(1));
	}

	for (auto& Submitter : Submitters)
		Submitter.join();

	// One more drain after submitters are done, in case Update() raced with the final pushes.
	Deadline = std::chrono::steady_clock::now() + std::chrono::seconds(10);
	while (CallbacksFired.load(std::memory_order_acquire) < TotalJobs && std::chrono::steady_clock::now() < Deadline)
	{
		THREAD_POOL.Update();
		std::this_thread::sleep_for(std::chrono::milliseconds(1));
	}

	EXPECT_EQ(JobsRan.load(), TotalJobs) << "Jobs lost during concurrent submit, race on FEThreadPool::Execute()";
	EXPECT_EQ(CallbacksFired.load(), TotalJobs) << "Callbacks lost during concurrent submit, race on FEThreadPool::Execute()/Update()";
}

// Concurrent WaitForLightThread + RemoveLightThread stress: passes only by surviving the window without heap corruption on the LightThreads vector.
TEST_F(FEThreadPoolTest, ConcurrentWaitAndRemoveLightThread)
{
	constexpr int InitialLightThreadCount = 32;
	constexpr int WaiterThreadCount = 4;
	constexpr int RemoverThreadCount = 4;
	constexpr auto StressDuration = std::chrono::milliseconds(500);

	std::vector<std::string> IDs;
	IDs.reserve(InitialLightThreadCount);
	for (int i = 0; i < InitialLightThreadCount; i++)
	{
		const std::string CurrentID = THREAD_POOL.CreateLightThread();
		ASSERT_FALSE(CurrentID.empty());
		// Execute a quick task so the thread handle is joinable.
		const bool bWasStarted = THREAD_POOL.ExecuteLightThread(CurrentID, []() {
			std::this_thread::sleep_for(std::chrono::milliseconds(1));
		});
		ASSERT_TRUE(bWasStarted);
		IDs.push_back(CurrentID);
	}

	std::atomic<bool> bStop{ false };
	std::vector<std::thread> Workers;
	Workers.reserve(WaiterThreadCount + RemoverThreadCount);

	for (int i = 0; i < WaiterThreadCount; i++)
	{
		Workers.emplace_back([&, i]() {
			std::mt19937 RandomGenerator(static_cast<unsigned int>(i) * 1000 + 42);
			while (!bStop.load(std::memory_order_acquire))
			{
				const size_t Index = RandomGenerator() % IDs.size();
				THREAD_POOL.WaitForLightThread(IDs[Index]);
			}
		});
	}

	for (int i = 0; i < RemoverThreadCount; i++)
	{
		Workers.emplace_back([&, i]() {
			std::mt19937 RandomGenerator(static_cast<unsigned int>(i) * 1000 + 99);
			while (!bStop.load(std::memory_order_acquire))
			{
				const size_t Index = RandomGenerator() % IDs.size();
				THREAD_POOL.RemoveLightThread(IDs[Index]);
			}
		});
	}

	std::this_thread::sleep_for(StressDuration);
	bStop.store(true, std::memory_order_release);

	for (auto& Worker : Workers)
		Worker.join();

	for (const auto& CurrentID : IDs)
		THREAD_POOL.RemoveLightThread(CurrentID);
}

// Deleting a JobThread must stop and join its worker.
TEST_F(FEThreadPoolTest, DeletedJobThread_DetachedWorkerDoesNotTouchFreedMemory)
{
	SuppressCrashDialogs(); // Route any CRT heap report to stderr instead of a dialog.

	const int OldDebugFlags = _CrtSetDbgFlag(_CRTDBG_REPORT_FLAG);
	_CrtSetDbgFlag(OldDebugFlags | _CRTDBG_ALLOC_MEM_DF | _CRTDBG_DELAY_FREE_MEM_DF);

	JobThread* Thread = new JobThread();
	std::this_thread::sleep_for(std::chrono::milliseconds(100)); // The worker is parked in its 5 ms polling loop.
	delete Thread;
	std::this_thread::sleep_for(std::chrono::milliseconds(200));

	EXPECT_TRUE(_CrtCheckMemory()) << "The worker wrote into the freed JobThread block";

	_CrtSetDbgFlag(OldDebugFlags);
}

namespace
{
	// The singleton's first touch must construct exactly one instance even when it happens on several threads at once.
	// This has to run in a death-test child process, in the main test process the pool is already constructed.
	[[noreturn]] void RunConcurrentSingletonFirstTouchScenario()
	{
		SuppressCrashDialogs();

		constexpr int ThreadCount = 8;
		void* Pointers[ThreadCount] = {};
		std::atomic<bool> bCanStart{ false };

		std::vector<std::thread> Threads;
		for (int i = 0; i < ThreadCount; i++)
		{
			Threads.emplace_back([&Pointers, &bCanStart, i]() {
				while (!bCanStart.load())
					std::this_thread::yield();

				Pointers[i] = FEThreadPool::GetInstancePointer();
			});
		}
		bCanStart.store(true);

		for (auto& CurrentThread : Threads)
			CurrentThread.join();

		for (int i = 1; i < ThreadCount; i++)
		{
			if (Pointers[i] != Pointers[0])
				::_exit(86);
		}

		::_exit(0);
	}
}

TEST(FEThreadPoolDeathTest, ConcurrentFirstTouchOfSingleton_CreatesExactlyOneInstance)
{
	EXPECT_EXIT(RunConcurrentSingletonFirstTouchScenario(), ::testing::ExitedWithCode(0), "");
}