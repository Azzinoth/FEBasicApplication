#include "FEThreadPool.h"
using namespace FocalEngine;

#ifdef FEBASICAPPLICATION_SHARED
extern "C" __declspec(dllexport) void* GetThreadPool()
{
	return FEThreadPool::GetInstancePointer();
}
#endif

JobThread::JobThread()
{
	bJobFinished = false;
	bHaveNewJob = false;
	bJobCollected = true;
	bNeedToExit = false;
	bReadyForDeletion = false;
	ThreadHandler = std::thread(&JobThread::ExecuteJob, this);
}

JobThread::~JobThread()
{
	bNeedToExit = true;
	if (ThreadHandler.joinable())
		ThreadHandler.join();
}

void JobThread::ExecuteJob()
{
	while (true)
	{
		if (bNeedToExit)
		{
			bReadyForDeletion = true;
			return;
		}

		bHaveNewJob = false;

		if (Job != nullptr)
		{
			try
			{
				Job(CurrentInputData, CurrentOutputData);
			}
			catch (const std::exception& Exception)
			{
				LOG.Add(std::string("Exception in thread pool job: ") + Exception.what(), "FE_THREAD_POOL", FE_LOG_ERROR);
			}
			catch (...)
			{
				LOG.Add("Unknown exception in thread pool job.", "FE_THREAD_POOL", FE_LOG_ERROR);
			}
		}

		bJobFinished = true;

		while (true)
		{
			if (bNeedToExit)
			{
				bReadyForDeletion = true;
				return;
			}

			Sleep(5);
			if (bHaveNewJob.load())
				break;
		}
	}
}

bool JobThread::IsJobFinished() const
{
	return bJobFinished.load();
}

bool JobThread::IsJobCollected() const
{
	return bJobCollected.load();
}

bool JobThread::AssignJob(const FEUnexecutedJob* NewJob)
{
	if (!IsJobFinished())
		return false;

	CurrentInputData = NewJob->InputData;
	CurrentOutputData = NewJob->OutputData;

	Job = NewJob->Job;
	CurrentCallBack = NewJob->CallBack;
	delete NewJob;

	bJobFinished = false;
	bJobCollected = false;
	bHaveNewJob = true;

	return true;
}

DedicatedJobThread::DedicatedJobThread()
{
	ThreadID = UNIQUE_ID.GetUniqueHexID();
	bShutdownRequested = false;
}

DedicatedJobThread::~DedicatedJobThread()
{
	for (size_t i = 0; i < JobsList.size(); i++)
		delete JobsList[i];
	
	JobsList.clear();
}

LightThread::LightThread()
{
	ThreadID = UNIQUE_ID.GetUniqueHexID();
}

LightThread::~LightThread() {}

FEThreadPool::FEThreadPool()
{
	Threads.resize(4);
	for (size_t i = 0; i < Threads.size(); i++)
		Threads[i] = new JobThread();
}

FEThreadPool::~FEThreadPool()
{
	for (size_t i = 0; i < Threads.size(); i++)
		delete Threads[i];

	Threads.clear();

	for (size_t i = 0; i < JobsList.size(); i++)
		delete JobsList[i];

	JobsList.clear();

	for (size_t i = 0; i < DedicatedThreads.size(); i++)
		delete DedicatedThreads[i];

	DedicatedThreads.clear();
	DedicatedThreadsToShutdown.clear();

	for (size_t i = 0; i < LightThreads.size(); i++)
	{
		if (LightThreads[i]->ThreadHandler.joinable())
			LightThreads[i]->ThreadHandler.join();

		delete LightThreads[i];
	}

	LightThreads.clear();
}

bool FEThreadPool::IsAnyThreadHaveActiveJob() const
{
	std::lock_guard<std::recursive_mutex> Lock(MainMutex);

	if (!JobsList.empty())
		return true;

	for (size_t i = 0; i < Threads.size(); i++)
	{
		if (!Threads[i]->IsJobCollected())
			return true;
	}

	return false;
}

bool FEThreadPool::SetConcurrentThreadCount(size_t NewValue)
{
	std::lock_guard<std::recursive_mutex> Lock(MainMutex);

	if (NewValue > FE_MAX_CONCURRENT_THREADS)
		NewValue = FE_MAX_CONCURRENT_THREADS;

	if (NewValue <= Threads.size())
		return false;

	const size_t NewThreadsToAdd = NewValue - Threads.size();
	for (size_t i = 0; i < NewThreadsToAdd; i++)
	{
		Threads.push_back(new JobThread());
	}

	return true;
}

