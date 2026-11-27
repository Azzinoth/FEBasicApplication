#include "FEProfilingRegistry.h"
using namespace FocalEngine;

void FEProfilingRegistry::RecordFunctionEntry(const char* FunctionName)
{
	CallStack.push_back(FunctionName);
}

void FEProfilingRegistry::RecordFunctionExit(const char* FunctionName, FE_CHRONO_TIME_POINT StartTimeStamp, FE_CHRONO_TIME_POINT EndTimeStamp, double Duration)
{
	if (CallStack.empty())
	{
		LOG.Add("CallStack is empty in FEProfilingRegistry::RecordFunctionExit call.", "FE_LOG_PROFILING", FE_LOG_ERROR);
		return;
	}

	CallStack.pop_back();
	FunctionInfo& Info = FunctionsData[FunctionName];
	Info.Timestamps.emplace_back(StartTimeStamp, EndTimeStamp);
	Info.TotalDuration += Duration;
	Info.CallCount++;

	if (!CallStack.empty())
		Info.ParentFunctions[CallStack.back()]++;

	EventCount++;
}

void FEProfilingRegistry::RecordSectionExit(const char* SectionName, FE_CHRONO_TIME_POINT StartTimeStamp, FE_CHRONO_TIME_POINT EndTimeStamp)
{
	if (CallStack.empty())
	{
		LOG.Add("CallStack is empty in FEProfilingRegistry::RecordSectionExit call.", "FE_LOG_PROFILING", FE_LOG_ERROR);
		return;
	}

	FunctionsData[CallStack.back()].SectionTimestamps[SectionName].emplace_back(StartTimeStamp, EndTimeStamp);
}

void FEProfilingRegistry::ClearData()
{
	FunctionsData.clear();
	CallStack.clear();
	EventCount = 0;
}