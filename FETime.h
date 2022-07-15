#pragma once

#include "windows.h"
#include <string>
#include <unordered_map>
#include <chrono>

#define SINGLETON_PUBLIC_PART(CLASS_NAME)  \
static CLASS_NAME& getInstance()           \
{										   \
	if (!Instance)                         \
		Instance = new CLASS_NAME();       \
	return *Instance;				       \
}                                          \
										   \
~CLASS_NAME();

#define SINGLETON_PRIVATE_PART(CLASS_NAME) \
static CLASS_NAME* Instance;               \
CLASS_NAME();                              \
CLASS_NAME(const CLASS_NAME &);            \
void operator= (const CLASS_NAME &);

namespace FocalEngine
{
#define FE_CHRONO_TIME_POINT std::chrono::time_point<std::chrono::high_resolution_clock>

	enum FE_TIME_RESOLUTION
	{
		FE_TIME_RESOLUTION_SECONDS = 0,
		FE_TIME_RESOLUTION_MILLISECONDS = 1,
		FE_TIME_RESOLUTION_MICROSECONS = 2,
		FE_TIME_RESOLUTION_NANOSECONDS = 3
	};

	class FETime
	{
		std::unordered_map<std::string, FE_CHRONO_TIME_POINT> TimeStamps;

		SINGLETON_PRIVATE_PART(FETime)
	public:
		SINGLETON_PUBLIC_PART(FETime)

		void BeginTimeStamp(std::string Label = "");
		double EndTimeStamp(std::string Label = "", FE_TIME_RESOLUTION TimeResolution = FE_TIME_RESOLUTION_MILLISECONDS);
	};

#define TIME FETime::getInstance()
}