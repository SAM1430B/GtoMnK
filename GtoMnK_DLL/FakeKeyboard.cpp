#include "pch.h"
#include "FakeKeyboard.h"

namespace GtoMnK {

    std::set<int> FakeKeyboard::heldVirtualKeys;

    bool IsVirtualKeyPressed(int vkCode) {
        if (vkCode < 0 || vkCode >= 256) return false;

        if (vkCode == VK_SHIFT)
            return FakeKeyboard::heldVirtualKeys.count(VK_SHIFT) || FakeKeyboard::heldVirtualKeys.count(VK_LSHIFT) || FakeKeyboard::heldVirtualKeys.count(VK_RSHIFT);
        if (vkCode == VK_CONTROL)
            return FakeKeyboard::heldVirtualKeys.count(VK_CONTROL) || FakeKeyboard::heldVirtualKeys.count(VK_LCONTROL) || FakeKeyboard::heldVirtualKeys.count(VK_RCONTROL);
        if (vkCode == VK_MENU)
            return FakeKeyboard::heldVirtualKeys.count(VK_MENU) || FakeKeyboard::heldVirtualKeys.count(VK_LMENU) || FakeKeyboard::heldVirtualKeys.count(VK_RMENU);
        return FakeKeyboard::heldVirtualKeys.count(vkCode);
    }

    SHORT WINAPI FakeKeyboard::GetAsyncKeyStateHook(int vKey) {
        if (IsVirtualKeyPressed(vKey)) {
            return (short)0x8001;
        }

        return GetAsyncKeyState(vKey);
    }

    SHORT WINAPI FakeKeyboard::GetKeyStateHook(int nVirtKey) {
        if (IsVirtualKeyPressed(nVirtKey)) {
            return (short)0x8000;
        }

        return GetKeyState(nVirtKey);
    }

    BOOL WINAPI FakeKeyboard::GetKeyboardStateHook(PBYTE lpKeyState) {
        if (!lpKeyState) {
            return FALSE;
        }

        BOOL result = GetKeyboardState(lpKeyState);

        if (!result) return FALSE;

        for (int i = 0; i < 256; ++i) {
            if (IsVirtualKeyPressed(i)) {
                lpKeyState[i] |= 0x80;
            }
        }
        return TRUE;
    }
}