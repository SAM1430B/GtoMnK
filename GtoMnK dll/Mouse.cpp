#include "pch.h"
#include "Mouse.h"

extern HWND hwnd;
extern bool scrollmap;
extern POINT scroll;
extern int ignorerect;
extern POINT rectignore;
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


    BOOL WINAPI Mouse::MyGetCursorPos(PPOINT lpPoint) {
        if (!lpPoint) return FALSE;

        POINT mpos;
        if (!scrollmap) {
            if (ignorerect == 1) {
                mpos.x = Xf + rectignore.x;
                mpos.y = Yf + rectignore.y;
            }
            else {
                mpos.x = Xf;
                mpos.y = Yf;
                ClientToScreen(hwnd, &mpos);
            }
        }
        else {
            mpos.x = scroll.x;
            mpos.y = scroll.y;
            ClientToScreen(hwnd, &mpos);
        }
        *lpPoint = mpos;
        return TRUE;
    }

    BOOL WINAPI Mouse::MySetCursorPos(int X, int Y) {
        POINT point = { X, Y };
        ScreenToClient(hwnd, &point);
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