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
POINT startdrag;
bool loop = true;
HWND hwnd = 0;
HWND pointerwindow = 0;

int width1 = 0;
int width2 = 0;
int height1 = 0;
int height2 = 0;



//fake cursor
int X = 20;
int Y = 20;
int OldX = 0;
int OldY = 0;
int ydrag;
int xdrag;

bool pausedraw = false;

bool Apressed = false;
bool Bpressed = false;
bool Xpressed = false;
bool Ypressed = false;
bool leftPressedold;
bool rightPressedold;

int x = 0;

HBITMAP hbm;

std::vector<BYTE> largePixels, smallPixels;
SIZE screenSize;
int strideLarge, strideSmall;
int smallW, smallH;

int mode = 0;
int sovetid = 16;


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


bool FindSubImage24(
    const BYTE* largeData, int largeW, int largeH, int strideLarge,
    const BYTE* smallData, int smallW, int smallH, int strideSmall,
    POINT& foundAt, int Xstart, int Ystart
) {
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
        if (windowPID == pData->pid && GetWindow(hWnd, GW_OWNER) == nullptr && IsWindowVisible(hWnd) && hWnd != pointerwindow) {
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

bool IsTriggerPressed(BYTE triggerValue, BYTE threshold = 30) {
    return triggerValue > threshold;
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
bool SaveWindow10x10BMP(HWND hwnd, const wchar_t* filename, int x, int y) {
    HDC hdcWindow = GetDC(hwnd);
    HDC hdcMem = CreateCompatibleDC(hdcWindow);

    // Size: 10x10
    int width = 10;
    int height = 10;
    int stride = ((width * 3 + 3) & ~3);
    std::vector<BYTE> pixels(stride * height);

    // Create a 24bpp bitmap
    BITMAPINFO bmi = {};
    bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bmi.bmiHeader.biWidth = width;
    bmi.bmiHeader.biHeight = -height; // top-down DIB
    bmi.bmiHeader.biPlanes = 1;
    bmi.bmiHeader.biBitCount = 24;
    bmi.bmiHeader.biCompression = BI_RGB;


    BYTE* pBits = nullptr;

    HBITMAP hbm24 = CreateDIBSection(hdcWindow, &bmi, DIB_RGB_COLORS, (void**)&pBits, 0, 0);
    if (!hbm24) {
        DeleteDC(hdcMem);
        ReleaseDC(hwnd, hdcWindow);
        return false;
    }

    HGDIOBJ oldbmp = SelectObject(hdcMem, hbm24);

    BitBlt(hdcMem, 0, 0, width, height, hdcWindow, x, y, SRCCOPY);

    // Prepare to retrieve bits
    BITMAPINFOHEADER bih = {};
    bih.biSize = sizeof(BITMAPINFOHEADER);
    bih.biWidth = width;
    bih.biHeight = -height; // top-down for easier use
    bih.biPlanes = 1;
    bih.biBitCount = 24;
    bih.biCompression = BI_RGB;

    GetDIBits(hdcMem, hbm24, 0, height, pixels.data(), (BITMAPINFO*)&bih, DIB_RGB_COLORS);

    // Save
    bool ok = Save24BitBMP(filename, pixels.data(), width, height);

    // Cleanup
    SelectObject(hdcMem, oldbmp);
    DeleteObject(hbm24);
    DeleteDC(hdcMem);
    ReleaseDC(hwnd, hdcWindow);

    return ok;
}
HBITMAP Draw(HWND hwnd, int X, int Y, bool screenshot) 
{

}

HBITMAP CaptureWindow24Bit(HWND hwnd, SIZE& capturedwindow, std::vector<BYTE>& pixels, int& strideOut, bool draw) {
    HDC hdcWindow = GetDC(hwnd);
    HDC hdcMem = CreateCompatibleDC(hdcWindow);
    HBITMAP hbm24 = NULL;

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

    BYTE* pBits = nullptr;

   

    hbm24 = CreateDIBSection(hdcWindow, &bmi, DIB_RGB_COLORS, (void**)&pBits, NULL, 0);
    if (hbm24 != 0)   
    { 
   
        HGDIOBJ oldBmp = SelectObject(hdcMem, hbm24);
 
    

        // Copy window contents to memory DC
        BitBlt(hdcMem, 0, 0, width, height, hdcWindow, 0, 0, SRCCOPY);

        if (draw) {
            //hbm24 = NULL;
            HBRUSH hBrush = CreateSolidBrush(RGB(255, 60, 2));
            RECT rect = { X, Y, X + 4, Y + 4 };
            FillRect(hdcWindow, &rect, hBrush);
            DeleteObject(hBrush);
            GetDIBits(hdcMem, hbm24, 0, height, pixels.data(), &bmi, DIB_RGB_COLORS);
            SelectObject(hdcMem, oldBmp);
            DeleteDC(hdcMem);
            ReleaseDC(hwnd, hdcWindow);
            DeleteObject(hbm24);
            return 0;
        }

        // Copy bits out
        GetDIBits(hdcMem, hbm24, 0, height, pixels.data(), &bmi, DIB_RGB_COLORS);
   
        // Cleanup
        SelectObject(hdcMem, oldBmp);
    }
    DeleteDC(hdcMem);
    ReleaseDC(hwnd, hdcWindow);

    return hbm24;
}

DWORD WINAPI CursorDrawThread(LPVOID lpParam)
{
    Sleep(5000);
    while (loop == true && hwnd != NULL)
    {
        if (pausedraw == false)
        { 
            CaptureWindow24Bit(hwnd, screenSize, largePixels, strideLarge, true);
        }
        Sleep(16);
    }
    return 1;
}
// Helper: Get stick magnitude
float GetStickMagnitude(SHORT x, SHORT y) {
    return sqrtf(static_cast<float>(x) * x + static_cast<float>(y) * y);
}

// Helper: Clamp value to range [-1, 1]
float Clamp(float v) {
    if (v < -1.0f) return -1.0f;
    if (v > 1.0f) return 1.0f;
    return v;
}
#define DEADZONE 8000
#define MAX_SPEED 30.0f        // Maximum pixels per poll
#define ACCELERATION 2.0f      // Controls non-linear ramp (higher = more acceleration)


bool SendMouseClick(int x, int y, int z) {
    // Create a named mutex
    HANDLE hMutex = CreateMutexA(
        NULL,      // Default security
        FALSE,     // Initially not owned
        "Global\\PuttingInputByMessenils" // Name of mutex
    );
    if (hMutex == NULL) {
        std::cerr << "CreateMutex failed: " << GetLastError() << std::endl;
        MessageBox(NULL, "Error!", "Failed to create mutex", MB_OK | MB_ICONINFORMATION);
        return false;
    }
    // Check if mutex already exists
    if (GetLastError() == ERROR_ALREADY_EXISTS)
    {
        std::cout << "Mutex exists, waiting for it to be released...\n";
        // Wait for mutex to be released by other process (INFINITE = wait forever)
        DWORD waitResult = WaitForSingleObject(hMutex, INFINITE);
        if (waitResult == WAIT_OBJECT_0)
        {
            std::cout << "Acquired mutex after waiting!\n";
        }
        else {

            MessageBox(NULL, "Error!", "Wait for mutex failed", MB_OK | MB_ICONINFORMATION);
            return false;

        }
    }
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
    else if (z == 3)
    { //right button press, drag and release
        // Simulate mouse left button down
        input[1].type = INPUT_MOUSE;
        input[1].mi.dwFlags = MOUSEEVENTF_RIGHTDOWN;
    }
    else if (z == 5)
    { //right button press, drag and release
        // Simulate mouse left button up
        input[2].type = INPUT_MOUSE;
        input[2].mi.dwFlags = MOUSEEVENTF_RIGHTUP;
    }
    else if (z == 4)
    { //right button press, drag and release
        // Simulate mouse left button up
        input[2].type = INPUT_MOUSE;
        input[2].mi.dwFlags = 0;
    }
    //z is 3 or anything just move mouse
    SendInput(3, input, sizeof(INPUT));
    ReleaseMutex(hMutex);
    CloseHandle(hMutex);
    return true;
}
bool Buttonaction(const char key[3], int mode, int serchnum, HBITMAP hbmdsktop)
{
    if (mode != 2)
    {
        pausedraw = true;
        Sleep(25);
        bool foundit = false;
        int i = 0;
        while (!foundit && i < serchnum)
        {
            i++;
            std::string path = GetExecutableFolder() + key + std::to_string(i) + ".bmp";
            std::wstring wpath(path.begin(), path.end());
            // std::string iniPath = GetExecutableFolder() + "\\click.ini";
            

            // Load the subimage 
            if (LoadBMP24Bit(wpath.c_str(), smallPixels, smallW, smallH, strideSmall))
            {
                // Capture screen
                if (hbmdsktop = CaptureWindow24Bit(hwnd, screenSize, largePixels, strideLarge, false))
                {

                    POINT pt;
                    if (FindSubImage24(largePixels.data(), screenSize.cx, screenSize.cy, strideLarge, smallPixels.data(), smallW, smallH, strideSmall, pt, X + 2, Y))
                    {
                        X = pt.x;
                        Y = pt.y;
                        ClientToScreen(hwnd, &pt);
                        SendMouseClick(pt.x, pt.y, 1);
                        foundit = true;

                    }
                    DeleteObject(hbmdsktop);
                    Sleep(20); //to avoid double press
                }

            }
        }
        i = 0;
        while (!foundit && i < serchnum)
        {
                 i++;
                std::string path = GetExecutableFolder() + key + std::to_string(i) + ".bmp";
                std::wstring wpath(path.begin(), path.end());
                // std::string iniPath = GetExecutableFolder() + "\\click.ini";


                // Load the subimage 
                if (LoadBMP24Bit(wpath.c_str(), smallPixels, smallW, smallH, strideSmall))
                {
                    // Capture screen
                    if (hbmdsktop = CaptureWindow24Bit(hwnd, screenSize, largePixels, strideLarge, false))
                    {

                        POINT pt;

                        if (FindSubImage24(largePixels.data(), screenSize.cx, screenSize.cy, strideLarge, smallPixels.data(), smallW, smallH, strideSmall, pt, 0, 0))
                        {
                           ;
                            X = pt.x;
                            Y = pt.y;
                            ClientToScreen(hwnd, &pt);
                            SendMouseClick(pt.x, pt.y, 1);
                            foundit = true;

                        }
                        DeleteObject(hbmdsktop);
                        Sleep(20); //to avoid double press
                    }

                }

                else return false;
        }


        return true;

    }
    else //mode 2 button mapping
    {

        Sleep(500); //to make sure red flicker expired
        std::string path = GetExecutableFolder() + key + std::to_string(serchnum) + ".bmp";
        std::wstring wpath(path.begin(), path.end());
        SaveWindow10x10BMP(hwnd, wpath.c_str(), X, Y);
        MessageBox(NULL, "Mapped spot!", "A button is now mapped to red spot", MB_OK | MB_ICONINFORMATION);
        //numphotoA++;

    }
}
DWORD WINAPI ThreadFunction(LPVOID lpParam)
{
    Sleep(2000);
   // CreateThread(nullptr, 0, CursorDrawThread, nullptr, 0, nullptr); //not recommended? but seem to work well
    //std::string iniPath = GetExecutableFolder() + "\\click.ini";
    std::string path = GetExecutableFolder() + "\\image.bmp";
    std::wstring wpath(path.begin(), path.end());

    // std::string windowtitle;
     //windowtitle = getIniString("Settings", "Window", "NOP", iniPath);
    
    hwnd = GetMainWindowHandle(GetCurrentProcessId());

    int mode = 0;
    int numphotoA = -1;
    int numphotoB = -1;
    int numphotoX = -1;
    int numphotoY = -1;
    HBITMAP hbmdsktop = NULL;

    //image numeration
    while (numphotoA == -1 && x < 50)
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
    while (numphotoB == -1 && x < 50)
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
    while (numphotoY == -1 && x < 50)
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
    while (numphotoX == -1 && x < 50)
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
    bool Aprev = false;
        
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
            bool leftPressed = IsTriggerPressed(state.Gamepad.bLeftTrigger);
            bool rightPressed = IsTriggerPressed(state.Gamepad.bRightTrigger);
            



            if (dwResult == ERROR_SUCCESS)
            {
                // Controller is connected
                WORD buttons = state.Gamepad.wButtons;
                bool currA = (buttons & XINPUT_GAMEPAD_A) != 0;
                bool Apressed = (buttons & XINPUT_GAMEPAD_A);

                if (Apressed)
                {
                    if (Aprev == false)
                    { 
                        Buttonaction("\\A", mode, numphotoA, hbmdsktop);
                        if ( mode == 2)
                        { 
                            numphotoA++;
                        }
                        Aprev = true;
                    }
                }
                if (Apressed == false)
                {
                    Aprev = false;
                }
                if (buttons & XINPUT_GAMEPAD_B)
                {
                    Buttonaction("\\B", mode, numphotoB, hbmdsktop);
                    if (mode == 2)
                    {
                        numphotoB++;
                    }
                }
                if (buttons & XINPUT_GAMEPAD_DPAD_UP)
                {
                    fakecursor.x = X;
                    fakecursor.y = Y;

                    ClientToScreen(hwnd, &fakecursor);
                    SendMouseClick(fakecursor.x, fakecursor.y, 1);
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
                    Buttonaction("\\X", mode, numphotoX, hbmdsktop);
                    if (mode == 2)
                    {
                        numphotoX++;
                    }
                }
                if (buttons & XINPUT_GAMEPAD_Y)
                {
                    Buttonaction("\\Y", mode, numphotoY, hbmdsktop);
                    if (mode == 2)
                    {
                        numphotoY++;
                    }
                }

                if (mode > 0 )
                { 
                    //fake cursor poll
                    RECT rect;
                    GetClientRect(hwnd, &rect);
                    int width = rect.right - rect.left;
                        int height = rect.bottom - rect.top;
                    int Xaxis = state.Gamepad.sThumbLX;
                    int Yaxis = state.Gamepad.sThumbLY;

                    

                    OldX = X;
                    if (Xaxis < -7849) //strange values. but tested many before choosing this
                    { 
                        if (X > 0)
                        { 
                            sovetid = 75 - (std::abs(Xaxis) / 450);
                        X = X - 2;
                        }
                    }
                    else if (Xaxis > 12000) //strange values. but tested many before choosing this
                    {
                        if (X < width)
                        {
                            sovetid = 75 - (Xaxis / 450);
                            X = X + 2;
                        }
                    }

                    /////////////////////
                    if (Yaxis > 0) //strange values. but tested many before choosing this
                    {
                        if (Y > 0)
                        {
                            sovetid = 75 - (std::abs(Yaxis) / 450);
                            Y = Y - 2;
                        }
                    }
                    else  if (Yaxis < -16049) //strange values. but tested many before choosing this
                    { //my controller is not calibrated maybe
                        if (Y < height)
                        {
                            sovetid = 75 - (std::abs(Yaxis) / 450); //a litt
                            Y = Y + 2;
                        }
                    }
                    CaptureWindow24Bit(hwnd, screenSize, largePixels, strideLarge, true); //draw
                    //MessageBox(NULL, "failed to load bmp:", "Message Box", MB_OK | MB_ICONINFORMATION);

                }


                
                if (buttons & XINPUT_GAMEPAD_START) 
                {
                    
                    if (mode == 0)
                    { 
                        mode = 1;
          
                        MessageBox(NULL, "Bmp + Emulated cursor mode", "Move the flickering red dot and use right trigger for left click. left trigger for right click", MB_OK | MB_ICONINFORMATION);
                    }
                    else if (mode == 1)
                    { 
                        mode = 2;
                        MessageBox(NULL, "Edit Mode", "Button mapping. will map buttons you click with the flickering red dot as an input coordinate", MB_OK | MB_ICONINFORMATION);

                    }
                    else if (mode == 2)
                    {
                        mode = 0;
                        MessageBox(NULL, "Bmp mode", "only send input on bmp match", MB_OK | MB_ICONINFORMATION);
                    }
                    Sleep(1000); //have time to release button. this is no hurry anyway
                }

                if (leftPressed)
                { 
                    if (leftPressedold == false)
                    {
                        //save coordinates
                        startdrag.x = X;
                        startdrag.y = Y;
                    }
                    leftPressedold = true;
                }
                if (leftPressedold)
                {
                    if (!leftPressed)
                    {
                        fakecursor.x = X;
                        fakecursor.y = Y;


                        ClientToScreen(hwnd, &startdrag);

                        SendMouseClick(startdrag.x, startdrag.y, 1); //4 4 move //5 release
                      // ClientToScreen(hwnd, &fakecursor);
                        //Sleep(500); 
                      //  SendMouseClick(fakecursor.x, fakecursor.y, 4);
                        //Sleep(500);
                      //  SendMouseClick(fakecursor.x, fakecursor.y, 5);
                        leftPressedold = false;
                    }
                }


                if (rightPressed)
                {
                    if (rightPressedold == false)
                    {
                        //save coordinates
                        startdrag.x = X;
                        startdrag.y = Y;
                    }
                    rightPressedold = true;
                }
                if (rightPressedold)
                {
                    if (!rightPressed)
                    {
                        fakecursor.x = X;
                        fakecursor.y = Y;


                        ClientToScreen(hwnd, &startdrag);

                        SendMouseClick(startdrag.x, startdrag.y, 2); //4 4 move //5 release
                       // ClientToScreen(hwnd, &fakecursor);
                        //Sleep(500); 
                       // SendMouseClick(fakecursor.x, fakecursor.y, 4);
                        //Sleep(500);
                       // SendMouseClick(fakecursor.x, fakecursor.y, 5);
                        rightPressedold = false;
                    }
                }

            } //no controller

        } // no hwnd
        if (mode == 0)
            Sleep(100);
        if (mode > 0)
            Sleep(sovetid); //15-80 //

    } //loop end
    return 0;
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD  ul_reason_for_call, LPVOID lpReserved)
{
    switch (ul_reason_for_call) {
    case DLL_PROCESS_ATTACH:
        //may slow down parent process if it continues on this thread
        CreateThread(nullptr, 0, ThreadFunction, nullptr, 0, nullptr); //not recommended? but seem to work well
        
        break;
    }
    return TRUE;
}