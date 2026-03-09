#pragma once

#include <windows.h>
#include <string>

__declspec(selectany) bool g_NoJoyEnabled = true;
__declspec(selectany) bool g_CooperativeLevelUnlockerEnabled = true;

namespace INISettings
{
	inline std::string GetExecutableFolder()
	{
		char path[MAX_PATH];
		GetModuleFileNameA(NULL, path, MAX_PATH);
		std::string exePath(path);
		size_t lastSlash = exePath.find_last_of("\\/");
		return exePath.substr(0, lastSlash);
	}

	inline std::string GetDllFolder(HMODULE hModule)
	{
		char path[MAX_PATH];
		GetModuleFileNameA(hModule, path, MAX_PATH);
		std::string dllPath(path);
		size_t lastSlash = dllPath.find_last_of("\\/");
		return dllPath.substr(0, lastSlash);
	}

	inline void Load(HMODULE hModule)
	{
		std::string iniPath = "";
		std::string exeIniPath = GetExecutableFolder() + "\\GtoMnK.ini";

		if (GetFileAttributesA(exeIniPath.c_str()) != INVALID_FILE_ATTRIBUTES)
		{
			iniPath = exeIniPath;
		}
		else
		{
			std::string dllIniPath = GetDllFolder(hModule) + "\\GtoMnK.ini";
			if (GetFileAttributesA(dllIniPath.c_str()) != INVALID_FILE_ATTRIBUTES)
			{
				iniPath = dllIniPath;
			}
		}

		// Read the settings if the INI file is exist.
		if (!iniPath.empty())
		{
			g_NoJoyEnabled = GetPrivateProfileIntA("dinput8", "NoJoy", 1, iniPath.c_str()) != 0;
			g_CooperativeLevelUnlockerEnabled = GetPrivateProfileIntA("dinput8", "CooperativeLevelUnlocker", 1, iniPath.c_str()) != 0;
		}
	}
}