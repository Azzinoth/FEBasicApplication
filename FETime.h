#pragma once

#include "windows.h"
#include <string>
#include <unordered_map>
#include <chrono>

#define SINGLETON_PUBLIC_PART(CLASS_NAME)  \
static CLASS_NAME& getInstance()           \
{										   \
	if (!_instance)                        \
		_instance = new CLASS_NAME();      \
	return *_instance;				       \
}                                          \
										   \
~CLASS_NAME();

#define SINGLETON_PRIVATE_PART(CLASS_NAME) \
static CLASS_NAME* _instance;              \
CLASS_NAME();                              \
CLASS_NAME(const CLASS_NAME &);            \
void operator= (const CLASS_NAME &);

namespace FocalEngine
{
#define chronoTimePoint std::chrono::time_point<std::chrono::high_resolution_clock>

	enum FE_TIME_RESOLUTION
	{
		FE_TIME_RESOLUTION_SECONDS = 0,
		FE_TIME_RESOLUTION_MILLISECONDS = 1,
		FE_TIME_RESOLUTION_MICROSECONS = 2,
		FE_TIME_RESOLUTION_NANOSECONDS = 3
	};

	class FETime
	{
		std::unordered_map<std::string, chronoTimePoint> timeStamps;

		SINGLETON_PRIVATE_PART(FETime)
	public:
		SINGLETON_PUBLIC_PART(FETime)

		void beginTimeStamp(std::string label = "");
		double endTimeStamp(std::string label = "", FE_TIME_RESOLUTION timeResolution = FE_TIME_RESOLUTION_MILLISECONDS);
	};

#define TIME FETime::getInstance()
}