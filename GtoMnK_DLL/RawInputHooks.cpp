#include "pch.h"
#include "RawInputHooks.h"
#include "RawInput.h"
#include "Logging.h"
#include "Mouse.h"
#include "Keyboard.h"
#include "FakeCursor.h"
#include "OverlayMenu.h"

// Thanks to ProtoInput.

UINT(WINAPI* GtoMnK::RawInputHooks::TrueGetRawInputData)(HRAWINPUT hRawInput, UINT uiCommand, LPVOID pData, PUINT pcbSize, UINT cbSizeHeader) = nullptr;
BOOL(WINAPI* GtoMnK::RawInputHooks::TrueRegisterRawInputDevices)(PCRAWINPUTDEVICE pRawInputDevices, UINT uiNumDevices, UINT cbSize) = nullptr;

UINT WINAPI GtoMnK::RawInputHooks::GetRawInputDataHook(HRAWINPUT hRawInput, UINT uiCommand, LPVOID pData, PUINT pcbSize, UINT cbSizeHeader) {
    UINT handleValue = (UINT)(UINT_PTR)hRawInput;
    if ((handleValue & 0xFF000000) == 0xAB000000) {
        UINT bufferIndex = handleValue & 0x00FFFFFF;

        if (bufferIndex >= GtoMnK::RawInput::RAWINPUT_BUFFER_SIZE) {
            return TrueGetRawInputData(hRawInput, uiCommand, pData, pcbSize, cbSizeHeader);
        }

        if (pData == NULL) {
            *pcbSize = sizeof(RAWINPUT);
            return 0;
        }

        RAWINPUT* storedData = &GtoMnK::RawInput::g_inputBuffer[bufferIndex];
        
        if (uiCommand == RID_HEADER) {
            if (pData == NULL) { *pcbSize = sizeof(RAWINPUTHEADER); return 0; }
            memcpy(pData, &storedData->header, sizeof(RAWINPUTHEADER));
            return sizeof(RAWINPUTHEADER);
        }
        else if (uiCommand == RID_INPUT) {
            if (pData == NULL) { *pcbSize = sizeof(RAWINPUT); return 0; }
            memcpy(pData, storedData, sizeof(RAWINPUT));
            return *pcbSize;
        }
        return (UINT)-1;
    }

    return TrueGetRawInputData(hRawInput, uiCommand, pData, pcbSize, cbSizeHeader);
}

BOOL WINAPI GtoMnK::RawInputHooks::RegisterRawInputDevicesHook(PCRAWINPUTDEVICE pRawInputDevices, UINT uiNumDevices, UINT cbSize) {
    for (UINT i = 0; i < uiNumDevices; ++i) {

        bool isGenericDesktop = (pRawInputDevices[i].usUsagePage == 1);
        bool isMouse = (isGenericDesktop && pRawInputDevices[i].usUsage == 2);
        bool isKeyboard = (isGenericDesktop && pRawInputDevices[i].usUsage == 6);

        if (isMouse || isKeyboard) {
            HWND targetHwnd = pRawInputDevices[i].hwndTarget;

            if (targetHwnd != NULL && targetHwnd != GtoMnK::RawInput::g_rawInputHwnd) {
                if (targetHwnd == GtoMnK::FakeCursor::GetPointerWindow() ||
                    targetHwnd == GtoMnK::OverlayMenu::state.menuWindow) {
                    continue;
                }

                auto& windows = GtoMnK::RawInput::g_forwardingWindows;
                if (std::find(windows.begin(), windows.end(), targetHwnd) == windows.end()) {
                    LOG("Captured new game window HWND: 0x%p", targetHwnd);
                    windows.push_back(targetHwnd);
                }
            }
        }
    }
    return TrueRegisterRawInputDevices(pRawInputDevices, uiNumDevices, cbSize);
}