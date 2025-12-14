#pragma once

namespace GtoMnK {
    class Keyboard {
    public:
        static SHORT WINAPI GetAsyncKeyStateHook(int vKey);
        static SHORT WINAPI GetKeyStateHook(int nVirtKey);
        static BOOL WINAPI GetKeyboardStateHook(PBYTE lpKeyState);
    };
}