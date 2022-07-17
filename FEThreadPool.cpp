#include "FEThreadPool.h"
using namespace FocalEngine;

FEThreadPool* FEThreadPool::Instance = nullptr;

JobThread::JobThread()
{
	bJobFinished = false;
	bHaveNewJob = false;
	bJobCollected = true;
	ThreadHandler = std::thread(&JobThread::ExecuteJob, this);
	ThreadHandler.detach();
}

JobThread::~JobThread() {}

void JobThread::ExecuteJob()
{
	while (true)
	{
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

	// If all thread in poll are working we should save new job for later execution.
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
}