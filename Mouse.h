#pragma once
#include "pch.h"

namespace ScreenshotInput {

    class Mouse {
    public:
        // Virtual cursor state
        static int Xf;
        static int Yf;
        static HICON hCursor;

        // Hooked functions
        static BOOL WINAPI MyGetCursorPos(PPOINT lpPoint);
        static BOOL WINAPI MySetCursorPos(int X, int Y);

        // Input simulation
        static bool SendMouseClick(int x, int y, int z, int many);
        static bool Mutexlock(bool lock);

        // Cursor drawing
        static void DrawBeautifulCursor(HDC hdc);

    private:
        // Data for the custom cursor
        static int colorfulSword[20][20];
        static COLORREF colors[5];
    };

}