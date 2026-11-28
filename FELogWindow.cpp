#include "FELogWindow.h"
#include <algorithm>

using namespace FocalEngine;

#ifdef FEBASICAPPLICATION_SHARED
extern "C" __declspec(dllexport) void* GetLogWindow()
{
	return FELogWindow::GetInstancePointer();
}
#endif

// Levels of the filter buttons: info, debug, warning and error (with fatal errors).
static const int LogLevelCount = 4;

// Sizes of the window are set for this font size, they change with the font of the application.
static const float LogWindowBaseFontSize = 20.0f;

struct FocalEngine::FELogWindowData
{
	std::string Channel;
	std::string TextFilter;
	bool bLevelShown[LogLevelCount] = { true, true, true, true };
	bool bFollow = true;
	uint64_t ClearedAt = 0;

	// Copy of the messages of the log, it is updated a few times a second.
	std::vector<std::string> Channels;
	std::vector<std::vector<LogItem>> ChannelItems;
	// Messages of all channels in order of time.
	std::vector<const LogItem*> Items;
	std::vector<size_t> ChannelCounts;
	std::vector<LOG_SEVERITY> ChannelWorstSeverity;
	LOG_SEVERITY WorstSeverity = FE_LOG_INFO;
	uint64_t LastUpdateTime = 0;
	double LastUpdateDuration = 0.0;
	bool bUpdateRequested = true;

	// Messages that pass the filters.
	std::vector<const LogItem*> ShownItems;
	size_t ShownCount = 0;
	size_t LevelCounts[LogLevelCount] = { 0, 0, 0, 0 };
	std::string AppliedFilters;
	int FramesToScrollToEnd = 0;
};

static float GetLogWindowScale()
{
	return ImGui::GetFontSize() / LogWindowBaseFontSize;
}

static FELogWindowColors GetColorsOfImGuiStyle()
{
	FELogWindowColors Result;
	Result.Text = ImGui::GetColorU32(ImGuiCol_Text);
	Result.TextMuted = ImGui::GetColorU32(ImGuiCol_Text, 0.7f);
	Result.TextDisabled = ImGui::GetColorU32(ImGuiCol_TextDisabled);

	Result.Button = ImGui::GetColorU32(ImGuiCol_Button);
	Result.ButtonHovered = ImGui::GetColorU32(ImGuiCol_ButtonHovered);
	Result.Border = ImGui::GetColorU32(ImGuiCol_Border);

	Result.ChannelListBackground = ImGui::GetColorU32(ImGuiCol_FrameBg);
	Result.SelectedChannel = ImGui::GetColorU32(ImGuiCol_Header);
	Result.HoveredChannel = ImGui::GetColorU32(ImGuiCol_HeaderHovered);

	Result.TableHeader = ImGui::GetColorU32(ImGuiCol_TableHeaderBg);
	Result.TableBorder = ImGui::GetColorU32(ImGuiCol_TableBorderLight);
	Result.TableAlternateRow = ImGui::GetColorU32(ImGuiCol_TableRowBgAlt);

	Result.Info = Result.TextMuted;
	Result.Debug = IM_COL32(0x00, 0xA2, 0xE8, 255);
	Result.Warning = IM_COL32(0xF0, 0xB8, 0x40, 255);
	Result.Error = IM_COL32(0xFF, 0x7A, 0x70, 255);
	return Result;
}

static ImU32 GetLogSeverityColor(const FELogWindowColors& Colors, const LOG_SEVERITY Severity)
{
	if (Severity == FE_LOG_WARNING)
		return Colors.Warning;

	if (Severity == FE_LOG_ERROR || Severity == FE_LOG_FATAL_ERROR)
		return Colors.Error;

	if (Severity == FE_LOG_DEBUG)
		return Colors.Debug;

	return Colors.Info;
}

static const char* GetLogSeverityName(const LOG_SEVERITY Severity)
{
	switch (Severity)
	{
		case FE_LOG_INFO: return "Info";
		case FE_LOG_DEBUG: return "Debug";
		case FE_LOG_WARNING: return "Warning";
		case FE_LOG_ERROR: return "Error";
		case FE_LOG_FATAL_ERROR: return "Fatal";
		default: return "";
	}
}

static int GetLogLevelIndex(const LOG_SEVERITY Severity)
{
	if (Severity == FE_LOG_FATAL_ERROR)
		return 3;

	return static_cast<int>(Severity);
}

