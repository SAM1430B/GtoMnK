// dllmain.cpp : Defines the entry point for the DLL application.
#include "pch.h"
#include <windows.h>
#include "MinHook.h"
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

typedef UINT(WINAPI* GetCursorPos_t)(LPPOINT lpPoint);
GetCursorPos_t fpGetCursorPos = nullptr;

POINT fakecursor;
POINT startdrag;
POINT activatewindow;
POINT scroll;
bool loop = true;
HWND hwnd;

//syncronization control
HANDLE hMutex;

int width1 = 0;
int width2 = 0;
int height1 = 0;
int height2 = 0;
int getmouseonkey = 0;
int keyrespondtime = 50;
int message = 0;


//hooks



//fake cursor
int X = 20;
int Y = 20;
int OldX = 0;
int OldY = 0;
int ydrag;
int xdrag;
bool scrollmap = false;
bool pausedraw = false;

bool Apressed = false;
bool Bpressed = false;
bool Xpressed = false;
bool Ypressed = false;
bool leftPressedold;
bool rightPressedold;

int startsearch = 0;
int startsearchA = 0;
int startsearchB = 0;
int startsearchX = 0;
int startsearchY = 0;

int Atype = 0;
int Btype = 0;
int Xtype = 0;
int Ytype = 0;


int x = 0;

HBITMAP hbm;

std::vector<BYTE> largePixels, smallPixels;
SIZE screenSize;
int strideLarge, strideSmall;
int smallW, smallH;

int mode = 0;
int sovetid = 16;
int knappsovetid = 100;

BOOL WINAPI MyGetCursorPos(PPOINT lpPoint) {
    if (lpPoint) {
        POINT mpos;
        if (scrollmap == false)
        { 
            mpos.x = X;
            mpos.y = Y;
        ClientToScreen(hwnd, &mpos);
		
        lpPoint->x = mpos.x;
        lpPoint->y = mpos.y;
        ScreenToClient(hwnd, &mpos);
        }
		else
		{
            mpos.x = scroll.x;
            mpos.y = scroll.y;
            ClientToScreen(hwnd, &mpos);
			lpPoint->x = mpos.x;
			lpPoint->y = mpos.y;
            ScreenToClient(hwnd, &mpos);
		}
        return TRUE;
    }
    return FALSE;
}

bool Mutexlock(bool lock) {
    // Create a named mutex
    if (lock == true)
    {
        hMutex = CreateMutexA(
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
            Sleep(5);
            ReleaseMutex(hMutex);
            CloseHandle(hMutex);
            Mutexlock(true); //is this okay?
            
        }
    }
    if (lock == false)
    {
        ReleaseMutex(hMutex);

        CloseHandle(hMutex);
       // hMutex = nullptr; // Optional: Prevent dangling pointer

    }
    return true;
}
bool SendMouseClick(int x, int y, int z, int many) {
    // Create a named mutex
    if (z == 1 || z == 2 || z == 3 || z == 6 || z == 10)
    {
        Mutexlock(true);
           // SetForegroundWindow(hwnd);
        
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
        \
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
        input[1].mi.dwFlags = MOUSEEVENTF_LEFTDOWN;
    }
    else if (z == 5)
    { //right button press, drag and release
        // Simulate mouse left button up
        input[1].type = INPUT_MOUSE;
        input[1].mi.dwFlags = MOUSEEVENTF_LEFTUP;

        input[2].type = INPUT_MOUSE;
        input[2].mi.dwFlags = MOUSEEVENTF_LEFTUP;
    }
    else if (z == 6 || z == 8 || z == 10 || z == 11) //only mousemove but 6 no mutex. only make mutex while 10 both make and release
    { //right button press, drag and release
        // Simulate mouse left button down
    }
    else if (z == 7)
    { //right button press, drag and release
        // Simulate mouse left button up
        input[1].type = INPUT_MOUSE;
        input[1].mi.dwFlags = MOUSEEVENTF_RIGHTUP;

        input[2].type = INPUT_MOUSE;
        input[2].mi.dwFlags = MOUSEEVENTF_RIGHTUP;
    }
    else if (z == 4)
    { //right button press, drag and release
        // Simulate mouse left button up
    }
    //z is 3 or anything just move mouse
    SendInput(many, input, sizeof(INPUT));
    if (z == 1 || z == 2 || z == 5 || z == 7 || z == 10 || z == 11)
    {
        Mutexlock(false);
    }
    return true;
}

