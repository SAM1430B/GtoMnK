#pragma once
#include <windows.h>
#include <xinput.h>

namespace GtoMnK {
    namespace XInputHooks {
        typedef DWORD(WINAPI* XInputGetState_t)(DWORD, XINPUT_STATE*);

        extern XInputGetState_t TrueXInputGetState;
        extern XInputGetState_t TrueXInputGetStateEx;

        DWORD WINAPI MaskHook_XInputGetState(DWORD dwUserIndex, XINPUT_STATE* pState);
        DWORD WINAPI MaskHook_XInputGetStateEx(DWORD dwUserIndex, XINPUT_STATE* pState);
    }
}