// "FE_LOG_LOADING" is shown as "Loading", "FE_SCRIPT_SYSTEM" as "Script system".
static std::string GetLogChannelName(std::string Channel)
{
	if (Channel.rfind("FE_LOG_", 0) == 0)
	{
		Channel = Channel.substr(7);
	}
	else if (Channel.rfind("FE_", 0) == 0)
	{
		Channel = Channel.substr(3);
	}

	for (size_t i = 0; i < Channel.size(); i++)
	{
		if (Channel[i] == '_')
		{
			Channel[i] = ' ';
		}
		else if (i > 0)
		{
			Channel[i] = static_cast<char>(std::tolower(static_cast<unsigned char>(Channel[i])));
		}
	}

	return Channel;
}

// Hours, minutes and seconds of the date that TIME gives, it looks like "Nov 27 2026 21:14:05.123.456.789".
static std::string GetLogTimeText(const uint64_t NanosecondsSinceEpoch)
{
	const std::string Date = TIME.NanosecondTimeStampToDate(NanosecondsSinceEpoch);
	return Date.size() >= 20 ? Date.substr(12, 8) : Date;
}

// The log stops counting a message after 1000 repeats.
static std::string GetLogCountText(const int Count)
{
	if (Count <= 1)
		return "";

	return Count < 1000 ? "x" + std::to_string(Count) : "x1000+";
}

static std::string GetLogNumberWithSeparators(const size_t Number)
{
	std::string Result = std::to_string(Number);
	for (int Position = static_cast<int>(Result.size()) - 3; Position > 0; Position -= 3)
		Result.insert(Position, ",");

	return Result;
}

static std::string GetLogLowerCaseText(std::string Text)
{
	std::transform(Text.begin(), Text.end(), Text.begin(), [](const unsigned char Character) { return static_cast<char>(std::tolower(Character)); });
	return Text;
}

static void ShowLogToolTip(const char* Text)
{
	if (ImGui::IsItemHovered())
	{
		ImGui::BeginTooltip();
		ImGui::PushTextWrapPos(ImGui::GetFontSize() * 35.0f);
		ImGui::TextUnformatted(Text);
		ImGui::PopTextWrapPos();
		ImGui::EndTooltip();
	}
}

static void UpdateLogItems(FELogWindowData& Data)
{
	TIME.BeginTimeStamp("LogWindowUpdate");

	// Shown items point to the old copies.
	Data.ShownItems.clear();
	Data.Items.clear();

	Data.Channels = LOG.GetTopicList();
	Data.ChannelItems.resize(Data.Channels.size());
	Data.ChannelCounts.assign(Data.Channels.size(), 0);
	Data.ChannelWorstSeverity.assign(Data.Channels.size(), FE_LOG_INFO);
	Data.WorstSeverity = FE_LOG_INFO;
	for (size_t i = 0; i < Data.Channels.size(); i++)
	{
		Data.ChannelItems[i] = LOG.GetLogItems(Data.Channels[i]);
		for (size_t j = 0; j < Data.ChannelItems[i].size(); j++)
		{
			const LogItem& Item = Data.ChannelItems[i][j];
			if (Item.TimeStamp <= Data.ClearedAt)
				continue;

			// Debug messages do not make a channel look worse than its info messages do.
			if (Item.Severity != FE_LOG_DEBUG && Item.Severity > Data.ChannelWorstSeverity[i])
				Data.ChannelWorstSeverity[i] = Item.Severity;

			Data.ChannelCounts[i]++;
			Data.Items.push_back(&Item);
		}

		if (Data.ChannelWorstSeverity[i] > Data.WorstSeverity)
			Data.WorstSeverity = Data.ChannelWorstSeverity[i];
	}

	std::sort(Data.Items.begin(), Data.Items.end(), [](const LogItem* A, const LogItem* B) -> bool { return A->TimeStamp < B->TimeStamp; });

	Data.AppliedFilters = "";
	Data.bUpdateRequested = false;
	Data.LastUpdateTime = TIME.GetTimeStamp(FE_TIME_RESOLUTION_MILLISECONDS);
	Data.LastUpdateDuration = TIME.EndTimeStamp("LogWindowUpdate");
}

