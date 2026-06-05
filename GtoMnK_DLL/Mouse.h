#pragma once
#include "pch.h"

namespace GtoMnK {

    class Mouse {
    public:
        static int Xf;
        static int Yf;
        static HICON hCursor;

        static BOOL WINAPI GetCursorPosHook(PPOINT lpPoint);
        static BOOL WINAPI SetCursorPosHook(int X, int Y);

    };

}