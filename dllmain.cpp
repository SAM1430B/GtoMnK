// dllmain.cpp : Defines the entry point for the DLL application.
#include "pch.h"
#include <windows.h>
#include <windowsx.h>
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

HMODULE g_hModule = nullptr;

typedef UINT(WINAPI* GetCursorPos_t)(LPPOINT lpPoint);
typedef UINT(WINAPI* SetCursorPos_t)(LPPOINT lpPoint);

typedef SHORT(WINAPI* GetAsyncKeyState_t)(int);
typedef SHORT(WINAPI* GetKeyState_t)(int);
typedef BOOL(WINAPI* ClipCursor_t)(const RECT*);
typedef HCURSOR(WINAPI* SetCursor_t)(HCURSOR);





GetCursorPos_t fpGetCursorPos = nullptr;
GetCursorPos_t fpSetCursorPos = nullptr;
GetAsyncKeyState_t originalGetAsyncKeyState = nullptr;
GetKeyState_t originalGetKeyState = nullptr;
ClipCursor_t originalClipCursor = nullptr;
SetCursor_t originalSetCursor = nullptr;



POINT fakecursor;
POINT startdrag;
POINT activatewindow;
POINT scroll;
bool loop = true;
HWND hwnd;
int showmessage = 0; //0 = no message, 1 = initializing, 2 = bmp mode, 3 = bmp and cursor mode, 4 = edit mode   
int counter = 0;

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
int keystatesend = 0; //key to send
int clipcursorhook = 0;
int getkeystatehook = 0;
int getasynckeystatehook = 0;
int getcursorposhook = 0;
int setcursorhook = 0;
int userealmouse = 0;
HHOOK hMouseHook;



//fake cursor
int X = 20;
int Y = 20;
int OldX = 0;
int OldY = 0;
int ydrag;
int xdrag;
bool scrollmap = false;
bool pausedraw = false;
int cursorimage = 0;
int drawfakecursor = 0;
HICON hCursor = 0;