static void ApplyLogFilters(FELogWindowData& Data)
{
	Data.ShownItems.clear();
	for (int i = 0; i < LogLevelCount; i++)
		Data.LevelCounts[i] = 0;

	const std::string LowerCaseFilter = GetLogLowerCaseText(Data.TextFilter);
	for (size_t i = 0; i < Data.Items.size(); i++)
	{
		const LogItem* Item = Data.Items[i];
		if (!Data.Channel.empty() && Item->Topic != Data.Channel)
			continue;

		// Counts of the level buttons are for the selected channel, whatever the levels and the text hide.
		const int Level = GetLogLevelIndex(Item->Severity);
		Data.LevelCounts[Level]++;
		if (!Data.bLevelShown[Level])
			continue;

		if (!LowerCaseFilter.empty() && GetLogLowerCaseText(Item->Text).find(LowerCaseFilter) == std::string::npos)
			continue;

		Data.ShownItems.push_back(Item);
	}

	if (Data.ShownItems.size() != Data.ShownCount)
		Data.FramesToScrollToEnd = 2;

	Data.ShownCount = Data.ShownItems.size();
}

// Button of a level with the number of its messages, it shows or hides that level.
static bool RenderLogLevelButton(const FELogWindowColors& Colors, const char* Name, const ImU32 Color, const size_t Count, const bool bShown)
{
	const float Scale = GetLogWindowScale();
	const std::string Label = std::string(Name) + " " + GetLogNumberWithSeparators(Count);
	const ImVec2 Size = ImVec2(ImGui::CalcTextSize(Label.c_str()).x + 38.0f * Scale, ImGui::GetFrameHeight());
	const ImVec2 Min = ImGui::GetCursorScreenPos();
	const ImVec2 Max = ImVec2(Min.x + Size.x, Min.y + Size.y);
	const bool bPressed = ImGui::InvisibleButton(Name, Size);

	ImDrawList* DrawList = ImGui::GetWindowDrawList();
	if (bShown)
		DrawList->AddRectFilled(Min, Max, ImGui::IsItemHovered() ? Colors.ButtonHovered : Colors.Button, 4.0f);

	DrawList->AddRect(Min, Max, Colors.Border, 4.0f);
	DrawList->AddCircleFilled(ImVec2(Min.x + 14.0f * Scale, floorf(Min.y + Size.y / 2.0f)), 5.0f * Scale, bShown ? Color : Colors.TextDisabled);
	DrawList->AddText(ImVec2(Min.x + 26.0f * Scale, Min.y + (Size.y - ImGui::GetFontSize()) / 2.0f), bShown ? Colors.Text : Colors.TextDisabled, Label.c_str());
	return bPressed;
}

static void RenderLogChannelList(const FELogWindowColors& Colors, FELogWindowData& Data, const ImVec2 Size)
{
	const float Scale = GetLogWindowScale();
	ImGui::PushStyleColor(ImGuiCol_ChildBg, Colors.ChannelListBackground);
	ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, 4.0f);
	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(6.0f, 6.0f));
	ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(8.0f, 2.0f));
	ImGui::BeginChild("##LogChannels", Size, ImGuiChildFlags_AlwaysUseWindowPadding);
	ImDrawList* DrawList = ImGui::GetWindowDrawList();

	// The first row is for all channels.
	for (int i = -1; i < static_cast<int>(Data.Channels.size()); i++)
	{
		const std::string Channel = i == -1 ? "" : Data.Channels[i];
		const size_t Count = i == -1 ? Data.Items.size() : Data.ChannelCounts[i];
		const LOG_SEVERITY WorstSeverity = i == -1 ? Data.WorstSeverity : Data.ChannelWorstSeverity[i];
		const bool bSelected = Data.Channel == Channel;
		if (i != -1 && Count == 0 && !bSelected)
			continue;

		const ImVec2 Min = ImGui::GetCursorScreenPos();
		const ImVec2 RowSize = ImVec2(ImGui::GetContentRegionAvail().x, ImGui::GetFrameHeight() + 4.0f);
		const ImVec2 Max = ImVec2(Min.x + RowSize.x, Min.y + RowSize.y);
		ImGui::PushID(i);
		if (ImGui::InvisibleButton("##Channel", RowSize))
			Data.Channel = Channel;
		ImGui::PopID();

		if (bSelected)
		{
			DrawList->AddRectFilled(Min, Max, Colors.SelectedChannel, 4.0f);
		}
		else if (ImGui::IsItemHovered())
		{
			DrawList->AddRectFilled(Min, Max, Colors.HoveredChannel, 4.0f);
		}

		const float TextY = Min.y + (RowSize.y - ImGui::GetFontSize()) / 2.0f;
		const std::string CountText = GetLogNumberWithSeparators(Count);
		const float CountX = Max.x - ImGui::CalcTextSize(CountText.c_str()).x - 8.0f;
		DrawList->AddCircleFilled(ImVec2(Min.x + 13.0f * Scale, floorf(Min.y + RowSize.y / 2.0f)), 5.0f * Scale, GetLogSeverityColor(Colors, WorstSeverity));
		DrawList->PushClipRect(Min, ImVec2(CountX - 8.0f, Max.y), true);
		DrawList->AddText(ImVec2(Min.x + 26.0f * Scale, TextY), bSelected ? Colors.Text : Colors.TextMuted, i == -1 ? "All channels" : GetLogChannelName(Channel).c_str());
		DrawList->PopClipRect();
		DrawList->AddText(ImVec2(CountX, TextY), Colors.TextMuted, CountText.c_str());
		if (i != -1)
			ShowLogToolTip(Channel.c_str());
	}

	ImGui::EndChild();
	ImGui::PopStyleVar(3);
	ImGui::PopStyleColor();
}

