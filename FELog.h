#pragma once

#include "FETime.h"

namespace FocalEngine
{
	enum LOG_SEVERITY
	{
		FE_LOG_INFO = 0,
		FE_LOG_DEBUG = 1,
		FE_LOG_WARNING = 2,
		FE_LOG_ERROR = 3,
		FE_LOG_FATAL_ERROR = 4
	};

	struct LogItem
	{
		std::string Text;
		LOG_SEVERITY Severity;
		int Count;
		std::string Topic;
		uint64_t TimeStamp;
		DWORD ThreadID;

		LogItem();
		~LogItem();

		bool operator==(const LogItem& Other) const
		{
			return (Text == Other.Text && Severity == Other.Severity);
		}
	};
}

namespace std
{
	template <> struct std::hash<FocalEngine::LogItem>
	{
		size_t operator()(const FocalEngine::LogItem& Object) const
		{
			return ((std::hash<std::string>()(Object.Text) ^ (Object.Severity << 1)) >> 1);
		}
	};
}

namespace FocalEngine
{
	class FEBASICAPPLICATION_API FELog
	{
	public:
		SINGLETON_PUBLIC_PART(FELog)

		void Add(std::string Text, std::string Topic = "FE_LOG_GENERAL", LOG_SEVERITY Severity = FE_LOG_INFO);

		std::vector<LogItem> GetLogItems(std::string Topic);

		const int SeverityLevelsCount = 5;

		std::string SeverityLevelToString(LOG_SEVERITY Severity);

		bool IsTopicFileOutputActive(std::string Topic);
		void DisableTopicFileOutput(std::string TopicToDisable);
		void EnableTopicFileOutput(std::string TopicToEnable);

		bool IsFileOutputActive();
		void SetFileOutput(bool NewValue);

		std::vector<std::string> GetTopicList();

		bool IsAppendingMsgWithTimeStamp();
		void SetShouldAppendMsgWithTimeStamp(bool NewValue);

		bool IsAppendingMsgWithThreadID();
		void SetShouldAppendMsgWithThreadID(bool NewValue);
	private:
		SINGLETON_PRIVATE_PART(FELog)

		std::unordered_map<std::string, std::unordered_map<LogItem, LogItem>> Topics;

		std::unordered_map<std::string, bool> DisabledTopics;
		bool bFileOutput = false;

		std::unordered_map<std::string, std::fstream*> TopicFiles;
		void OutputToFile(const LogItem* Item);

		bool bShouldAppendMsgWithTimeStamp = false;
		bool bShouldAppendMsgWithThreadID = false;
	};

#ifdef FEBASICAPPLICATION_SHARED
	extern "C" __declspec(dllexport) void* GetLog();
	#define LOG (*static_cast<FELog*>(GetLog()))
#else
	#define LOG FELog::GetInstance()
#endif
}