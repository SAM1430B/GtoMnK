#include "pch.h"
#include "ScreenshotInteract.h"
#include "Mouse.h"

extern HWND hwnd;
extern int showmessage;
extern bool onoroff;
extern bool gotcursoryet;
extern int alwaysdrawcursor;
extern int Xoffset;
extern int Yoffset;
extern int controllerID;
extern bool pausedraw;
extern bool foundit;

extern int bmpAtype, bmpBtype, bmpXtype, bmpYtype, bmpCtype, bmpDtype, bmpEtype, bmpFtype;
extern int startsearchA, startsearchB, startsearchX, startsearchY, startsearchC, startsearchD, startsearchE, startsearchF;

extern std::vector<BYTE> largePixels, smallPixels;
extern SIZE screenSize;
extern int strideLarge, strideSmall;
extern int smallW, smallH;


namespace ScreenshotInput {

    void vibrateController(int controllerId, WORD strength) {
        XINPUT_VIBRATION vibration = {};
        vibration.wLeftMotorSpeed = strength;
        vibration.wRightMotorSpeed = strength;
        XInputSetState(controllerId, &vibration);
        Sleep(50);
        vibration.wLeftMotorSpeed = 0;
        vibration.wRightMotorSpeed = 0;
        XInputSetState(controllerId, &vibration);
    }

    std::string UGetExecutableFolder_local() {
        char path[MAX_PATH];
        GetModuleFileNameA(NULL, path, MAX_PATH);
        std::string exePath(path);
        size_t lastSlash = exePath.find_last_of("\\/");
        return exePath.substr(0, lastSlash);
    }


    int ScreenshotInteract::CalculateStride(int width) {
        return ((width * 3 + 3) & ~3);
    }

    bool ScreenshotInteract::Save24BitBMP(const wchar_t* filename, const BYTE* pixels, int width, int height) {
        int stride = CalculateStride(width);
        int imageSize = stride * height;

        BITMAPFILEHEADER bfh = {};
        bfh.bfType = 0x4D42;  // "BM"
        bfh.bfOffBits = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER);
        bfh.bfSize = bfh.bfOffBits + imageSize;

        BITMAPINFOHEADER bih = {};
        bih.biSize = sizeof(BITMAPINFOHEADER);
        bih.biWidth = width;
        bih.biHeight = height; // bottom-up BMP
        bih.biPlanes = 1;
        bih.biBitCount = 24;
        bih.biCompression = BI_RGB;
        bih.biSizeImage = imageSize;

        HANDLE hFile = CreateFileW(filename, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
        if (hFile == INVALID_HANDLE_VALUE) return false;

        DWORD written;
        WriteFile(hFile, &bfh, sizeof(bfh), &written, NULL);
        WriteFile(hFile, &bih, sizeof(bih), &written, NULL);
        WriteFile(hFile, pixels, imageSize, &written, NULL);
        CloseHandle(hFile);
        return true;
    }


    bool ScreenshotInteract::FindSubImage24(const BYTE* largeData, int largeW, int largeH, int strideLarge,
        const BYTE* smallData, int smallW, int smallH, int strideSmall,
        POINT& foundAt, int Xstart, int Ystart) {
        for (int y = Ystart; y <= largeH - smallH; ++y) {
            for (int x = Xstart; x <= largeW - smallW; ++x) {
                bool match = true;
                for (int j = 0; j < smallH && match; ++j) {
                    const BYTE* pLarge = largeData + (y + j) * strideLarge + x * 3;
                    const BYTE* pSmall = smallData + j * strideSmall;
                    if (memcmp(pLarge, pSmall, smallW * 3) != 0) {
                        match = false;
                    }
                }
                if (match) {
                    foundAt.x = x;
                    foundAt.y = y;
                    return true;
                }
            }
        }
        return false;
    }

