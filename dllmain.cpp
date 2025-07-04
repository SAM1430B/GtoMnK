// dllmain.cpp : Defines the entry point for the DLL application.
#include "pch.h"
#include <windows.h>
#include <Xinput.h>
#include <tlhelp32.h>
#include <tchar.h>
#include <iostream>
#include <vector>
#include <cstdio>  // for swprintf
#include <psapi.h>
#include <string>
#include <cstdlib> // For strtoul
#include <dwmapi.h>
#pragma comment(lib, "dwmapi.lib")

#pragma comment(lib, "Xinput9_1_0.lib")

POINT fakecursor;
bool loop = true;
HWND hwnd = 0;

int width1 = 0;
int width2 = 0;
int height1 = 0;
int height2 = 0;

int numphotoA = -1;
int numphotoB = -1;
int numphotoX = -1;
int numphotoY = -1;

bool pausedraw = false;

int x = 0;

HBITMAP hbm;


std::string GetExecutableFolder() {
    TCHAR path[MAX_PATH];
    GetModuleFileName(NULL, path, MAX_PATH);
    std::string exePath(path);

    // Remove the executable name to get the folder
    size_t lastSlash = exePath.find_last_of("\\/");
    return exePath.substr(0, lastSlash);
}

void sendKey(char key) {
    INPUT ip = {};
    ip.type = INPUT_KEYBOARD;
    ip.ki.wVk = VkKeyScan(key); // Converts character to virtual key code
    ip.ki.dwFlags = 0; // Key down
    SendInput(1, &ip, sizeof(INPUT));

    // Key up event
    ip.ki.dwFlags = KEYEVENTF_KEYUP;
    SendInput(1, &ip, sizeof(INPUT));
}
void SendMouseClick(int x, int y, int z) {
    // Convert screen coordinates to absolute values
    double screenWidth = GetSystemMetrics(SM_CXSCREEN) - 1;
    double screenHeight = GetSystemMetrics(SM_CYSCREEN) - 1;
    double fx = x * (65535.0f / screenWidth);
    double fy = y * (65535.0f / screenHeight);

    INPUT input[3] = {};

    // Move the mouse to the specified position
    input[0].type = INPUT_MOUSE;
    input[0].mi.dx = static_cast<LONG>(fx);
    input[0].mi.dy = static_cast<LONG>(fy);
    input[0].mi.dwFlags = MOUSEEVENTF_MOVE | MOUSEEVENTF_ABSOLUTE;
    if (z == 1) { //left button press
        // Simulate mouse left button down
        input[1].type = INPUT_MOUSE;
        input[1].mi.dwFlags = MOUSEEVENTF_LEFTDOWN;

        // Simulate mouse left button up
        input[2].type = INPUT_MOUSE;
        input[2].mi.dwFlags = MOUSEEVENTF_LEFTUP;
    }
    else if (z == 2) { //right button press
        // Simulate mouse left button down
        input[1].type = INPUT_MOUSE;
        input[1].mi.dwFlags = MOUSEEVENTF_RIGHTDOWN;

        // Simulate mouse left button up
        input[2].type = INPUT_MOUSE;
        input[2].mi.dwFlags = MOUSEEVENTF_RIGHTUP;
    }

    SendInput(3, input, sizeof(INPUT));
}

bool FindSubImage24(
    const BYTE* largeData, int largeW, int largeH, int strideLarge,
    const BYTE* smallData, int smallW, int smallH, int strideSmall,
    POINT& foundAt
) {
    for (int y = 0; y <= largeH - smallH; ++y) {
        for (int x = 0; x <= largeW - smallW; ++x) {
            bool match = true;
            for (int j = 0; j < smallH && match; ++j) {
                const BYTE* pLarge = largeData + (y + j) * strideLarge + x * 3;
                const BYTE* pSmall = smallData + j * strideSmall;
                if (memcmp(pLarge, pSmall, smallW * 3) != 0) {
                    match = false;
                }
            }
            // DeleteObject(largeData);
            if (match) {
                foundAt.x = x;
                foundAt.y = y;
                return true;
            }
        }
    }
    return false;
}
HWND GetMainWindowHandle(DWORD targetPID) {
    HWND hwnd = nullptr;
    struct HandleData {
        DWORD pid;
        HWND hwnd;
    } data = { targetPID, nullptr };

    auto EnumWindowsCallback = [](HWND hWnd, LPARAM lParam) -> BOOL {
        HandleData* pData = reinterpret_cast<HandleData*>(lParam);
        DWORD windowPID = 0;
        GetWindowThreadProcessId(hWnd, &windowPID);
        if (windowPID == pData->pid && GetWindow(hWnd, GW_OWNER) == nullptr && IsWindowVisible(hWnd)) {
            pData->hwnd = hWnd;
            return FALSE; // Stop enumeration
        }
        return TRUE; // Continue
        };

    EnumWindows(EnumWindowsCallback, reinterpret_cast<LPARAM>(&data));
    return data.hwnd;
}

