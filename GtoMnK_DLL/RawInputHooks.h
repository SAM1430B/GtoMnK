#pragma once

namespace GtoMnK {
    namespace RawInputHooks {

		extern UINT(WINAPI* TrueGetRawInputData)(HRAWINPUT hRawInput, UINT uiCommand, LPVOID pData, PUINT pcbSize, UINT cbSizeHeader);
		extern BOOL(WINAPI* TrueRegisterRawInputDevices)(PCRAWINPUTDEVICE pRawInputDevices, UINT uiNumDevices, UINT cbSize);

		UINT WINAPI GetRawInputDataHook(HRAWINPUT hRawInput, UINT uiCommand, LPVOID pData, PUINT pcbSize, UINT cbSizeHeader);
		BOOL WINAPI RegisterRawInputDevicesHook(PCRAWINPUTDEVICE pRawInputDevices, UINT uiNumDevices, UINT cbSize);
    }
}