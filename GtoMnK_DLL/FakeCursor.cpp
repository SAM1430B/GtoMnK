/*
This file is imported from ProtoInput project, created by Ilyaki https://github.com/Ilyaki/ProtoInput
And is modified for GtoMnK usage. All credits to the original author.
*/

#include "pch.h"
#include "FakeCursor.h"
#include "Mouse.h"
#include "MainThread.h"
#include "Logging.h"

extern int drawProtoFakeCursor;
extern int createdWindowIsOwned;

namespace GtoMnK
{

    FakeCursor FakeCursor::state{};
#define WM_MOVE_pointerWindow (WM_APP + 1)
    LRESULT WINAPI FakeCursorWndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
    {
        switch (msg)
        {
        case WM_MOVE_pointerWindow:
            FakeCursor::state.GetWindowDimensions(hWnd);
            break;
        case WM_DESTROY:
            PostQuitMessage(0);
            return 0;
        default:
            break;
        }

        return DefWindowProc(hWnd, msg, wParam, lParam);
    }

    void FakeCursor::GetWindowDimensions(HWND pointerWindow)
    {
        HWND tHwnd = hwnd;
        if (pointerWindow == tHwnd)
            return;

        if (IsWindow(tHwnd))
        {
            RECT cRect;
            GetClientRect(tHwnd, &cRect);

            POINT topLeft = { cRect.left, cRect.top };
            ClientToScreen(tHwnd, &topLeft);

            HWND zOrderValue;
            if (createdWindowIsOwned)
                zOrderValue = HWND_TOP;
            else
                zOrderValue = HWND_TOPMOST;

            SetWindowPos(pointerWindow, zOrderValue,
                topLeft.x,
                topLeft.y,
                cRect.right - cRect.left,
                cRect.bottom - cRect.top,
                SWP_NOACTIVATE);
        }
    }
    
    void FakeCursor::DrawCursor()
    {

        if (oldHadShowCursor) //erase cursor
        {
            RECT fill{ oldX, oldY, oldX + cursorWidth, oldY + cursorHeight };
            FillRect(hdc, &fill, transparencyBrush); // Note: window, not screen coordinates!

        }

        oldHadShowCursor = showCursor;

		// Origanally from PrtotInput and modified for GtoMnK
        POINT pos = { GtoMnK::Mouse::Xf, GtoMnK::Mouse::Yf };
        if (hwnd) {
            ClientToScreen(hwnd, &pos);
        }

        ScreenToClient(pointerWindow, &pos);

        if (drawProtoFakeCursor == 2)
        {
            pos.x -= cursoroffsetx;
            pos.y -= cursoroffsety;

            if (pos.x < 0) pos.x = 0;
            if (pos.y < 0) pos.y = 0;

            if (showCursor)// && hdc && hCursor
            {
                if (DrawIconEx(hdc, pos.x, pos.y, hCursor, cursorWidth, cursorHeight, 0, transparencyBrush, DI_NORMAL))
                {
                    if (offsetSET == 1 && hCursor != LoadCursorW(NULL, MAKEINTRESOURCEW(32512)) && IsWindowVisible(pointerWindow)) //offset setting
                    {
                        HDC hdcMem = CreateCompatibleDC(hdc);
                        HBITMAP hbmScreen = CreateCompatibleBitmap(hdc, cursorWidth, cursorHeight);
                        SelectObject(hdcMem, hbmScreen);
                        BitBlt(hdcMem, 0, 0, cursorWidth, cursorHeight, hdc, pos.x, pos.y, SRCCOPY);
                        // Scan within the cursor screenshot pixel area
                        cursoroffsetx = -1;
                        cursoroffsety = -1;
                        for (int y = 0; y < cursorHeight; y++)
                        {
                            for (int x = 0; x < cursorWidth; x++)
                            {
                                COLORREF pixelColor = GetPixel(hdcMem, x, y); // Get copied pixel color
                                if (pixelColor != transparencyKey)
                                {
                                    cursoroffsetx = x;
                                    cursoroffsety = y;
                                    break;
                                }
                            }
                            if (cursoroffsetx != -1) break;
                        }
                        if (cursoroffsetx < 2) cursoroffsetx = 0;
                        if (cursoroffsety < 2) cursoroffsety = 0;
                        offsetSET++; //offset set to 2 should do drawing only now
                        DeleteDC(hdcMem);
                        DeleteObject(hbmScreen);
                    }
                    if (offsetSET == 0 && hCursor != LoadCursorW(NULL, MAKEINTRESOURCEW(32512)) && IsWindowVisible(pointerWindow)) //size setting
                    {
                        ICONINFO ii;
                        BITMAP bitmap;
                        if (GetIconInfo(hCursor, &ii))
                        {
                            if (GetObject(ii.hbmMask, sizeof(BITMAP), &bitmap))
                            {
                                cursorWidth = bitmap.bmWidth;
                                if (ii.hbmColor == NULL)
                                {//For monochrome icons, the hbmMask is twice the height of the icon and hbmColor is NULL
                                    cursorHeight = bitmap.bmHeight / 2;
                                }
                                else
                                {
                                    cursorHeight = bitmap.bmHeight;
                                }
                                DeleteObject(ii.hbmColor);
                                DeleteObject(ii.hbmMask);
                            }

                        }
                        offsetSET++; //size set, doing offset next run
                    }
                }
            }
        }
        else if (showCursor)
            DrawIcon(hdc, pos.x, pos.y, hCursor);
        oldX = pos.x;
        oldY = pos.y;
    }

    DWORD WINAPI FakeCursorDrawLoopThread(LPVOID lpParameter)
    {
        LOG("Fake cursor draw loop thread start");
        FakeCursor::state.StartDrawLoopInternal();

        return 0;
    }

