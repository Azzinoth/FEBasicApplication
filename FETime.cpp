#include "FETime.h"
using namespace FocalEngine;

#ifdef FEBASICAPPLICATION_SHARED
extern "C" __declspec(dllexport) void* GetTime()
{
	return FETime::GetInstancePointer();
}
#endif

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

		const auto TimeElapsed = EndTime - StartTime;

		switch (TimeResolution)
		{
		case FocalEngine::FE_TIME_RESOLUTION_SECONDS:
			return static_cast<double>(TimeElapsed.count()) * 0.000000001;
		case FocalEngine::FE_TIME_RESOLUTION_MILLISECONDS:
			return static_cast<double>(TimeElapsed.count()) * 0.000001;
		case FocalEngine::FE_TIME_RESOLUTION_MICROSECONDS:
			return static_cast<double>(TimeElapsed.count()) * 0.001;
		case FocalEngine::FE_TIME_RESOLUTION_NANOSECONDS:
			return static_cast<double>(TimeElapsed.count());
		default:
			break;
		}
	}
	
	return -1.0;
}

uint64_t FETime::GetTimeStamp(const FE_TIME_RESOLUTION TimeResolution)
{
	uint64_t Result = std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::system_clock::now().time_since_epoch()).count();

	if (TimeResolution == FE_TIME_RESOLUTION_MICROSECONDS)
	{
		Result /= static_cast<uint64_t>(pow(10.0, 3));
	}
	else if (TimeResolution == FE_TIME_RESOLUTION_MILLISECONDS)
	{
		Result /= static_cast<uint64_t>(pow(10.0, 6));
	}
	else if (TimeResolution == FE_TIME_RESOLUTION_SECONDS)
	{
		Result /= static_cast<uint64_t>(pow(10.0, 9));
	}

	return Result;
}

std::string FETime::NanosecondTimeStampToDate(uint64_t NanosecondsSinceEpoch)
{
	auto FillZeros = [&](std::string Data) {
		if (Data.size() == 2)
			Data.insert(0, "0");

		if (Data.size() == 1)
			Data.insert(0, "00");

		return Data;
	};

	if (NanosecondsSinceEpoch == 0)
		NanosecondsSinceEpoch = std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::system_clock::now().time_since_epoch()).count();

	const std::time_t Seconds = NanosecondsSinceEpoch / static_cast<uint64_t>(pow(10.0, 9));

	char TempStr[26];
	ctime_s(TempStr, 26, &Seconds);
	std::string Result = TempStr;

	// Deleting day of week.
	Result.erase(Result.begin(), Result.begin() + 4);
	// Deleting new line symbol.
	Result.erase(Result.end() - 1, Result.end());
	// Moving year.
	Result.insert(7, Result.substr(Result.size() - 4, 4) + " ");
	Result.erase(Result.size() - 5, 5);
	// Add milliseconds.
	const uint64_t Milliseconds = (NanosecondsSinceEpoch - Seconds * static_cast<uint64_t>(pow(10.0, 9))) / static_cast<uint64_t>(pow(10.0, 6));
	Result.insert(Result.size(), "." + FillZeros(std::to_string(Milliseconds)));
	// Add microsecond.
	const uint64_t Microseconds = (NanosecondsSinceEpoch - Seconds * static_cast<uint64_t>(pow(10.0, 9)) - Milliseconds * static_cast<uint64_t>(pow(10.0, 6))) / static_cast<uint64_t>(pow(10.0, 3));
	Result.insert(Result.size(), "." + FillZeros(std::to_string(Microseconds)));
	// Add nanosecond.
	const uint64_t Nanoseconds = NanosecondsSinceEpoch - Seconds * static_cast<uint64_t>(pow(10.0, 9)) - Milliseconds * static_cast<uint64_t>(pow(10.0, 6)) - Microseconds * static_cast<uint64_t>(pow(10.0, 3));
	Result.insert(Result.size(), "." + FillZeros(std::to_string(Nanoseconds)));

	return Result;
}

std::string FETime::TimeToFormattedString(uint64_t TimeValue, FE_TIME_RESOLUTION Resolution)
{
	// Convert input time to nanoseconds based on resolution.
	std::chrono::nanoseconds Duration;
	switch (Resolution)
	{
	case FE_TIME_RESOLUTION_SECONDS:
		Duration = std::chrono::seconds(TimeValue);
		break;
	case FE_TIME_RESOLUTION_MILLISECONDS:
		Duration = std::chrono::milliseconds(TimeValue);
		break;
	case FE_TIME_RESOLUTION_MICROSECONDS:
		Duration = std::chrono::microseconds(TimeValue);
		break;
	case FE_TIME_RESOLUTION_NANOSECONDS:
		Duration = std::chrono::nanoseconds(TimeValue);
		break;
	default:
		return "Invalid time resolution";
	}

	// Extract hours.
	auto Hours = std::chrono::duration_cast<std::chrono::hours>(Duration);
	Duration -= Hours;

	// Extract minutes.
	auto Minutes = std::chrono::duration_cast<std::chrono::minutes>(Duration);
	Duration -= Minutes;

	// Extract seconds.
	auto Seconds = std::chrono::duration_cast<std::chrono::seconds>(Duration);
	Duration -= Seconds;

	// Extract milliseconds.
	auto Milliseconds = std::chrono::duration_cast<std::chrono::milliseconds>(Duration);

	std::string Result;

	// Build the formatted string.
	if (Hours.count() > 0)
		Result += std::to_string(Hours.count()) + " hour" + (Hours.count() > 1 ? "s " : " ");

	if (Minutes.count() > 0)
		Result += std::to_string(Minutes.count()) + " minute" + (Minutes.count() > 1 ? "s " : " ");

	if (Seconds.count() > 0)
		Result += std::to_string(Seconds.count()) + " second" + (Seconds.count() > 1 ? "s " : " ");

	// Handle case when duration is 0.
	if (Result.empty())
		Result = "0 seconds";

	// Remove trailing space if exists.
	if (!Result.empty() && Result.back() == ' ')
		Result.pop_back();

	return Result;
}