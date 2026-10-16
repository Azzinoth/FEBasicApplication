#include "FEFileSystem.h"
#include <algorithm>
#include <chrono>
#include <cstdlib>
#include <thread>

#if FE_PLATFORM(WINDOWS)
#include <windows.h>
#include <shobjidl.h>
#endif

#if FE_PLATFORM(EMSCRIPTEN)
#include <emscripten.h>

EM_JS_DEPS(FEBrowserFileDialog, "$FS,$UTF8ToString,$stringToNewUTF8");

// The chosen file is copied into the in-memory file system and its path is kept until FEFileSystem::Update takes it.
// Only the latest request can finish, older ones are ignored because a browser without the cancel event never reports them.
EM_JS(void, FEBrowserOpenFileDialog, (int RequestID, const char* Accept, const char* TargetDirectory), {
	Module.FEBrowserFileDialogRequestID = RequestID;
	const Directory = UTF8ToString(TargetDirectory);
	const Finish = (Path) => {
		if (Module.FEBrowserFileDialogRequestID === RequestID)
			Module.FEBrowserFileDialogResult = Path;
	};

	const Input = document.createElement("input");
	Input.type = "file";
	Input.accept = UTF8ToString(Accept);
	Input.style.display = "none";
	document.body.appendChild(Input);

	Input.addEventListener("cancel", () => {
		Input.remove();
		Finish("");
	});

	Input.addEventListener("change", async () => {
		Input.remove();
		const ChosenFile = Input.files && Input.files[0];
		if (!ChosenFile)
		{
			Finish("");
			return;
		}

		try
		{
			const Data = new Uint8Array(await ChosenFile.arrayBuffer());
			FS.mkdirTree(Directory);
			const Path = Directory + "/" + ChosenFile.name;
			FS.writeFile(Path, Data);
			Finish(Path);
		}
		catch (Exception)
		{
			console.error("FEBrowserOpenFileDialog: could not copy the chosen file.", Exception);
			Finish("");
		}
	});

	Input.click();
});

EM_JS(char*, FEBrowserTakeFileDialogResult, (), {
	if (Module.FEBrowserFileDialogResult === undefined)
		return 0;

	const Path = Module.FEBrowserFileDialogResult;
	Module.FEBrowserFileDialogResult = undefined;
	return stringToNewUTF8(Path);
});

// The file data is copied into a Blob, so the file can be removed as soon as this returns.
EM_JS(void, FEBrowserDownloadFile, (const char* Path, const char* FileName), {
	const Data = FS.readFile(UTF8ToString(Path));
	const FileURL = URL.createObjectURL(new Blob([Data], { type: "application/octet-stream" }));
	const Link = document.createElement("a");
	Link.href = FileURL;
	Link.download = UTF8ToString(FileName);
	Link.style.display = "none";
	document.body.appendChild(Link);
	Link.click();
	Link.remove();
	setTimeout(() => URL.revokeObjectURL(FileURL), 10000);
});
#endif

using namespace FocalEngine;

#if FE_PLATFORM(WINDOWS) || FE_PLATFORM(EMSCRIPTEN)
namespace
{
	std::string NormalizeDialogExtension(const std::string& Extension)
	{
		size_t Start = 0;
		while (Start < Extension.size() && (Extension[Start] == '*' || Extension[Start] == '.'))
			Start++;

		return Extension.substr(Start);
	}

	std::string GetFirstDialogExtension(const std::vector<FEFileSystem::FileDialogFilter>& Filters)
	{
		for (const FEFileSystem::FileDialogFilter& Filter : Filters)
		{
			for (const std::string& Extension : Filter.Extensions)
			{
				const std::string NormalizedExtension = NormalizeDialogExtension(Extension);
				if (!NormalizedExtension.empty())
					return NormalizedExtension;
			}
		}

		return "";
	}

#if FE_PLATFORM(WINDOWS)
	std::wstring ToWideString(const std::string& Text)
	{
		const int WideLength = MultiByteToWideChar(CP_UTF8, 0, Text.c_str(), -1, nullptr, 0);
		if (WideLength <= 1)
			return std::wstring();

		std::wstring Result(WideLength - 1, L'\0');
		MultiByteToWideChar(CP_UTF8, 0, Text.c_str(), -1, Result.data(), WideLength);
		return Result;
	}

