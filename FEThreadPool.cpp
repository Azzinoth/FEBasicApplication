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
	ThreadHandler.detach();
}

JobThread::~JobThread() {}

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
			Job(CurrentInputData, CurrentOutputData);
		}
		else
		{
			bJobCollected = true;
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

	bHaveNewJob = true;
	bJobFinished = false;
	bJobCollected = false;

	return true;
}

DedicatedJobThread::DedicatedJobThread()
{
	ThreadID = UNIQUE_ID.GetUniqueHexID();
}

DedicatedJobThread::~DedicatedJobThread() {}

LightThread::LightThread()
{
	ThreadID = UNIQUE_ID.GetUniqueHexID();
}

LightThread::~LightThread() {}

FEThreadPool::FEThreadPool()
{
	Threads.resize(4);
	for (size_t i = 0; i < Threads.size(); i++)
	{
		Threads[i] = new JobThread();
	}
}

FEThreadPool::~FEThreadPool()
{
	for (size_t i = 0; i < Threads.size(); i++)
	{
		delete Threads[i];
	}

	Threads.clear();
}

bool FEThreadPool::IsAnyThreadHaveActiveJob() const
{
	for (size_t i = 0; i < Threads.size(); i++)
	{
		if (!Threads[i]->IsJobCollected())
			return true;
	}

	return false;
}

bool FEThreadPool::SetConcurrentThreadCount(size_t NewValue)
{
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

void FEThreadPool::CollectJob(JobThread* FromThread)
{
	if (FromThread->CurrentCallBack != nullptr)
		FromThread->CurrentCallBack(FromThread->CurrentOutputData);

	FromThread->bJobCollected.store(true);
}

void FEThreadPool::Update()
{
	for (size_t i = 0; i < Threads.size(); i++)
	{
		if (Threads[i]->IsJobFinished() && !Threads[i]->IsJobCollected())
		{
			CollectJob(Threads[i]);
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
		if (DedicatedThreads[i]->bNeedToExit || DedicatedThreads[i]->bReadyForDeletion)
			continue;

		if (DedicatedThreads[i]->IsJobFinished() && !DedicatedThreads[i]->IsJobCollected())
		{
			CollectJob(DedicatedThreads[i]);
		}
		else if (!DedicatedThreads[i]->JobsList.empty() && DedicatedThreads[i]->IsJobFinished() && DedicatedThreads[i]->IsJobCollected())
		{
			if (DedicatedThreads[i]->AssignJob(DedicatedThreads[i]->JobsList[0]))
				DedicatedThreads[i]->JobsList.erase(DedicatedThreads[i]->JobsList.begin());
		}
	}
}

unsigned int FEThreadPool::GetLogicalCoreCount() const
{
	return std::thread::hardware_concurrency();
}

unsigned int FEThreadPool::GetThreadCount() const
{
	return static_cast<int>(Threads.size());
}

std::string FEThreadPool::CreateDedicatedThread()
{
	DedicatedThreads.push_back(new DedicatedJobThread());
	return DedicatedThreads.back()->ThreadID;
}

bool FEThreadPool::IsAnyDedicatedThreadHaveActiveJob() const
{
	for (size_t i = 0; i < DedicatedThreads.size(); i++)
	{
		if (!DedicatedThreads[i]->IsJobCollected())
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
	DedicatedJobThread* Thread = GetDedicatedThread(DedicatedThreadID);
	if (!Thread)
		return;

	if (Thread->bNeedToExit || Thread->bReadyForDeletion)
		return;

	FEUnexecutedJob* NewJob = new FEUnexecutedJob();
	NewJob->Job = Job;
	NewJob->InputData = InputData;
	NewJob->OutputData = OutputData;
	NewJob->CallBack = CallBack;

	// Check if we can start it right away
	if (Thread->IsJobFinished() && Thread->IsJobCollected())
	{
		if (Thread->AssignJob(NewJob))
			return;
	}

	// If all thread in pool are working we should save new job for later execution.
	Thread->JobsList.push_back(NewJob);
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
	DedicatedJobThread* Thread = GetDedicatedThread(DedicatedThreadID);
	if (!Thread)
		return false;

	Thread->bJobFinished = false;
	Thread->bHaveNewJob = false;
	Thread->bJobCollected = true;
	Thread->JobsList.clear();
	Thread->bNeedToExit = true;

	MarkDedicatedThreadForShutdown(Thread);

	return true;
}

bool FEThreadPool::WaitForDedicatedThread(const std::string& DedicatedThreadID)
{
	DedicatedJobThread* Thread = GetDedicatedThread(DedicatedThreadID);
	if (!Thread)
		return false;

	while (!Thread->JobsList.empty() || !Thread->IsJobFinished())
	{
		if (Thread->IsJobFinished() && !Thread->IsJobCollected())
		{
			CollectJob(Thread);
			if (Thread->JobsList.empty())
				return true;
		}
		else if (!Thread->JobsList.empty() && Thread->IsJobFinished() && Thread->IsJobCollected())
		{
			if (Thread->AssignJob(Thread->JobsList[0]))
				Thread->JobsList.erase(Thread->JobsList.begin());
		}
		Sleep(10);
	}

	return true;
}

std::string FEThreadPool::CreateLightThread()
{
	std::lock_guard<std::mutex> Lock(LightThreadsMutex);

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
	LightThread* Thread = GetLightThread(LightThreadID);
	if (!Thread)
		return false;

	if (!Thread->ThreadHandler.joinable())
		return false;

	Thread->ThreadHandler.join();
	return true;
}

bool FEThreadPool::RemoveLightThread(const std::string& LightThreadID)
{
	std::lock_guard<std::mutex> Lock(LightThreadsMutex);

	WaitForLightThread(LightThreadID);

	for (size_t i = 0; i < LightThreads.size(); i++)
	{
		if (LightThreads[i]->ThreadID == LightThreadID)
		{
			delete LightThreads[i];
			LightThreads.erase(LightThreads.begin() + i, LightThreads.begin() + i + 1);
			return true;
		}
	}

	return false;
}