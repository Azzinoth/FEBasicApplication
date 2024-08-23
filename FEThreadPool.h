#pragma once

#include "FELog.h"

#include <thread>
#include <atomic>
#include <functional>
#include <utility>
#include <mutex>

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

	class LightThread
	{
		friend FEThreadPool;
		std::string ThreadID;

		std::thread ThreadHandler;
	public:
		LightThread();
		~LightThread();
	};

	class FEBASICAPPLICATION_API FEThreadPool
	{
	public:
		SINGLETON_PUBLIC_PART(FEThreadPool)

		bool SetConcurrentThreadCount(size_t NewValue);
		void Execute(FE_THREAD_JOB_FUNC Job, void* InputData = nullptr, void* OutputData = nullptr, FE_THREAD_CALLBACK_FUNC CallBack = nullptr);
		void Update();

		bool IsAnyThreadHaveActiveJob() const;
		unsigned int GetLogicalCoreCount() const;
		unsigned int GetThreadCount() const;

		std::string CreateDedicatedThread();
		bool IsAnyDedicatedThreadHaveActiveJob() const;
		void Execute(const std::string& DedicatedThreadID, FE_THREAD_JOB_FUNC Job, void* InputData = nullptr, void* OutputData = nullptr, FE_THREAD_CALLBACK_FUNC CallBack = nullptr);
		bool WaitForDedicatedThread(const std::string& DedicatedThreadID);
		bool ShutdownDedicatedThread(const std::string& DedicatedThreadID);

		std::string CreateLightThread();

		template <typename Callable, typename... Args>
		bool ExecuteLightThread(const std::string& LightThreadID, Callable&& Func, Args&&... ArgsList)
		{
			LightThread* Thread = GetLightThread(LightThreadID);
			if (!Thread)
				return false;

			Thread->ThreadHandler = std::thread(std::forward<Callable>(Func), std::forward<Args>(ArgsList)...);
			return true;
		}

		bool WaitForLightThread(const std::string& LightThreadID);
		bool RemoveLightThread(const std::string& LightThreadID);
	private:
		SINGLETON_PRIVATE_PART(FEThreadPool)

		std::mutex MainMutex;
		std::mutex LightThreadsMutex;

		std::vector<JobThread*> Threads;
		std::vector<FEUnexecutedJob*> JobsList;

		std::vector<DedicatedJobThread*> DedicatedThreads;
		std::vector<DedicatedJobThread*> DedicatedThreadsToShutdown;
		void MarkDedicatedThreadForShutdown(DedicatedJobThread* DedicatedThread);
		DedicatedJobThread* GetDedicatedThread(const std::string& ThreadID);

		void CollectJob(JobThread* FromThread);

		std::vector<LightThread*> LightThreads;
		LightThread* GetLightThread(const std::string& ThreadID);
	};

#ifdef FEBASICAPPLICATION_SHARED
	extern "C" __declspec(dllexport) void* GetThreadPool();
	#define THREAD_POOL (*static_cast<FEThreadPool*>(GetThreadPool()))
#else
	#define THREAD_POOL FEThreadPool::GetInstance()
#endif
}