    bool ScreenshotInteract::LoadBMP24Bit(const wchar_t* filename, std::vector<BYTE>& pixels, int& width, int& height, int& stride) {
        HBITMAP hbm = (HBITMAP)LoadImageW(NULL, filename, IMAGE_BITMAP, 0, 0, LR_LOADFROMFILE | LR_CREATEDIBSECTION);
        if (!hbm) return false;

        BITMAP bmp;
        GetObject(hbm, sizeof(BITMAP), &bmp);
        width = bmp.bmWidth;
        height = bmp.bmHeight;
        stride = CalculateStride(width);
        pixels.resize(stride * height);

        BITMAPINFO bmi = {};
        bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
        bmi.bmiHeader.biWidth = width;
        bmi.bmiHeader.biHeight = -height; // Request top-down DIB
        bmi.bmiHeader.biPlanes = 1;
        bmi.bmiHeader.biBitCount = 24;
        bmi.bmiHeader.biCompression = BI_RGB;

        HDC hdc = CreateCompatibleDC(NULL);
        GetDIBits(hdc, hbm, 0, height, pixels.data(), &bmi, DIB_RGB_COLORS);
        DeleteDC(hdc);
        DeleteObject(hbm);
        return true;
    }

    bool ScreenshotInteract::SaveWindow10x10BMP(const wchar_t* filename, int x, int y) {
        if (!hwnd) return false;
        HDC hdcWindow = GetDC(hwnd);
        HDC hdcMem = CreateCompatibleDC(hdcWindow);

        int width = 10, height = 10;
        HBITMAP hbmMem = CreateCompatibleBitmap(hdcWindow, width, height);
        SelectObject(hdcMem, hbmMem);

        BitBlt(hdcMem, 0, 0, width, height, hdcWindow, x, y, SRCCOPY);

        BITMAPINFO bmi = {};
        bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
        bmi.bmiHeader.biWidth = width;
        bmi.bmiHeader.biHeight = -height; // top-down
        bmi.bmiHeader.biPlanes = 1;
        bmi.bmiHeader.biBitCount = 24;
        bmi.bmiHeader.biCompression = BI_RGB;

        std::vector<BYTE> pixels(CalculateStride(width) * height);
        GetDIBits(hdcMem, hbmMem, 0, height, pixels.data(), &bmi, DIB_RGB_COLORS);

        bool ok = Save24BitBMP(filename, pixels.data(), width, height);

        DeleteObject(hbmMem);
        DeleteDC(hdcMem);
        ReleaseDC(hwnd, hdcWindow);
        return ok;
    }


    HBITMAP ScreenshotInteract::CaptureWindow(bool draw) {
        if (!hwnd) return NULL;
        HDC hdcWindow = GetDC(hwnd);
        HDC hdcMem = CreateCompatibleDC(hdcWindow);

        RECT rcClient;
        GetClientRect(hwnd, &rcClient);
        int width = rcClient.right - rcClient.left;
        int height = rcClient.bottom - rcClient.top;
        screenSize = { width, height };
        strideLarge = CalculateStride(width);
        largePixels.resize(strideLarge * height);

        HBITMAP hbm = CreateCompatibleBitmap(hdcWindow, width, height);
        SelectObject(hdcMem, hbm);

        BitBlt(hdcMem, 0, 0, width, height, hdcWindow, 0, 0, SRCCOPY);

        if (draw) {
            if (showmessage == 1) TextOutA(hdcWindow, Mouse::Xf, Mouse::Yf, "BMP MODE", 8);
            else if (showmessage == 2) TextOutA(hdcWindow, Mouse::Xf, Mouse::Yf, "CURSOR MODE", 11);
            else if (showmessage == 3) TextOutA(hdcWindow, Mouse::Xf, Mouse::Yf, "EDIT MODE", 9);
            else if (showmessage == 10) TextOutA(hdcWindow, Mouse::Xf, Mouse::Yf, "BUTTON MAPPED", 13);
            else if (showmessage == 12) TextOutA(hdcWindow, 20, 20, "DISCONNECTED!", 14);
            else if (showmessage == 69) TextOutA(hdcWindow, Mouse::Xf, Mouse::Yf, "SHUTTING DOWN", 13);
            else if (showmessage == 70) TextOutA(hdcWindow, Mouse::Xf, Mouse::Yf, "STARTING!", 10);
            else if (Mouse::hCursor != 0 && onoroff == true) {
                gotcursoryet = true;
                DrawIconEx(hdcWindow, Mouse::Xf - Xoffset, Mouse::Yf - Yoffset, Mouse::hCursor, 32, 32, 0, NULL, DI_NORMAL);
            }
            else if (onoroff == true && (alwaysdrawcursor == 1 || !gotcursoryet)) {
                Mouse::DrawBeautifulCursor(hdcWindow);
            }
        }

        BITMAPINFO bmi = {};
        bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
        bmi.bmiHeader.biWidth = width;
        bmi.bmiHeader.biHeight = -height;
        bmi.bmiHeader.biPlanes = 1;
        bmi.bmiHeader.biBitCount = 24;
        bmi.bmiHeader.biCompression = BI_RGB;

        GetDIBits(hdcWindow, hbm, 0, height, largePixels.data(), &bmi, DIB_RGB_COLORS);

        DeleteDC(hdcMem);
        ReleaseDC(hwnd, hdcWindow);
        return hbm;
    }


