#include "FEProfilingManager.h"
#include <iomanip>
using namespace FocalEngine;

#ifdef FEBASICAPPLICATION_SHARED
extern "C" __declspec(dllexport) void* GetProfilingManager()
{
	return FEProfilingManager::GetInstancePointer();
}
#endif

FEProfilingManager::FEProfilingManager() {}

FEProfilingManager::~FEProfilingManager() {}

FEProfilingRegistry& FEProfilingManager::GetCurrentThreadRegistry()
{
	thread_local FEProfilingRegistry* Registry = nullptr;
	if (Registry == nullptr)
	{
		std::lock_guard Lock(ThreadDataMutex);
		Registry = &ThreadData[std::this_thread::get_id()];
		Registry->TimeResolution = TimeResolution;
	}

	return *Registry;
}

void FEProfilingManager::RecordFunctionEntry(const char* FunctionName)
{
	if (!bActive)
		return;

	FEProfilingRegistry& Registry = GetCurrentThreadRegistry();
	std::lock_guard Lock(Registry.DataMutex);
	Registry.RecordFunctionEntry(FunctionName);
}

void FEProfilingManager::RecordFunctionExit(const char* FunctionName, FE_CHRONO_TIME_POINT StartTimeStamp, FE_CHRONO_TIME_POINT EndTimeStamp, double Duration)
{
	if (!bActive)
		return;

	FEProfilingRegistry& Registry = GetCurrentThreadRegistry();
	std::lock_guard Lock(Registry.DataMutex);
	Registry.RecordFunctionExit(FunctionName, StartTimeStamp, EndTimeStamp, Duration);
	EventCount++;
}

void FEProfilingManager::RecordSectionExit(const char* SectionName, FE_CHRONO_TIME_POINT StartTimeStamp, FE_CHRONO_TIME_POINT EndTimeStamp)
{
	if (!bActive)
		return;

	FEProfilingRegistry& Registry = GetCurrentThreadRegistry();
	std::lock_guard Lock(Registry.DataMutex);
	Registry.RecordSectionExit(SectionName, StartTimeStamp, EndTimeStamp);
	EventCount++;
}

//std::string FEProfilingManager::CreateReport()
//{
//	std::unique_lock Lock(ThreadDataMutex);
//
//	std::string Report = "Profiling Report\n";
//
//	auto ThreadIterator = ThreadData.begin();
//	while (ThreadIterator != ThreadData.end())
//	{
//		std::thread::id ThreadID = ThreadIterator->first;
//		std::stringstream ThreadIDString;
//		ThreadIDString << ThreadID;
//		Report += "Thread ID:" + ThreadIDString.str() + "\n";
//
//		for (auto& Entry : ThreadData[ThreadID].FunctionsData)
//		{
//			const std::string& FunctionName = Entry.first;
//			const FEProfilingRegistry::FunctionInfo& Info = Entry.second;
//			double AverageDuration = Info.TotalDuration / Info.CallCount;
//
//			Report += "----------------\n";
//			Report += "Function: " + FunctionName + "\n";
//			Report += "Count: " + std::to_string(Info.CallCount) + "\n";
//			Report += "Total Duration: " + std::to_string(Info.TotalDuration) + " microseconds\n";
//			Report += "Average Duration: " + std::to_string(AverageDuration) + " microseconds\n";
//			Report += "Called By:\n";
//
//			for (const auto& ParentEntry : Info.ParentFunctions)
//			{
//				const std::string& ParentFunction = ParentEntry.first;
//				int CallCount = ParentEntry.second;
//				Report += "  - " + ParentFunction + " (" + std::to_string(CallCount) + " times)\n";
//			}
//		}
//
//		ThreadIterator++;
//	}
//
//	return Report;
//}

void FEProfilingManager::SaveTimelineToJSON(const std::string& Filename)
{
	std::unique_lock Lock(ThreadDataMutex);

	std::ofstream File(Filename);
	if (!File.is_open())
		return;

	// Fractional microseconds, so short events do not become zero long.
	File << std::fixed << std::setprecision(3);
	const auto ToMicroseconds = [](const FE_CHRONO_TIME_POINT::duration Duration) {
		return std::chrono::duration<double, std::micro>(Duration).count();
	};

	File << "{\n";
	File << "  \"traceEvents\": [\n";

	bool bFirstEvent = true;
	for (auto& [ThreadID, Registry] : ThreadData)
	{
		std::lock_guard RegistryLock(Registry.DataMutex);

		std::stringstream ThreadIDString;
		ThreadIDString << ThreadID;

		for (auto& [FunctionName, Info] : Registry.FunctionsData)
		{
			for (const auto& TimeStamp : Info.Timestamps)
			{
				if (!bFirstEvent)
					File << ",\n";
				bFirstEvent = false;

				File << "    {\"name\": \"" << FunctionName << "\", \"cat\": \"function\", \"ph\": \"X\", \"ts\": " << ToMicroseconds(TimeStamp.first.time_since_epoch())
					 << ", \"dur\": " << ToMicroseconds(TimeStamp.second - TimeStamp.first) << ", \"pid\": 0, \"tid\": " << ThreadIDString.str() << "}";
			}

			for (const auto& [SectionName, SectionTimestamps] : Info.SectionTimestamps)
			{
				for (const auto& SectionTimestamp : SectionTimestamps)
				{
					if (!bFirstEvent)
						File << ",\n";
					bFirstEvent = false;

					File << "    {\"name\": \"" << SectionName << "\", \"cat\": \"section\", \"ph\": \"X\", \"ts\": " << ToMicroseconds(SectionTimestamp.first.time_since_epoch())
						 << ", \"dur\": " << ToMicroseconds(SectionTimestamp.second - SectionTimestamp.first) << ", \"pid\": 0, \"tid\": " << ThreadIDString.str()
						 << ", \"args\": {\"function\": \"" << FunctionName << "\"}}";
				}
			}
		}
	}

	File << "\n  ]\n";
	File << "}\n";

	File.close();
}

void FEProfilingManager::ClearData()
{
	std::unique_lock Lock(ThreadDataMutex);
	for (auto& [ThreadID, Registry] : ThreadData)
	{
		std::lock_guard RegistryLock(Registry.DataMutex);
		Registry.ClearData();
	}

	EventCount = 0;
}

int FEProfilingManager::GetEventCount()
{
	return EventCount;
}

void FEProfilingManager::StartProfiling()
{
	ClearData();
	bActive = true;
}

void FEProfilingManager::StopProfiling()
{
	bActive = false;
}

bool FEProfilingManager::IsActive() const
{
	return bActive;
}