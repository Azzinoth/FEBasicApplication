#pragma once

#include "FELog.h"

#include <thread>
#include <atomic>
#include <functional>

namespace FocalEngine
{
#define FE_THREAD_CALLBACK_FUNC std::function<void (void* OutputData)>
#define FE_THREAD_JOB_FUNC std::function<void (void* InputData, void* OutputData)>
#define FE_MAX_CONCURRENT_THREADS 1024

	class JobThread;
	class FEThreadPool;

	class FEUnexecutedJob
	{
		friend JobThread;
		friend FEThreadPool;

		void* InputData = nullptr;
		void* OutputData = nullptr;

		FE_THREAD_JOB_FUNC Job = nullptr;
		FE_THREAD_CALLBACK_FUNC CallBack = nullptr;
	};

	class JobThread
	{
		friend FEThreadPool;
	protected:
		std::thread ThreadHandler;
		std::atomic<bool> bJobFinished;
		std::atomic<bool> bHaveNewJob;
		std::atomic<bool> bJobCollected;
		bool bNeedToExit = false;
		bool bReadyForDeletion = false;

		void* CurrentInputData = nullptr;
		void* CurrentOutputData = nullptr;

		FE_THREAD_JOB_FUNC Job = nullptr;
		FE_THREAD_CALLBACK_FUNC CurrentCallBack = nullptr;

		void ExecuteJob();
		bool AssignJob(const FEUnexecutedJob* NewJob);
	public:
		JobThread();
		~JobThread();

		bool IsJobFinished() const;
		bool IsJobCollected() const;
	};

	class DedicatedJobThread : public JobThread
	{
		friend FEThreadPool;
		std::string ThreadID;

		std::vector<FEUnexecutedJob*> JobsList;
	public:
		DedicatedJobThread();
		~DedicatedJobThread();
	};

	class FEThreadPool
	{
	public:
		SINGLETON_PUBLIC_PART(FEThreadPool)

		bool SetConcurrentThreadCount(size_t NewValue);
		void Execute(FE_THREAD_JOB_FUNC Job, void* InputData = nullptr, void* OutputData = nullptr, FE_THREAD_CALLBACK_FUNC CallBack = nullptr);
		void Update();

		bool IsAnyThreadHaveActiveJob() const;
		unsigned int GetLogicalCoreCount() const;
		unsigned int GetThreadCount() const;

		std::string CreateDedicatedThreadID();
		bool IsAnyDedicatedThreadHaveActiveJob() const;
		void Execute(std::string DedicatedThreadID, FE_THREAD_JOB_FUNC Job, void* InputData = nullptr, void* OutputData = nullptr, FE_THREAD_CALLBACK_FUNC CallBack = nullptr);

		bool ShutdownDedicatedThread(std::string DedicatedThreadID);
	private:
		SINGLETON_PRIVATE_PART(FEThreadPool)

		std::vector<JobThread*> Threads;
		std::vector<FEUnexecutedJob*> JobsList;

		std::vector<DedicatedJobThread*> DedicatedThreads;
		std::vector<DedicatedJobThread*> DedicatedThreadsToShutdown;
		void MarkDedicatedThreadForShutdown(DedicatedJobThread* DedicatedThread);

		void CollectJob(JobThread* FromThread);

		DedicatedJobThread* GetDedicatedThread(std::string ThreadID);
	};

	#define THREAD_POOL FEThreadPool::getInstance()
}