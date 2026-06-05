#include "pch.h"
#include "OverlayNotification.h"
#include "INISettings.h"
#include <gdiplus.h>
#include <string>
#include "Logging.h"

#pragma comment (lib,"Gdiplus.lib")

bool IsOverlayNotificationEnabled = false;

namespace GtoMnK
{
    OverlayNotification OverlayNotification::state{};
#define WM_MOVE_overlayNotificationWindow (WM_APP + 3)

LRESULT WINAPI OverlayNotificationWndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg)
    {
    case WM_MOVE_overlayNotificationWindow:
        OverlayNotification::state.GetWindowDimensions(hWnd);
        break;
    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    default:
        break;
    }

    return DefWindowProc(hWnd, msg, wParam, lParam);
}

void OverlayNotification::GetWindowDimensions(HWND overlayNotificationWindow)
{
    if (!IsWindow(hwnd)) return;

    RECT cRect;
    GetClientRect(hwnd, &cRect);
    POINT topLeft = { cRect.left, cRect.top };
    ClientToScreen(hwnd, &topLeft);

    SetWindowPos(overlayNotificationWindow, HWND_TOP,
        topLeft.x, topLeft.y,
        cRect.right - cRect.left, cRect.bottom - cRect.top,
        SWP_NOZORDER | SWP_NOACTIVATE | SWP_NOREDRAW | SWP_SHOWWINDOW);
}

void OverlayNotification::UpdatePositionLoopInternal() {
    while (true) {
        if (IsWindow(hwnd)) {
            PostMessage(overlayNotificationWindow, WM_MOVE_overlayNotificationWindow, 0, 0);
        }
        Sleep(200);
    }
}

void OverlayNotification::StartInternal() {
    const auto hInstance = GetModuleHandle(NULL);
    Sleep(2000);

    Gdiplus::GdiplusStartupInput gdiplusStartupInput;
    Gdiplus::GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, NULL);

    WNDCLASSEXW wc = { 0 };
    wc.cbSize = sizeof(WNDCLASSEXW);
    wc.lpfnWndProc = OverlayNotificationWndProc;
    wc.hInstance = GetModuleHandle(NULL);
    wc.lpszClassName = L"GtoMnK_OverlayNotification";


    HWND parentWindow;
    if (createdWindowIsOwned)
        parentWindow = hwnd;
    else
        parentWindow = nullptr;

    RegisterClassExW(&wc);

    overlayNotificationWindow = CreateWindowExW(
        WS_EX_LAYERED | WS_EX_TRANSPARENT | WS_EX_TOOLWINDOW | WS_EX_NOACTIVATE,
        wc.lpszClassName, L"GtoMnK_OverlayNotification_Window",
        WS_POPUP,
        0, 0, 800, 600,
        parentWindow, // Is it `GW_OWNER`?
        nullptr, hInstance, nullptr);

    //ShowWindow(overlayNotificationWindow, SW_SHOWNA);

    IsOverlayNotificationEnabled = true;

    CreateThread(nullptr, 0, (LPTHREAD_START_ROUTINE)([](LPVOID) -> DWORD {
        OverlayNotification::state.UpdatePositionLoopInternal(); return 0;
        }), 0, 0, 0);

    LOG("Overlay Notification Window created successfully.");

    MSG msg;
    ZeroMemory(&msg, sizeof(msg));
    while (GetMessageW(&msg, NULL, 0, 0) > 0) {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }
}

