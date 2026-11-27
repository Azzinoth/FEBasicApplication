#include "FEProfilingManager.h"
#include <iomanip>
#include <algorithm>
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

	struct TimelineEvent
	{
		const char* Name = nullptr;
		// Function that section belongs to, nullptr for function events.
		const char* SectionFunctionName = nullptr;
		FE_CHRONO_TIME_POINT Start;
		FE_CHRONO_TIME_POINT End;
	};

	bool bFirstEvent = true;
	std::vector<TimelineEvent> Events;
	for (auto& [ThreadID, Registry] : ThreadData)
	{
		std::lock_guard RegistryLock(Registry.DataMutex);

		Events.clear();
		for (auto& [FunctionName, Info] : Registry.FunctionsData)
		{
			for (const auto& TimeStamp : Info.Timestamps)
				Events.push_back({ FunctionName, nullptr, TimeStamp.first, TimeStamp.second });

			for (const auto& [SectionName, SectionTimestamps] : Info.SectionTimestamps)
			{
				for (const auto& SectionTimestamp : SectionTimestamps)
					Events.push_back({ SectionName, FunctionName, SectionTimestamp.first, SectionTimestamp.second });
			}
		}

		// Trace viewers nest events that start at the same time in file order, so parent has to be written first.
		std::sort(Events.begin(), Events.end(), [](const TimelineEvent& First, const TimelineEvent& Second) {
			if (First.Start != Second.Start)
				return First.Start < Second.Start;

			return First.End > Second.End;
		});

		std::stringstream ThreadIDString;
		ThreadIDString << ThreadID;
		for (const TimelineEvent& Event : Events)
		{
			if (!bFirstEvent)
				File << ",\n";
			bFirstEvent = false;

			File << "    {\"name\": \"" << Event.Name << "\", \"cat\": \"" << (Event.SectionFunctionName == nullptr ? "function" : "section") << "\", \"ph\": \"X\", \"ts\": " << ToMicroseconds(Event.Start.time_since_epoch())
				 << ", \"dur\": " << ToMicroseconds(Event.End - Event.Start) << ", \"pid\": 0, \"tid\": " << ThreadIDString.str();

			if (Event.SectionFunctionName != nullptr)
				File << ", \"args\": {\"function\": \"" << Event.SectionFunctionName << "\"}";

			File << "}";
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