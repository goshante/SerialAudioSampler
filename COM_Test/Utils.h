#pragma once
#include <vector>
#include <Windows.h>
#include <memory>
#include <string>

namespace Utils
{
	template<class T>
	T findMostFrequentlyElement(const std::vector<T> vec)
	{
		T biggest = 0;
		size_t biggestCount = 0;

		for (size_t i = 0; i < vec.size(); i++)
		{
			size_t count = 0;
			for (auto& el_j : vec)
			{
				if (vec[i] == el_j)
					count++;
			}

			if (count > biggestCount)
			{
				biggest = vec[i];
				biggestCount = count;
			}
		}

		return biggest;
	}

	int64_t getTimeMs();
	bool fileExists(const std::string& file);
	std::vector<std::string> getAudioDeviceList();
	void RemoveBOMFromFile(const std::string& path);
}