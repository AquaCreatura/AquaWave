#include "file_helpers.h"
#include <Windows.h>
#include <string>
#include <vector>
#include <algorithm>

static const char* TEMP_PREFIX = "tmp";

std::string FilesystemHelper::CreateTempFile(std::string ext) {
	char tempPath[MAX_PATH + 1];
	if (!GetTempPathA(MAX_PATH, tempPath))
		return "";

	char tempFile[MAX_PATH + 1];
	if (GetTempFileNameA(tempPath, TEMP_PREFIX, 0, tempFile) == 0)
		return "";

	std::string oldPath(tempFile);
	std::string newPath = oldPath.substr(0, oldPath.find_last_of('.')) + ext;

	if (!MoveFileA(oldPath.c_str(), newPath.c_str())) {
		DeleteFileA(oldPath.c_str());
		return "";
	}
	return newPath;
}

bool FilesystemHelper::ErasePrecisedFile(std::string file_path) {
	return DeleteFileA(file_path.c_str()) != 0;
}

void FilesystemHelper::ClearTempFiles() {
	char tempPath[MAX_PATH + 1];
	if (!GetTempPathA(MAX_PATH, tempPath))
		return;

	std::string searchPath = std::string(tempPath) + TEMP_PREFIX + "*.*";
	WIN32_FIND_DATAA findData;
	HANDLE hFind = FindFirstFileA(searchPath.c_str(), &findData);
	if (hFind == INVALID_HANDLE_VALUE)
		return;

	do {
		if (!(findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {
			std::string fullPath = std::string(tempPath) + findData.cFileName;
			DeleteFileA(fullPath.c_str());
		}
	} while (FindNextFileA(hFind, &findData));

	FindClose(hFind);
}

std::string FilesystemHelper::GetTempFileDirectory() {
	char tempPath[MAX_PATH + 1];
	if (GetTempPathA(MAX_PATH, tempPath))
		return std::string(tempPath);
	return "";
}

bool FilesystemHelper::CreateDirectories(const std::string & dirPath)
{

	
	if (dirPath.empty())
		return false;

	size_t pos = 0;
	bool result = true;

	while ((pos = dirPath.find_first_of("\\/", pos + 1)) != std::string::npos)
	{
		std::string subDir = dirPath.substr(0, pos);
		if (subDir.empty())
			continue;

		if (!CreateDirectoryA(subDir.c_str(), NULL))
		{
			DWORD err = GetLastError();
			if (err != ERROR_ALREADY_EXISTS)
				result = false;
		}
	}

	// Создать последнюю папку
	if (!CreateDirectoryA(dirPath.c_str(), NULL))
	{
		DWORD err = GetLastError();
		if (err != ERROR_ALREADY_EXISTS)
			result = false;
	}

	return result;
	
}

int64_t FilesystemHelper::GetFileSize(const std::string & file_path)
{
	struct _stat64 st;
	if (_stat64(file_path.c_str(), &st) == 0)
		return st.st_size;
	return -1; // ошибка
}

bool FilesystemHelper::CopyPrecisedFile(const std::string & from, const std::string & to)

{
	// Получаем путь к директории назначения
	size_t slashPos = to.find_last_of("\\/");
	if (slashPos != std::string::npos)
	{
		std::string dir = to.substr(0, slashPos);
		if (!CreateDirectories(dir))
			return false;
	}

	// FALSE — перезаписывать существующий файл
	if (!CopyFileA(from.c_str(), to.c_str(), FALSE))
		return false;

	return true;
}
//=============================== FileWriter ======================================

bool FileWriter::CaptureFile(const std::string& file_path, bool do_rewrite)
{
	if (IsCaptured())
		return false;   // уже захвачен другой файл

						// Извлечь директорию из пути
	size_t pos = file_path.find_last_of("/\\");
	if (pos != std::string::npos)
	{
		std::string dir_path = file_path.substr(0, pos);
		if (!FilesystemHelper::CreateDirectories(dir_path))
			return false;
	}

	// Определяем режим открытия
	std::ios::openmode mode = std::ios::out | std::ios::binary;
	if (do_rewrite)
		mode |= std::ios::trunc;
	else
		mode |= std::ios::app;

	file_stream_.open(file_path, mode);
	if (!file_stream_.is_open())
		return false;

	file_path_ = std::move(file_path);
	return true;
}

bool FileWriter::IsCaptured() const
{
	return file_stream_.is_open();
}

bool FileWriter::WriteData(std::vector<uint8_t> &data_to_write)
{
	return WriteData((char*)data_to_write.data(), data_to_write.size());
}

bool FileWriter::WriteData(const char * data, int data_size)
{
	if (!IsCaptured())
		return false;

	file_stream_.write(data, data_size);
	return file_stream_.good();
}

std::string FileWriter::GetFilePath()
{
	return file_path_;
}

std::string FileWriter::ReleaseFile()
{
	if (!IsCaptured())
		return std::string();

	file_stream_.close();
	std::string released_path = std::move(file_path_);
	file_path_.clear();
	return released_path;
}

bool FileWriter::DeleteCurrentFile()
{
	if (!IsCaptured())
		return false;
	file_stream_.close();
	bool success = (std::remove(file_path_.c_str()) == 0);
	file_path_.clear();
	return success;
}