//beautiful cursor
int colorfulSword[20][20] = {
{1,1,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
{1,2,2,1,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
{1,2,2,2,2,1,1,0,0,0,0,0,0,0,0,0,0,0,0,0},
{1,2,2,2,2,2,2,1,1,0,0,0,0,0,0,0,0,0,0,0},
{1,2,2,2,2,2,2,2,2,1,1,0,0,0,0,0,0,0,0,0},
{1,2,2,2,2,2,2,2,2,2,2,1,1,0,0,0,0,0,0,0},
{1,2,2,2,2,2,2,2,2,2,2,2,2,1,1,0,0,0,0,0},
{1,2,2,2,2,2,2,2,2,2,2,2,2,2,2,1,1,0,0,0},
{1,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,1,0,0},
{1,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,1,0,0,0},
{1,2,2,2,2,2,2,2,2,2,2,2,2,1,1,1,0,0,0,0},
{1,2,2,2,2,2,2,2,2,2,2,2,2,1,0,0,0,0,0,0},
{1,2,2,2,2,2,2,2,2,2,2,2,1,0,0,0,0,0,0,0},
{1,2,2,2,2,2,2,2,2,2,2,2,1,0,0,0,0,0,0,0},
{1,2,2,2,2,2,2,1,1,2,2,2,1,0,0,0,0,0,0,0},
{1,2,2,2,2,2,1,0,0,1,2,2,2,2,1,0,0,0,0,0},
{1,2,2,2,2,1,0,0,0,0,1,2,2,2,1,0,0,0,0,0},
{1,1,2,2,1,0,0,0,0,0,0,1,2,2,2,1,0,0,0,0},
{1,2,2,1,0,0,0,0,0,0,0,1,2,2,2,1,0,0,0,0},
{1,1,1,0,0,0,0,0,0,0,0,0,1,1,1,0,0,0,0,0},
};
//temporary cursor on success

COLORREF colors[5] = {
    RGB(0, 0, 0),          // Transparent - won't be drawn
    RGB(140, 140, 140),    // Gray for blade
    RGB(255, 255, 255),    // White shine
    RGB(139, 69, 19),       // Brown handle
    RGB(50, 150, 255)     // Light blue accent

};


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
int startsearchC = 0;
int startsearchD = 0;
int startsearchE = 0;
int startsearchF = 0;

int righthanded = 0;


int Atype = 0;
int Btype = 0;
int Xtype = 0;
int Ytype = 0;
int Ctype = 0;
int Dtype = 0;
int Etype = 0;
int Ftype = 0;


int x = 0;

HBITMAP hbm;

std::vector<BYTE> largePixels, smallPixels;
SIZE screenSize;
int strideLarge, strideSmall;
int smallW, smallH;

int mode = 0;
int sovetid = 16;
int knappsovetid = 100;

int samekey = 0;
int samekeyA = 0;

// Hook callback function
//LRESULT CALLBACK LowLevelMouseProc(int nCode, WPARAM wParam, LPARAM lParam) {
//    if (nCode == HC_ACTION) {
//        MSLLHOOKSTRUCT* pMouseStruct = (MSLLHOOKSTRUCT*)lParam;
//        if (pMouseStruct != nullptr) {
//            std::cout << "Mouse at (" << pMouseStruct->pt.x << ", " << pMouseStruct->pt.y << ")" << std::endl;
//        }
//    }
 //   return CallNextHookEx(hMouseHook, nCode, wParam, lParam);
//}

HCURSOR WINAPI HookedSetCursor(HCURSOR hcursor) {
    // Example: log or replace cursor
   // OutputDebugString(L"SetCursor was called!\n");
		hCursor = hcursor; // Store the cursor handle   
    
    
    // Optionally replace the cursor
    // hCursor = LoadCursor(NULL, IDC_HAND);

   // return originalSetCursor(hcursor); // Call original
        return hcursor;
}

SHORT WINAPI HookedGetAsyncKeyState(int vKey) 
{
    
    if (samekeyA == vKey) {
        return 8001; 
        //8001 on hold key. but this may not work
    }
    else samekeyA = 0;

    if (vKey == keystatesend)
    {
        samekeyA = vKey;
        return 8000; //8001 ?
    }
    else
    {
        SHORT result = originalGetAsyncKeyState(vKey);
        return result;
    }
}

// Hooked GetKeyState
SHORT WINAPI HookedGetKeyState(int vKey) {
    if (samekey == vKey) {
        return 8001;
        //8001 on hold key. but this may not work
    }
    else samekey = 0;

    if (vKey == keystatesend)
    {
        samekey = vKey;
        return 8000; //8001 ?
    }
    else
    {
        SHORT result = originalGetKeyState(vKey);
        return result;
    }
}

BOOL WINAPI MyGetCursorPos(PPOINT lpPoint) {
    if (lpPoint) 
    {
        POINT mpos;
            if (scrollmap == false)
            { 
                mpos.x = X; //hwnd coordinates 0-800 on a 800x600 window
                mpos.y = Y;//hwnd coordinate s0-600 on a 800x600 window
            ClientToScreen(hwnd, &mpos);
		
            lpPoint->x = mpos.x; //desktop coordinates
            lpPoint->y = mpos.y;
            ScreenToClient(hwnd, &mpos); //revert so i am sure its done
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
        return true;
    }
    return false;
}
BOOL WINAPI MySetCursorPos(PPOINT lpPoint) {
        //POINT mpos;
        //ClientToScreen(hwnd, &mpos);

        X = lpPoint->x; //desktop coordinates? or hwnd?
        Y = lpPoint->y;
        //ScreenToClient(hwnd, &mpos); //revert so i am sure its done

    return false;
}
BOOL WINAPI HookedClipCursor(const RECT* lpRect) {
    return true; //nonzero bool or int
    //return originalClipCursor(nullptr);

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
bool IsCursorInWindow(HWND hwnd) {
    POINT pt;
    DWORD pos = GetMessagePos();
    pt.x = GET_X_LPARAM(pos);
    pt.y = GET_Y_LPARAM(pos);
    //ScreenToClient(hwnd, &pt);

    RECT clientRect;
    GetClientRect(hwnd, &clientRect);

    return PtInRect(&clientRect, pt);
}
void vibrateController(int controllerId, WORD strength)
{
    XINPUT_VIBRATION vibration = {};
    vibration.wLeftMotorSpeed = strength;   // range: 0 - 65535
    vibration.wRightMotorSpeed = strength;

    // Activate vibration
    XInputSetState(controllerId, &vibration);

    // Keep vibration on for 1 second
    Sleep(50); // milliseconds

    // Stop vibration
    vibration.wLeftMotorSpeed = 0;
    vibration.wRightMotorSpeed = 0;
    XInputSetState(controllerId, &vibration);
}

bool SendMouseClick(int x, int y, int z, int many) {
    // Create a named mutex
    
    if (userealmouse == 0) 
        {
        POINT heer;
        heer.x = x;
        heer.y = y;
        ScreenToClient(hwnd, &heer);
        LPARAM clickPos = MAKELPARAM(heer.x, heer.y);
        if ( z == 1){
            
            PostMessage(hwnd, WM_LBUTTONDOWN, MK_LBUTTON, clickPos);
            PostMessage(hwnd, WM_LBUTTONUP, 0, clickPos);
            keystatesend = VK_LEFT;
        }
        if (z == 2) {
            
            PostMessage(hwnd, WM_RBUTTONDOWN, MK_RBUTTON, clickPos);
            PostMessage(hwnd, WM_RBUTTONUP, 0, clickPos);
        }
        if (z == 3) {
            PostMessage(hwnd, WM_LBUTTONDOWN, MK_LBUTTON, clickPos);
            keystatesend = VK_LEFT;
        }
        if (z == 5)
        {
            PostMessage(hwnd, WM_LBUTTONUP, 0, clickPos);

        }
        else if (z == 6 || z == 8 || z == 10 || z == 11 || z == 4) //only mousemove
        {
            PostMessage(hwnd, WM_MOUSEMOVE, 0, clickPos);
            //PostMessage(hwnd, WM_SETCURSOR, (WPARAM)hwnd, MAKELPARAM(HTCLIENT, WM_MOUSEMOVE));

        }
        else if (z == 4) //only mousemove
        {
            PostMessage(hwnd, WM_RBUTTONUP, 0, clickPos);
        }
        return true;
    }
    if (z == 1 || z == 2 || z == 3 || z == 6 || z == 10)
    {
        if (Mutexlock(true)) {

        }
        else
        {
            //errorhandling
        }
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
        keystatesend = VK_LEFT;
        
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
        keystatesend = VK_LEFT;
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

   // HDC hdc = GetDC(NULL);
	HDC hdc = CreateCompatibleDC(NULL);
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

            if (cursorimage == 1)
            {
                
              //  CURSORINFO ci = { sizeof(CURSORINFO) };
            //    if (IsCursorInWindow(hwnd) == true) {
                 //       hCursor = ci.hCursor;
                 //   }
                    //ICONINFO iconInfo;  
                    // 
                // Fill bitmap with transparent background
                    RECT rect = { 0, 0, 32, 32 }; //need bmp width height
                    FillRect(hdcMem, &rect, (HBRUSH)(COLOR_WINDOW + 1));


                    // Draw icon into memory DC
                    if (hCursor != 0)
                    { 
                        
                        if (showmessage == 1)
                        {
                            TextOut(hdcWindow, X, Y, TEXT("BMP MODE"), 8);
                            TextOut(hdcWindow, X, Y + 17, TEXT("only mapping searches"), 21);
                        }
                        else if (showmessage == 2)
                        {
                            TextOut(hdcWindow, X, Y, TEXT("CURSOR MODE"), 11);
                            TextOut(hdcWindow, X, Y + 17, TEXT("mapping searches + cursor"), 25);
                        }
                        else if (showmessage == 3)
                        {
                            TextOut(hdcWindow, X, Y, TEXT("EDIT MODE"), 9);
                            TextOut(hdcWindow, X, Y + 15, TEXT("tap a button to bind it to coordinate"), 37);
                            TextOut(hdcWindow, X, Y + 30, TEXT("A,B,X,Y,R2,R3,L2,L3 can be mapped"), 32);
                        }
                        else if (showmessage == 10)
                        {
                            TextOut(hdcWindow, X, Y, TEXT("BUTTON MAPPED"), 13);
                        }
                        else DrawIconEx(hdcWindow, 0 + X, 0 + Y, hCursor, 32, 32, 0, NULL, DI_NORMAL);//need bmp width height

                    }
                    else {
                        for (int y = 0; y < 20; y++)
                        {
                            for (int x = 0; x < 20; x++)
                            {
                                int val = colorfulSword[y][x];
                                if (val != 0)
                                {
                                    HBRUSH hBrush = CreateSolidBrush(colors[val]);
                                    RECT rect = { X + x , Y + y , X + x + 1, Y + y + 1 };
                                    FillRect(hdcWindow, &rect, hBrush);
                                    DeleteObject(hBrush);
                                }
                            }
                        }
                    }

            }
            else
            {
               for (int y = 0; y < 20; y++)
                  {
                 for (int x = 0; x < 20; x++)
                     {
                      int val = colorfulSword[y][x];
                       if (val != 0)
                       {
                          HBRUSH hBrush = CreateSolidBrush(colors[val]);
                           RECT rect = { X + x , Y + y , X + x + 1, Y + y + 1 };
                           FillRect(hdcWindow, &rect, hBrush);
                          DeleteObject(hBrush);
                       }
                    }
               }
            }
           
                //hbm24 = NULL;
              //  HBRUSH hBrush = CreateSolidBrush(RGB(255, 60, 2));
                // todo: for loop get cursor bmp and fillrect each pixel. maybe cpu intensive
             //   RECT rect = { X, Y, X + 4, Y + 4 };
              //  RECT rect2 = { X - 1, Y +1, X + 6, Y + 4 };
             //   FillRect(hdcWindow, &rect, hBrush);
             //   FillRect(hdcWindow, &rect2, hBrush);
             //   DeleteObject(hBrush);
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
        bool clicknotmove = false;
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

                          //char buffer[100];
 // wsprintf(buffer, "A is %d", i);
 //  MessageBoxA(NULL, buffer, "Info", MB_OK);
                        vibrateController(0, 15000);
                        if (strcmp(key, "\\A") == 0) {
                            if (Atype == 1)
                            {
                                movenotclick = true;
                            }
                            if (Atype == 2)
                            {
                                clicknotmove = true;
                            }
                            startsearchA = i + 1;
                        }
                        else if (strcmp(key, "\\B") == 0) {
                            if (Btype == 1)
                            {
                                movenotclick = true;
                            }
                            if (Btype == 2)
                            {
                                clicknotmove = true;
                            }
                            startsearchB = i + 1;
                        }
                        else if (strcmp(key, "\\X") == 0) {
                            if (Xtype == 1)
                            {
                                movenotclick = true;
                            }
                            if (Xtype == 2)
                            {
                                clicknotmove = true;
                            }
                            startsearchX = i + 1;
                        }
                        else if (strcmp(key, "\\Y") == 0) {
                            if (Ytype == 1)
                            {
                                movenotclick = true;
                            }
                            if (Ytype == 2)
                            {
                                clicknotmove = true;
                            }
                            startsearchY = i + 1;
                        }
                        else if (strcmp(key, "\\C") == 0) {
                            if (Ctype == 1)
                            {
                                movenotclick = true;
                            }
                            if (Ctype == 2)
                            {
                                clicknotmove = true;
                            }
                            startsearchC = i + 1;
                        }
                        else if (strcmp(key, "\\D") == 0) {
                            if (Dtype == 1)
                            {
                                movenotclick = true;
                            }
                            if (Dtype == 2)
                            {
                                clicknotmove = true;
                            }
                            startsearchD = i + 1;
                        }
                        else if (strcmp(key, "\\E") == 0) {
                            if (Etype == 1)
                            {
                                movenotclick = true;
                            }
                            if (Etype == 2)
                            {
                                clicknotmove = true;
                            }
                            startsearchE = i + 1;
                        }
                        else if (strcmp(key, "\\F") == 0) {
                            if (Ftype == 1)
                            {
                                movenotclick = true;
                            }
                            if (Ftype == 2)
                            {
                                clicknotmove = true;
                            }
                            startsearchF = i + 1;
                        }
                        // else return false;
                        if ( clicknotmove == false)
                        { 
                            X = pt.x;
                            Y = pt.y;
                            //MessageBox(NULL, "some kind of error", "found image", MB_OK | MB_ICONINFORMATION);
                        }
                        if (movenotclick == false) {
                            ClientToScreen(hwnd, &pt);
                            SendMouseClick(pt.x, pt.y, 1, 3);
                            ScreenToClient(hwnd, &pt);
                        }
                        foundit = true;
                        break;
                    }

                    // else  return false; 
                    


                }
                if (hbmdsktop != 0)
                DeleteObject(hbmdsktop);
                // else  

            }

            // else  
        }
        if (foundit == false)
        {
              
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
                       // wsprintf(buffer, "second A is %d", i);
                       // MessageBoxA(NULL, buffer, "Info", MB_OK);
                        // MessageBox(NULL, "some kind of error", "captured desktop", MB_OK | MB_ICONINFORMATION);
                        if (FindSubImage24(largePixels.data(), screenSize.cx, screenSize.cy, strideLarge, smallPixels.data(), smallW, smallH, strideSmall, pt, 0, 0))
                        {
                             //char buffer[100];

                            vibrateController(0, 15000);
                            if (strcmp(key, "\\A") == 0) {
                                if (Atype == 1)
                                {
                                    movenotclick = true;
                                }
                                if (Atype == 2)
                                {
                                    clicknotmove = true;
                                }
                                startsearchA = i + 1;
                            }
                            else if (strcmp(key, "\\B") == 0) {
                                if (Btype == 1)
                                {
                                    movenotclick = true;
                                }
                                if (Btype == 2)
                                {
                                    clicknotmove = true;
                                }
                                startsearchB = i + 1;
                            }
                            else if (strcmp(key, "\\X") == 0) {
                                if (Xtype == 1)
                                {
                                    movenotclick = true;
                                }
                                if (Xtype == 2)
                                {
                                    clicknotmove = true;
                                }
                                startsearchX = i + 1;
                            }
                            else if (strcmp(key, "\\Y") == 0) {
                                if (Ytype == 1)
                                {
                                    movenotclick = true;
                                }
                                if (Ytype == 2)
                                {
                                    clicknotmove = true;
                                }
                                startsearchY = i + 1;
                            }
                            else if (strcmp(key, "\\C") == 0) {
                                if (Ctype == 1)
                                {
                                    movenotclick = true;
                                }
                                if (Ctype == 2)
                                {
                                    clicknotmove = true;
                                }
                                startsearchC = i + 1;
                            }
                            else if (strcmp(key, "\\D") == 0) {
                                if (Dtype == 1)
                                {
                                    movenotclick = true;
                                }
                                if (Dtype == 2)
                                {
                                    clicknotmove = true;
                                }
                                startsearchD = i + 1;
                            }
                            else if (strcmp(key, "\\E") == 0) {
                                if (Etype == 1)
                                {
                                    movenotclick = true;
                                }
                                if (Etype == 2)
                                {
                                    clicknotmove = true;
                                }
                                startsearchE = i + 1;
                            }
                            else if (strcmp(key, "\\F") == 0) {
                                if (Ftype == 1)
                                {
                                    movenotclick = true;
                                }
                                if (Ftype == 2)
                                {
                                    clicknotmove = true;
                                }
                                startsearchF = i + 1;
                            }
                           // X = pt.x;
                           // Y = pt.y;
                            if (clicknotmove == false)
                            {
                                X = pt.x;
                                Y = pt.y;
                               // MessageBox(NULL, "some kind of error", "found image", MB_OK | MB_ICONINFORMATION);
                            }
                            if (movenotclick == false)
                            { 
                                ClientToScreen(hwnd, &pt);
                                SendMouseClick(pt.x, pt.y, 1, 3);
                                ScreenToClient(hwnd, &pt);
                            }
                            break;
                        }
                        

                    }
                    //  else  return false;
                    if (hbmdsktop != 0)
                    DeleteObject(hbmdsktop);

                }
            }


        }
    }
    else if (showmessage == 0) //mode 2 button mapping //showmessage var to make sure no double map or map while message
    {
		//RepaintWindow(hwnd, NULL, FALSE); 
        Sleep(100); //to make sure red flicker expired
        std::string path = UGetExecutableFolder() + key + std::to_string(serchnum) + ".bmp";
        std::wstring wpath(path.begin(), path.end());
        SaveWindow10x10BMP(hwnd, wpath.c_str(), X, Y);
       // MessageBox(NULL, "Mapped spot!", key, MB_OK | MB_ICONINFORMATION);
        showmessage = 10;
        return true;
    }
    Sleep(50); //to avoid double press
    pausedraw = false;
    return true;
}
//LRESULT CALLBACK MouseHookProc(int nCode, WPARAM wParam, LPARAM lParam);
//LRESULT CALLBACK MouseHookProc(int nCode, WPARAM wParam, LPARAM lParam) {
 //   if (nCode == HC_ACTION) {
