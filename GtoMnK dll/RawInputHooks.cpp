#include "pch.h"
#include "RawInputHooks.h"
#include "RawInput.h"
#include "Logging.h"
#include <easyhook.h>


// Thanks to ProtoInput.

HOOK_TRACE_INFO g_getRawInputDataHook = { NULL };
HOOK_TRACE_INFO g_registerRawInputDevicesHook = { NULL };


UINT WINAPI Hooked_GetRawInputData(HRAWINPUT hRawInput, UINT uiCommand, LPVOID pData, PUINT pcbSize, UINT cbSizeHeader) {
    UINT handleValue = (UINT)(UINT_PTR)hRawInput;
    if ((handleValue & 0xFF000000) == 0xAB000000) {
        UINT bufferIndex = handleValue & 0x00FFFFFF;

        if (bufferIndex >= GtoMnK::RawInput::RAWINPUT_BUFFER_SIZE) {
            return GetRawInputData(hRawInput, uiCommand, pData, pcbSize, cbSizeHeader);
        }

        if (pData == NULL) {
            *pcbSize = sizeof(RAWINPUT);
            return 0;
        }

        RAWINPUT* storedData = &GtoMnK::RawInput::g_inputBuffer[bufferIndex];
        memcpy(pData, storedData, sizeof(RAWINPUT));
        return sizeof(RAWINPUT);

    }
    else {
        return GetRawInputData(hRawInput, uiCommand, pData, pcbSize, cbSizeHeader);
    }
}

BOOL WINAPI Hooked_RegisterRawInputDevices(PCRAWINPUTDEVICE pRawInputDevices, UINT uiNumDevices, UINT cbSize) {
    for (UINT i = 0; i < uiNumDevices; ++i) {
        HWND targetHwnd = pRawInputDevices[i].hwndTarget;

        if (targetHwnd != NULL && targetHwnd != GtoMnK::RawInput::g_rawInputHwnd) {
            auto& windows = GtoMnK::RawInput::g_forwardingWindows;
            if (std::find(windows.begin(), windows.end(), targetHwnd) == windows.end()) {
                LOG("Captured new game window HWND: 0x%p", targetHwnd);
                windows.push_back(targetHwnd);
            }
        }
    }
    return TRUE;
}


namespace GtoMnK {
    namespace RawInputHooks {
        void Install() {
            HMODULE hUser32 = GetModuleHandleA("user32");
            if (!hUser32) {
                LOG("FATAL: Could not get a handle to user32.dll! RawInput hooks will not be installed.");
                return;
            }

            LhInstallHook(GetProcAddress(hUser32, "GetRawInputData"), Hooked_GetRawInputData, NULL, &g_getRawInputDataHook);
            LhInstallHook(GetProcAddress(hUser32, "RegisterRawInputDevices"), Hooked_RegisterRawInputDevices, NULL, &g_registerRawInputDevicesHook);
            
            ULONG threadIdList = 0;
            LhSetExclusiveACL(&threadIdList, 0, &g_getRawInputDataHook);
            LhSetExclusiveACL(&threadIdList, 0, &g_registerRawInputDevicesHook);
            LOG("RawInput hooks installed and enabled.");
        }

        void Uninstall() {
            LhUninstallAllHooks();
            LOG("RawInput hooks uninstalled.");
        }
    }
}