void FEThreadPool::Execute(const FE_THREAD_JOB_FUNC Job, void* InputData, void* OutputData, const FE_THREAD_CALLBACK_FUNC CallBack)
{
	std::lock_guard<std::recursive_mutex> Lock(MainMutex);

	FEUnexecutedJob* NewJob = new FEUnexecutedJob();
	NewJob->Job = Job;
	NewJob->InputData = InputData;
	NewJob->OutputData = OutputData;
	NewJob->CallBack = CallBack;

	// Check if we can start it right away
	for (size_t i = 0; i < Threads.size(); i++)
	{
		if (Threads[i]->IsJobFinished() && Threads[i]->IsJobCollected())
		{
			if (Threads[i]->AssignJob(NewJob))
				return;
		}
	}

	// If all threads in the pool are working, we should save the new job for later execution.
	JobsList.push_back(NewJob);
}

std::pair<FE_THREAD_CALLBACK_FUNC, void*> FEThreadPool::CollectJob(JobThread* FromThread)
{
	// Copy the callback data and mark the job as collected BEFORE the callback can run.
	const FE_THREAD_CALLBACK_FUNC CallBack = FromThread->CurrentCallBack;
	void* OutputData = FromThread->CurrentOutputData;

	FromThread->bJobCollected.store(true);

	return { CallBack, OutputData };
}

void FEThreadPool::Update()
{
	// Callbacks are gathered under the lock but invoked after it is released, so user callbacks do not block other threads that are using the pool.
	std::vector<std::pair<FE_THREAD_CALLBACK_FUNC, void*>> CallBacksToInvoke;

	{
		std::lock_guard<std::recursive_mutex> Lock(MainMutex);

		for (size_t i = 0; i < Threads.size(); i++)
		{
			if (Threads[i]->IsJobFinished() && !Threads[i]->IsJobCollected())
			{
				CallBacksToInvoke.push_back(CollectJob(Threads[i]));
			}
			else if (!JobsList.empty() && Threads[i]->IsJobFinished() && Threads[i]->IsJobCollected())
			{
				if (Threads[i]->AssignJob(JobsList[0]))
					JobsList.erase(JobsList.begin());
			}
		}

		for (size_t i = 0; i < DedicatedThreadsToShutdown.size(); i++)
		{
			if (DedicatedThreadsToShutdown[i]->bNeedToExit && DedicatedThreadsToShutdown[i]->bReadyForDeletion)
			{
				for (size_t j = 0; j < DedicatedThreads.size(); j++)
				{
					if (DedicatedThreads[j]->ThreadID == DedicatedThreadsToShutdown[i]->ThreadID)
					{
						DedicatedThreads.erase(DedicatedThreads.begin() + j, DedicatedThreads.begin() + j + 1);
						break;
					}
				}

				delete DedicatedThreadsToShutdown[i];
				DedicatedThreadsToShutdown.erase(DedicatedThreadsToShutdown.begin() + i, DedicatedThreadsToShutdown.begin() + i + 1);
				i--;
				if (DedicatedThreads.empty())
					break;
			}
		}

		for (size_t i = 0; i < DedicatedThreads.size(); i++)
		{
			DedicatedJobThread* CurrentDedicatedThread = DedicatedThreads[i];
			if (CurrentDedicatedThread->bNeedToExit || CurrentDedicatedThread->bReadyForDeletion)
				continue;

			if (CurrentDedicatedThread->IsJobFinished() && !CurrentDedicatedThread->IsJobCollected())
			{
				CallBacksToInvoke.push_back(CollectJob(CurrentDedicatedThread));
			}
			else if (!CurrentDedicatedThread->JobsList.empty() && CurrentDedicatedThread->IsJobFinished() && CurrentDedicatedThread->IsJobCollected())
			{
				if (CurrentDedicatedThread->AssignJob(CurrentDedicatedThread->JobsList[0]))
					CurrentDedicatedThread->JobsList.erase(CurrentDedicatedThread->JobsList.begin());
			}

			// If a graceful shutdown was requested and the thread has finished its queue, promote the request to a real exit signal.
			if (CurrentDedicatedThread->bShutdownRequested
				&& CurrentDedicatedThread->JobsList.empty()
				&& CurrentDedicatedThread->IsJobFinished()
				&& CurrentDedicatedThread->IsJobCollected())
			{
				CurrentDedicatedThread->bNeedToExit = true;
				MarkDedicatedThreadForShutdown(CurrentDedicatedThread);
			}
		}
	}

	for (size_t i = 0; i < CallBacksToInvoke.size(); i++)
	{
		if (CallBacksToInvoke[i].first != nullptr)
			CallBacksToInvoke[i].first(CallBacksToInvoke[i].second);
	}
}

