#pragma once

namespace GtoMnK {

    class Hooks {
    public:
        static void SetupHooks();
        static void RemoveHooks();

    private:
        static HCURSOR WINAPI SetCursorHook(HCURSOR hcursor);
        static BOOL WINAPI SetRectHook(LPRECT lprc, int xLeft, int yTop, int xRight, int yBottom);
        static BOOL WINAPI AdjustWindowRectHook(LPRECT lprc, DWORD dwStyle, BOOL bMenu);
        static BOOL WINAPI ClipCursorHook(const RECT* lpRect);
    };

}