static void RenderLogTable(const FELogWindowColors& Colors, FELogWindowData& Data, const ImVec2 Size)
{
	const float Scale = GetLogWindowScale();
	const bool bShowChannel = Data.Channel.empty();
	ImGui::PushStyleColor(ImGuiCol_TableHeaderBg, Colors.TableHeader);
	ImGui::PushStyleColor(ImGuiCol_TableRowBg, IM_COL32(0, 0, 0, 0));
	ImGui::PushStyleColor(ImGuiCol_TableRowBgAlt, Colors.TableAlternateRow);
	ImGui::PushStyleColor(ImGuiCol_TableBorderLight, Colors.TableBorder);
	ImGui::PushStyleVar(ImGuiStyleVar_CellPadding, ImVec2(10.0f, 5.0f));
	if (ImGui::BeginTable("##LogTable", bShowChannel ? 5 : 4, ImGuiTableFlags_ScrollY | ImGuiTableFlags_RowBg | ImGuiTableFlags_Resizable | ImGuiTableFlags_BordersInnerV | ImGuiTableFlags_SizingFixedFit, Size))
	{
		ImGui::TableSetupScrollFreeze(0, 1);
		ImGui::TableSetupColumn("Time", ImGuiTableColumnFlags_WidthFixed, 96.0f * Scale);
		ImGui::TableSetupColumn("Level", ImGuiTableColumnFlags_WidthFixed, 110.0f * Scale);
		ImGui::TableSetupColumn("Message", ImGuiTableColumnFlags_WidthStretch);
		ImGui::TableSetupColumn("Count", ImGuiTableColumnFlags_WidthFixed, 80.0f * Scale);
		if (bShowChannel)
			ImGui::TableSetupColumn("Channel", ImGuiTableColumnFlags_WidthFixed, 240.0f * Scale);

		ImGui::PushStyleColor(ImGuiCol_Text, Colors.TextMuted);
		ImGui::TableHeadersRow();
		ImGui::PopStyleColor();

		// Only rows that are in view are rendered.
		ImGuiListClipper Clipper;
		Clipper.Begin(static_cast<int>(Data.ShownItems.size()));
		while (Clipper.Step())
		{
			for (int i = Clipper.DisplayStart; i < Clipper.DisplayEnd; i++)
			{
				const LogItem& Item = *Data.ShownItems[i];
				const ImU32 SeverityColor = GetLogSeverityColor(Colors, Item.Severity);
				ImGui::PushID(i);
				ImGui::TableNextRow();

				ImGui::TableNextColumn();
				ImGui::PushStyleColor(ImGuiCol_Text, Colors.TextMuted);
				ImGui::Selectable(GetLogTimeText(Item.TimeStamp).c_str(), false, ImGuiSelectableFlags_SpanAllColumns);
				ImGui::PopStyleColor();
				const bool bRowHovered = ImGui::IsItemHovered();
				if (ImGui::BeginPopupContextItem("##LogRowMenu"))
				{
					if (ImGui::MenuItem("Copy message"))
						ImGui::SetClipboardText(Item.Text.c_str());

					ImGui::EndPopup();
				}

				ImGui::TableNextColumn();
				const ImVec2 DotPosition = ImGui::GetCursorScreenPos();
				ImGui::GetWindowDrawList()->AddCircleFilled(ImVec2(DotPosition.x + 5.0f * Scale, floorf(DotPosition.y + ImGui::GetFontSize() / 2.0f)), 5.0f * Scale, SeverityColor);
				ImGui::SetCursorPosX(ImGui::GetCursorPosX() + 18.0f * Scale);
				ImGui::PushStyleColor(ImGuiCol_Text, SeverityColor);
				ImGui::TextUnformatted(GetLogSeverityName(Item.Severity));
				ImGui::PopStyleColor();

				// Only the first line of a message is in the row, the whole text is in the tooltip when it does not fit.
				ImGui::TableNextColumn();
				const size_t FirstLineEnd = Item.Text.find('\n');
				const char* TextEnd = FirstLineEnd == std::string::npos ? Item.Text.c_str() + Item.Text.size() : Item.Text.c_str() + FirstLineEnd;
				const bool bCut = FirstLineEnd != std::string::npos || ImGui::CalcTextSize(Item.Text.c_str(), TextEnd).x > ImGui::GetContentRegionAvail().x;
				ImGui::PushStyleColor(ImGuiCol_Text, Item.Severity >= FE_LOG_WARNING ? SeverityColor : Colors.Text);
				ImGui::TextUnformatted(Item.Text.c_str(), TextEnd);
				ImGui::PopStyleColor();
				if (bRowHovered && bCut)
				{
					ImGui::BeginTooltip();
					ImGui::PushTextWrapPos(ImGui::GetFontSize() * 45.0f);
					ImGui::TextUnformatted(Item.Text.c_str());
					ImGui::PopTextWrapPos();
					ImGui::EndTooltip();
				}

				ImGui::TableNextColumn();
				ImGui::PushStyleColor(ImGuiCol_Text, Colors.TextMuted);
				ImGui::TextUnformatted(GetLogCountText(Item.Count).c_str());
				if (bShowChannel)
				{
					ImGui::TableNextColumn();
					ImGui::TextUnformatted(GetLogChannelName(Item.Topic).c_str());
				}
				ImGui::PopStyleColor();
				ImGui::PopID();
			}
		}

		// Size of the content is known a frame after the rows change.
		if (Data.FramesToScrollToEnd > 0)
		{
			if (Data.bFollow)
				ImGui::SetScrollY(ImGui::GetScrollMaxY());

			Data.FramesToScrollToEnd--;
		}

		ImGui::EndTable();
	}

	ImGui::PopStyleVar();
	ImGui::PopStyleColor(4);
}

