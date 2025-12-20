#include "pch.h"
#include "Mouse.h"

extern HWND hwnd;
extern HANDLE hMutex;

namespace GtoMnK {

    int Mouse::Xf = 20;
    int Mouse::Yf = 20;
    HICON Mouse::hCursor = 0;

    int Mouse::colorfulSword[20][20] = {
        {1,1,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0}, {1,2,2,1,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
        {1,2,2,2,2,1,1,0,0,0,0,0,0,0,0,0,0,0,0,0}, {1,2,2,2,2,2,2,1,1,0,0,0,0,0,0,0,0,0,0,0},
        {1,2,2,2,2,2,2,2,2,1,1,0,0,0,0,0,0,0,0,0}, {1,2,2,2,2,2,2,2,2,2,2,1,1,0,0,0,0,0,0,0},
        {1,2,2,2,2,2,2,2,2,2,2,2,2,1,1,0,0,0,0,0}, {1,2,2,2,2,2,2,2,2,2,2,2,2,2,2,1,1,0,0,0},
        {1,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,1,0,0}, {1,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,1,0,0,0},
        {1,2,2,2,2,2,2,2,2,2,2,2,2,1,1,1,0,0,0,0}, {1,2,2,2,2,2,2,2,2,2,2,2,2,1,0,0,0,0,0,0},
        {1,2,2,2,2,2,2,2,2,2,2,2,1,0,0,0,0,0,0,0}, {1,2,2,2,2,2,2,2,2,2,2,2,1,0,0,0,0,0,0,0},
        {1,2,2,2,2,2,2,1,1,2,2,2,1,0,0,0,0,0,0,0}, {1,2,2,2,2,2,1,0,0,1,2,2,2,2,1,0,0,0,0,0},
        {1,2,2,2,2,1,0,0,0,0,1,2,2,2,1,0,0,0,0,0}, {1,1,2,2,1,0,0,0,0,0,0,1,2,2,2,1,0,0,0,0},
        {1,2,2,1,0,0,0,0,0,0,0,1,2,2,2,1,0,0,0,0}, {1,1,1,0,0,0,0,0,0,0,0,0,1,1,1,0,0,0,0,0},
    };

    COLORREF Mouse::colors[5] = {
        RGB(0, 0, 0),       // Transparent
        RGB(140, 140, 140), // Gray
        RGB(255, 255, 255), // White
        RGB(139, 69, 19),   // Brown
        RGB(50, 150, 255)   // Blue
    };


    BOOL WINAPI Mouse::GetCursorPosHook(PPOINT lpPoint) {
        if (!lpPoint) return FALSE;

        POINT mpos;
        mpos.x = Xf;
        mpos.y = Yf;

        ClientToScreen(hwnd, &mpos);

        *lpPoint = mpos;
        return TRUE;
    }

    BOOL WINAPI Mouse::SetCursorPosHook(int X, int Y) {
        POINT point = { X, Y };
        ScreenToClient(hwnd, &point);

        RECT rc;
        if (GetClientRect(hwnd, &rc)) {
            if (point.x < rc.left) point.x = rc.left;
            else if (point.x >= rc.right) point.x = rc.right - 1;

            if (point.y < rc.top) point.y = rc.top;
            else if (point.y >= rc.bottom) point.y = rc.bottom - 1;
        }

        Xf = point.x;
        Yf = point.y;
        return TRUE;
    }

    bool Mouse::Mutexlock(bool lock) {
        if (lock) {
            hMutex = CreateMutexA(NULL, FALSE, "Global\\PuttingInputByMessenils");
            if (hMutex == NULL) {
                return false;
            }
            if (GetLastError() == ERROR_ALREADY_EXISTS) {
                Sleep(5);
                ReleaseMutex(hMutex);
                CloseHandle(hMutex);
                return Mutexlock(true);
            }
        }
        else {
            if (hMutex) {
                ReleaseMutex(hMutex);
                CloseHandle(hMutex);
                hMutex = NULL;
            }
        }
        return true;
    }

    void Mouse::DrawBeautifulCursor(HDC hdc) {
        for (int y = 0; y < 20; y++) {
            for (int x = 0; x < 20; x++) {
                int val = colorfulSword[y][x];
                if (val != 0) {
                    HBRUSH hBrush = CreateSolidBrush(colors[val]);
                    RECT rect = { Xf + x, Yf + y, Xf + x + 1, Yf + y + 1 };
                    FillRect(hdc, &rect, hBrush);
                    DeleteObject(hBrush);
                }
            }
        }
    }

}