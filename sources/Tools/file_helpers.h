#pragma once

#include <fstream>
#include <vector>
#include <string>


class FilesystemHelper {
public:
	static std::string CreateTempFile(std::string ext = ".pcm");
	static bool		   ErasePrecisedFile(std::string file_path);
	static void		   ClearTempFiles();
	
	static std::string GetTempFileDirectory();
	static bool CreateDirectories(const std::string& dirPath);
	static bool CopyPrecisedFile(const std::string& from, const std::string& to);
	static int64_t GetFileSize(const std::string& file_path);

};

class FileWriter
{
public:
	bool CaptureFile(const std::string& file_path, bool do_rewrite = false);
	bool IsCaptured() const;
	bool WriteData(std::vector<uint8_t> &data_to_write);
	bool WriteData(const char* data, int data_size);
	std::string GetFilePath();
	std::string ReleaseFile();
	bool DeleteCurrentFile();
private:
	std::string file_path_;
	std::ofstream file_stream_;
};