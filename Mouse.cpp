#include "pch.h"
#include "Mouse.h"
#include "Keyboard.h"

extern HWND hwnd;
extern bool scrollmap;
extern POINT scroll;
extern int ignorerect;
extern POINT rectignore;
extern int userealmouse;
extern HANDLE hMutex;

namespace ScreenshotInput {

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

    bool Mouse::SendMouseClick(int x, int y, int z, int many) {
        if (userealmouse == 0) {
            POINT heer = { x, y };
            ScreenToClient(hwnd, &heer);
            LPARAM clickPos = MAKELPARAM(heer.x, heer.y);
            switch (z) {
            case 1: PostMessage(hwnd, WM_LBUTTONDOWN, MK_LBUTTON, clickPos); PostMessage(hwnd, WM_LBUTTONUP, 0, clickPos); Keyboard::keystatesend = VK_LEFT; break;
            case 2: PostMessage(hwnd, WM_RBUTTONDOWN, MK_RBUTTON, clickPos); PostMessage(hwnd, WM_RBUTTONUP, 0, clickPos); break;
            case 3: PostMessage(hwnd, WM_LBUTTONDOWN, MK_LBUTTON, clickPos); Keyboard::keystatesend = VK_LEFT; break;
            case 4: PostMessage(hwnd, WM_LBUTTONUP, 0, clickPos); break;
            case 5: PostMessage(hwnd, WM_RBUTTONDOWN, MK_RBUTTON, clickPos); Keyboard::keystatesend = VK_RIGHT; break;
            case 6: PostMessage(hwnd, WM_RBUTTONUP, 0, clickPos); break;
            case 20: PostMessage(hwnd, WM_MOUSEWHEEL, MAKEWPARAM(0, -120), clickPos); break;
            case 21: PostMessage(hwnd, WM_MOUSEWHEEL, MAKEWPARAM(0, 120), clickPos); break;
            case 30: PostMessage(hwnd, WM_LBUTTONDBLCLK, 0, clickPos); break;
            case 8: case 10: case 11: PostMessage(hwnd, WM_MOUSEMOVE, 0, clickPos); break;
            }
            return true;
        }

        // Logic for userealmouse == 1
        if (z == 1 || z == 2 || z == 3 || z == 6 || z == 10) {
            if (!Mutexlock(true)) return false;
        }
        double screenWidth = GetSystemMetrics(SM_CXSCREEN) - 1;
        double screenHeight = GetSystemMetrics(SM_CYSCREEN) - 1;
        double fx = x * (65535.0f / screenWidth);
        double fy = y * (65535.0f / screenHeight);

        INPUT input[3] = {};
        input[0].type = INPUT_MOUSE;
        input[0].mi.dx = static_cast<LONG>(fx);
        input[0].mi.dy = static_cast<LONG>(fy);
        input[0].mi.dwFlags = MOUSEEVENTF_MOVE | MOUSEEVENTF_ABSOLUTE;

        int inputCount = 1;
        switch (z) {
        case 1: input[1] = { INPUT_MOUSE, MOUSEEVENTF_LEFTDOWN }; input[2] = { INPUT_MOUSE, MOUSEEVENTF_LEFTUP }; inputCount = 3; Keyboard::keystatesend = VK_LEFT; break;
        case 2: input[1] = { INPUT_MOUSE, MOUSEEVENTF_RIGHTDOWN }; input[2] = { INPUT_MOUSE, MOUSEEVENTF_RIGHTUP }; inputCount = 3; break;
        case 3: input[1] = { INPUT_MOUSE, MOUSEEVENTF_LEFTDOWN }; inputCount = 2; Keyboard::keystatesend = VK_LEFT; break;
        case 5: input[1] = { INPUT_MOUSE, MOUSEEVENTF_LEFTUP }; inputCount = 2; break;
        case 7: input[1] = { INPUT_MOUSE, MOUSEEVENTF_RIGHTUP }; inputCount = 2; break;
        }
        SendInput(inputCount, input, sizeof(INPUT));

        if (z == 1 || z == 2 || z == 5 || z == 7 || z == 10 || z == 11) {
            Mutexlock(false);
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