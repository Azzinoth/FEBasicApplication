#pragma once

#include "FELog.h"
#include "FEPlatform.h"
#include <filesystem>
#include <fstream>
#include <functional>
#include <string>
#include <vector>

namespace FocalEngine
{
	class FEBASICAPPLICATION_API FEFileSystem
	{
	public:
		SINGLETON_PUBLIC_PART(FEFileSystem)

		bool DoesFileExist(const std::string& Path);
		bool RenameFile(const std::string& Path, const std::string& NewPath);
		bool CopyFileTo(const std::string& Path, const std::string& NewPath);
		bool RemoveFile(const std::string& Path);
		std::vector<std::string> GetFileList(const std::string& Path);
		size_t GetFileSize(const std::string& Path);
		// Returns last write time in nanoseconds since epoch.
		uint64_t GetFileLastWriteTime(const std::string& Path);
		bool WaitForFileAccess(const std::string& FilePath, int TimeoutInMilliseconds = 1000);

		std::string GetFileExtension(const std::string& Path);
		std::string GetDirectoryPath(const std::string& FullPath);
		std::string GetFileName(const std::string& FullPath, bool bWithExtension = true);
		std::string GetAbsolutePath(const std::string& Path);

		bool DoesDirectoryExist(const std::string& Path);
		bool RenameDirectory(const std::string& Path, const std::string& NewPath);
		bool MakeDirectory(const std::string& Path);
		bool CopyDirectory(const std::string& Path, const std::string& NewPath);
		bool DeleteDirectory(const std::string& Path);

		std::vector<std::string> GetFilesInDirectory(const std::string& Path, bool bRecursive = false);
		std::vector<std::string> GetFileNamesInDirectory(const std::string& Path, bool bRecursive = false);
		std::vector<std::string> GetDirectoryList(const std::string& Path);

		std::vector<std::string> GetFolderChain(const std::string& Path);

		struct TextReplacementRule
		{
			// Pattern to identify the line where replacement should occur
			std::string ContextPattern;
			// Text to be replaced
			std::string TargetText;
			std::string ReplacementText;
		};

		bool PerformTextReplacements(const std::string& FilePath, const std::vector<TextReplacementRule>& Rules);

		// For example { "Images", { "png", "jpg" } }.
		struct FileDialogFilter
		{
			std::string Description;
			std::vector<std::string> Extensions;
		};

		void OpenFileDialog(const std::vector<FileDialogFilter>& Filters, std::function<void(const std::string& FilePath)> OnClose, const std::string& InitialDirectory = "");
		void SaveFileDialog(const std::string& SuggestedFileName, const std::vector<FileDialogFilter>& Filters, std::function<bool(const std::string& FilePath, int ChosenFilterIndex)> WriteFile);

		// Only available on Windows, elsewhere OnClose is called with an empty path.
		void OpenFolderDialog(std::function<void(const std::string& FolderPath)> OnClose);
		bool IsFolderDialogSupported() const;

		void Update();

#if FE_PLATFORM(WINDOWS)
		std::string GetExecutablePath();
#endif
		std::string GetCurrentWorkingPath();
		std::string ReadFEString(std::fstream& File);
	private:
		SINGLETON_PRIVATE_PART(FEFileSystem)

		int OpenFileDialogRequestID = 0;
		std::function<void(const std::string& FilePath)> PendingOpenFileDialogCallback;
	};

#ifdef FEBASICAPPLICATION_SHARED
	extern "C" __declspec(dllexport) void* GetFileSystem();
	#define FILE_SYSTEM (*static_cast<FEFileSystem*>(GetFileSystem()))
#else
	#define FILE_SYSTEM FEFileSystem::GetInstance()
#endif
}