    bool ScreenshotInteract::Buttonaction(const char key[3], int mode, int serchnum, int& startsearch) {
        if (mode == 2) { // Handle Edit Mode first
            if (showmessage == 0) {
                Sleep(100);
                std::string path = UGetExecutableFolder_local() + key + std::to_string(serchnum) + ".bmp";
                std::wstring wpath(path.begin(), path.end());
                SaveWindow10x10BMP(wpath.c_str(), Mouse::Xf, Mouse::Yf);
                showmessage = 10;
                return true;
            }
            else {
                showmessage = 11;
                return false;
            }
        }

        pausedraw = true;
        Sleep(25);

        auto performSearch = [&](int index) {
            std::string path = UGetExecutableFolder_local() + key + std::to_string(index) + ".bmp";
            std::wstring wpath(path.begin(), path.end());

            if (LoadBMP24Bit(wpath.c_str(), smallPixels, smallW, smallH, strideSmall)) {
                HBITMAP hbmdsktop = CaptureWindow(false);
                if (hbmdsktop) {
                    POINT pt;
                    if (FindSubImage24(largePixels.data(), screenSize.cx, screenSize.cy, strideLarge, smallPixels.data(), smallW, smallH, strideSmall, pt, 0, 0)) {
                        vibrateController(controllerID, 15000);

                        int bmpType = 0;
                        if (strcmp(key, "\\A") == 0) { bmpType = bmpAtype; startsearch = index + 1; }
                        else if (strcmp(key, "\\B") == 0) { bmpType = bmpBtype; startsearch = index + 1; }
                        else if (strcmp(key, "\\X") == 0) { bmpType = bmpXtype; startsearch = index + 1; }
                        else if (strcmp(key, "\\Y") == 0) { bmpType = bmpYtype; startsearch = index + 1; }
                        else if (strcmp(key, "\\C") == 0) { bmpType = bmpCtype; startsearch = index + 1; }
                        else if (strcmp(key, "\\D") == 0) { bmpType = bmpDtype; startsearch = index + 1; }
                        else if (strcmp(key, "\\E") == 0) { bmpType = bmpEtype; startsearch = index + 1; }
                        else if (strcmp(key, "\\F") == 0) { bmpType = bmpFtype; startsearch = index + 1; }

                        bool movenotclick = (bmpType == 1);
                        bool clicknotmove = (bmpType == 2);

                        if (!clicknotmove) {
                            Mouse::Xf = pt.x + smallW / 2;
                            Mouse::Yf = pt.y + smallH / 2;
                        }

                        POINT clickPt = { Mouse::Xf, Mouse::Yf };
                        ClientToScreen(hwnd, &clickPt);

                        if (!movenotclick) {
                            Mouse::SendMouseClick(clickPt.x, clickPt.y, 8, 1); // Move
                            Sleep(10);
                            Mouse::SendMouseClick(clickPt.x, clickPt.y, 1, 3); // Left click (down and up)
                        }
                        else {
                            Mouse::SendMouseClick(clickPt.x, clickPt.y, 8, 1); // Move only
                        }

                        foundit = true;
                    }
                    DeleteObject(hbmdsktop);
                }
            }
            };

        for (int i = startsearch; i < serchnum; i++) {
            performSearch(i);
            if (foundit) break;
        }

        if (!foundit) {
            for (int i = 0; i < startsearch; i++) {
                performSearch(i);
                if (foundit) break;
            }
        }

        pausedraw = false;
        return foundit;
    }

}