std::string UGetExecutableFolder() {
    TCHAR path[MAX_PATH];
    GetModuleFileName(NULL, path, MAX_PATH);
    std::string exePath(path);

    // Remove the executable name to get the folder
    size_t lastSlash = exePath.find_last_of("\\/");
    return exePath.substr(0, lastSlash);
}

std::wstring WGetExecutableFolder() {
    wchar_t path[MAX_PATH];
    GetModuleFileNameW(NULL, path, MAX_PATH);
    std::wstring exePath(path);
    size_t lastSlash = exePath.find_last_of(L"\\/");

    if (lastSlash == std::wstring::npos)
        return L"";

    return exePath.substr(0, lastSlash);
}


bool sendKey(char key, int typeofkey) { //VK_LEFT VK_RIGHT VK_DOWN VK_UP
    // Create a named mutex
     if (Mutexlock(true))
     { 
         if (getmouseonkey == 1){
             
             ClientToScreen(hwnd, &fakecursor);
             SendMouseClick(fakecursor.x, fakecursor.y, 8, 1);
             ScreenToClient(hwnd, &fakecursor);
             
         }

        INPUT ip = {};
        ip.type = INPUT_KEYBOARD;
        if (typeofkey == 0)
            ip.ki.wVk = VkKeyScan(key); // Converts character to virtual key code
        else if (typeofkey == 1)
            ip.ki.wVk = VK_LEFT; // Converts character to virtual key code
        else if (typeofkey == 2)
            ip.ki.wVk = VK_RIGHT; // Converts character to virtual key code
        else if (typeofkey == 3) 
            ip.ki.wVk = VK_UP; // Converts character to virtual key code
    
        else if (typeofkey == 4) 
            ip.ki.wVk = VK_DOWN; // Converts character to virtual key code
    
        else if (typeofkey == 5) 
            ip.ki.wVk = VK_ESCAPE; // Converts character to virtual key code
        ip.ki.dwFlags = 0; // Key down
        SendInput(1, &ip, sizeof(INPUT));
        // Key up event
        Sleep(keyrespondtime);
        ip.ki.dwFlags = KEYEVENTF_KEYUP;
        SendInput(1, &ip, sizeof(INPUT));
        Mutexlock(false);
        knappsovetid = 20;
        return true;
     }
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

bool IsTriggerPressed(BYTE triggerValue, BYTE threshold = 30) {
    return triggerValue > threshold;
}
bool LoadBMP24Bit(const wchar_t* filename, std::vector<BYTE>& pixels, int& width, int& height, int& stride) {
    HBITMAP hbm = (HBITMAP)LoadImageW(NULL, filename, IMAGE_BITMAP, 0, 0, LR_LOADFROMFILE | LR_CREATEDIBSECTION);
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


    //stretchblt

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
            RECT rect2 = { X - 1, Y +1, X + 6, Y + 4 };
            FillRect(hdcWindow, &rect, hBrush);
            FillRect(hdcWindow, &rect2, hBrush);
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



bool Buttonaction(const char key[3], int mode, int serchnum, int startsearch)
{
    if (mode != 2)
    {
        pausedraw = true;
        Sleep(25);
		bool movenotclick = false;
        //int i = startsearch;
        bool foundit = false;
        HBITMAP hbmdsktop;
        
        char buffer[100];
        wsprintf(buffer, "searchnum is %d", serchnum);
       // MessageBoxA(NULL, buffer, "Info", MB_OK);
        wsprintf(buffer, "starting search at %d", startsearch);
        //MessageBoxA(NULL, buffer, "Info", MB_OK);

        for (int i = startsearch; i < serchnum; i++) //memory problem here somewhere
        {
            
            std::string path = UGetExecutableFolder() + key + std::to_string(i) + ".bmp";
            std::wstring wpath(path.begin(), path.end());

            if (LoadBMP24Bit(wpath.c_str(), smallPixels, smallW, smallH, strideSmall))
            {
               // MessageBox(NULL, "some kind of error", "loaded bmp", MB_OK | MB_ICONINFORMATION);
                // Capture screen
                if (hbmdsktop = CaptureWindow24Bit(hwnd, screenSize, largePixels, strideLarge, false))
                {
                   // MessageBox(NULL, "some kind of error", "captured desktop", MB_OK | MB_ICONINFORMATION);
                    
                    POINT pt;
                    if (FindSubImage24(largePixels.data(), screenSize.cx, screenSize.cy, strideLarge, smallPixels.data(), smallW, smallH, strideSmall, pt, 0, 0))
                    {
                        //MessageBox(NULL, "some kind of error", "found image", MB_OK | MB_ICONINFORMATION);
                        X = pt.x;
                        Y = pt.y;
                       //  char buffer[100];
// wsprintf(buffer, "A is %d", i);
 // MessageBoxA(NULL, buffer, "Info", MB_OK);
                        ClientToScreen(hwnd, &pt);
                        if (strcmp(key, "\\A") == 0) {
                            if (Atype == 1)
                            {
                                movenotclick = true;
                            }
                            startsearchA = i + 1;
                        }
                        else if (strcmp(key, "\\B") == 0) {
                            if (Btype == 1)
                            {
                                movenotclick = true;
                            }
                            startsearchB = i + 1;
                        }
                        else if (strcmp(key, "\\X") == 0) {
                            if (Xtype == 1)
                            {
                                movenotclick = true;
                            }
                            startsearchX = i + 1;
                        }
                        else if (strcmp(key, "\\Y") == 0) {
                            if (Ytype == 1)
                            {
                                movenotclick = true;
                            }
                            startsearchY = i + 1;
                        }
                       // else return false;
						
                        if (movenotclick == true)
                        {   
                        }
                        else {
                            SendMouseClick(pt.x, pt.y, 1, 3);
                            ScreenToClient(hwnd, &pt);
                            foundit = true;
                            break;
                        }
                    }
                    
                   // else  return false; 
                    DeleteObject(hbmdsktop);
                    
                    
                }
               // else  
                
            }
            
           // else  
        }
        if (!foundit) {
          //  MessageBox(NULL, "some kind of error", "captured desktop", MB_OK | MB_ICONINFORMATION);
            for (int i = 0; i < serchnum; i++) //memory problem here somewhere
            {
                std::string path = UGetExecutableFolder() + key + std::to_string(i) + ".bmp";
                std::wstring wpath(path.begin(), path.end());


                // Load the subimage 
                if (LoadBMP24Bit(wpath.c_str(), smallPixels, smallW, smallH, strideSmall))
                {
                   // MessageBox(NULL, "Bmp + Emulated cursor mode", "other search", MB_OK | MB_ICONINFORMATION);
                    // Capture screen
                    if (hbmdsktop = CaptureWindow24Bit(hwnd, screenSize, largePixels, strideLarge, false))
                    {

                        POINT pt;

                        if (FindSubImage24(largePixels.data(), screenSize.cx, screenSize.cy, strideLarge, smallPixels.data(), smallW, smallH, strideSmall, pt, 0, 0))
                        {
                            // char buffer[100];
                           // wsprintf(buffer, "second A is %d", i);
                            // MessageBoxA(NULL, buffer, "Info", MB_OK);
                            X = pt.x;
                            Y = pt.y;
                            if (strcmp(key, "\\A") == 0) {
                                startsearchA = i + 1;
                                 char buffer[100];
                            }
                            else if (strcmp(key, "\\B") == 0) {
                                startsearchB = i + 1;
                            }
                            else if (strcmp(key, "\\X") == 0) {
                                startsearchX = i + 1;
                            }
                            else if (strcmp(key, "\\Y") == 0) {
                                startsearchY = i + 1;
                            }
                            if (movenotclick == true)
                            {
                            }
                            else {
                                ClientToScreen(hwnd, &pt);
                                SendMouseClick(pt.x, pt.y, 1, 3);
                                ScreenToClient(hwnd, &pt);
                            }
                            break;

                        }
                        //  else  return false;
                        DeleteObject(hbmdsktop);

                    }
                }


               
            }
        }
        Sleep(200); //to avoid double press
        return true;

    }
    else //mode 2 button mapping
    {
		//RepaintWindow(hwnd, NULL, FALSE); 
        Sleep(500); //to make sure red flicker expired
        std::string path = UGetExecutableFolder() + key + std::to_string(serchnum) + ".bmp";
        std::wstring wpath(path.begin(), path.end());
        SaveWindow10x10BMP(hwnd, wpath.c_str(), X, Y);
        MessageBox(NULL, "Mapped spot!", "A button is now mapped to red spot", MB_OK | MB_ICONINFORMATION);
        return true;
    }
}
DWORD WINAPI ThreadFunction(LPVOID lpParam)
{
    Sleep(2000);

    // settings reporting
    std::string iniPath = UGetExecutableFolder() + "\\screenshotinput.ini";
    std::string iniSettings = "Settings";
   // std::string controllerID = getIniString(iniSettings.c_str(), "Controller ID", "0", iniPath);

    int controllerID = GetPrivateProfileInt(iniSettings.c_str(), "Controllerid", 0, iniPath.c_str());
    int AxisLeftsens = GetPrivateProfileInt(iniSettings.c_str(), "AxisLeftsens", -7849, iniPath.c_str());
    int AxisRightsens = GetPrivateProfileInt(iniSettings.c_str(), "AxisRightsens", 12000, iniPath.c_str());
    int AxisUpsens = GetPrivateProfileInt(iniSettings.c_str(), "AxisUpsens", 0, iniPath.c_str());
    int AxisDownsens = GetPrivateProfileInt(iniSettings.c_str(), "AxisDownsens", -16049, iniPath.c_str());

    int InitialMode = GetPrivateProfileInt(iniSettings.c_str(), "Initial Mode", 0, iniPath.c_str());
    int Modechange = GetPrivateProfileInt(iniSettings.c_str(), "Allow modechange", 1, iniPath.c_str());

    int sens = GetPrivateProfileInt(iniSettings.c_str(), "Dot Speed", 40, iniPath.c_str());
    int sens2 = GetPrivateProfileInt(iniSettings.c_str(), "CA Dot Speed", 75, iniPath.c_str());

    int sendfocus = GetPrivateProfileInt(iniSettings.c_str(), "Sendfocus", 0, iniPath.c_str());
    keyrespondtime = GetPrivateProfileInt(iniSettings.c_str(), "Keyresponsetime", 50, iniPath.c_str());
    getmouseonkey = GetPrivateProfileInt(iniSettings.c_str(), "GetMouseOnKey", 0, iniPath.c_str());
    Atype = GetPrivateProfileInt(iniSettings.c_str(), "Aonlymove", 0, iniPath.c_str());
    Btype = GetPrivateProfileInt(iniSettings.c_str(), "Bonlymove", 0, iniPath.c_str());
    Xtype = GetPrivateProfileInt(iniSettings.c_str(), "Xonlymove", 0, iniPath.c_str());
    Ytype = GetPrivateProfileInt(iniSettings.c_str(), "Yonlymove", 0, iniPath.c_str());

    Sleep(1000);
    
    hwnd = GetMainWindowHandle(GetCurrentProcessId());
    int mode = InitialMode;
    int numphotoA = -1;
    int numphotoB = -1;
    int numphotoX = -1;
    int numphotoY = -1;

    HBITMAP hbmdsktop = NULL;

    //image numeration
    while (numphotoA == -1 && x < 50)
    {
        std::wstring wpath = WGetExecutableFolder() + L"\\A" + std::to_wstring(x) + L".bmp";
        if (hbm = (HBITMAP)LoadImageW(NULL, wpath.c_str(), IMAGE_BITMAP, 0, 0, LR_LOADFROMFILE | LR_CREATEDIBSECTION))
        {
            x++;
            DeleteObject(hbm);
        }
        else
            numphotoA = x;
       // char buffer[100];
      // wsprintf(buffer, "A is %d", numphotoA);
      //  MessageBoxA(NULL, buffer, "Info", MB_OK);
        Sleep(2);
    }
    x = 0;
    while (numphotoB == -1 && x < 50)
    {
        std::string path = UGetExecutableFolder() + "\\B" + std::to_string(x) + ".bmp";
        std::wstring wpath(path.begin(), path.end());
        if (HBITMAP hbm = (HBITMAP)LoadImageW(NULL, wpath.c_str(), IMAGE_BITMAP, 0, 0, LR_LOADFROMFILE | LR_CREATEDIBSECTION))
        {
            x++;
            DeleteObject(hbm);
        }
        else {
            numphotoB = x;

        }
        Sleep(2);
    }
    x = 0;
    while (numphotoY == -1 && x < 50)
    {
        std::string path = UGetExecutableFolder() + "\\Y" + std::to_string(x) + ".bmp";
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
        std::string path = UGetExecutableFolder() + "\\X" + std::to_string(x) + ".bmp";
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
        RECT rect;
        GetClientRect(hwnd, &rect);
        

        if (hwnd != NULL)
        {
            XINPUT_STATE state;
            ZeroMemory(&state, sizeof(XINPUT_STATE));
            // Check controller 0
            DWORD dwResult = XInputGetState(controllerID, &state);
            bool leftPressed = IsTriggerPressed(state.Gamepad.bLeftTrigger);
            bool rightPressed = IsTriggerPressed(state.Gamepad.bRightTrigger);
            



            if (dwResult == ERROR_SUCCESS)
            {
                // Controller is connected
                WORD buttons = state.Gamepad.wButtons;
                bool currA = (buttons & XINPUT_GAMEPAD_A) != 0;
                bool Apressed = (buttons & XINPUT_GAMEPAD_A);

                if (buttons & XINPUT_GAMEPAD_A)
                {
                    startsearch = startsearchA;
                    if (hbmdsktop = CaptureWindow24Bit(hwnd, screenSize, largePixels, strideLarge, false))
                    {
                        if (Buttonaction("\\A", mode, numphotoA, startsearch))
                        {
                        }
                        else
                        { //error handling
                        }
                        if (mode == 2)
                        {
                            numphotoA++;
                        }
                    }
                }
                if (buttons & XINPUT_GAMEPAD_B)
                {
                    startsearch = startsearchB;
                    if (Buttonaction("\\B", mode, numphotoB, startsearch))
                    { 
                    }
                    else 
                    { //error handling
                    }
                    if (mode == 2)
                    {
                        numphotoB++;
                    }
                }



                if (buttons & XINPUT_GAMEPAD_DPAD_UP)
                {
                   
                    
                    scroll.x = rect.left + (rect.right - rect.left) / 2;
				    scroll.y = rect.top + 1;
                    scrollmap = true;
                }
               else if (buttons & XINPUT_GAMEPAD_DPAD_DOWN)
                {
                    
                    scroll.x = rect.left + (rect.right - rect.left) / 2;
                    scroll.y = rect.bottom - 1;
                    scrollmap = true;
                }
                else if (buttons & XINPUT_GAMEPAD_DPAD_LEFT)
                {
                    
                    scroll.x = rect.left + 1;
                    scroll.y = rect.top + (rect.bottom - rect.top) / 2;

                    scrollmap = true;

                }
                else if (buttons & XINPUT_GAMEPAD_DPAD_RIGHT)
                {

                    
                    scroll.x = rect.right - 1;
                    scroll.y = rect.top + (rect.bottom - rect.top) / 2;

                    scrollmap = true;
                }
                else
                {
                    scrollmap = false;
                }



                if (buttons & XINPUT_GAMEPAD_X)
                {
                    startsearch = startsearchX;
                    Buttonaction("\\X", mode, numphotoX, startsearch);
                    if (mode == 2)
                    {
                        numphotoX++;
                    }
                }
                if (buttons & XINPUT_GAMEPAD_Y)
                {
                    Buttonaction("\\Y", mode, numphotoY, startsearch);
                    if (mode == 2)
                    {
                        numphotoY++;
                    }
                }
                if (buttons & XINPUT_GAMEPAD_START)
                {

                    if (mode == 0 && Modechange == 1)
                    {
                        mode = 1;

                        MessageBox(NULL, "Bmp + Emulated cursor mode", "Move the flickering red dot and use right trigger for left click. left trigger for right click", MB_OK | MB_ICONINFORMATION);
                    }
                    else if (mode == 1 && Modechange == 1)
                    {
                        mode = 2;
                        MessageBox(NULL, "Edit Mode", "Button mapping. will map buttons you click with the flickering red dot as an input coordinate", MB_OK | MB_ICONINFORMATION);

                    }
                    else if (mode == 2 && Modechange == 1)
                    {
                        mode = 0;
                        MessageBox(NULL, "Bmp mode", "only send input on bmp match", MB_OK | MB_ICONINFORMATION);
                    }
                    Sleep(1000); //have time to release button. this is no hurry anyway
                }
                if (mode > 0 )
                { 
                    //fake cursor poll

                    int width = rect.right - rect.left;
                        int height = rect.bottom - rect.top;
                    int Xaxis = state.Gamepad.sThumbRX;
                    int Yaxis = state.Gamepad.sThumbRY;

                    

                    OldX = X;
                    if (Xaxis < AxisLeftsens) //strange values. but tested many before choosing this
                    { 
                        if (X > 0)
                        { 
                            sovetid = sens - (std::abs(Xaxis) / 450);
                        X = X - 2;
                        }
                    }
                    else if (Xaxis > AxisRightsens) //strange values. but tested many before choosing this
                    {
                        if (X < width - 1)
                        {
                            sovetid = sens - (Xaxis / 450);
                            X = X + 2;
                        }
                    }
                    int accumulater = std::abs(Xaxis) + std::abs(Yaxis);
                     
                    /////////////////////
                    if (Yaxis > AxisUpsens) //strange values. but tested many before choosing this
                    {
                        if (Y > 0)
                        {
                            sovetid = sens - (std::abs(Yaxis) / 450);
                            Y = Y - 2;
                        }
                    }
                    else  if (Yaxis < AxisDownsens) //strange values. but tested many before choosing this
                    { //my controller is not calibrated maybe
                        if (Y < height - 1)
                        {
                            sovetid = sens - (std::abs(Yaxis) / 450); //a litt
                            Y = Y + 2;
                        }
                    }
                    int nysovetid = sens2 - (accumulater / 700);
                    if (nysovetid < sovetid)
                        sovetid = nysovetid;
                    if (sovetid < 3) //speedlimit
                        sovetid = 3;
                    CaptureWindow24Bit(hwnd, screenSize, largePixels, strideLarge, true); //draw
                    //MessageBox(NULL, "failed to load bmp:", "Message Box", MB_OK | MB_ICONINFORMATION);

              
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

                        if (abs(startdrag.x - fakecursor.x) <= 5)
                        { 
                            ClientToScreen(hwnd, &startdrag);
                            SendMouseClick(startdrag.x, startdrag.y, 2, 3 ); //4 4 move //5 release
                            ScreenToClient(hwnd, &startdrag);
                            leftPressedold = false;
                            //closing curtains
                        }
                        else
                        { 
                        ClientToScreen(hwnd, &startdrag);

                        SendMouseClick(startdrag.x, startdrag.y, 6, 2); //4 4 move //5 release
                        ClientToScreen(hwnd, &fakecursor);
                        Sleep(30);
                       
                        SendMouseClick(fakecursor.x, fakecursor.y, 8, 1);
                        Sleep(20);
                        SendMouseClick(fakecursor.x, fakecursor.y, 7, 2);
                        ScreenToClient(hwnd, &fakecursor);
                        ScreenToClient(hwnd, &startdrag);
                        leftPressedold = false;
                        }
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

                        if (abs(startdrag.x - fakecursor.x) <= 5)
                        {
                            ClientToScreen(hwnd, &startdrag);
                            SendMouseClick(startdrag.x, startdrag.y, 1, 3); //4 4 move //5 release
                            ScreenToClient(hwnd, &startdrag);
                            rightPressedold = false;
                        }
                        else
                        {
                            ClientToScreen(hwnd, &startdrag);

                            SendMouseClick(startdrag.x, startdrag.y, 3, 2); //4 4 move //5 release
                            ClientToScreen(hwnd, &fakecursor);
                            Sleep(30);
                            SendMouseClick(fakecursor.x, fakecursor.y, 8, 1); //4 skal vere 3
                            Sleep(20);
                            SendMouseClick(fakecursor.x, fakecursor.y, 5, 2);
                            ScreenToClient(hwnd, &fakecursor);
                            ScreenToClient(hwnd, &startdrag);
                            rightPressedold = false;
                        }
                    }
                } //rightpress
                } // mode above 0
            } //no controller

        } // no hwnd
        if (knappsovetid > 20)
        {
          //  sovetid = 20;
          //  knappsovetid = 100;
            
		}


        if (mode == 0)
            Sleep(50);
        if (mode > 0)
            Sleep(sovetid); //15-80 //ini value

    } //loop end but endless
    return 0;
}
//DWORD WINAPI ThreadFunction(LPVOID lpParam)
void RemoveHook() {
    MH_DisableHook(MH_ALL_HOOKS);
    MH_Uninitialize();
}
void SetupHook() {
    MH_Initialize();
	MH_CreateHookApi(L"user32", "GetCursorPos", &MyGetCursorPos, reinterpret_cast<LPVOID*>(&fpGetCursorPos));   
    MH_EnableHook(MH_ALL_HOOKS);
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved) {
    switch (ul_reason_for_call) {
    case DLL_PROCESS_ATTACH:
        SetupHook();
      CreateThread(nullptr, 0,
          (LPTHREAD_START_ROUTINE)ThreadFunction, GetModuleHandle(0), 0, 0);
        break;
    case DLL_PROCESS_DETACH:
        RemoveHook();
        break;
    }
    return TRUE;
}