	// COMDLG_FILTERSPEC only points to its strings, so they are kept alive in Strings.
	void BuildWindowsFilterSpecs(const std::vector<FEFileSystem::FileDialogFilter>& Filters, std::vector<std::wstring>& Strings, std::vector<COMDLG_FILTERSPEC>& Specs)
	{
		Strings.clear();
		Specs.clear();

		for (const FEFileSystem::FileDialogFilter& Filter : Filters)
		{
			std::string Pattern;
			for (const std::string& Extension : Filter.Extensions)
			{
				const std::string NormalizedExtension = NormalizeDialogExtension(Extension);
				if (NormalizedExtension.empty())
					continue;

				if (!Pattern.empty())
					Pattern += ";";
				Pattern += "*." + NormalizedExtension;
			}

			if (Pattern.empty())
				Pattern = "*.*";

			Strings.push_back(ToWideString(Filter.Description.empty() ? Pattern : Filter.Description));
			Strings.push_back(ToWideString(Pattern));
		}

		for (size_t i = 0; i + 1 < Strings.size(); i += 2)
			Specs.push_back({ Strings[i].c_str(), Strings[i + 1].c_str() });
	}

	std::string PWSTRtoString(const PWSTR WideString)
	{
		const std::wstring WideText = WideString;
		char* Buffer = new char[WideText.length() + 1];
		Buffer[WideText.size()] = '\0';
		WideCharToMultiByte(CP_ACP, 0, WideText.c_str(), -1, Buffer, static_cast<int>(WideText.length()), nullptr, nullptr);
		std::string Result = Buffer;
		delete[] Buffer;

		return Result;
	}

