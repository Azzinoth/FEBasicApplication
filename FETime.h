#pragma once

#include "FEUniqueID.h"

namespace FocalEngine
{
#define FE_CHRONO_TIME_POINT std::chrono::time_point<std::chrono::high_resolution_clock>

	enum FE_TIME_RESOLUTION
	{
		FE_TIME_RESOLUTION_SECONDS = 0,
		FE_TIME_RESOLUTION_MILLISECONDS = 1,
		FE_TIME_RESOLUTION_MICROSECONDS = 2,
		FE_TIME_RESOLUTION_NANOSECONDS = 3
	};

	class FEBASICAPPLICATION_API FETime
	{
		std::unordered_map<std::string, FE_CHRONO_TIME_POINT> TimeStamps;

		SINGLETON_PRIVATE_PART(FETime)
	public:
		SINGLETON_PUBLIC_PART(FETime)

		void BeginTimeStamp(std::string Label = "");
		double EndTimeStamp(std::string Label = "", FE_TIME_RESOLUTION TimeResolution = FE_TIME_RESOLUTION_MILLISECONDS);
	
		uint64_t GetTimeStamp(FE_TIME_RESOLUTION TimeResolution = FE_TIME_RESOLUTION_MILLISECONDS);

		std::string NanosecondTimeStampToDate(uint64_t NanosecondsSinceEpoch = 0);
		std::string TimeToFormattedString(uint64_t TimeValue, FE_TIME_RESOLUTION Resolution = FE_TIME_RESOLUTION_NANOSECONDS);
	};

#ifdef FEBASICAPPLICATION_SHARED
	extern "C" __declspec(dllexport) void* GetTime();
	#define TIME (*static_cast<FETime*>(GetTime()))
#else
	#define TIME FETime::GetInstance()
#endif
}