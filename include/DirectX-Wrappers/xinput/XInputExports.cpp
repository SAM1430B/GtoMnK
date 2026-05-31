#include "XInputExports.h"

// Actual pointer definitions
FPXGetState SystemXInputGetState = nullptr;
FPXSetState SystemXInputSetState = nullptr;
FPXGetCapabilities SystemXInputGetCapabilities = nullptr;
FPXEnable SystemXInputEnable = nullptr;
FPXGetBatteryInformation SystemXInputGetBatteryInformation = nullptr;
FPXGetKeystroke SystemXInputGetKeystroke = nullptr;
FPXGetDSoundAudioDeviceGuids SystemXInputGetDSoundAudioDeviceGuids = nullptr;
FPXGetAudioDeviceIds SystemXInputGetAudioDeviceIds = nullptr;
FPXGetStateEx SystemXInputGetStateEx = nullptr;
FPXWaitForGuideButton SystemXInputWaitForGuideButton = nullptr;
FPXCancelGuideButtonWait SystemXInputCancelGuideButtonWait = nullptr;
FPXPowerOffController SystemXInputPowerOffController = nullptr;

// Exported wrapper functions
DWORD APIENTRY _XInputGetState(DWORD dwUserIndex, XINPUT_STATE* pState) {
    if (SystemXInputGetState) return SystemXInputGetState(dwUserIndex, pState);
    return ERROR_DEVICE_NOT_CONNECTED;
}

DWORD APIENTRY _XInputSetState(DWORD dwUserIndex, XINPUT_VIBRATION* pVibration) {
    if (SystemXInputSetState) return SystemXInputSetState(dwUserIndex, pVibration);
    return ERROR_DEVICE_NOT_CONNECTED;
}

DWORD APIENTRY _XInputGetCapabilities(DWORD dwUserIndex, DWORD dwFlags, XINPUT_CAPABILITIES* pCapabilities) {
    if (SystemXInputGetCapabilities) return SystemXInputGetCapabilities(dwUserIndex, dwFlags, pCapabilities);
    return ERROR_DEVICE_NOT_CONNECTED;
}

void APIENTRY _XInputEnable(BOOL enable) {
    if (SystemXInputEnable) SystemXInputEnable(enable);
}

DWORD APIENTRY _XInputGetBatteryInformation(DWORD dwUserIndex, BYTE devType, XINPUT_BATTERY_INFORMATION* pBatteryInformation) {
    if (SystemXInputGetBatteryInformation) return SystemXInputGetBatteryInformation(dwUserIndex, devType, pBatteryInformation);
    return ERROR_DEVICE_NOT_CONNECTED;
}

DWORD APIENTRY _XInputGetKeystroke(DWORD dwUserIndex, DWORD dwReserved, PXINPUT_KEYSTROKE pKeystroke) {
    if (SystemXInputGetKeystroke) return SystemXInputGetKeystroke(dwUserIndex, dwReserved, pKeystroke);
    return ERROR_DEVICE_NOT_CONNECTED;
}

DWORD APIENTRY _XInputGetDSoundAudioDeviceGuids(DWORD dwUserIndex, GUID* pDSoundRenderGuid, GUID* pDSoundCaptureGuid) {
    if (SystemXInputGetDSoundAudioDeviceGuids) {
        return SystemXInputGetDSoundAudioDeviceGuids(dwUserIndex, pDSoundRenderGuid, pDSoundCaptureGuid);
    }
    return ERROR_DEVICE_NOT_CONNECTED;
}

DWORD APIENTRY _XInputGetAudioDeviceIds(DWORD dwUserIndex, LPWSTR pRenderDeviceId, UINT* pRenderCount, LPWSTR pCaptureDeviceId, UINT* pCaptureCount) {
    if (SystemXInputGetAudioDeviceIds) {
        return SystemXInputGetAudioDeviceIds(dwUserIndex, pRenderDeviceId, pRenderCount, pCaptureDeviceId, pCaptureCount);
    }
    return ERROR_DEVICE_NOT_CONNECTED;
}

DWORD APIENTRY _XInputGetStateEx(DWORD dwUserIndex, XINPUT_STATE* pState) {
    if (SystemXInputGetStateEx) return SystemXInputGetStateEx(dwUserIndex, pState);
    return ERROR_DEVICE_NOT_CONNECTED;
}

DWORD APIENTRY _XInputWaitForGuideButton(DWORD dwUserIndex, DWORD dwFlag, void* pUnKnown) {
    if (SystemXInputWaitForGuideButton) return SystemXInputWaitForGuideButton(dwUserIndex, dwFlag, pUnKnown);
    return ERROR_DEVICE_NOT_CONNECTED;
}

DWORD APIENTRY _XInputCancelGuideButtonWait(DWORD dwUserIndex) {
    if (SystemXInputCancelGuideButtonWait) return SystemXInputCancelGuideButtonWait(dwUserIndex);
    return ERROR_DEVICE_NOT_CONNECTED;
}

DWORD APIENTRY _XInputPowerOffController(DWORD dwUserIndex) {
    if (SystemXInputPowerOffController) return SystemXInputPowerOffController(dwUserIndex);
    return ERROR_DEVICE_NOT_CONNECTED;
}