unsigned int FEThreadPool::GetLogicalCoreCount() const
{
	return std::thread::hardware_concurrency();
}

unsigned int FEThreadPool::GetThreadCount() const
{
	std::lock_guard<std::recursive_mutex> Lock(MainMutex);
	return static_cast<int>(Threads.size());
}

std::string FEThreadPool::CreateDedicatedThread()
{
	std::lock_guard<std::recursive_mutex> Lock(MainMutex);

	DedicatedThreads.push_back(new DedicatedJobThread());
	return DedicatedThreads.back()->ThreadID;
}

bool FEThreadPool::IsAnyDedicatedThreadHaveActiveJob() const
{
	std::lock_guard<std::recursive_mutex> Lock(MainMutex);

	for (size_t i = 0; i < DedicatedThreads.size(); i++)
	{
		if (!DedicatedThreads[i]->JobsList.empty() || !DedicatedThreads[i]->IsJobCollected())
			return true;
	}

	return false;
}

DedicatedJobThread* FEThreadPool::GetDedicatedThread(const std::string& ThreadID)
{
	for (size_t i = 0; i < DedicatedThreads.size(); i++)
	{
		if (DedicatedThreads[i]->ThreadID == ThreadID)
			return DedicatedThreads[i];
	}

	return nullptr;
}

void FEThreadPool::Execute(const std::string& DedicatedThreadID, const FE_THREAD_JOB_FUNC Job, void* InputData, void* OutputData, const FE_THREAD_CALLBACK_FUNC CallBack)
{
	std::lock_guard<std::recursive_mutex> Lock(MainMutex);

	DedicatedJobThread* Thread = GetDedicatedThread(DedicatedThreadID);
	if (!Thread)
		return;

	if (Thread->bShutdownRequested || Thread->bNeedToExit || Thread->bReadyForDeletion)
		return;

	FEUnexecutedJob* NewJob = new FEUnexecutedJob();
	NewJob->Job = Job;
	NewJob->InputData = InputData;
	NewJob->OutputData = OutputData;
	NewJob->CallBack = CallBack;

	// Check if we can start it right away, but never ahead of already queued jobs, dedicated thread must run jobs in submission order.
	if (Thread->JobsList.empty() && Thread->IsJobFinished() && Thread->IsJobCollected())
	{
		if (Thread->AssignJob(NewJob))
			return;
	}

	// The thread is busy or older jobs are waiting, so save the new job for later execution.
	Thread->JobsList.push_back(NewJob);

	// If the thread is idle while older jobs are waiting, start the oldest one now instead of waiting for the next Update().
	if (Thread->IsJobFinished() && Thread->IsJobCollected())
	{
		if (Thread->AssignJob(Thread->JobsList[0]))
			Thread->JobsList.erase(Thread->JobsList.begin());
	}
}

void FEThreadPool::MarkDedicatedThreadForShutdown(DedicatedJobThread* DedicatedThread)
{
	for (size_t i = 0; i < DedicatedThreadsToShutdown.size(); i++)
	{
		if (DedicatedThreadsToShutdown[i]->ThreadID == DedicatedThread->ThreadID)
			return;
	}

	DedicatedThreadsToShutdown.push_back(DedicatedThread);
}

bool FEThreadPool::ShutdownDedicatedThread(const std::string& DedicatedThreadID)
{
	std::lock_guard<std::recursive_mutex> Lock(MainMutex);

	DedicatedJobThread* Thread = GetDedicatedThread(DedicatedThreadID);
	if (!Thread)
		return false;

	// Request graceful shutdown. The dedicated-thread loop in Update() drains active jobs and any queued ones (firing their callbacks) before
	// promoting this to bNeedToExit and freeing the slot.
	Thread->bShutdownRequested = true;
	return true;
}

