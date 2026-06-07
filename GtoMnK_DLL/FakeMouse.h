#pragma once
#include "pch.h"

namespace GtoMnK {

    class FakeMouse {
    public:
        static int Xf;
        static int Yf;
        static HICON hCursor;

        static BOOL WINAPI GetCursorPosHook(PPOINT lpPoint);
        static BOOL WINAPI SetCursorPosHook(int X, int Y);
        static BOOL WINAPI ClipCursorHook(const RECT* lpRect);

    };

}