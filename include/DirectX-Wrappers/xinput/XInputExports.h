#pragma once
#include <Windows.h>
#include <Xinput.h>

// Typedefs for XInput functions
typedef DWORD(WINAPI* FPXGetState)(DWORD, XINPUT_STATE*);
typedef DWORD(WINAPI* FPXSetState)(DWORD, XINPUT_VIBRATION*);
typedef DWORD(WINAPI* FPXGetCapabilities)(DWORD, DWORD, XINPUT_CAPABILITIES*);
typedef void  (WINAPI* FPXEnable)(BOOL);
typedef DWORD(WINAPI* FPXGetBatteryInformation)(DWORD, BYTE, XINPUT_BATTERY_INFORMATION*);
typedef DWORD(WINAPI* FPXGetKeystroke)(DWORD, DWORD, PXINPUT_KEYSTROKE);
typedef DWORD(WINAPI* FPXGetDSoundAudioDeviceGuids)(DWORD, GUID*, GUID*);
typedef DWORD(WINAPI* FPXGetAudioDeviceIds)(DWORD, LPWSTR, UINT*, LPWSTR, UINT*);
typedef DWORD(WINAPI* FPXGetStateEx)(DWORD, XINPUT_STATE*);
typedef DWORD(WINAPI* FPXWaitForGuideButton)(DWORD, DWORD, void*);
typedef DWORD(WINAPI* FPXCancelGuideButtonWait)(DWORD);
typedef DWORD(WINAPI* FPXPowerOffController)(DWORD);

// Extern declarations so DllMain can set them, and the exports can use them
extern FPXGetState SystemXInputGetState;
extern FPXSetState SystemXInputSetState;
extern FPXGetCapabilities SystemXInputGetCapabilities;
extern FPXEnable SystemXInputEnable;
extern FPXGetBatteryInformation SystemXInputGetBatteryInformation;
extern FPXGetKeystroke SystemXInputGetKeystroke;
extern FPXGetDSoundAudioDeviceGuids SystemXInputGetDSoundAudioDeviceGuids;
extern FPXGetAudioDeviceIds SystemXInputGetAudioDeviceIds;
extern FPXGetStateEx SystemXInputGetStateEx;
extern FPXWaitForGuideButton SystemXInputWaitForGuideButton;
extern FPXCancelGuideButtonWait SystemXInputCancelGuideButtonWait;
extern FPXPowerOffController SystemXInputPowerOffController;