void OverlayNotification::DrawOverlay(int showmessage) {
    static int lastShowMessage = -1;
    static int lastWidth = -1;
    static int lastHeight = -1;

    if (!overlayNotificationWindow || !IsWindow(overlayNotificationWindow)) return;

    RECT rect;
    GetClientRect(overlayNotificationWindow, &rect);
    int targetWidth = rect.right;
    int targetHeight = rect.bottom;

    if (showmessage == lastShowMessage && targetWidth == lastWidth && targetHeight == lastHeight) {
        return;
    }

    lastShowMessage = showmessage;
    lastWidth = targetWidth;
    lastHeight = targetHeight;

    if (targetWidth <= 0 || targetHeight <= 0) return;

    HDC hdcScreen = GetDC(NULL);
    HDC memDC = CreateCompatibleDC(hdcScreen);

    BITMAPINFO bmi = {};
    bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bmi.bmiHeader.biWidth = targetWidth;
    bmi.bmiHeader.biHeight = -targetHeight; 
    bmi.bmiHeader.biPlanes = 1;
    bmi.bmiHeader.biBitCount = 32;
    bmi.bmiHeader.biCompression = BI_RGB;

    void* pBits;
    HBITMAP memBitmap = CreateDIBSection(hdcScreen, &bmi, DIB_RGB_COLORS, &pBits, NULL, 0);
    HBITMAP oldBitmap = (HBITMAP)SelectObject(memDC, memBitmap);

    {
        Gdiplus::Graphics graphics(memDC);

        graphics.Clear(Gdiplus::Color(0, 0, 0, 0));

        if (showmessage != 0) {
            graphics.SetSmoothingMode(Gdiplus::SmoothingModeAntiAlias);
            graphics.SetTextRenderingHint(Gdiplus::TextRenderingHintSingleBitPerPixelGridFit);

            float scale = targetHeight / 1080.0f;
            if (scale < 0.5f) scale = 0.5f;

            Gdiplus::FontFamily fontFamily(L"Consolas");
            Gdiplus::Font font(&fontFamily, 24.0f * scale, Gdiplus::FontStyleRegular, Gdiplus::UnitPixel);

            std::wstring msgText = L"";
            if (showmessage == 1) msgText = L"Keyboard Mode";
            else if (showmessage == 2) msgText = L"Cursor Mode";
            else if (showmessage == 69) msgText = L"Disabled";
            else if (showmessage == 70) msgText = L"Enabled";
            else if (showmessage == 12) msgText = L"Controller disconnected";

            if (!msgText.empty()) {
                Gdiplus::RectF boundingBox;
                Gdiplus::PointF origin(0.0f, 0.0f);
                graphics.MeasureString(msgText.c_str(), -1, &font, origin, &boundingBox);

                float paddingX = 16.0f * scale;
                float paddingY = 10.0f * scale;
                float boxWidth = boundingBox.Width + (paddingX * 2.0f);
                float boxHeight = boundingBox.Height + (paddingY * 2.0f);

                float margin = 24.0f * scale;
                float startX = margin;
                float startY = targetHeight - boxHeight - margin;

                Gdiplus::GraphicsPath path;
                float cornerRadius = 8.0f * scale;
                Gdiplus::RectF boxRect(startX, startY, boxWidth, boxHeight);

                path.AddArc(boxRect.X, boxRect.Y, cornerRadius * 2, cornerRadius * 2, 180, 90);
                path.AddArc(boxRect.X + boxRect.Width - cornerRadius * 2, boxRect.Y, cornerRadius * 2, cornerRadius * 2, 270, 90);
                path.AddArc(boxRect.X + boxRect.Width - cornerRadius * 2, boxRect.Y + boxRect.Height - cornerRadius * 2, cornerRadius * 2, cornerRadius * 2, 0, 90);
                path.AddArc(boxRect.X, boxRect.Y + boxRect.Height - cornerRadius * 2, cornerRadius * 2, cornerRadius * 2, 90, 90);
                path.CloseFigure();

                // Alpha Transparency (180 out of 255). 0 = Completely invisible, 255 = Solid pitch black.
                Gdiplus::SolidBrush bgBrush(Gdiplus::Color(80, 0, 0, 0));
                graphics.FillPath(&bgBrush, &path);

                Gdiplus::SolidBrush textBrush(Gdiplus::Color(255, 255, 235, 50));
                Gdiplus::PointF textPos(startX + paddingX, startY + paddingY);

                graphics.DrawString(msgText.c_str(), -1, &font, textPos, &textBrush);
            }
        }
    }

    POINT ptSrc = { 0, 0 };
    SIZE szWin = { targetWidth, targetHeight };
    BLENDFUNCTION blend = { AC_SRC_OVER, 0, 255, AC_SRC_ALPHA };

    UpdateLayeredWindow(overlayNotificationWindow, hdcScreen, NULL, &szWin, memDC, &ptSrc, 0, &blend, ULW_ALPHA);

    // Cleanup
    SelectObject(memDC, oldBitmap);
    DeleteObject(memBitmap);
    DeleteDC(memDC);
    ReleaseDC(NULL, hdcScreen);
}

DWORD WINAPI OverlayNotificationThreadStart(LPVOID lpParameter)
{
    //LOG("Overlay Notification thread start");
    OverlayNotification::state.StartInternal();
    return 0;
}

void OverlayNotification::Initialise()
{
    const auto threadHandle = CreateThread(nullptr, 0,
        (LPTHREAD_START_ROUTINE)OverlayNotificationThreadStart, GetModuleHandle(0), 0, 0);

    if (threadHandle != nullptr)
        CloseHandle(threadHandle);

}

void OverlayNotification::CleanupGDIOverlay() {
    if (overlayNotificationWindow) DestroyWindow(overlayNotificationWindow);
    UnregisterClassA("GtoMnK_OverlayNotification", GetModuleHandle(NULL));
    Gdiplus::GdiplusShutdown(gdiplusToken);
}

}