//        PMSLLHOOKSTRUCT pMouse = (PMSLLHOOKSTRUCT)lParam;
//
//        HWND hwndUnderCursor = WindowFromPoint(pMouse->pt);
//        if (hwndUnderCursor == hwnd) {
            // Swallow the message
           // MessageBox(NULL, "failed to load bmp:", "Message Box", MB_OK | MB_ICONINFORMATION);
 //           return 1; // Non-zero return value blocks the message
 //       }
 //   }
//    return CallNextHookEx(NULL, nCode, wParam, lParam);
//}


//DWORD WINAPI hhmousehookthread(LPVOID lpParam)
//{
  ///  hMouseHook = SetWindowsHookEx(WH_MOUSE_LL, LowLevelMouseProc, GetModuleHandle(NULL), 0);
//    hMouseHook = SetWindowsHookEx(WH_MOUSE, MouseProc, GetModuleHandle(NULL), GetCurrentThreadId());
//
//    HHOOK hMouseHook = SetWindowsHookEx(
//        WH_MOUSE_LL,          // Low-level mouse hook
 //       MouseHookProc,        // Your hook procedure
 //       NULL,            // NULL if in same process; DLL handle if injected
 //       0                     // Hook all threads
 //   );


 //   if (hMouseHook == NULL) 
 //   {
 //       
 //       return 1;
