#pragma once
#include "pch.h"

namespace GtoMnK {

    class Mouse {
    public:
        static int Xf;
        static int Yf;
        static HICON hCursor;

        static BOOL WINAPI MyGetCursorPos(PPOINT lpPoint);
        static BOOL WINAPI MySetCursorPos(int X, int Y);


        static bool Mutexlock(bool lock);

        static void DrawBeautifulCursor(HDC hdc);

    private:
        static int colorfulSword[20][20];
        static COLORREF colors[5];
    };

}