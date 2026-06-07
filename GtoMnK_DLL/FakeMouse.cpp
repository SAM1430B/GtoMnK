#include "pch.h"
#include "FakeMouse.h"

extern HWND hwnd;
extern HANDLE hMutex;

namespace GtoMnK {

    int FakeMouse::Xf = 20;
    int FakeMouse::Yf = 20;
    HICON FakeMouse::hCursor = 0;

    BOOL WINAPI FakeMouse::GetCursorPosHook(PPOINT lpPoint) {
        if (!lpPoint) return FALSE;

        POINT mpos;
        mpos.x = Xf;
        mpos.y = Yf;

        ClientToScreen(hwnd, &mpos);

        *lpPoint = mpos;
        return TRUE;
    }

    BOOL WINAPI FakeMouse::SetCursorPosHook(int X, int Y) {
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

    BOOL WINAPI FakeMouse::ClipCursorHook(const RECT* lpRect) {
        return TRUE;
    }
}