bool FEThreadPool::ForceShutdownDedicatedThread(const std::string& DedicatedThreadID)
{
	std::lock_guard<std::recursive_mutex> Lock(MainMutex);

	DedicatedJobThread* Thread = GetDedicatedThread(DedicatedThreadID);
	if (!Thread)
		return false;

	// Reject any subsequent Execute(ID, ...) calls.
	Thread->bShutdownRequested = true;

	// Drop queued work without running it. Active job runs to completion but its pending callback is intentionally not fired.
	for (size_t i = 0; i < Thread->JobsList.size(); i++)
		delete Thread->JobsList[i];
	Thread->JobsList.clear();

	// Signal the worker to exit on its next inner sleep wake (every ~5 ms).
	Thread->bNeedToExit = true;

	MarkDedicatedThreadForShutdown(Thread);
	return true;
}

bool FEThreadPool::WaitForDedicatedThread(const std::string& DedicatedThreadID)
{
	while (true)
	{
		std::pair<FE_THREAD_CALLBACK_FUNC, void*> CallBackToInvoke = { nullptr, nullptr };
		bool bAllJobsDone = false;

		{
			std::lock_guard<std::recursive_mutex> Lock(MainMutex);

			// Look up again under the lock so a concurrent Shutdown+Update can not free the thread out from under us between iterations.
			DedicatedJobThread* Thread = GetDedicatedThread(DedicatedThreadID);
			if (!Thread)
				return false;

			// The thread is shutting down so, it is no longer waitable and its pending callback is intentionally suppressed.
			if (Thread->bNeedToExit || Thread->bReadyForDeletion)
				return false;

			if (Thread->JobsList.empty() && Thread->IsJobFinished() && Thread->IsJobCollected())
				return true;

			if (Thread->IsJobFinished() && !Thread->IsJobCollected())
			{
				CallBackToInvoke = CollectJob(Thread);
				bAllJobsDone = Thread->JobsList.empty();
			}
			else if (!Thread->JobsList.empty() && Thread->IsJobFinished() && Thread->IsJobCollected())
			{
				if (Thread->AssignJob(Thread->JobsList[0]))
					Thread->JobsList.erase(Thread->JobsList.begin());
			}
		}

		// Invoke the callback outside of the lock, so user callbacks do not block other threads that are using the pool.
		if (CallBackToInvoke.first != nullptr)
			CallBackToInvoke.first(CallBackToInvoke.second);

		if (bAllJobsDone)
			return true;

		// Sleep is in a separate block so we do not hold the lock while sleeping.
		Sleep(10);
	}
}

std::string FEThreadPool::CreateLightThread()
{
	std::lock_guard<std::recursive_mutex> Lock(LightThreadsMutex);

	LightThread* NewThread = new LightThread();
	LightThreads.push_back(NewThread);
	return NewThread->ThreadID;
}

LightThread* FEThreadPool::GetLightThread(const std::string& ThreadID)
{
	for (size_t i = 0; i < LightThreads.size(); i++)
	{
		if (LightThreads[i]->ThreadID == ThreadID)
			return LightThreads[i];
	}

	return nullptr;
}

bool FEThreadPool::WaitForLightThread(const std::string& LightThreadID)
{
	std::thread ThreadToJoin;

	{
		std::lock_guard<std::recursive_mutex> Lock(LightThreadsMutex);

		LightThread* Thread = GetLightThread(LightThreadID);
		if (!Thread)
			return false;

		if (!Thread->ThreadHandler.joinable())
			return false;

		// Take ownership of the handle so the join can happen without holding the mutex.
		ThreadToJoin = std::move(Thread->ThreadHandler);
	}

	ThreadToJoin.join();
	return true;
}

bool FEThreadPool::RemoveLightThread(const std::string& LightThreadID)
{
	std::thread ThreadToJoin;
	bool bWasFound = false;

	{
		std::lock_guard<std::recursive_mutex> Lock(LightThreadsMutex);

		for (size_t i = 0; i < LightThreads.size(); i++)
		{
			if (LightThreads[i]->ThreadID == LightThreadID)
			{
				ThreadToJoin = std::move(LightThreads[i]->ThreadHandler);
				delete LightThreads[i];
				LightThreads.erase(LightThreads.begin() + i, LightThreads.begin() + i + 1);
				bWasFound = true;
				break;
			}
		}
	}

	if (!bWasFound)
		return false;

	// Join outside the mutex, to not deadlock with a light thread that uses the light thread API.
	if (ThreadToJoin.joinable())
		ThreadToJoin.join();

	return true;
}