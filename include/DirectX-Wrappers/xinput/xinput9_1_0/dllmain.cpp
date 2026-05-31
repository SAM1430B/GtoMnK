#include <Windows.h>
#include <Xinput.h>
#include "..\XInputExports.h"

#include "..\Common\Logging.h"
#include "ChainLoad.h"

std::ofstream Log::LOG("xinput9_1_0.log");

bool WINAPI DllMain(HMODULE hModule, DWORD dwReason, LPVOID lpReserved)
{
    static HMODULE hSystem = nullptr;

    switch (dwReason)
    {
    case DLL_PROCESS_ATTACH:
        DisableThreadLibraryCalls(hModule);

        // Load dll
        char path[MAX_PATH];
        if (GetFileAttributesA("xinput9_1_0.Chained.dll") != INVALID_FILE_ATTRIBUTES)
        {
            // If xinput9_1_0.Chained.dll exists, load it
            strcpy_s(path, "xinput9_1_0.Chained.dll");
        }
        else
        {
            // Load system xinput9_1_0.dll
            GetSystemDirectoryA(path, MAX_PATH);
            strcat_s(path, "\\xinput9_1_0.dll");
        }
        Log() << "Loading " << path;
        hSystem = LoadLibraryA(path);

        if (hSystem)
        {
            SystemXInputGetState = (FPXGetState)GetProcAddress(hSystem, "XInputGetState");
            SystemXInputSetState = (FPXSetState)GetProcAddress(hSystem, "XInputSetState");
            SystemXInputGetCapabilities = (FPXGetCapabilities)GetProcAddress(hSystem, "XInputGetCapabilities");
            SystemXInputEnable = (FPXEnable)GetProcAddress(hSystem, "XInputEnable");
            SystemXInputGetBatteryInformation = (FPXGetBatteryInformation)GetProcAddress(hSystem, "XInputGetBatteryInformation");
            SystemXInputGetKeystroke = (FPXGetKeystroke)GetProcAddress(hSystem, "XInputGetKeystroke");
            SystemXInputGetDSoundAudioDeviceGuids = (FPXGetDSoundAudioDeviceGuids)GetProcAddress(hSystem, "XInputGetDSoundAudioDeviceGuids");
            SystemXInputGetAudioDeviceIds = (FPXGetAudioDeviceIds)GetProcAddress(hSystem, "XInputGetAudioDeviceIds");

            SystemXInputGetStateEx = (FPXGetStateEx)GetProcAddress(hSystem, (LPCSTR)100);
            SystemXInputWaitForGuideButton = (FPXWaitForGuideButton)GetProcAddress(hSystem, (LPCSTR)101);
            SystemXInputCancelGuideButtonWait = (FPXCancelGuideButtonWait)GetProcAddress(hSystem, (LPCSTR)102);
            SystemXInputPowerOffController = (FPXPowerOffController)GetProcAddress(hSystem, (LPCSTR)103);
        }
        else
        {
            Log() << "Failed to load xinput9_1_0 library!";
        }

        // Load GtoMnK dll
#ifdef _WIN64
        if (LoadLibraryA("GtoMnK64.dll")) Log() << "Successfully loaded GtoMnK64.dll";
        else Log() << "Failed to load GtoMnK64.dll. Error code: " << GetLastError();
#else
        if (LoadLibraryA("GtoMnK32.dll")) Log() << "Successfully loaded GtoMnK32.dll";
        else Log() << "Failed to load GtoMnK32.dll. Error code: " << GetLastError();
#endif
        // Load any .ChainLoad$.dll files
        ChainLoader::LoadDlls();
        break;

    case DLL_PROCESS_DETACH:
        if (hSystem)
        {
            FreeLibrary(hSystem);
            hSystem = nullptr;
        }
        break;
    }

    return true;
}