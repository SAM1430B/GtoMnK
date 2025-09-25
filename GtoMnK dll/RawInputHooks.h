#pragma once

namespace GtoMnK {
    namespace RawInputHooks {
		UINT WINAPI GetRawInputDataHook(HRAWINPUT hRawInput, UINT uiCommand, LPVOID pData, PUINT pcbSize, UINT cbSizeHeader);
		BOOL WINAPI RegisterRawInputDevicesHook(PCRAWINPUTDEVICE pRawInputDevices, UINT uiNumDevices, UINT cbSize);
    }
}