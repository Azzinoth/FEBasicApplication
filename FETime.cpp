#include "FETime.h"
using namespace FocalEngine;

FETime* FETime::Instance = nullptr;

FETime::FETime()
{

}

void FETime::BeginTimeStamp(const std::string Label)
{
	TimeStamps[Label] = std::chrono::high_resolution_clock::now();
}

double FETime::EndTimeStamp(const std::string Label, const FE_TIME_RESOLUTION TimeResolution)
{
	if (TimeStamps.find(Label) != TimeStamps.end())
	{
		const FE_CHRONO_TIME_POINT EndTimePoint = std::chrono::high_resolution_clock::now();

		const auto StartTime = std::chrono::time_point_cast<std::chrono::nanoseconds>(TimeStamps[Label]).time_since_epoch();
		const auto EndTime = std::chrono::time_point_cast<std::chrono::nanoseconds>(EndTimePoint).time_since_epoch();

		const auto TimeEscaped = EndTime - StartTime;

		switch (TimeResolution)
		{
		case FocalEngine::FE_TIME_RESOLUTION_SECONDS:
			return static_cast<double>(TimeEscaped.count()) * 0.000000001;
		case FocalEngine::FE_TIME_RESOLUTION_MILLISECONDS:
			return static_cast<double>(TimeEscaped.count()) * 0.000001;
		case FocalEngine::FE_TIME_RESOLUTION_MICROSECONS:
			return static_cast<double>(TimeEscaped.count()) * 0.001;
		case FocalEngine::FE_TIME_RESOLUTION_NANOSECONDS:
			return static_cast<double>(TimeEscaped.count());
		default:
			break;
		}
	}
	
	return -1.0;
}