////  }
 //      MSG msg;
 //  while (GetMessage(&msg, NULL, 0, 0)) 
// {
//       TranslateMessage(&msg);
//       //MessageBox(NULL, "failed to load bmp:", "Message Box", MB_OK | MB_ICONINFORMATION);
//      DispatchMessage(&msg);
//  }
//}
//LRESULT CALLBACK LowLevelMouseProc(int nCode, WPARAM wParam, LPARAM lParam) {
//    if (nCode == HC_ACTION) {
//        MSLLHOOKSTRUCT* info = (MSLLHOOKSTRUCT*)lParam;
//
//    }
//    return CallNextHookEx(NULL, nCode, wParam, lParam);
//}

//LRESULT CALLBACK MouseProc(int nCode, WPARAM wParam, LPARAM lParam) {
//    if (nCode >= 0) {
//        MOUSEHOOKSTRUCT* mhs = (MOUSEHOOKSTRUCT*)lParam;

//        if (mhs->hwnd == hwnd) {
            

 //       }

 //   }

 //   return CallNextHookEx(hMouseHook, nCode, wParam, lParam);
//}
//LRESULT CALLBACK CallWndProcHook(int nCode, WPARAM wParam, LPARAM lParam)
//{
//    if (nCode >= 0)
//    {
//        CWPSTRUCT* cwp = (CWPSTRUCT*)lParam;
//        if (cwp->message == WM_PAINT)
//        {
//            if ( hwnd != 0)
//                CaptureWindow24Bit(hwnd, screenSize, largePixels, strideLarge, true); //draw fake cursor
 //       }
//    }

 //   return CallNextHookEx(hMouseHook, nCode, wParam, lParam);
//}

