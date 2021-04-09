#pragma once
#include <chrono>
#include "Utils.h"
#include <mmeapi.h>

namespace Utils
{
	int64_t getTimeMs()
	{
		std::chrono::time_point<std::chrono::system_clock> now = std::chrono::system_clock::now();
		auto duration = now.time_since_epoch();
		auto millis = std::chrono::duration_cast<std::chrono::milliseconds>(duration).count();
		return millis;
	}

	bool fileExists(const std::string& file)
	{
		struct stat info;
		if (stat(file.c_str(), &info) != 0)
			return false;
		else if (info.st_mode & S_IFREG)
			return true;
		else
			return false;
	}

	std::vector<std::string> getAudioDeviceList()
	{
		std::vector<std::string> devices;
		size_t devCount = waveOutGetNumDevs();
		for (UINT dev = 0; dev < devCount; dev++)
		{
			WAVEOUTCAPSA caps;
			waveOutGetDevCapsA(dev, &caps, sizeof(caps));
			devices.push_back(caps.szPname);
		}
		return devices;
	}

	void RemoveBOMFromFile(const std::string& path)
	{
		HANDLE hFile = CreateFileA(path.c_str(), GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
		DWORD dw = 0;
		DWORD file_size = GetFileSize(hFile, NULL);
		BYTE* file_buffer = new BYTE[file_size];
		ReadFile(hFile, file_buffer, file_size, &dw, NULL);
		CloseHandle(hFile);

		if (file_buffer[0] == 0xEF && file_buffer[1] == 0xBB && file_buffer[2] == 0xBF)
		{
			DWORD file_size_new = file_size - 3;
			BYTE* file_buffer_new = new BYTE[file_size_new];

			for (DWORD i = 0; i < file_size_new; i++)
			{
				file_buffer_new[i] = file_buffer[i + 3];
			}
			hFile = CreateFileA(path.c_str(), GENERIC_WRITE, FILE_SHARE_WRITE, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
			WriteFile(hFile, file_buffer_new, file_size_new, &dw, NULL);
			CloseHandle(hFile);
			delete[] file_buffer_new;
		}

		delete[] file_buffer;
	}
}