	void ShowFileOpenDialog(std::string& Path, const COMDLG_FILTERSPEC* Filter, const int FilterCount, const std::string& InitialDirectory)
	{
		HRESULT Result = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);
		if (SUCCEEDED(Result))
		{
			IFileOpenDialog* FileOpenDialog;
			// Create the FileOpenDialog object.
			Result = CoCreateInstance(CLSID_FileOpenDialog, nullptr, CLSCTX_ALL, IID_IFileOpenDialog, reinterpret_cast<void**>(&FileOpenDialog));

			if (SUCCEEDED(Result))
			{
				Result = FileOpenDialog->SetFileTypes(FilterCount, Filter);

				if (!InitialDirectory.empty() && FILE_SYSTEM.DoesDirectoryExist(InitialDirectory))
				{
					const int WideLength = MultiByteToWideChar(CP_UTF8, 0, InitialDirectory.c_str(), -1, nullptr, 0);
					if (WideLength > 0)
					{
						std::wstring WideDirectory(WideLength - 1, L'\0');
						MultiByteToWideChar(CP_UTF8, 0, InitialDirectory.c_str(), -1, WideDirectory.data(), WideLength);

						IShellItem* Folder = nullptr;
						if (SUCCEEDED(SHCreateItemFromParsingName(WideDirectory.c_str(), nullptr, IID_PPV_ARGS(&Folder))))
						{
							FileOpenDialog->SetFolder(Folder);
							Folder->Release();
						}
					}
				}

				// Show the Open dialog box.
				Result = FileOpenDialog->Show(nullptr);

				// Get the file name from the dialog box.
				if (SUCCEEDED(Result))
				{
					IShellItem* SelectedItem;
					Result = FileOpenDialog->GetResult(&SelectedItem);
					if (SUCCEEDED(Result))
					{
						PWSTR WideFilePath;
						Result = SelectedItem->GetDisplayName(SIGDN_FILESYSPATH, &WideFilePath);

						// Display the file name to the user.
						if (SUCCEEDED(Result))
						{
							Path = PWSTRtoString(WideFilePath);
							CoTaskMemFree(WideFilePath);
						}
						SelectedItem->Release();
					}
				}
				FileOpenDialog->Release();
			}
			CoUninitialize();
		}
	}

	void ShowFileSaveDialog(std::string& Path, const COMDLG_FILTERSPEC* Filter, const int FilterCount, int& ChosenFilterIndex, const std::string& SuggestedFileName, const std::string& DefaultExtension)
	{
		HRESULT Result = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);
		if (SUCCEEDED(Result))
		{
			IFileSaveDialog* FileSaveDialog;
			// Create the FileSaveDialog object.
			Result = CoCreateInstance(CLSID_FileSaveDialog, nullptr, CLSCTX_ALL, IID_IFileSaveDialog, reinterpret_cast<void**>(&FileSaveDialog));

			if (SUCCEEDED(Result))
			{
				if (FilterCount > 0)
					FileSaveDialog->SetFileTypes(FilterCount, Filter);

				// With a default extension the dialog adds the extension of the selected file type when the user does not type one.
				if (!DefaultExtension.empty())
					FileSaveDialog->SetDefaultExtension(ToWideString(DefaultExtension).c_str());

				if (!SuggestedFileName.empty())
					FileSaveDialog->SetFileName(ToWideString(SuggestedFileName).c_str());

				// Show the Save dialog box.
				Result = FileSaveDialog->Show(nullptr);

				// Get the file name from the dialog box.
				if (SUCCEEDED(Result))
				{
					IShellItem* SelectedItem;
					Result = FileSaveDialog->GetResult(&SelectedItem);
					if (SUCCEEDED(Result))
					{
						PWSTR WideFilePath;
						Result = SelectedItem->GetDisplayName(SIGDN_FILESYSPATH, &WideFilePath);

						// Display the file name to the user.
						if (SUCCEEDED(Result))
						{
							Path = PWSTRtoString(WideFilePath);
							CoTaskMemFree(WideFilePath);
						}
						SelectedItem->Release();
					}

					// Retrieve the index of the selected filter
					unsigned int TemporaryChosenFilterIndex;
					Result = FileSaveDialog->GetFileTypeIndex(&TemporaryChosenFilterIndex);
					ChosenFilterIndex = TemporaryChosenFilterIndex - 1;

					if (FAILED(Result))
						ChosenFilterIndex = -1;
				}
				FileSaveDialog->Release();
			}
			CoUninitialize();
		}
	}

	void ShowFolderOpenDialog(std::string& Path)
	{
		HRESULT Result = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);
		if (SUCCEEDED(Result))
		{
			IFileDialog* FolderOpenDialog;
			// Create the FileOpenDialog object.
			Result = CoCreateInstance(CLSID_FileOpenDialog, nullptr, CLSCTX_ALL, IID_IFileOpenDialog, reinterpret_cast<void**>(&FolderOpenDialog));

			if (SUCCEEDED(Result))
			{
				DWORD Options;
				if (SUCCEEDED(FolderOpenDialog->GetOptions(&Options)))
					FolderOpenDialog->SetOptions(Options | FOS_PICKFOLDERS);

				Result = FolderOpenDialog->SetFileTypes(0, nullptr);
				// Show the Open dialog box.
				Result = FolderOpenDialog->Show(nullptr);

				// Get the file name from the dialog box.
				if (SUCCEEDED(Result))
				{
					IShellItem* SelectedItem;
					Result = FolderOpenDialog->GetResult(&SelectedItem);
					if (SUCCEEDED(Result))
					{
						PWSTR WideFolderPath;
						Result = SelectedItem->GetDisplayName(SIGDN_FILESYSPATH, &WideFolderPath);

						// Display the file name to the user.
						if (SUCCEEDED(Result))
						{
							Path = PWSTRtoString(WideFolderPath);
							CoTaskMemFree(WideFolderPath);
						}
						SelectedItem->Release();
					}
				}
				FolderOpenDialog->Release();
			}
			CoUninitialize();
		}
	}
#endif