    DWORD WINAPI PointerWindowLoopThread(LPVOID lpParameter)
    {
        LOG("Pointer window loop thread start");
        FakeCursor::state.UpdatePointerWindowLoopInternal();

        return 0;
    }

    void FakeCursor::UpdatePointerWindowLoopInternal()
    {
        while (true)
        {
            HWND tHwnd = hwnd;
            if (!IsWindow(tHwnd))
            {
                Sleep(2000);
                continue;
            }

            PostMessage(pointerWindow, WM_MOVE_pointerWindow, 0, 0);

            Sleep(5000);
        }
    }

    void FakeCursor::StartDrawLoopInternal()
    {
        int tick = 0;

        while (true)
        {
            std::unique_lock<std::mutex> lock(mutex);
            conditionvar.wait(lock);

            DrawCursor();

            //TODO: is this ok? (might eat cpu)
            Sleep(drawingEnabled ? 12 : 500);
        }
    }

    void FakeCursor::StartInternal()
    {
        const auto hInstance = GetModuleHandle(NULL);

        /*if (createdWindowIsOwned)
        {
            hwnd = GetMainWindowHandle(GetCurrentProcessId(), iniWindowName, iniClassName, 60000);

            if (!hwnd || !IsWindow(hwnd)) {
                LOG("Timeout: Game Window never appeared. Fake Cursor aborting.");
                return;
            }
            LOG("Pointer window is owned by the game window.");
        }*/

		LOG("Creating pointer window...");

        // Like a green screen
        transparencyBrush = (HBRUSH)CreateSolidBrush(transparencyKey);

        WNDCLASSW wc = { 0 };

        wc.lpfnWndProc = FakeCursorWndProc;
        wc.hInstance = hInstance;
        wc.hbrBackground = transparencyBrush;
        std::srand(static_cast<unsigned int>(std::time(nullptr)));
        const std::wstring classNameStr = std::wstring{ L"GtoMnK_Pointer_Window" } + std::to_wstring(std::rand());

        wc.lpszClassName = classNameStr.c_str();

        wc.style = CS_OWNDC | CS_NOCLOSE;

		HWND parentWindow;
        DWORD ws_ex;
        if (createdWindowIsOwned)
        {
            parentWindow = hwnd;
			ws_ex = 0;
        }
        else
        {
			parentWindow = nullptr;
			ws_ex = WS_EX_TOPMOST;
        }

        if (!RegisterClassW(&wc)) {
			LOG("Failed to register pointer window class!");
            return;
        }
        else
        {
            pointerWindow = CreateWindowExW(WS_EX_NOACTIVATE | WS_EX_NOINHERITLAYOUT | WS_EX_NOPARENTNOTIFY |
                ws_ex | WS_EX_TRANSPARENT | WS_EX_TOOLWINDOW | WS_EX_LAYERED,
                wc.lpszClassName, classNameStr.c_str(),
                WS_POPUP | WS_VISIBLE,
                0, 0, 800, 600,
                parentWindow, nullptr, hInstance, nullptr);

            SetLayeredWindowAttributes(pointerWindow, transparencyKey, 0, LWA_COLORKEY);

            if (!createdWindowIsOwned)
            {
                // Nucleus can put the game window above the pointer without this
                SetWindowPos(pointerWindow, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOREDRAW | SWP_NOSIZE);

                // ShowWindow(pointerWindow, SW_SHOWDEFAULT);
                // UpdateWindow(pointerWindow);

                EnableDisableFakeCursor(drawingEnabled);
            }

            hdc = GetDC(pointerWindow);

            //TODO: configurable cursor
            hCursor = LoadCursorW(NULL, MAKEINTRESOURCEW(32512));

            const auto threadHandle = CreateThread(nullptr, 0,
                (LPTHREAD_START_ROUTINE)FakeCursorDrawLoopThread, GetModuleHandle(0), 0, 0);

            if (threadHandle != nullptr)
                CloseHandle(threadHandle);

            const auto pointerThreadHandle = CreateThread(nullptr, 0,
                (LPTHREAD_START_ROUTINE)PointerWindowLoopThread, GetModuleHandle(0), 0, 0);

            if (pointerThreadHandle != nullptr)
                CloseHandle(pointerThreadHandle);

            if (createdWindowIsOwned)
                FakeCursor::EnableDisableFakeCursor(true);

            // Want to avoid doing anything in the message loop that might cause it to not respond, as the entire screen will say not responding...
            MSG msg;
            ZeroMemory(&msg, sizeof(msg));
            while (msg.message != WM_QUIT)
            {
                if (GetMessageW(&msg, pointerWindow, 0U, 0U))// Blocks
                {
                    TranslateMessage(&msg);
                    DispatchMessage(&msg);
                    continue;
                }
            }
        }
    }

    DWORD WINAPI FakeCursorThreadStart(LPVOID lpParameter)
    {
        LOG("Fake Cursor thread start");
        FakeCursor::state.StartInternal();
        return 0;
    }


    void FakeCursor::EnableDisableFakeCursor(bool enable)
    {
        state.drawingEnabled = enable;

        ShowWindow(state.pointerWindow, enable ? SW_SHOWDEFAULT : SW_HIDE);
        UpdateWindow(state.pointerWindow);
    }

    void FakeCursor::Initialise()
    {
        const auto threadHandle = CreateThread(nullptr, 0,
            (LPTHREAD_START_ROUTINE)FakeCursorThreadStart, GetModuleHandle(0), 0, 0);

        if (threadHandle != nullptr)
            CloseHandle(threadHandle);
    }

}
