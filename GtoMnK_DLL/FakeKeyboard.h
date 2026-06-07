#pragma once
#include <set>

namespace GtoMnK {
    class FakeKeyboard {
    public:
        static std::set<int> heldVirtualKeys;

        static SHORT WINAPI GetAsyncKeyStateHook(int vKey);
        static SHORT WINAPI GetKeyStateHook(int nVirtKey);
        static BOOL WINAPI GetKeyboardStateHook(PBYTE lpKeyState);
    };
}