#if FE_PLATFORM(EMSCRIPTEN)
	// Value for the accept attribute of a file input, for example ".png,.jpg". Empty when a filter accepts all files.
	std::string BuildBrowserAcceptString(const std::vector<FEFileSystem::FileDialogFilter>& Filters)
	{
		std::string Accept;
		for (const FEFileSystem::FileDialogFilter& Filter : Filters)
		{
			bool bHasExtension = false;
			for (const std::string& Extension : Filter.Extensions)
			{
				const std::string NormalizedExtension = NormalizeDialogExtension(Extension);
				if (NormalizedExtension.empty())
					continue;

				if (!Accept.empty())
					Accept += ",";
				Accept += "." + NormalizedExtension;
				bHasExtension = true;
			}

			if (!bHasExtension)
				return "";
		}

		return Accept;
	}
#endif
}
#endif

#ifdef FEBASICAPPLICATION_SHARED
extern "C" __declspec(dllexport) void* GetFileSystem()
{
	return FEFileSystem::GetInstancePointer();
}
#endif

FEFileSystem::FEFileSystem() {}
FEFileSystem::~FEFileSystem() {}

bool FEFileSystem::DoesFileExist(const std::string& Path)
{
	std::filesystem::path FilePath(Path);
	return std::filesystem::exists(FilePath) && std::filesystem::is_regular_file(FilePath);
}

bool FEFileSystem::RenameFile(const std::string& Path, const std::string& NewPath)
{
	try
	{
		std::filesystem::rename(Path, NewPath);
		return true;
	}
	catch (const std::exception& Exception)
	{
		LOG.Add("Error in FEFileSystem::RenameFile: " + std::string(Exception.what()), "FE_FILE_SYSTEM", FE_LOG_ERROR);
		return false;
	}
}

bool FEFileSystem::CopyFileTo(const std::string& Path, const std::string& NewPath)
{
	try
	{
		std::filesystem::copy(Path, NewPath);
		return true;
	}
	catch (const std::exception& Exception)
	{
		LOG.Add("Error in FEFileSystem::CopyFileTo: " + std::string(Exception.what()), "FE_FILE_SYSTEM", FE_LOG_ERROR);
		return false;
	}
}

bool FEFileSystem::RemoveFile(const std::string& Path)
{
	try
	{
		return std::filesystem::remove(Path);
	}
	catch (const std::exception& Exception)
	{
		LOG.Add("Error in FEFileSystem::RemoveFile: " + std::string(Exception.what()), "FE_FILE_SYSTEM", FE_LOG_ERROR);
		return false;
	}
}

size_t FEFileSystem::GetFileSize(const std::string& Path)
{
	std::filesystem::path FilePath(Path);
	if (std::filesystem::exists(FilePath) && std::filesystem::is_regular_file(FilePath))
	{
		return std::filesystem::file_size(FilePath);
	}

	return 0;
}

uint64_t FEFileSystem::GetFileLastWriteTime(const std::string& Path)
{
	std::filesystem::path FilePath(Path);
	if (std::filesystem::exists(FilePath) && std::filesystem::is_regular_file(FilePath))
	{
		auto FileTime = std::filesystem::last_write_time(FilePath);
		auto TimePoint = std::chrono::time_point_cast<std::chrono::system_clock::duration>(FileTime - std::filesystem::file_time_type::clock::now() + std::chrono::system_clock::now());
		auto Duration = TimePoint.time_since_epoch();
		auto NanosecondsCount = std::chrono::duration_cast<std::chrono::nanoseconds>(Duration);
		return NanosecondsCount.count();
	}

	return 0;
}

//void FEFileSystem::WaitForFileAccess(std::string FilePath, int TimeoutInMS)
//{
//	std::filesystem::path Path(FilePath);
//	std::chrono::milliseconds Timeout(TimeoutInMS);
//	std::chrono::time_point<std::chrono::system_clock> StartTime = std::chrono::system_clock::now();
//	std::filesystem::file_status Status = std::filesystem::status(Path);
//	while (std::filesystem::exists(Path) && std::filesystem::is_regular_file(Path))
//	{
//		std::this_thread::sleep_for(std::chrono::milliseconds(10));
//		if (std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now() - StartTime) > Timeout)
//			break;
//	}
//}

