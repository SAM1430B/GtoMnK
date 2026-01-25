#pragma once

#include <windows.h>
#include <vector>
#include <string>
#include <algorithm>

// If a specific project uses "Logger::Msg" or "DebugPrint", you can change it here 
// or define CHAINLOAD_LOG before including this file.
#ifndef CHAINLOAD_LOG
#define CHAINLOAD_LOG(msg) Log() << msg
#endif

namespace ChainLoader
{
	inline void LoadDlls()
	{
		// Find files matching *.ChainLoad*.dll
		WIN32_FIND_DATAA findData;
		HANDLE hFind = FindFirstFileA("*.ChainLoad*.dll", &findData);
		std::vector<std::string> chainLoadFiles;

		if (hFind != INVALID_HANDLE_VALUE)
		{
			do
			{
				if (!(findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
				{
					std::string fileName = findData.cFileName;

					// Search for ".ChainLoad" (must include the dot)
					size_t chainPos = fileName.find(".ChainLoad");
					size_t dllPos = fileName.rfind(".dll");

					// 1. Must contain ".ChainLoad" and ".dll"
					if (chainPos != std::string::npos && dllPos != std::string::npos && dllPos > chainPos)
					{
						size_t suffixStart = chainPos + 10; // Length of ".ChainLoad"
						bool isValid = false;

						// Strict check:
						if (suffixStart == dllPos)
						{
							isValid = true;
						}
						else
						{
							bool allDigits = true;
							for (size_t i = suffixStart; i < dllPos; i++)
							{
								if (!isdigit(fileName[i]))
								{
									allDigits = false;
									break;
								}
							}
							if (allDigits) isValid = true;
						}

						if (isValid)
						{
							chainLoadFiles.push_back(fileName);
						}
					}
				}
			} while (FindNextFileA(hFind, &findData));
			FindClose(hFind);
		}

		// Sort files alphabetically
		std::sort(chainLoadFiles.begin(), chainLoadFiles.end());

		// Load the sorted DLLs
		for (const auto& file : chainLoadFiles)
		{
			CHAINLOAD_LOG("Processing ChainLoad DLL: " << file);
			
			if (LoadLibraryA(file.c_str()))
			{
				CHAINLOAD_LOG("Successfully loaded " << file);
			}
			else
			{
				CHAINLOAD_LOG("Failed to load " << file << ". Error code: " << GetLastError());
			}
		}
	}
}