#include "pch.h"
#include "Keyboard.h"
#include "KeyboardState.h"

namespace GtoMnK {

    bool IsVirtualKeyPressed(int vkCode) {
        if (vkCode < 0 || vkCode >= 256) return false;

        if (vkCode == VK_SHIFT)
            return g_heldVirtualKeys.count(VK_SHIFT) || g_heldVirtualKeys.count(VK_LSHIFT) || g_heldVirtualKeys.count(VK_RSHIFT);
        if (vkCode == VK_CONTROL)
            return g_heldVirtualKeys.count(VK_CONTROL) || g_heldVirtualKeys.count(VK_LCONTROL) || g_heldVirtualKeys.count(VK_RCONTROL);
        if (vkCode == VK_MENU)
            return g_heldVirtualKeys.count(VK_MENU) || g_heldVirtualKeys.count(VK_LMENU) || g_heldVirtualKeys.count(VK_RMENU);

        return g_heldVirtualKeys.count(vkCode);
    }

    SHORT WINAPI Keyboard::GetAsyncKeyStateHook(int vKey) {
        if (IsVirtualKeyPressed(vKey)) {
            return (short)0x8001;
        }
        return GetAsyncKeyState(vKey);
    }

    SHORT WINAPI Keyboard::GetKeyStateHook(int nVirtKey) {
        if (IsVirtualKeyPressed(nVirtKey)) {
            return (short)0x8000;
        }
        return GetKeyState(nVirtKey);
    }

    BOOL WINAPI Keyboard::GetKeyboardStateHook(PBYTE lpKeyState) {
        if (!lpKeyState) {
            return FALSE;
        }

        BOOL originalResult = GetKeyboardState(lpKeyState);
        if (!originalResult) return FALSE;

        for (int i = 0; i < 256; ++i) {
            if (IsVirtualKeyPressed(i)) {
                lpKeyState[i] |= 0x80;
            }
            else {
                lpKeyState[i] &= ~0x80;
            }
        }

        return TRUE;
    }
}