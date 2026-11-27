#include "FEScopedTimer.h"
using namespace FocalEngine;

FEScopedTimer::FEScopedTimer(const char* EventName, bool bIsSection)
{
	this->EventName = EventName;
	bIsForSection = bIsSection;

	bIsStopped = !PROFILING.IsActive();
	if (bIsStopped)
		return;

	if (!bIsForSection)
		PROFILING.RecordFunctionEntry(EventName);

	StartTime = std::chrono::high_resolution_clock::now();
}

void FEScopedTimer::Stop()
{
	// Not to stop twice if User calls Stop() manually and then destructor calls it again.
	if (bIsStopped)
		return;

	bIsStopped = true;

	FE_CHRONO_TIME_POINT EndTime = std::chrono::high_resolution_clock::now();

	const auto TimeElapsed = EndTime - StartTime;

	double Result = 0.0;
	switch (PROFILING.TimeResolution)
	{
	case FocalEngine::FE_TIME_RESOLUTION_SECONDS:
		Result = static_cast<double>(TimeElapsed.count()) * 0.000000001;
		break;
	case FocalEngine::FE_TIME_RESOLUTION_MILLISECONDS:
		Result = static_cast<double>(TimeElapsed.count()) * 0.000001;
		break;
	case FocalEngine::FE_TIME_RESOLUTION_MICROSECONDS:
		Result = static_cast<double>(TimeElapsed.count()) * 0.001;
		break;
	case FocalEngine::FE_TIME_RESOLUTION_NANOSECONDS:
		Result = static_cast<double>(TimeElapsed.count());
		break;
	default:
		break;
	}

	if (bIsForSection)
	{
		PROFILING.RecordSectionExit(EventName, StartTime, EndTime);
	}
	else
	{
		PROFILING.RecordFunctionExit(EventName, StartTime, EndTime, Result);
	}
}

FEScopedTimer::~FEScopedTimer()
{
	Stop();
}