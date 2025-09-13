#pragma once
#include "pch.h"

namespace GtoMnK {

    // Typedefs for original function pointers
    typedef BOOL(WINAPI* GetCursorPos_t)(LPPOINT lpPoint);
    typedef BOOL(WINAPI* SetCursorPos_t)(int X, int Y);
    typedef SHORT(WINAPI* GetAsyncKeyState_t)(int vKey);
    typedef SHORT(WINAPI* GetKeyState_t)(int nVirtKey);
    typedef BOOL(WINAPI* ClipCursor_t)(const RECT*);
    typedef HCURSOR(WINAPI* SetCursor_t)(HCURSOR hCursor);
    typedef BOOL(WINAPI* SetRect_t)(LPRECT lprc, int xLeft, int yTop, int xRight, int yBottom);
    typedef BOOL(WINAPI* AdjustWindowRect_t)(LPRECT lprc, DWORD dwStyle, BOOL bMenu);

    class Hooks {
    public:
        // Pointers to the original functions
        static GetCursorPos_t fpGetCursorPos;
        static SetCursorPos_t fpSetCursorPos;
        static GetAsyncKeyState_t fpGetAsyncKeyState;
        static GetKeyState_t fpGetKeyState;
        static ClipCursor_t fpClipCursor;
        static SetCursor_t fpSetCursor;
        static SetRect_t fpSetRect;
        static AdjustWindowRect_t fpAdjustWindowRect;

        // Main control functions
        static void SetupHooks();
        static void RemoveHooks();

        // The actual hook handler functions (proxies)
        static HCURSOR WINAPI HookedSetCursor(HCURSOR hcursor);
        static BOOL WINAPI HookedSetRect(LPRECT lprc, int xLeft, int yTop, int xRight, int yBottom);
        static BOOL WINAPI HookedAdjustWindowRect(LPRECT lprc, DWORD dwStyle, BOOL bMenu);
        static BOOL WINAPI HookedClipCursor(const RECT* lpRect);
    };

}