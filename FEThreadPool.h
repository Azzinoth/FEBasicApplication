#pragma once

#include "FETime.h"

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

		std::thread ThreadHandler;
		std::atomic<bool> bJobFinished;
		std::atomic<bool> bHaveNewJob;
		std::atomic<bool> bJobCollected;

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
	private:
		SINGLETON_PRIVATE_PART(FEThreadPool)

		std::vector<JobThread*> Threads;
		std::vector<FEUnexecutedJob*> JobsList;

		void CollectJob(JobThread* FromThread);
	};

	#define THREAD_POOL FEThreadPool::getInstance()
}