bool FEFileSystem::WaitForFileAccess(const std::string& FilePath, int TimeoutInMilliseconds)
{
	const auto Start = std::chrono::steady_clock::now();
	const auto End = Start + std::chrono::milliseconds(TimeoutInMilliseconds);

	while (std::chrono::steady_clock::now() < End)
	{
		try
		{
			// Check if file exists
			if (!std::filesystem::exists(FilePath))
			{
				std::this_thread::sleep_for(std::chrono::milliseconds(10));
				continue;
			}

			// Try to open the file
			std::ifstream File(FilePath);
			if (File.is_open())
			{
				// File is accessible, close it and return
				File.close();
				return true;
			}
		}
		catch (const std::filesystem::filesystem_error&)
		{
			// Ignore filesystem errors and continue trying
		}
		catch (const std::ios_base::failure&)
		{
			// Ignore file open errors and continue trying
		}

		// Wait for 10ms before next attempt
		std::this_thread::sleep_for(std::chrono::milliseconds(10));
	}

	// If we've reached here, we've timed out
	return false;
}

bool FEFileSystem::DoesDirectoryExist(const std::string& Path)
{
	std::filesystem::path FolderPath(Path);
	return std::filesystem::exists(FolderPath) && std::filesystem::is_directory(FolderPath);
}

bool FEFileSystem::RenameDirectory(const std::string& Path, const std::string& NewPath)
{
	try
	{
		if (std::filesystem::exists(Path) && std::filesystem::is_directory(Path))
		{
			std::filesystem::rename(Path, NewPath);
			return true;
		}
		else
		{
			// Directory doesn't exist or is not a directory
			return false;
		}
	}
	catch (const std::exception& Exception)
	{
		LOG.Add("Error in FEFileSystem::RenameDirectory: " + std::string(Exception.what()), "FE_FILE_SYSTEM", FE_LOG_ERROR);
		return false;
	}
}

bool FEFileSystem::MakeDirectory(const std::string& Path)
{
	try
	{
		return std::filesystem::create_directory(Path);
	}
	catch (const std::exception& Exception)
	{
		LOG.Add("Error in FEFileSystem::MakeDirectory: " + std::string(Exception.what()), "FE_FILE_SYSTEM", FE_LOG_ERROR);
		return false;
	}
}

bool FEFileSystem::CopyDirectory(const std::string& Path, const std::string& NewPath)
{
	try
	{
		std::filesystem::copy(Path, NewPath, std::filesystem::copy_options::recursive);
	}
	catch (const std::exception& Exception)
	{
		LOG.Add("Error in FEFileSystem::CopyDirectory: " + std::string(Exception.what()), "FE_FILE_SYSTEM", FE_LOG_ERROR);
		return false;
	}

	return true;
}

bool FEFileSystem::DeleteDirectory(const std::string& Path)
{
	try
	{
		return std::filesystem::remove_all(Path) > 0;
	}
	catch (const std::exception& Exception)
	{
		LOG.Add("Error in FEFileSystem::DeleteDirectory: " + std::string(Exception.what()), "FE_FILE_SYSTEM", FE_LOG_ERROR);
		return false;
	}
}

std::vector<std::string> FEFileSystem::GetFileNamesInDirectory(const std::string& Path, bool bRecursive)
{
	std::vector<std::string> Result;

	try
	{
		std::filesystem::path Directory(Path);
		if (std::filesystem::exists(Directory) && std::filesystem::is_directory(Directory))
		{
			for (const auto& Entry : std::filesystem::directory_iterator(Directory))
			{
				const auto& EntryPath = Entry.path();
				if (std::filesystem::is_regular_file(EntryPath))
				{
					Result.push_back(EntryPath.filename().string());
				}
				else if (bRecursive && std::filesystem::is_directory(EntryPath))
				{
					std::vector<std::string> SubDirectoryFiles = GetFileNamesInDirectory(EntryPath.string(), bRecursive);
					Result.insert(Result.end(), SubDirectoryFiles.begin(), SubDirectoryFiles.end());
				}
			}
		}
	}
	catch (const std::exception& Exception)
	{
		LOG.Add("Error in FEFileSystem::GetFileNamesInDirectory: " + std::string(Exception.what()), "FE_FILE_SYSTEM", FE_LOG_ERROR);
	}

	return Result;
}

