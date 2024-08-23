#include "FEProfilingManager.h"
using namespace FocalEngine;

#ifdef FEBASICAPPLICATION_SHARED
extern "C" __declspec(dllexport) void* GetProfilingManager()
{
	return FEProfilingManager::GetInstancePointer();
}
#endif

FEProfilingManager::FEProfilingManager()
{
	RegisterThread(std::this_thread::get_id());
}

FEProfilingManager::~FEProfilingManager() {}

bool FEProfilingManager::IsThreadRegistered(const std::thread::id& ThreadID)
{
	std::shared_lock Lock(ThreadDataMutex);
	return ThreadData.find(ThreadID) != ThreadData.end();
}

void FEProfilingManager::RegisterThread(std::thread::id ThreadID)
{
	std::unique_lock Lock(ThreadDataMutex);
	ThreadData[ThreadID] = FEProfilingRegistry();
	ThreadData[ThreadID].TimeResolution = TimeResolution;
}

void FEProfilingManager::RecordFunctionEntry(const std::string& FunctionName, FE_CHRONO_TIME_POINT StartTimeStamp)
{
	if (!bActive)
		return;

	std::thread::id ThreadID = std::this_thread::get_id();
	if (!IsThreadRegistered(ThreadID))
		RegisterThread(ThreadID);

	// Do I need to lock the mutex here?
	std::unique_lock Lock(ThreadDataMutex);
	ThreadData[ThreadID].RecordFunctionEntry(FunctionName, StartTimeStamp);
	Lock.unlock();
}

void FEProfilingManager::RecordFunctionExit(const std::string& FunctionName, FE_CHRONO_TIME_POINT EndTimeStamp, double Duration)
{
	if (!bActive)
		return;

	std::thread::id ThreadID = std::this_thread::get_id();
	if (!IsThreadRegistered(ThreadID))
		RegisterThread(ThreadID);

	// Do I need to lock the mutex here?
	std::unique_lock Lock(ThreadDataMutex);
	ThreadData[ThreadID].RecordFunctionExit(FunctionName, EndTimeStamp, Duration);
	Lock.unlock();

	std::unique_lock EventCountLock(EventCountMutex);
	EventCount++;
}

void FEProfilingManager::RecordSectionEntry(const std::string& SectionName, FE_CHRONO_TIME_POINT StartTimeStamp)
{
	if (!bActive)
		return;

	std::thread::id ThreadID = std::this_thread::get_id();
	if (!IsThreadRegistered(ThreadID))
		RegisterThread(ThreadID);

	// Do I need to lock the mutex here?
	std::unique_lock Lock(ThreadDataMutex);
	ThreadData[ThreadID].RecordSectionEntry(SectionName, StartTimeStamp);
	Lock.unlock();
}

void FEProfilingManager::RecordSectionExit(const std::string& SectionName, FE_CHRONO_TIME_POINT EndTimeStamp, double Duration)
{
	if (!bActive)
		return;

	std::thread::id ThreadID = std::this_thread::get_id();
	if (!IsThreadRegistered(ThreadID))
		RegisterThread(ThreadID);

	// Do I need to lock the mutex here?
	std::unique_lock Lock(ThreadDataMutex);
	ThreadData[ThreadID].RecordSectionExit(SectionName, EndTimeStamp, Duration);
	Lock.unlock();

	std::unique_lock EventCountLock(EventCountMutex);
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

	File << "{\n";
	File << "  \"traceEvents\": [\n";

	auto ThreadIterator = ThreadData.begin();
	while (ThreadIterator != ThreadData.end())
	{
		std::thread::id ThreadID = ThreadIterator->first;
		std::stringstream ThreadIDString;
		ThreadIDString << ThreadID;

		bool bFirstEvent = true;
		for (auto& Entry : ThreadData[ThreadID].FunctionsData)
		{
			const std::string& FunctionName = Entry.first;
			const FEProfilingRegistry::FunctionInfo& Info = Entry.second;

			for (const auto& TimeStamp : Info.Timestamps)
			{
				if (!bFirstEvent)
					File << ",\n";
				
				bFirstEvent = false;

				File << "    {\n";
				File << "      \"name\": \"" << FunctionName << "\",\n";
				File << "      \"cat\": \"function\",\n";
				File << "      \"ph\": \"X\",\n";
				File << "      \"ts\": " << std::chrono::duration_cast<std::chrono::microseconds>(TimeStamp.first.time_since_epoch()).count() << ",\n";
				File << "      \"dur\": " << std::chrono::duration_cast<std::chrono::microseconds>(TimeStamp.second - TimeStamp.first).count() << ",\n";
				File << "      \"pid\": 0,\n";
				File << "      \"tid\": " << ThreadIDString.str() << "\n";
				File << "    }";
			}

			// Section events.
			for (const auto& SectionEntry : Info.SectionTimestamps) {
				const std::string& SectionName = SectionEntry.first;
				const auto& SectionTimestamps = SectionEntry.second;

				for (const auto& ts : SectionTimestamps) {
					if (!bFirstEvent) {
						File << ",\n";
					}
					bFirstEvent = false;

					File << "    {\n";
					File << "      \"name\": \"" << SectionName << "\",\n";
					File << "      \"cat\": \"section\",\n";
					File << "      \"ph\": \"X\",\n";
					File << "      \"ts\": " << std::chrono::duration_cast<std::chrono::microseconds>(ts.first.time_since_epoch()).count() << ",\n";
					File << "      \"dur\": " << std::chrono::duration_cast<std::chrono::microseconds>(ts.second - ts.first).count() << ",\n";
					File << "      \"pid\": 0,\n";
					File << "      \"tid\": " << ThreadID << ",\n";
					File << "      \"args\": {\n";
					File << "        \"function\": \"" << FunctionName << "\"\n";
					File << "      }\n";
					File << "    }";
				}
			}
		}

		ThreadIterator++;
		if (ThreadIterator != ThreadData.end())
		{
			File << ",\n";
		}
		else
		{
			File << "\n";
		}
	}

	File << "\n  ]\n";
	File << "}\n";

	File.close();
}

void FEProfilingManager::ClearData()
{
	std::unique_lock Lock(ThreadDataMutex);
	
	auto ThreadIterator = ThreadData.begin();
	while (ThreadIterator != ThreadData.end())
	{
		ThreadIterator->second.ClearData();
		ThreadIterator->second.EventCount = 0;

		ThreadIterator++;
	}

	ThreadData.clear();
	Lock.unlock();

	std::unique_lock EventCountLock(EventCountMutex);
	EventCount = 0;
}

int FEProfilingManager::GetEventCount()
{
	std::shared_lock EventCountLock(EventCountMutex);
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