FELogWindow::FELogWindow()
{
	Data = new FELogWindowData();
}

FELogWindow::~FELogWindow()
{
	delete Data;
}

bool FELogWindow::IsVisible() const
{
	return bVisible;
}

void FELogWindow::SetVisible(const bool NewValue)
{
	bVisible = NewValue;
}

bool FELogWindow::IsCloseButtonShown() const
{
	return bCloseButtonShown;
}

void FELogWindow::SetCloseButtonShown(const bool NewValue)
{
	bCloseButtonShown = NewValue;
}

FELogWindowColors FELogWindow::GetColors() const
{
	return bOwnColors ? Colors : GetColorsOfImGuiStyle();
}

void FELogWindow::SetColors(const FELogWindowColors& NewValue)
{
	Colors = NewValue;
	bOwnColors = true;
}

void FELogWindow::UseColorsOfImGuiStyle()
{
	bOwnColors = false;
}

void FELogWindow::Render()
{
	// An application without graphics has no ImGui.
	if (!bVisible || ImGui::GetCurrentContext() == nullptr)
		return;

	const float Scale = GetLogWindowScale();
	const float WindowPadding = 15.0f * Scale;
	// Wide enough for the top row and for about 40 characters of a message.
	ImGui::SetNextWindowSize(ImVec2(1400.0f * Scale, 600.0f * Scale), ImGuiCond_FirstUseEver);
	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(WindowPadding, WindowPadding));
	const bool bWindowIsShown = ImGui::Begin("Log", bCloseButtonShown ? &bVisible : nullptr, ImGuiWindowFlags_None);
	ImGui::PopStyleVar();
	if (!bWindowIsShown)
	{
		ImGui::End();
		return;
	}

	const FELogWindowColors CurrentColors = GetColors();

	// Messages are taken from the log at most four times a second, and less often when it takes long.
	const uint64_t CurrentTime = TIME.GetTimeStamp(FE_TIME_RESOLUTION_MILLISECONDS);
	const uint64_t UpdateInterval = std::max(static_cast<uint64_t>(250), static_cast<uint64_t>(Data->LastUpdateDuration * 20.0));
	if (Data->bUpdateRequested || CurrentTime - Data->LastUpdateTime >= UpdateInterval)
		UpdateLogItems(*Data);

	std::string Filters = Data->Channel + "\n" + Data->TextFilter + "\n";
	for (int i = 0; i < LogLevelCount; i++)
		Filters += Data->bLevelShown[i] ? "1" : "0";

	if (Filters != Data->AppliedFilters)
	{
		ApplyLogFilters(*Data);
		Data->AppliedFilters = Filters;
	}

	ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(8.0f, 8.0f));

	// Top row: levels and text filter at the left, actions at the right.
	const char* LevelNames[] = { "Info", "Debug", "Warnings", "Errors" };
	const LOG_SEVERITY LevelSeverities[] = { FE_LOG_INFO, FE_LOG_DEBUG, FE_LOG_WARNING, FE_LOG_ERROR };
	for (int i = LogLevelCount - 1; i >= 0; i--)
	{
		if (RenderLogLevelButton(CurrentColors, LevelNames[i], GetLogSeverityColor(CurrentColors, LevelSeverities[i]), Data->LevelCounts[i], Data->bLevelShown[i]))
			Data->bLevelShown[i] = !Data->bLevelShown[i];

		ImGui::SameLine();
	}

	ImGui::SetNextItemWidth(320.0f * Scale);
	ImGui::InputTextWithHint("##LogTextFilter", "Filter by text...", &Data->TextFilter);

	const ImGuiStyle& Style = ImGui::GetStyle();
	const float FollowWidth = ImGui::GetFrameHeight() + Style.ItemInnerSpacing.x + ImGui::CalcTextSize("Follow").x;
	const float CopyWidth = ImGui::CalcTextSize("Copy").x + Style.FramePadding.x * 2.0f;
	const float ClearWidth = ImGui::CalcTextSize("Clear").x + Style.FramePadding.x * 2.0f;
	ImGui::SameLine();
	ImGui::SetCursorPosX(std::max(ImGui::GetCursorPosX(), ImGui::GetWindowWidth() - WindowPadding - FollowWidth - CopyWidth - ClearWidth - 8.0f * 2.0f));
	if (ImGui::Checkbox("Follow", &Data->bFollow) && Data->bFollow)
		Data->FramesToScrollToEnd = 2;
	ShowLogToolTip("Keeps the newest message in view.");

	ImGui::SameLine();
	if (ImGui::Button("Copy"))
	{
		std::string Text;
		for (size_t i = 0; i < Data->ShownItems.size(); i++)
		{
			// One line for a message, so the text can be pasted as a table.
			const LogItem& Item = *Data->ShownItems[i];
			std::string Message = Item.Text;
			std::replace(Message.begin(), Message.end(), '\n', ' ');
			Text += GetLogTimeText(Item.TimeStamp) + "\t" + GetLogSeverityName(Item.Severity) + "\t" + Message + "\t" + GetLogCountText(Item.Count) + "\t" + Item.Topic + "\n";
		}

		ImGui::SetClipboardText(Text.c_str());
	}
	ShowLogToolTip("Copies the messages that are shown.");

	ImGui::SameLine();
	if (ImGui::Button("Clear"))
	{
		Data->ClearedAt = TIME.GetTimeStamp(FE_TIME_RESOLUTION_NANOSECONDS);
		Data->bUpdateRequested = true;
	}
	ShowLogToolTip("Hides the messages that are in the log now. Files of the log are not changed.");

	const float SmallFontSize = 16.0f * Scale;
	const float ChannelListWidth = 300.0f * Scale;
	const ImVec2 Available = ImVec2(ImGui::GetContentRegionAvail().x, ImGui::GetContentRegionAvail().y - SmallFontSize - 16.0f);
	RenderLogChannelList(CurrentColors, *Data, ImVec2(ChannelListWidth, Available.y));
	ImGui::SameLine();
	RenderLogTable(CurrentColors, *Data, ImVec2(Available.x - ChannelListWidth - 8.0f, Available.y));

	ImGui::PushFont(nullptr, SmallFontSize);
	ImGui::PushStyleColor(ImGuiCol_Text, CurrentColors.TextMuted);
	ImGui::Text("%s of %s messages are shown. A message that repeats is one row with a count.", GetLogNumberWithSeparators(Data->ShownItems.size()).c_str(), GetLogNumberWithSeparators(Data->Items.size()).c_str());
	ImGui::PopStyleColor();
	ImGui::PopFont();

	ImGui::PopStyleVar();
	ImGui::End();
}