std::vector<std::string> FEFileSystem::GetFilesInDirectory(const std::string& Path, bool bRecursive)
{
	std::vector<std::string> Result;

	try
	{
		std::filesystem::path Directory(Path);
		if (std::filesystem::exists(Directory) && std::filesystem::is_directory(Directory))
		{
			for (const auto& Entry : std::filesystem::directory_iterator(Directory))
			{
				const auto& EntryPath = Entry.path();
				if (std::filesystem::is_regular_file(EntryPath))
				{
					Result.push_back(EntryPath.string());
				}
				else if (bRecursive && std::filesystem::is_directory(EntryPath))
				{
					std::vector<std::string> SubDirectoryFiles = GetFilesInDirectory(EntryPath.string(), bRecursive);
					Result.insert(Result.end(), SubDirectoryFiles.begin(), SubDirectoryFiles.end());
				}
			}
		}
	}
	catch (const std::exception& Exception)
	{
		LOG.Add("Error in FEFileSystem::GetFilesInDirectory: " + std::string(Exception.what()), "FE_FILE_SYSTEM", FE_LOG_ERROR);
	}

	return Result;
}

std::vector<std::string> FEFileSystem::GetFolderChain(const std::string& Path)
{
	std::vector<std::string> Result;

	try
	{
		std::filesystem::path Directory(Path);
		while (!Directory.string().empty())
		{
			//Directory.
			//std::filesystem::path test = std::filesystem::canonical(Directory.string() + "/");
			//if (std::filesystem::is_directory(Directory.string() + "/"))

			if (!Result.empty())
			{
				if (Result.back() == Directory.string())
					break;
			}
			Result.push_back(Directory.string());
			Directory = Directory.parent_path();
		}

		std::reverse(Result.begin(), Result.end());
	}
	catch (const std::exception& Exception)
	{
		LOG.Add("Error in FEFileSystem::GetFolderChain: " + std::string(Exception.what()), "FE_FILE_SYSTEM", FE_LOG_ERROR);
	}

	return Result;
}

std::vector<std::string> FEFileSystem::GetDirectoryList(const std::string& Path)
{
	std::vector<std::string> Result;

	try
	{
		std::filesystem::path Directory(Path);
		if (std::filesystem::exists(Directory) && std::filesystem::is_directory(Directory))
		{
			for (const auto& Entry : std::filesystem::directory_iterator(Directory))
			{
				const auto& EntryPath = Entry.path();
				if (std::filesystem::is_directory(EntryPath))
				{
					std::string DirectoryName = EntryPath.filename().string();
					if (DirectoryName != "." && DirectoryName != "..")
					{
						Result.push_back(DirectoryName);
					}
				}
			}
		}
	}
	catch (const std::exception& Exception)
	{
		LOG.Add("Error in FEFileSystem::GetDirectoryList: " + std::string(Exception.what()), "FE_FILE_SYSTEM", FE_LOG_ERROR);
	}

	return Result;
}

std::vector<std::string> FEFileSystem::GetFileList(const std::string& Path)
{
	std::vector<std::string> Result;

	try
	{
		std::filesystem::path Directory(Path);
		if (std::filesystem::exists(Directory) && std::filesystem::is_directory(Directory))
		{
			for (const auto& Entry : std::filesystem::directory_iterator(Directory))
			{
				const auto& EntryPath = Entry.path();
				if (std::filesystem::is_regular_file(EntryPath))
				{
					Result.push_back(EntryPath.filename().string());
				}
			}
		}
	}
	catch (const std::exception& Exception)
	{
		LOG.Add("Error in FEFileSystem::GetFileList: " + std::string(Exception.what()), "FE_FILE_SYSTEM", FE_LOG_ERROR);
	}

	return Result;
}

#if FE_PLATFORM(WINDOWS)
std::string FEFileSystem::GetExecutablePath()
{
	char Buffer[MAX_PATH] = { 0 };
	GetModuleFileNameA(nullptr, Buffer, MAX_PATH);
	return Buffer;
}

#endif

std::string FEFileSystem::GetCurrentWorkingPath()
{
	std::filesystem::path ApplicationPath = std::filesystem::current_path();
	return ApplicationPath.string();
}