int CalculateStride(int width) {
    return ((width * 3 + 3) & ~3);
}

bool Save24BitBMP(const wchar_t* filename, const BYTE* pixels, int width, int height) { //for testing purposes
    int stride = ((width * 3 + 3) & ~3);
    int imageSize = stride * height;

    BITMAPFILEHEADER bfh = {};
    bfh.bfType = 0x4D42;  // "BM"
    bfh.bfOffBits = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER);
    bfh.bfSize = bfh.bfOffBits + imageSize;

    BITMAPINFOHEADER bih = {};
    bih.biSize = sizeof(BITMAPINFOHEADER);
    bih.biWidth = width;
    bih.biHeight = -height;  // bottom-up BMP (positive height)
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


bool LoadBMP24Bit(const wchar_t* filename, std::vector<BYTE>& pixels, int& width, int& height, int& stride) {
    hbm = (HBITMAP)LoadImageW(NULL, filename, IMAGE_BITMAP, 0, 0, LR_LOADFROMFILE | LR_CREATEDIBSECTION);
    if (!hbm) return false;

    BITMAP bmp;
    GetObject(hbm, sizeof(BITMAP), &bmp);
    width = bmp.bmWidth - 1;
    height = bmp.bmHeight - 1;
    stride = CalculateStride(width);

    pixels.resize(stride * height);

    BITMAPINFO bmi = {};
    bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bmi.bmiHeader.biWidth = width;
    bmi.bmiHeader.biHeight = -height;
    bmi.bmiHeader.biPlanes = 1;
    bmi.bmiHeader.biBitCount = 24;
    bmi.bmiHeader.biCompression = BI_RGB;

    HDC hdc = GetDC(NULL);
    GetDIBits(hdc, hbm, 0, height, pixels.data(), &bmi, DIB_RGB_COLORS);
    ReleaseDC(NULL, hdc);
    DeleteObject(hbm);
    return true;
}

std::string getIniString(const std::string& section, const std::string& key, const std::string& defaultValue, const std::string& iniPath) {
    char buffer[256]; // Buffer to hold the retrieved string
    GetPrivateProfileString(section.c_str(), key.c_str(), defaultValue.c_str(), buffer, sizeof(buffer), iniPath.c_str());
    return std::string(buffer);
}
HBITMAP Draw(HWND hwnd, int X, int Y, bool screenshot) 
{

}
HBITMAP CaptureWindow24Bit(HWND hwnd, SIZE& capturedwindow, std::vector<BYTE>& pixels, int& strideOut) {
    RECT rcClient;
    GetClientRect(hwnd, &rcClient);
    int width = rcClient.right - rcClient.left;
    int height = rcClient.bottom - rcClient.top;

    capturedwindow.cx = width;
    capturedwindow.cy = height;

    int stride = ((width * 3 + 3) & ~3);
    strideOut = stride;
    pixels.resize(stride * height);

    BITMAPINFO bmi = {};
    bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bmi.bmiHeader.biWidth = width;
    bmi.bmiHeader.biHeight = -height; // top-down
    bmi.bmiHeader.biPlanes = 1;
    bmi.bmiHeader.biBitCount = 24;
    bmi.bmiHeader.biCompression = BI_RGB;

   // HBITMAP hbm24 = Draw(hwnd, 0, 0, true);
    HDC hdcWindow = GetDC(hwnd);
    HDC hdcMem = CreateCompatibleDC(hdcWindow);

    BYTE* pBits = nullptr;
    HBITMAP hbm24 = CreateDIBSection(hdcWindow, &bmi, DIB_RGB_COLORS, (void**)&pBits, NULL, 0);
    HGDIOBJ oldBmp = nullptr;
    if (hbm24 != NULL)
    {
        oldBmp = SelectObject(hdcMem, hbm24);

        BitBlt(hdcMem, 0, 0, width, height, hdcWindow, 0, 0, SRCCOPY);

        GetDIBits(hdcMem, hbm24, 0, height, pixels.data(), &bmi, DIB_RGB_COLORS);

        // Always restore old object before deleting the DC!
        SelectObject(hdcMem, oldBmp);
    }

    DeleteDC(hdcMem);
    ReleaseDC(hwnd, hdcWindow);


     
    return hbm24;  //always delete after use, alright. also remember to assign to variable then
}
DWORD WINAPI CursorDrawThread(LPVOID lpParam)
{
    if (pausedraw == false)
    {


    }
    return 1;
}

