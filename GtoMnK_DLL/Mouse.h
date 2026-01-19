#pragma once
#include "pch.h"

namespace GtoMnK {

    class Mouse {
    public:
        static int Xf;
        static int Yf;

        static BOOL WINAPI GetCursorPosHook(PPOINT lpPoint);
        static BOOL WINAPI SetCursorPosHook(int X, int Y);


        static bool Mutexlock(bool lock);
    private:
        static int colorfulSword[20][20];
        static COLORREF colors[5];
    };

}