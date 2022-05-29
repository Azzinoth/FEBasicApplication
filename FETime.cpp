#include "FETime.h"
using namespace FocalEngine;

FETime* FETime::_instance = nullptr;

FETime::FETime()
{

}

void FETime::beginTimeStamp(std::string label)
{
	timeStamps[label] = std::chrono::high_resolution_clock::now();
}

double FETime::endTimeStamp(std::string label, FE_TIME_RESOLUTION timeResolution)
{
	if (timeStamps.find(label) != timeStamps.end())
	{
		chronoTimePoint endTimePoint = std::chrono::high_resolution_clock::now();

		auto startTime = std::chrono::time_point_cast<std::chrono::nanoseconds>(timeStamps[label]).time_since_epoch();
		auto endTime = std::chrono::time_point_cast<std::chrono::nanoseconds>(endTimePoint).time_since_epoch();

		auto timeEscaped = endTime - startTime;

		switch (timeResolution)
		{
		case FocalEngine::FE_TIME_RESOLUTION_SECONDS:
			return double(timeEscaped.count()) * 0.000000001;
		case FocalEngine::FE_TIME_RESOLUTION_MILLISECONDS:
			return double(timeEscaped.count()) * 0.000001;
		case FocalEngine::FE_TIME_RESOLUTION_MICROSECONS:
			return double(timeEscaped.count()) * 0.001;
		case FocalEngine::FE_TIME_RESOLUTION_NANOSECONDS:
			return double(timeEscaped.count());
		default:
			break;
		}
	}
	
	return -1.0;
}