std::string FEFileSystem::GetFileExtension(const std::string& Path)
{
	std::filesystem::path FilePath(Path);
	return FilePath.extension().string();
}

std::string FEFileSystem::GetDirectoryPath(const std::string& FullPath)
{
	std::filesystem::path Path(FullPath);
	return Path.parent_path().string();
}

std::string FEFileSystem::GetFileName(const std::string& FullPath, bool bWithExtension)
{
	std::filesystem::path Path(FullPath);
	if (bWithExtension)
		return Path.filename().string();
	else
		return Path.stem().string();
}

std::string FEFileSystem::GetAbsolutePath(const std::string& Path)
{
	try
	{
		std::filesystem::path AbsolutePath = std::filesystem::absolute(Path);
		return AbsolutePath.string();
	}
	catch (const std::filesystem::filesystem_error& Exception)
	{
		LOG.Add("Error in FEFileSystem::GetAbsolutePath: failed to get absolute path " + std::string(Exception.what()), "FE_FILE_SYSTEM", FE_LOG_ERROR);
		return Path;
	}
}

std::string FEFileSystem::ReadFEString(std::fstream& File)
{
	char* Buffer = new char[4];

	File.read(Buffer, 4);
	const int TemporaryCharSize = *(int*)Buffer;
	char* TemporaryChar = new char[TemporaryCharSize + 1];
	File.read(TemporaryChar, TemporaryCharSize);
	TemporaryChar[TemporaryCharSize] = '\0';

	std::string Result = TemporaryChar;
	delete[] TemporaryChar;
	delete[] Buffer;

	return Result;
}

bool FEFileSystem::PerformTextReplacements(const std::string& FilePath, const std::vector<TextReplacementRule>& Rules)
{
	if (FilePath.empty())
	{
		LOG.Add("FEFileSystem::PerformTextReplacements: File path is empty", "FE_FILE_SYSTEM", FE_LOG_WARNING);
		return false;
	}

	std::fstream File(FilePath, std::ios::in);
	if (!File.is_open())
	{
		LOG.Add("FEFileSystem::PerformTextReplacements: Error opening file " + FilePath, "FE_FILE_SYSTEM", FE_LOG_ERROR);
		return false;
	}

	std::vector<std::string> FileContent;
	std::string Line;
	while (std::getline(File, Line))
		FileContent.push_back(Line);

	File.close();

	// Apply replacements
	for (size_t i = 0; i < FileContent.size(); i++)
	{
		for (size_t j = 0; j < Rules.size(); j++)
		{
			if (FileContent[i].find(Rules[j].ContextPattern) != std::string::npos)
			{
				size_t Position = FileContent[i].find(Rules[j].TargetText);
				if (Position != std::string::npos)
					FileContent[i].replace(Position, Rules[j].TargetText.length(), Rules[j].ReplacementText);
			}
		}
	}

	File.open(FilePath, std::ios::out | std::ios::trunc);
	if (!File.is_open())
	{
		LOG.Add("FEFileSystem::PerformTextReplacements: Error opening file " + FilePath, "FE_FILE_SYSTEM", FE_LOG_ERROR);
		return false;
	}

	// Write the modified content back to the file
	for (size_t i = 0; i < FileContent.size(); i++)
		File << FileContent[i] + "\n";

	File.close();
	return true;
}

void FEFileSystem::OpenFileDialog(const std::vector<FileDialogFilter>& Filters, std::function<void(const std::string& FilePath)> OnClose, const std::string& InitialDirectory)
{
	if (!OnClose)
		return;

#if FE_PLATFORM(WINDOWS)
	std::vector<std::wstring> FilterStrings;
	std::vector<COMDLG_FILTERSPEC> FilterSpecs;
	BuildWindowsFilterSpecs(Filters, FilterStrings, FilterSpecs);

	std::string FilePath;
	ShowFileOpenDialog(FilePath, FilterSpecs.empty() ? nullptr : FilterSpecs.data(), static_cast<int>(FilterSpecs.size()), InitialDirectory);
	OnClose(FilePath);
#elif FE_PLATFORM(EMSCRIPTEN)
	// Only one browser file picker can be open, so an unfinished request is reported as cancelled.
	if (PendingOpenFileDialogCallback)
	{
		std::function<void(const std::string& FilePath)> PreviousCallback = std::move(PendingOpenFileDialogCallback);
		PendingOpenFileDialogCallback = nullptr;
		PreviousCallback("");
	}

	PendingOpenFileDialogCallback = std::move(OnClose);
	OpenFileDialogRequestID++;
	FEBrowserOpenFileDialog(OpenFileDialogRequestID, BuildBrowserAcceptString(Filters).c_str(), "/FEFileDialog/Open");
#else
	LOG.Add("FEFileSystem::OpenFileDialog: file dialogs are not supported on this platform yet.", "FE_FILE_SYSTEM", FE_LOG_WARNING);
	OnClose("");
#endif
}