DWORD WINAPI ThreadFunction(LPVOID lpParam)
{

    //std::string iniPath = GetExecutableFolder() + "\\click.ini";
    std::string path = GetExecutableFolder() + "\\image.bmp";
    std::wstring wpath(path.begin(), path.end());

    // std::string windowtitle;
     //windowtitle = getIniString("Settings", "Window", "NOP", iniPath);
    Sleep(2000);
    hwnd = GetMainWindowHandle(GetCurrentProcessId());


    HBITMAP hbmdsktop;
    //image numeration
    while (numphotoA == -1 && x < 10)
    {
        std::string path = GetExecutableFolder() + "\\A" + std::to_string(x) + ".bmp";
        std::wstring wpath(path.begin(), path.end());
        if (hbm = (HBITMAP)LoadImageW(NULL, wpath.c_str(), IMAGE_BITMAP, 0, 0, LR_LOADFROMFILE | LR_CREATEDIBSECTION))
        {
            x++;
            DeleteObject(hbm);
        }
        else
            numphotoA = x;
        Sleep(2);
    }
    x = 0;
    while (numphotoB == -1 && x < 10)
    {
        std::string path = GetExecutableFolder() + "\\B" + std::to_string(x) + ".bmp";
        std::wstring wpath(path.begin(), path.end());
        if (HBITMAP hbm = (HBITMAP)LoadImageW(NULL, wpath.c_str(), IMAGE_BITMAP, 0, 0, LR_LOADFROMFILE | LR_CREATEDIBSECTION))
        {
            x++;
            DeleteObject(hbm);
        }
        else
            numphotoB = x;
        Sleep(2);
    }
    x = 0;
    while (numphotoY == -1 && x < 10)
    {
        std::string path = GetExecutableFolder() + "\\Y" + std::to_string(x) + ".bmp";
        std::wstring wpath(path.begin(), path.end());
        if (HBITMAP hbm = (HBITMAP)LoadImageW(NULL, wpath.c_str(), IMAGE_BITMAP, 0, 0, LR_LOADFROMFILE | LR_CREATEDIBSECTION))
        {
            x++;
            DeleteObject(hbm);
        }
        else
            numphotoY = x;
        Sleep(2);
    }
    x = 0;
    while (numphotoX == -1 && x < 10)
    {
        std::string path = GetExecutableFolder() + "\\X" + std::to_string(x) + ".bmp";
        std::wstring wpath(path.begin(), path.end());
        if (HBITMAP hbm = (HBITMAP)LoadImageW(NULL, wpath.c_str(), IMAGE_BITMAP, 0, 0, LR_LOADFROMFILE | LR_CREATEDIBSECTION))
        {
            x++;
            DeleteObject(hbm);
        }


        else
            numphotoX = x;

        Sleep(2);
    }
    //////////////////////////////////////
    while (loop == true)
    {
        if (hwnd == NULL)
        {
            hwnd = GetMainWindowHandle(GetCurrentProcessId());
        }
        else
        {
            XINPUT_STATE state;
            ZeroMemory(&state, sizeof(XINPUT_STATE));
            // Check controller 0
            DWORD dwResult = XInputGetState(0, &state);

            if (dwResult == ERROR_SUCCESS)
            {
                // Controller is connected
                WORD buttons = state.Gamepad.wButtons;

                if (buttons & XINPUT_GAMEPAD_A)
                {
                    for (int i = 0; i < numphotoA; i++) //memory problem here somewhere
                    {
                        std::string path = GetExecutableFolder() + "\\A" + std::to_string(i) + ".bmp";
                        std::wstring wpath(path.begin(), path.end());
                        // std::string iniPath = GetExecutableFolder() + "\\click.ini";
                        std::vector<BYTE> largePixels, smallPixels;
                        SIZE screenSize;
                        int strideLarge, strideSmall;
                        int smallW, smallH;

                        // Load the subimage 
                        if (LoadBMP24Bit(wpath.c_str(), smallPixels, smallW, smallH, strideSmall))
                        {
                            // Capture screen
                            if (hbmdsktop = CaptureWindow24Bit(hwnd, screenSize, largePixels, strideLarge))
                            {

                                POINT pt;
                                if (FindSubImage24(largePixels.data(), screenSize.cx, screenSize.cy, strideLarge, smallPixels.data(), smallW, smallH, strideSmall, pt))
                                {
                                    ClientToScreen(hwnd, &pt);
                                    SendMouseClick(pt.x, pt.y, 1);

                                }
                                DeleteObject(hbmdsktop);
                            }

                        }
                        else MessageBox(NULL, "failed to load bmp:", "Message Box", MB_OK | MB_ICONINFORMATION);

                    }
                }
                if (buttons & XINPUT_GAMEPAD_B)
                {
                    for (int i = 0; i < numphotoB; i++)
                    {
                        std::string path = GetExecutableFolder() + "\\B" + std::to_string(i) + ".bmp";
                        std::wstring wpath(path.begin(), path.end());
                        // std::string iniPath = GetExecutableFolder() + "\\click.ini";
                        std::vector<BYTE> largePixels, smallPixels;
                        SIZE screenSize;
                        int strideLarge, strideSmall;
                        int smallW, smallH;
                        

                        // Load the subimage
                        if (LoadBMP24Bit(wpath.c_str(), smallPixels, smallW, smallH, strideSmall))
                        {
                            // Capture screen
                            if (hbmdsktop = CaptureWindow24Bit(hwnd, screenSize, largePixels, strideLarge))
                            {

                                POINT pt;
                                if (FindSubImage24(largePixels.data(), screenSize.cx, screenSize.cy, strideLarge, smallPixels.data(), smallW, smallH, strideSmall, pt))
                                {
                                    ClientToScreen(hwnd, &pt);
                                    SendMouseClick(pt.x, pt.y, 1);

                                }
                                DeleteObject(hbmdsktop);
                            }

                        }
                        else MessageBox(NULL, "failed to load bmp:", "Message Box", MB_OK | MB_ICONINFORMATION);



                    }
                }
                if (buttons & XINPUT_GAMEPAD_DPAD_UP)
                {
                }
                if (buttons & XINPUT_GAMEPAD_DPAD_DOWN)
                {
                }
                if (buttons & XINPUT_GAMEPAD_DPAD_LEFT)
                {
                }
                if (buttons & XINPUT_GAMEPAD_DPAD_RIGHT)
                {
                }
                if (buttons & XINPUT_GAMEPAD_X)
                {
                    for (int i = 0; i < numphotoX; i++)
                    {
                        std::string path = GetExecutableFolder() + "\\X" + std::to_string(i) + ".bmp";
                        std::wstring wpath(path.begin(), path.end());
                        // std::string iniPath = GetExecutableFolder() + "\\click.ini";
                        std::vector<BYTE> largePixels, smallPixels;
                        SIZE screenSize;
                        int strideLarge, strideSmall;
                        int smallW, smallH;

                        // Load the subimage
                        if (LoadBMP24Bit(wpath.c_str(), smallPixels, smallW, smallH, strideSmall))
                        {
                            // Capture screen
                            if (hbmdsktop = CaptureWindow24Bit(hwnd, screenSize, largePixels, strideLarge))
                            {

                                POINT pt;
                                if (FindSubImage24(largePixels.data(), screenSize.cx, screenSize.cy, strideLarge, smallPixels.data(), smallW, smallH, strideSmall, pt))
                                {
                                    ClientToScreen(hwnd, &pt);
                                    SendMouseClick(pt.x, pt.y, 1);

                                }
                               DeleteObject(hbmdsktop);
                            }

                        }
                        else MessageBox(NULL, "failed to load bmp:", "Message Box", MB_OK | MB_ICONINFORMATION);
                    }
                }
                if (buttons & XINPUT_GAMEPAD_Y)
                {
                    for (int i = 0; i < numphotoY; i++)
                    {
                        std::string path = GetExecutableFolder() + "\\Y" + std::to_string(i) + ".bmp";
                        std::wstring wpath(path.begin(), path.end());
                        // std::string iniPath = GetExecutableFolder() + "\\click.ini";
                        std::vector<BYTE> largePixels, smallPixels;
                        SIZE screenSize;
                        int strideLarge, strideSmall;
                        int smallW, smallH;

                        // Load the subimage
                        if (LoadBMP24Bit(wpath.c_str(), smallPixels, smallW, smallH, strideSmall))
                        {
                            // Capture screen
                            if (CaptureWindow24Bit(hwnd, screenSize, largePixels, strideLarge))
                            {

                                POINT pt;
                                if (FindSubImage24(largePixels.data(), screenSize.cx, screenSize.cy, strideLarge, smallPixels.data(), smallW, smallH, strideSmall, pt))
                                {
                                    ClientToScreen(hwnd, &pt);
                                    SendMouseClick(pt.x, pt.y, 1);

                                }
                               // DeleteObject(hbmdsktop);
                            }

                        }
                        else MessageBox(NULL, "failed to load bmp:", "Message Box", MB_OK | MB_ICONINFORMATION);

                    }
                }
                if (buttons & XINPUT_GAMEPAD_START) {
                }
            } //no controller

        } // no hwnd
        Sleep(75);
    } //loop end
    return 0;
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD  ul_reason_for_call, LPVOID lpReserved)
{
    switch (ul_reason_for_call) {
    case DLL_PROCESS_ATTACH:
        //may slow down parent process if it continues on this thread
        CreateThread(nullptr, 0, ThreadFunction, nullptr, 0, nullptr); //not recommended? but seem to work well
        CreateThread(nullptr, 0, CursorDrawThread, nullptr, 0, nullptr); //not recommended? but seem to work well
        break;
    }
    return TRUE;
}