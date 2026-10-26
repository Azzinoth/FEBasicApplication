#include "FELog.h"
#include <fstream>
#include <thread>

using namespace FocalEngine;

#ifdef FEBASICAPPLICATION_SHARED
extern "C" __declspec(dllexport) void* GetLog()
{
	return FELog::GetInstancePointer();
}
#endif

LogItem::LogItem()
{
	Text = "";
	Severity = FE_LOG_INFO;
	Count = 0;
}

LogItem::~LogItem() {}

FELog::FELog() {}
FELog::~FELog() {}

void FELog::Add(const std::string Text, const std::string Topic, const LOG_SEVERITY Severity)
{
	if (Severity < 0 || Severity >= SeverityLevelsCount)
	{
		Add("Incorrect severity argument" + std::to_string(Severity) + " in FELog::Add function", "IncorrectCallArguments", FE_LOG_WARNING);
		return;
	}

	LogItem TemporaryItem;
	TemporaryItem.Text = Text;
	TemporaryItem.Severity = Severity;
	TemporaryItem.Count = 0;
	TemporaryItem.Topic = Topic;
	TemporaryItem.TimeStamp = TIME.GetTimeStamp(FE_TIME_RESOLUTION_NANOSECONDS);
	TemporaryItem.ThreadID = std::hash<std::thread::id>{}(std::this_thread::get_id());

	if (bShouldAppendMsgWithTimeStamp)
	{
		TemporaryItem.Text += "\n";
		TemporaryItem.Text += "Time stamp: " + TIME.NanosecondTimeStampToDate(TemporaryItem.TimeStamp);
	}

	if (bShouldAppendMsgWithThreadID)
	{
		TemporaryItem.Text += "\n";
		TemporaryItem.Text += "ThreadID: " + std::to_string(TemporaryItem.ThreadID);
	}

	if (bShouldAppendMsgWithTimeStamp || bShouldAppendMsgWithThreadID)
	{
		TemporaryItem.Text += "\n";
		TemporaryItem.Text += "================================================================";
	}

	std::lock_guard<std::mutex> Lock(GlobalLogMutex);
		
	const auto CurrentLogItem = Topics[Topic].find(TemporaryItem);
	if (CurrentLogItem == Topics[Topic].end())
		Topics[Topic][TemporaryItem] = TemporaryItem;

	if (Topics[Topic][TemporaryItem].Count < 1000)
		Topics[Topic][TemporaryItem].Count++;

	OutputToFile(&Topics[Topic][TemporaryItem]);
}

void FELog::OutputToFile(const LogItem* Item)
{
	if (!bFileOutput)
		return;

	if (DisabledTopics.find(Item->Topic) != DisabledTopics.end())
		return;

	std::fstream* File = nullptr;
	if (TopicFiles.find(Item->Topic) != TopicFiles.end())
	{
		File = TopicFiles[Item->Topic];
	}
	else
	{
		File = new std::fstream;
		File->open(Item->Topic + ".txt", std::ios::out);
		TopicFiles[Item->Topic] = File;
	}

	const std::string Line = Item->Text + '\n';
	File->write(Line.c_str(), Line.size());
	File->flush();
}

std::vector<LogItem> FELog::GetLogItems(const std::string Topic)
{
	std::vector<LogItem> Result;

	std::lock_guard<std::mutex> Lock(GlobalLogMutex);
	const auto TopicIterator = Topics.find(Topic);
	if (TopicIterator == Topics.end())
		return Result;

	auto ItemIterator = TopicIterator->second.begin();
	while (ItemIterator != TopicIterator->second.end())
	{
		Result.push_back(ItemIterator->second);
		ItemIterator++;
	}

	return Result;
}

std::string FELog::SeverityLevelToString(const LOG_SEVERITY Severity)
{
	std::string Result;
	if (Severity < 0 || Severity >= SeverityLevelsCount)
	{
		Add("Incorrect severity argument" + std::to_string(Severity) + " in FELog::SeverityLevelToString function", "IncorrectCallArguments", FE_LOG_WARNING);
		return Result;
	}

	switch (Severity)
	{
		case FocalEngine::FE_LOG_INFO:
		{
			Result = "FE_LOG_INFO";
			break;
		}
		case FocalEngine::FE_LOG_DEBUG:
		{
			Result = "FE_LOG_DEBUG";
			break;
		}
		case FocalEngine::FE_LOG_WARNING:
		{
			Result = "FE_LOG_WARNING";
			break;
		}
		case FocalEngine::FE_LOG_ERROR:
		{
			Result = "FE_LOG_ERROR";
			break;
		}
		case FocalEngine::FE_LOG_FATAL_ERROR:
		{
			Result = "FE_LOG_FATAL_ERROR";
			break;
		}
	}

	return Result;
}

bool FELog::IsTopicFileOutputActive(const std::string Topic)
{
	std::lock_guard<std::mutex> Lock(GlobalLogMutex);
	if (DisabledTopics.find(Topic) == DisabledTopics.end())
		return true;

	return false;
}

void FELog::DisableTopicFileOutput(const std::string TopicToDisable)
{
	std::lock_guard<std::mutex> Lock(GlobalLogMutex);
	DisabledTopics[TopicToDisable] = true;
}

void FELog::EnableTopicFileOutput(const std::string TopicToEnable)
{
	std::lock_guard<std::mutex> Lock(GlobalLogMutex);
	DisabledTopics.erase(TopicToEnable);
}

bool FELog::IsFileOutputActive()
{
	return bFileOutput;
}

void FELog::SetFileOutput(const bool NewValue)
{
	bFileOutput = NewValue;
}

std::vector<std::string> FELog::GetTopicList()
{
	std::lock_guard<std::mutex> Lock(GlobalLogMutex);
	FE_MAP_TO_STR_VECTOR(Topics)
}

bool FELog::IsAppendingMsgWithTimeStamp()
{
	return bShouldAppendMsgWithTimeStamp;
}

void FELog::SetShouldAppendMsgWithTimeStamp(const bool NewValue)
{
	bShouldAppendMsgWithTimeStamp = NewValue;
}

bool FELog::IsAppendingMsgWithThreadID()
{
	return bShouldAppendMsgWithThreadID;
}

void FELog::SetShouldAppendMsgWithThreadID(const bool NewValue)
{
	bShouldAppendMsgWithThreadID = NewValue;
}