#include "pch.h"
#include "Mouse.h"

extern HWND hwnd;
extern HANDLE hMutex;

namespace GtoMnK {

    int Mouse::Xf = 20;
    int Mouse::Yf = 20;

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
}