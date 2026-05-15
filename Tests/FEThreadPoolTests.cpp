#include "FEThreadPoolTests.h"

#include <atomic>
#include <chrono>
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

	std::atomic<bool> JobReceivedInput{ false };
	std::atomic<bool> CallbackReceivedOutput{ false };

	auto Job = [&JobReceivedInput, &InputValue](void* InputData, void* OutputData) {
		if (InputData == &InputValue && *static_cast<int*>(InputData) == 42)
			JobReceivedInput.store(true, std::memory_order_release);

		// Write a result through OutputData so the callback can verify.
		if (OutputData != nullptr)
			*static_cast<int*>(OutputData) = 1337;
	};

	auto Callback = [&CallbackReceivedOutput, &OutputValue](void* OutputData) {
		if (OutputData == &OutputValue && *static_cast<int*>(OutputData) == 1337)
			CallbackReceivedOutput.store(true, std::memory_order_release);
	};

	THREAD_POOL.Execute(Job, &InputValue, &OutputValue, Callback);

	auto Deadline = std::chrono::steady_clock::now() + std::chrono::seconds(5);
	while (!CallbackReceivedOutput.load(std::memory_order_acquire) && std::chrono::steady_clock::now() < Deadline)
	{
		THREAD_POOL.Update();
		std::this_thread::sleep_for(std::chrono::milliseconds(5));
	}

	EXPECT_TRUE(JobReceivedInput.load());
	EXPECT_TRUE(CallbackReceivedOutput.load());
	EXPECT_EQ(OutputValue, 1337);
}

// Each callback synchronously submits the next job via Execute. The chain is many steps deep, so the
// callback->Execute->AssignJob->worker->bJobFinished->collect->callback cycle runs back-to-back rapidly.
TEST_F(FEThreadPoolTest, ChainedCallbackSubmits_DeepChainAllElementsFire)
{
	static constexpr int ChainLength = 200;
	static std::atomic<int> ChainStep;
	static std::atomic<bool> ChainComplete;
	ChainStep.store(0);
	ChainComplete.store(false);

	static std::function<void(void*)> ChainCallback;
	ChainCallback = [](void*) {
		const int Current = ChainStep.fetch_add(1, std::memory_order_acq_rel) + 1;
		if (Current < ChainLength)
		{
			THREAD_POOL.Execute([](void*, void*) {}, nullptr, nullptr, ChainCallback);
		}
		else
		{
			ChainComplete.store(true, std::memory_order_release);
		}
	};

	THREAD_POOL.Execute([](void*, void*) {}, nullptr, nullptr, ChainCallback);

	auto Deadline = std::chrono::steady_clock::now() + std::chrono::seconds(15);
	while (!ChainComplete.load(std::memory_order_acquire) && std::chrono::steady_clock::now() < Deadline)
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