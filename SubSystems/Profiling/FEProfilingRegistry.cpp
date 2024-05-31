#include "FEProfilingRegistry.h"
using namespace FocalEngine;

void FEProfilingRegistry::RecordFunctionEntry(const std::string& FunctionName, FE_CHRONO_TIME_POINT StartTimeStamp)
{
	CallStack.push_back(FunctionName);
	FunctionsData[FunctionName].Timestamps.push_back(std::make_pair(StartTimeStamp, StartTimeStamp));
}

void FEProfilingRegistry::RecordFunctionExit(const std::string& FunctionName, FE_CHRONO_TIME_POINT EndTimeStamp, double Duration)
{
	if (CallStack.empty() || FunctionsData.empty())
	{
		LOG.Add("CallStack or FunctionsData is empty in FEProfilingRegistry::RecordFunctionExit call.", "FE_LOG_PROFILING", FE_LOG_ERROR);
		return;
	}
	
	CallStack.pop_back();
	FunctionsData[FunctionName].Timestamps.back().second = EndTimeStamp;
	FunctionsData[FunctionName].TotalDuration += Duration;
	FunctionsData[FunctionName].CallCount++;

	if (!CallStack.empty())
	{
		const std::string& ParentFunction = CallStack.back();
		FunctionsData[FunctionName].ParentFunctions[ParentFunction]++;
	}

	EventCount++;
}

void FEProfilingRegistry::RecordSectionEntry(const std::string& SectionName, FE_CHRONO_TIME_POINT StartTimeStamp)
{
	if (CallStack.empty())
	{
		LOG.Add("CallStack is empty in FEProfilingRegistry::RecordSectionEntry call.", "FE_LOG_PROFILING", FE_LOG_ERROR);
		return;
	}

	FunctionsData[CallStack.back()].SectionTimestamps[SectionName].push_back(std::make_pair(StartTimeStamp, StartTimeStamp));
}

void FEProfilingRegistry::RecordSectionExit(const std::string& SectionName, FE_CHRONO_TIME_POINT EndTimeStamp, double Duration)
{
	if (CallStack.empty())
	{
		LOG.Add("CallStack is empty in FEProfilingRegistry::RecordSectionExit call.", "FE_LOG_PROFILING", FE_LOG_ERROR);
		return;
	}

	FunctionsData[CallStack.back()].SectionTimestamps[SectionName].back().second = EndTimeStamp;
}

void FEProfilingRegistry::ClearData()
{
	FunctionsData.clear();
	EventCount = 0;
}