//DWORD WINAPI Drawthread(LPVOID lpParam)
//{

 //   while (loop == true) {
 //       if (hwnd && pausedraw == false) {
 //           CaptureWindow24Bit(hwnd, screenSize, largePixels, strideLarge, true);
 //            
  //      }
 //       Sleep(3); // Sync with refresh rate
 //   }


  //  HHOOK hMouseHook = SetWindowsHookEx(WH_CALLWNDPROC, CallWndProcHook, GetModuleHandle(NULL), 0);


  //  MSG msg;
  //  while (GetMessage(&msg, NULL, 0, 0))
  //  {
   //     TranslateMessage(&msg);
  //      DispatchMessage(&msg);
  //  }
//    return 1;

//}


DWORD WINAPI ThreadFunction(LPVOID lpParam)
{
    Sleep(2000);

    // settings reporting
    std::string iniPath = UGetExecutableFolder() + "\\Xinput.ini";
    std::string iniSettings = "Settings";
   // std::string controllerID = getIniString(iniSettings.c_str(), "Controller ID", "0", iniPath);

    //controller settings
    int controllerID = GetPrivateProfileInt(iniSettings.c_str(), "Controllerid", 0, iniPath.c_str()); //simple test if settings read but write it wont work.
    int AxisLeftsens = GetPrivateProfileInt(iniSettings.c_str(), "AxisLeftsens", -7849, iniPath.c_str());
    int AxisRightsens = GetPrivateProfileInt(iniSettings.c_str(), "AxisRightsens", 12000, iniPath.c_str());
    int AxisUpsens = GetPrivateProfileInt(iniSettings.c_str(), "AxisUpsens", 0, iniPath.c_str());
    int AxisDownsens = GetPrivateProfileInt(iniSettings.c_str(), "AxisDownsens", -16049, iniPath.c_str());
    righthanded = GetPrivateProfileInt(iniSettings.c_str(), "Righthanded", 0, iniPath.c_str());

    //mode
    int InitialMode = GetPrivateProfileInt(iniSettings.c_str(), "Initial Mode", 1, iniPath.c_str());
    int Modechange = GetPrivateProfileInt(iniSettings.c_str(), "Allow modechange", 1, iniPath.c_str());


    int sens = GetPrivateProfileInt(iniSettings.c_str(), "Dot Speed", 40, iniPath.c_str());
    int sens2 = GetPrivateProfileInt(iniSettings.c_str(), "CA Dot Speed", 75, iniPath.c_str());
    int sendfocus = GetPrivateProfileInt(iniSettings.c_str(), "Sendfocus", 0, iniPath.c_str());
    keyrespondtime = GetPrivateProfileInt(iniSettings.c_str(), "Keyresponsetime", 50, iniPath.c_str());
    getmouseonkey = GetPrivateProfileInt(iniSettings.c_str(), "GetMouseOnKey", 0, iniPath.c_str());

    //clicknotmove 2
    //movenotclick 1
    Atype = GetPrivateProfileInt(iniSettings.c_str(), "Ainputtype", 0, iniPath.c_str());
    Btype = GetPrivateProfileInt(iniSettings.c_str(), "Binputtype", 0, iniPath.c_str());
    Xtype = GetPrivateProfileInt(iniSettings.c_str(), "Xinputtype", 1, iniPath.c_str());
    Ytype = GetPrivateProfileInt(iniSettings.c_str(), "Yinputtype", 0, iniPath.c_str());
    Ctype = GetPrivateProfileInt(iniSettings.c_str(), "Cinputtype", 2, iniPath.c_str());
    Dtype = GetPrivateProfileInt(iniSettings.c_str(), "Dinputtype", 0, iniPath.c_str());
    Etype = GetPrivateProfileInt(iniSettings.c_str(), "Einputtype", 0, iniPath.c_str());
    Ftype = GetPrivateProfileInt(iniSettings.c_str(), "Finputtype", 0, iniPath.c_str());
    cursorimage = GetPrivateProfileInt(iniSettings.c_str(), "Cursortype", 1, iniPath.c_str());

    //hooks
    drawfakecursor = GetPrivateProfileInt(iniSettings.c_str(), "DrawFakeCursor", 1, iniPath.c_str());
	userealmouse = GetPrivateProfileInt(iniSettings.c_str(), "UseRealMouse", 0, iniPath.c_str());   //scrolloutsidewindow
    int scrolloutsidewindow = GetPrivateProfileInt(iniSettings.c_str(), "Scrollmapfix", 1, iniPath.c_str());   //scrolloutsidewindow
    

    Sleep(1000);

   // CreateThread(nullptr, 0,
    //    (LPTHREAD_START_ROUTINE)hhmousehookthread, GetModuleHandle(0), 0, 0);
        //hhmousehook


 //   std::cout << "Mouse hook installed. Press Ctrl+C to exit." << std::endl;

    // Message loop to keep the hook alive
 //   MSG msg;
 //   while (GetMessage(&msg, NULL, 0, 0)) {
 //       TranslateMessage(&msg);
 //       DispatchMessage(&msg);
 //   }

  //  { // presume missing settings file. attempt to generate
	//	MessageBoxA(NULL, "Settings file not found. Generating default settings.", "Error", MB_OK | MB_ICONERROR);
       // controllerID = 0; //reset fault check
    //    std::string valueStr = "1"; //controlerid
    //    WritePrivateProfileString(iniSettings.c_str(), "Controllerid", valueStr.c_str(), iniPath.c_str());
    //    valueStr = "-7849"; 
    //    WritePrivateProfileString(iniSettings.c_str(), "AxisLeftsens", valueStr.c_str(), iniPath.c_str());
    //    valueStr = "12000"; 
    //    WritePrivateProfileString(iniSettings.c_str(), "AxisRightsens", valueStr.c_str(), iniPath.c_str());
    //    valueStr = "0"; 
   //     WritePrivateProfileString(iniSettings.c_str(), "AxisUpsens", valueStr.c_str(), iniPath.c_str());
    //    valueStr = "-16049"; 
    //    WritePrivateProfileString(iniSettings.c_str(), "AxisDownsens", valueStr.c_str(), iniPath.c_str());
    //    valueStr = "0"; 
    //    WritePrivateProfileString(iniSettings.c_str(), "Righthanded", valueStr.c_str(), iniPath.c_str());/////////////////////
   //     valueStr = "1";
   //    WritePrivateProfileString(iniSettings.c_str(), "Initial Mode", valueStr.c_str(), iniPath.c_str());
   //     WritePrivateProfileString(iniSettings.c_str(), "Allow modechange", valueStr.c_str(), iniPath.c_str());
   //     valueStr = "40"; 
   //     WritePrivateProfileString(iniSettings.c_str(), "Dot Speed", valueStr.c_str(), iniPath.c_str());
    //    valueStr = "75";
    //    WritePrivateProfileString(iniSettings.c_str(), "CA Dot Speed", valueStr.c_str(), iniPath.c_str());
    //    valueStr = "0";
    //    WritePrivateProfileString(iniSettings.c_str(), "Sendfocus", valueStr.c_str(), iniPath.c_str()); 
    //    valueStr = "50";
    //    WritePrivateProfileString(iniSettings.c_str(), "Keyresponsetime", valueStr.c_str(), iniPath.c_str()); 
    //    valueStr = "0";
    //    WritePrivateProfileString(iniSettings.c_str(), "GetMouseOnKey", valueStr.c_str(), iniPath.c_str()); 
    ///    WritePrivateProfileString(iniSettings.c_str(), "Ainputtype", valueStr.c_str(), iniPath.c_str()); 
    //    WritePrivateProfileString(iniSettings.c_str(), "Binputtype", valueStr.c_str(), iniPath.c_str()); 
    //    WritePrivateProfileString(iniSettings.c_str(), "Xinputtype", valueStr.c_str(), iniPath.c_str()); 
    //    WritePrivateProfileString(iniSettings.c_str(), "Yinputtype", valueStr.c_str(), iniPath.c_str()); 
    //    WritePrivateProfileString(iniSettings.c_str(), "Cinputtype", valueStr.c_str(), iniPath.c_str()); 
    //    WritePrivateProfileString(iniSettings.c_str(), "Dinputtype", valueStr.c_str(), iniPath.c_str()); 
    //    WritePrivateProfileString(iniSettings.c_str(), "Cinputtype", valueStr.c_str(), iniPath.c_str()); 
    //    WritePrivateProfileString(iniSettings.c_str(), "Dinputtype", valueStr.c_str(), iniPath.c_str()); 
    //    WritePrivateProfileString(iniSettings.c_str(), "Einputtype", valueStr.c_str(), iniPath.c_str()); 
   //     WritePrivateProfileString(iniSettings.c_str(), "Finputtype", valueStr.c_str(), iniPath.c_str()); 
    //    valueStr = "1";
    ///    WritePrivateProfileString(iniSettings.c_str(), "DrawFakeCursor", valueStr.c_str(), iniPath.c_str());
    //    WritePrivateProfileString(iniSettings.c_str(), "UseRealMouse", valueStr.c_str(), iniPath.c_str());
    //    iniSettings = "Hooks";
    //    WritePrivateProfileString(iniSettings.c_str(), "ClipCursorHook", valueStr.c_str(), iniPath.c_str());
   //     WritePrivateProfileString(iniSettings.c_str(), "GetKeystateHook", valueStr.c_str(), iniPath.c_str());
    //    WritePrivateProfileString(iniSettings.c_str(), "GetAsynckeystateHook", valueStr.c_str(), iniPath.c_str());
    //    WritePrivateProfileString(iniSettings.c_str(), "GetCursorposHook", valueStr.c_str(), iniPath.c_str());

    //CreateThread(nullptr, 0,
    //    (LPTHREAD_START_ROUTINE)Drawthread, g_hModule, 0, 0); //GetModuleHandle(0)

    hwnd = GetMainWindowHandle(GetCurrentProcessId());
    int mode = InitialMode;
    int numphotoA = -1;
    int numphotoB = -1;
    int numphotoX = -1;
    int numphotoY = -1;
    int numphotoC = -1;
    int numphotoD = -1;
    int numphotoE = -1;
    int numphotoF = -1;

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
    while (numphotoC == -1 && x < 50)
    {
        std::string path = UGetExecutableFolder() + "\\C" + std::to_string(x) + ".bmp";
        std::wstring wpath(path.begin(), path.end());
        if (HBITMAP hbm = (HBITMAP)LoadImageW(NULL, wpath.c_str(), IMAGE_BITMAP, 0, 0, LR_LOADFROMFILE | LR_CREATEDIBSECTION))
        {
            x++;
            DeleteObject(hbm);
        }


        else
            numphotoC = x;

        Sleep(2);
    }
    while (numphotoD == -1 && x < 50)
    {
        std::string path = UGetExecutableFolder() + "\\D" + std::to_string(x) + ".bmp";
        std::wstring wpath(path.begin(), path.end());
        if (HBITMAP hbm = (HBITMAP)LoadImageW(NULL, wpath.c_str(), IMAGE_BITMAP, 0, 0, LR_LOADFROMFILE | LR_CREATEDIBSECTION))
        {
            x++;
            DeleteObject(hbm);
        }


        else
            numphotoD = x;

        Sleep(2);
    }
    while (numphotoE == -1 && x < 50)
    {
        std::string path = UGetExecutableFolder() + "\\E" + std::to_string(x) + ".bmp";
        std::wstring wpath(path.begin(), path.end());
        if (HBITMAP hbm = (HBITMAP)LoadImageW(NULL, wpath.c_str(), IMAGE_BITMAP, 0, 0, LR_LOADFROMFILE | LR_CREATEDIBSECTION))
        {
            x++;
            DeleteObject(hbm);
        }


        else
            numphotoE = x;

        Sleep(2);
    }
    while (numphotoF == -1 && x < 50)
    {
        std::string path = UGetExecutableFolder() + "\\F" + std::to_string(x) + ".bmp";
        std::wstring wpath(path.begin(), path.end());
        if (HBITMAP hbm = (HBITMAP)LoadImageW(NULL, wpath.c_str(), IMAGE_BITMAP, 0, 0, LR_LOADFROMFILE | LR_CREATEDIBSECTION))
        {
            x++;
            DeleteObject(hbm);
        }


        else
            numphotoF = x;

        Sleep(2);
    }
    ///////////////////////////////////////////////////////////////////////////////////LLLLLLLOOOOOOOOOOOOOPPPPPPPPPPPPP
    bool Aprev = false;
        
    while (loop == true)
    {
        bool movedmouse = false; //reset
        keystatesend = 0; //reset keystate
        int calcsleep = 0;
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
                    startsearch = startsearchY;
                    Buttonaction("\\Y", mode, numphotoY, startsearch);
                    if (mode == 2)
                    {
                        numphotoY++;
                    }
                }
                if (buttons & XINPUT_GAMEPAD_RIGHT_SHOULDER)
                {
                    startsearch = startsearchC;
                    Buttonaction("\\C", mode, numphotoC, startsearch);
                    if (mode == 2)
                    {
                        numphotoC++;
                    }
                }
                if (buttons & XINPUT_GAMEPAD_LEFT_SHOULDER)
                {
                    startsearch = startsearchD;
                    Buttonaction("\\D", mode, numphotoD, startsearch);
                    if (mode == 2)
                    {
                        numphotoD++;
                    }
                }
                if (buttons & XINPUT_GAMEPAD_RIGHT_THUMB)
                {
                    startsearch = startsearchE;
                    Buttonaction("\\E", mode, numphotoE, startsearch);
                    if (mode == 2)
                    {
                        numphotoE++;
                    }
                }
                if (buttons & XINPUT_GAMEPAD_LEFT_THUMB)
                {
                    startsearch = startsearchF;
                    Buttonaction("\\F", mode, numphotoF, startsearch);
                    if (mode == 2)
                    {
                        numphotoF++;
                    }
                }

                if (buttons & XINPUT_GAMEPAD_DPAD_UP)
                {

                    scroll.x = rect.left + (rect.right - rect.left) / 2;
                    if (scrolloutsidewindow == 0)
                        scroll.y = rect.top + 1;
                    else
                        scroll.y = rect.top - 1;
                    scrollmap = true;
                }
               else if (buttons & XINPUT_GAMEPAD_DPAD_DOWN)
                {
                    
                    scroll.x = rect.left + (rect.right - rect.left) / 2;
                    if (scrolloutsidewindow == 0)
                        scroll.y = rect.bottom - 1;
                    else
                        scroll.y = rect.bottom + 1;
                    scrollmap = true;
                }
                else if (buttons & XINPUT_GAMEPAD_DPAD_LEFT)
                {
                    if (scrolloutsidewindow == 0)
                        scroll.x = rect.left + 1;
                    else 
                        scroll.x = rect.left - 1;
                    scroll.y = rect.top + (rect.bottom - rect.top) / 2;

                    scrollmap = true;

                }
                else if (buttons & XINPUT_GAMEPAD_DPAD_RIGHT)
                {

                    if (scrolloutsidewindow == 0)
                        scroll.x = rect.right - 1;
                    else
                        scroll.x = rect.right + 1;
                    scroll.y = rect.top + (rect.bottom - rect.top) / 2;

                    scrollmap = true;
                }
                else
                {
                    scrollmap = false;
                }




                if (buttons & XINPUT_GAMEPAD_START && showmessage == 0)
                {

                    if (mode == 0 && Modechange == 1)
                    {
                        mode = 1;

                       // MessageBox(NULL, "Bmp + Emulated cursor mode", "Move the flickering red dot and use right trigger for left click. left trigger for right click", MB_OK | MB_ICONINFORMATION);
                        showmessage = 2;
                    }
                    else if (mode == 1 && Modechange == 1)
                    {
                        mode = 2;
                        //MessageBox(NULL, "Edit Mode", "Button mapping. will map buttons you click with the flickering red dot as an input coordinate", MB_OK | MB_ICONINFORMATION);
                        showmessage = 3;
                        

                    }
                    else if (mode == 2 && Modechange == 1)
                    {
                       // mode = 0;
                       // MessageBox(NULL, "Bmp mode", "only send input on bmp match", MB_OK | MB_ICONINFORMATION);
                        showmessage = 1;
                    }
                    else { //assume modechange not allowed. send escape key instead
                        keystatesend = VK_ESCAPE;
                    }
                   // Sleep(1000); //have time to release button. this is no hurry anyway
                    
                }
                if (mode > 0 )
                { 
                    //fake cursor poll
                    int Xaxis = 0;
                    int Yaxis = 0;
                    int width = rect.right - rect.left;
                    int height = rect.bottom - rect.top;
                    
					 
                    if (righthanded == 1) {
                        Xaxis = state.Gamepad.sThumbRX;
                        Yaxis = state.Gamepad.sThumbRY;
                    }
                    else
                    {
                        Xaxis = state.Gamepad.sThumbLX;
                        Yaxis = state.Gamepad.sThumbLY;
                    }


                    //sleeptime adjust. slow movement on low axis
                    if (Xaxis < 0) //negative
                    {
                        if (Xaxis < -22000) //-7849
                            calcsleep = 3;
                        else if (Xaxis < -15000)
                            calcsleep = 2;
                        else if (Xaxis < -10000)
                            calcsleep = 1;
                        else calcsleep = 0;
                    }
                    else if (Xaxis > 0) //positive
                    {
                        if (Xaxis > 24000) //12000
                            calcsleep = 3;
                        else if (Xaxis > 18000)
                            calcsleep = 2;
                        else if (Xaxis > 14000)
                            calcsleep = 1;
                        else calcsleep = 0;
					}   
                    if (Yaxis < 0) //negative
                    {
                        if (Yaxis < -24000) //-16000
                            calcsleep = 3;
                        else if (Yaxis < -22000)
                            calcsleep = 2;
                        else if (Yaxis < -17000)
                            calcsleep = 1;
                        else calcsleep = 0;
                    }
                    else if (Yaxis > 0) //positive
                    {
                        if (Yaxis > 24000) //0
                            calcsleep = 3;
                        else if (Yaxis > 20000)
                            calcsleep = 2;
                        else if (Yaxis > 16000)
                            calcsleep = 1;
                        else calcsleep = 0;
                    }






                    OldX = X;
                    if (Xaxis < AxisLeftsens) //strange values. but tested many before choosing this
                    { 
                        if (Xaxis < -25000)
                            Xaxis = -25000;
                        if (X >= (std::abs(Xaxis) / 2000) + 14)
                        { 
                            sovetid = sens - (std::abs(Xaxis) / 450);
                        X = X - (std::abs(Xaxis) / 1500) + 4;
                        movedmouse = true;
                        }
                    }
                    else if (Xaxis > AxisRightsens) //strange values. but tested many before choosing this
                    {
                        if (Xaxis > 25000)
                            Xaxis = 25000;
                        if (X <= width - (Xaxis / 2000) - 16)
                        {
                            sovetid = sens - (Xaxis / 450);
                            X = X + (Xaxis / 1500) - 6;
                            movedmouse = true;
                        }
                    }
                    int accumulater = std::abs(Xaxis) + std::abs(Yaxis);
                     
                    /////////////////////
                    if (Yaxis > AxisUpsens) //strange values. but tested many before choosing this
                    {
                        if (Yaxis > 25000)
                            Yaxis = 25000;
                        if (Y >= (std::abs(Yaxis) / 2000) + 18)
                        {
                            sovetid = sens - (std::abs(Yaxis) / 450);
                            Y = Y - (std::abs(Yaxis) / 2000) - 1;
                            movedmouse = true;
                        }
                    }
                    else  if (Yaxis < AxisDownsens) //strange values. but tested many before choosing this
                    { //my controller is not calibrated maybe
                        if (Yaxis < -25000)
                            Yaxis = -25000;
                        if (Y <= height - (std::abs(Yaxis) / 2000) - 18)
                        {
                            sovetid = sens - (std::abs(Yaxis) / 450); // Loop poll rate
                            Y = Y + (std::abs(Yaxis) / 1700) - 6; // Y movement rate
                            movedmouse = true;
                        }
                    }
                    if (movedmouse == true) //fake cursor move message
                    {
                        pausedraw = true;
                        if (userealmouse == 0) 
                        {
                            SendMouseClick(fakecursor.x, fakecursor.y, 8, 1);
                        }
                       // movedmouse = false;
                        Sleep(1);
                        pausedraw = false;
                    }
                    int nysovetid = sens2 - (accumulater / 700);
                    if (nysovetid < sovetid)
                        sovetid = nysovetid;
                    if (sovetid < 3) 
                        sovetid = 3; //speedlimit
                    if (drawfakecursor == 1)
                        CaptureWindow24Bit(hwnd, screenSize, largePixels, strideLarge, true); //draw fake cursor
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
                        }
                        else
                        { 
                           ClientToScreen(hwnd, &startdrag);
                           ClientToScreen(hwnd, &fakecursor);
                           SendMouseClick(startdrag.x, startdrag.y, 6, 2); //4 4 move //5 release
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
        if (mode > 0) {
           // Sleep(sovetid); //15-80 //ini value
            if (movedmouse == true)
                Sleep(5 - calcsleep); //max 3. 0-2 on slow movement
            else Sleep(2); //max 3. 0-2 on slow movement
        }
        if (showmessage != 0)
        {
            counter++;
            if (counter > 500)
            {
                if (showmessage == 1) {
                    mode = 0;
                }
                showmessage = 0;
                counter = 0;

            }
        }

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

    CreateThread(nullptr, 0,
        (LPTHREAD_START_ROUTINE)ThreadFunction, g_hModule, 0, 0); //GetModuleHandle(0)



    if (getcursorposhook == 1) {
        MH_CreateHookApi(L"user32", "GetCursorPos", &MyGetCursorPos, reinterpret_cast<LPVOID*>(&fpGetCursorPos));
        MH_EnableHook(&GetCursorPos);
        MH_CreateHookApi(L"user32", "SetCursorPos", &MySetCursorPos, reinterpret_cast<LPVOID*>(&fpSetCursorPos));
        MH_EnableHook(&SetCursorPos);
    }
    if (getkeystatehook == 1) {
        MH_CreateHook(&GetAsyncKeyState, &HookedGetAsyncKeyState, reinterpret_cast<LPVOID*>(&originalGetAsyncKeyState));
        MH_EnableHook(&GetAsyncKeyState);
    }

    if (getasynckeystatehook == 1) {
        MH_CreateHook(&GetKeyState, &HookedGetKeyState, reinterpret_cast<LPVOID*>(&originalGetKeyState));
        MH_EnableHook(&GetKeyState);
    }

    if (clipcursorhook == 1) {
        MH_CreateHook(&ClipCursor, &HookedClipCursor, reinterpret_cast<LPVOID*>(&originalClipCursor));
        MH_EnableHook(&ClipCursor);
    }
    if (setcursorhook == 1)
    MH_CreateHookApi(
        L"user32", "SetCursor",
        &HookedSetCursor,
        reinterpret_cast<LPVOID*>(&originalSetCursor)
    );

    MH_EnableHook(&SetCursor);

     //MH_EnableHook(MH_ALL_HOOKS);
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved) 
{
    switch (ul_reason_for_call) 
    {
        case DLL_PROCESS_ATTACH:
        {

            //supposed to help againt crashes?
            WaitForInputIdle(GetCurrentProcess(), 1000); //wait for input to be ready
            DisableThreadLibraryCalls(hModule);
            g_hModule = hModule;

            std::string iniPath = UGetExecutableFolder() + "\\Xinput.ini";
            std::string iniSettings = "Hooks";
            // std::string controllerID = getIniString(iniSettings.c_str(), "Controller ID", "0", iniPath);

             //hook settings
            clipcursorhook = GetPrivateProfileInt(iniSettings.c_str(), "ClipCursorHook", 1, iniPath.c_str());
            getkeystatehook = GetPrivateProfileInt(iniSettings.c_str(), "GetKeystateHook", 1, iniPath.c_str());
            getasynckeystatehook = GetPrivateProfileInt(iniSettings.c_str(), "GetAsynckeystateHook", 1, iniPath.c_str());
            getcursorposhook = GetPrivateProfileInt(iniSettings.c_str(), "GetCursorposHook", 1, iniPath.c_str());
            setcursorhook = GetPrivateProfileInt(iniSettings.c_str(), "SetCursorHook", 1, iniPath.c_str());
            SetupHook();
            break;
        }
        case DLL_PROCESS_DETACH:
        {
            RemoveHook();
            exit(0);
            break;
        }
    }
    return true;
}