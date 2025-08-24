#include "FELog.h"
#include <fstream>

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
		Add("Incorrect severity argument" + std::to_string(Severity) + " in FELOG::add function", "IncorrectCallArguments", FE_LOG_WARNING);
		return;
	}

	LogItem TempItem;
	TempItem.Text = Text;
	TempItem.Severity = Severity;
	TempItem.Count = 0;
	TempItem.Topic = Topic;
	TempItem.TimeStamp = TIME.GetTimeStamp(FE_TIME_RESOLUTION_NANOSECONDS);
	TempItem.ThreadID = GetCurrentThreadId();

	if (bShouldAppendMsgWithTimeStamp)
	{
		TempItem.Text += "\n";
		TempItem.Text += "Time stamp: " + TIME.NanosecondTimeStampToDate(TempItem.TimeStamp);
	}

	if (bShouldAppendMsgWithThreadID)
	{
		TempItem.Text += "\n";
		TempItem.Text += "ThreadID: " + std::to_string(TempItem.ThreadID);
	}

	if (bShouldAppendMsgWithTimeStamp || bShouldAppendMsgWithThreadID)
	{
		TempItem.Text += "\n";
		TempItem.Text += "================================================================";
	}

	std::lock_guard<std::mutex> Lock(GlobalLogMutex);
		
	const auto CurrentLogItem = Topics[Topic].find(TempItem);
	if (CurrentLogItem == Topics[Topic].end())
		Topics[Topic][TempItem] = TempItem;

	if (Topics[Topic][TempItem].Count < 1000)
		Topics[Topic][TempItem].Count++;

	OutputToFile(&Topics[Topic][TempItem]);
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
	auto TopicIterator = Topics[Topic].begin();
	while (TopicIterator != Topics[Topic].end())
	{
		Result.push_back(TopicIterator->second);
		TopicIterator++;
	}

	return Result;
}

std::string FELog::SeverityLevelToString(const LOG_SEVERITY Severity)
{
	std::string Result;
	if (Severity < 0 || Severity >= SeverityLevelsCount)
	{
		Add("Incorrect severity argument" + std::to_string(Severity) + " in FELOG::SeverityLevelToString function", "IncorrectCallArguments", FE_LOG_WARNING);
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
	if (DisabledTopics.find(Topic) == DisabledTopics.end())
		return true;

	return false;
}

void FELog::DisableTopicFileOutput(const std::string TopicToDisable)
{
	DisabledTopics[TopicToDisable] = true;
}

void FELog::EnableTopicFileOutput(const std::string TopicToEnable)
{
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