void FEFileSystem::SaveFileDialog(const std::string& SuggestedFileName, const std::vector<FileDialogFilter>& Filters, std::function<bool(const std::string& FilePath, int ChosenFilterIndex)> WriteFile)
{
	if (!WriteFile)
		return;

#if FE_PLATFORM(WINDOWS)
	std::vector<std::wstring> FilterStrings;
	std::vector<COMDLG_FILTERSPEC> FilterSpecs;
	BuildWindowsFilterSpecs(Filters, FilterStrings, FilterSpecs);

	std::string FilePath;
	int ChosenFilterIndex = -1;
	ShowFileSaveDialog(FilePath, FilterSpecs.empty() ? nullptr : FilterSpecs.data(), static_cast<int>(FilterSpecs.size()), ChosenFilterIndex, SuggestedFileName, GetFirstDialogExtension(Filters));
	if (!FilePath.empty())
		WriteFile(FilePath, ChosenFilterIndex);
#elif FE_PLATFORM(EMSCRIPTEN)
	const std::string TemporaryDirectory = "/FEFileDialog/Save";
	std::error_code ErrorCode;
	std::filesystem::create_directories(TemporaryDirectory, ErrorCode);

	std::string FileName = GetFileName(SuggestedFileName);
	if (FileName.empty())
		FileName = "Untitled";

	const std::string DefaultExtension = GetFirstDialogExtension(Filters);
	if (GetFileExtension(FileName).empty() && !DefaultExtension.empty())
		FileName += "." + DefaultExtension;

	const std::string TemporaryPath = TemporaryDirectory + "/" + FileName;
	// The browser has no file type choice, so the first filter is reported.
	if (WriteFile(TemporaryPath, 0) && DoesFileExist(TemporaryPath))
		FEBrowserDownloadFile(TemporaryPath.c_str(), FileName.c_str());

	RemoveFile(TemporaryPath);
#else
	LOG.Add("FEFileSystem::SaveFileDialog: file dialogs are not supported on this platform yet.", "FE_FILE_SYSTEM", FE_LOG_WARNING);
#endif
}

void FEFileSystem::OpenFolderDialog(std::function<void(const std::string& FolderPath)> OnClose)
{
	if (!OnClose)
		return;

#if FE_PLATFORM(WINDOWS)
	std::string FolderPath;
	ShowFolderOpenDialog(FolderPath);
	OnClose(FolderPath);
#else
	LOG.Add("FEFileSystem::OpenFolderDialog: folder dialogs are not supported on this platform.", "FE_FILE_SYSTEM", FE_LOG_WARNING);
	OnClose("");
#endif
}

bool FEFileSystem::IsFolderDialogSupported() const
{
#if FE_PLATFORM(WINDOWS)
	return true;
#else
	return false;
#endif
}

void FEFileSystem::Update()
{
#if FE_PLATFORM(EMSCRIPTEN)
	if (!PendingOpenFileDialogCallback)
		return;

	char* ChosenPath = FEBrowserTakeFileDialogResult();
	if (ChosenPath == nullptr)
		return;

	const std::string FilePath = ChosenPath;
	free(ChosenPath);

	std::function<void(const std::string& FilePath)> Callback = std::move(PendingOpenFileDialogCallback);
	PendingOpenFileDialogCallback = nullptr;
	Callback(FilePath);
#endif
}