#pragma once

namespace GtoMnK {

    class InstallHooks {
    public:
        static void SetupHooks();
        static void RemoveHooks();

    private:
        static BOOL WINAPI SetRectHook(LPRECT lprc, int xLeft, int yTop, int xRight, int yBottom);
        static BOOL WINAPI AdjustWindowRectHook(LPRECT lprc, DWORD dwStyle, BOOL bMenu);
    };

}