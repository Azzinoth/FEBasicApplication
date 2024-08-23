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
	if (Topic.find("SCRIPT") != std::string::npos)
	{
		int y = 0;
		y++;
	}

	if (Severity < 0 || Severity >= SeverityLevelsCount)
	{
		Add("Incorrect severity argument" + std::to_string(Severity) + " in FELOG::add function", "IncorectCallArguments", FE_LOG_WARNING);
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

	std::fstream* file = nullptr;
	if (TopicFiles.find(Item->Topic) != TopicFiles.end())
	{
		file = TopicFiles[Item->Topic];
	}
	else
	{
		file = new std::fstream;
		file->open(Item->Topic + ".txt", std::ios::out);
		TopicFiles[Item->Topic] = file;
	}

	const std::string Line = Item->Text + '\n';
	file->write(Line.c_str(), Line.size());
	file->flush();
}

std::vector<LogItem> FELog::GetLogItems(const std::string Topic)
{
	std::vector<LogItem> result;
	auto iterator = Topics[Topic].begin();
	while (iterator != Topics[Topic].end())
	{
		result.push_back(iterator->second);
		iterator++;
	}

	return result;
}

std::string FELog::SeverityLevelToString(const LOG_SEVERITY Severity)
{
	std::string result;
	if (Severity < 0 || Severity >= SeverityLevelsCount)
	{
		Add("Incorrect severity argument" + std::to_string(Severity) + " in FELOG::SeverityLevelToString function", "IncorectCallArguments", FE_LOG_WARNING);
		return result;
	}

	switch (Severity)
	{
		case FocalEngine::FE_LOG_INFO:
		{
			result = "FE_LOG_INFO";
			break;
		}
		case FocalEngine::FE_LOG_DEBUG:
		{
			result = "FE_LOG_DEBUG";
			break;
		}
		case FocalEngine::FE_LOG_WARNING:
		{
			result = "FE_LOG_WARNING";
			break;
		}
		case FocalEngine::FE_LOG_ERROR:
		{
			result = "FE_LOG_ERROR";
			break;
		}
		case FocalEngine::FE_LOG_FATAL_ERROR:
		{
			result = "FE_LOG_FATAL_ERROR";
			break;
		}
	}

	return result;
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