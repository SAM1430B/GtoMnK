#include "pch.h"
#include "RawInputHooks.h"
#include "RawInput.h"
#include "Logging.h"
#include "Mouse.h"
#include "Keyboard.h"
#include <easyhook.h>
#include <algorithm>

// Thanks to ProtoInput.

UINT WINAPI GtoMnK::RawInputHooks::GetRawInputDataHook(HRAWINPUT hRawInput, UINT uiCommand, LPVOID pData, PUINT pcbSize, UINT cbSizeHeader) {
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

BOOL WINAPI GtoMnK::RawInputHooks::RegisterRawInputDevicesHook(PCRAWINPUTDEVICE pRawInputDevices, UINT uiNumDevices, UINT cbSize) {
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