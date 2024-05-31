#include "FEScopedTimer.h"
using namespace FocalEngine;

FEScopedTimer::FEScopedTimer(const std::string& EventName, bool bIsSection)
{
	StartTime = std::chrono::steady_clock::now();
    this->EventName = EventName;
	bIsForSection = bIsSection;
	if (bIsForSection)
	{
		PROFILING.RecordSectionEntry(EventName, StartTime);
	}
	else
	{
		PROFILING.RecordFunctionEntry(EventName, StartTime);
	}
}

void FEScopedTimer::Stop()
{
	// Not to stop twice if User calls Stop() manually and then destructor calls it again.
	if (bIsStopped)
		return;

	bIsStopped = true;

	FE_CHRONO_TIME_POINT EndTime = std::chrono::steady_clock::now();

	const auto Start = std::chrono::time_point_cast<std::chrono::nanoseconds>(StartTime).time_since_epoch();
	const auto End = std::chrono::time_point_cast<std::chrono::nanoseconds>(EndTime).time_since_epoch();

	const auto TimeEscaped = EndTime - StartTime;

	double Result = 0.0;
	switch (PROFILING.TimeResolution)
	{
	case FocalEngine::FE_TIME_RESOLUTION_SECONDS:
		Result = static_cast<double>(TimeEscaped.count()) * 0.000000001;
		break;
	case FocalEngine::FE_TIME_RESOLUTION_MILLISECONDS:
		Result = static_cast<double>(TimeEscaped.count()) * 0.000001;
		break;
	case FocalEngine::FE_TIME_RESOLUTION_MICROSECONS:
		Result = static_cast<double>(TimeEscaped.count()) * 0.001;
		break;
	case FocalEngine::FE_TIME_RESOLUTION_NANOSECONDS:
		Result = static_cast<double>(TimeEscaped.count());
		break;
	default:
		break;
	}

	if (bIsForSection)
	{
		PROFILING.RecordSectionExit(EventName, EndTime, Result);
	}
	else
	{
		PROFILING.RecordFunctionExit(EventName, EndTime, Result);
	}
}

FEScopedTimer::~FEScopedTimer()
{
	Stop();
}