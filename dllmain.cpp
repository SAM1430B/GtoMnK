// dllmain.cpp : Defines the entry point for the DLL application.


#include "pch.h"
#include <cmath>
#define NOMINMAX
#include <windows.h>
#include <algorithm>
//#include <windowsx.h>
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

typedef BOOL(WINAPI* GetCursorPos_t)(LPPOINT lpPoint);
typedef BOOL(WINAPI* SetCursorPos_t)(int X, int Y);

typedef SHORT(WINAPI* GetAsyncKeyState_t)(int vKey);
typedef SHORT(WINAPI* GetKeyState_t)(int nVirtKey);
typedef BOOL(WINAPI* ClipCursor_t)(const RECT*);
typedef HCURSOR(WINAPI* SetCursor_t)(HCURSOR hCursor);

typedef BOOL(WINAPI* SetRect_t)(LPRECT lprc, int xLeft, int yTop, int xRight, int yBottom);
typedef BOOL(WINAPI* AdjustWindowRect_t)(LPRECT lprc, DWORD  dwStyle, BOOL bMenu);



CRITICAL_SECTION critical; //window thread
//CRITICAL_SECTION criticalA; //Scannning thread

GetCursorPos_t fpGetCursorPos = nullptr;
GetCursorPos_t fpSetCursorPos = nullptr;
GetAsyncKeyState_t fpGetAsyncKeyState = nullptr;
GetKeyState_t fpGetKeyState = nullptr;
ClipCursor_t fpClipCursor = nullptr;
SetCursor_t fpSetCursor = nullptr;
SetRect_t fpSetRect = nullptr;
AdjustWindowRect_t fpAdjustWindowRect = nullptr;



//POINT fakecursor;
POINT fakecursorW;
POINT startdrag;
POINT activatewindow;
POINT scroll;
bool loop = true;
HWND hwnd;
int showmessage = 0; //0 = no message, 1 = initializing, 2 = bmp mode, 3 = bmp and cursor mode, 4 = edit mode   
int showmessageW = 0; //0 = no message, 1 = initializing, 2 = bmp mode, 3 = bmp and cursor mode, 4 = edit mode 
int counter = 0;

//syncronization control
HANDLE hMutex;

int getmouseonkey = 0;
int message = 0;
auto hInstance = nullptr;

//hooks
bool hooksinited = false;   
int keystatesend = 0; //key to send
int clipcursorhook = 0;
int getkeystatehook = 0;
int getasynckeystatehook = 0;
int getcursorposhook = 0;
int setcursorposhook = 0;
int setcursorhook = 0;

int ignorerect = 0;
POINT rectignore = { 0,0 }; //for getcursorposhook
int setrecthook = 0;

int leftrect = 0;
int toprect = 0;
int rightrect = 0;
int bottomrect = 0;

int userealmouse = 0;

int numphotoA = -1;
int numphotoB = -1;
int numphotoX = -1;
int numphotoY = -1;
int numphotoC = -1;
int numphotoD = -1;
int numphotoE = -1;
int numphotoF = -1;

//fake cursor
int controllerID = 0;
int Xf = 20;
int Yf = 20;
int OldX = 0;
int OldY = 0;
int ydrag;
int xdrag;
int Xoffset = 0; //offset for cursor    
int Yoffset = 0;
bool scrollmap = false;
bool pausedraw = false;
bool gotcursoryet = false;
int drawfakecursor = 0;
int alwaysdrawcursor = 0; //always draw cursor even if setcursor set cursor NULL
HICON hCursor = 0;
DWORD lastClickTime;

//bmp search
bool foundit = false;


//scroll type 3
int tick = 0;
bool doscrollyes = false;   

// 
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


bool onoroff = true;

//remember old keystates
int oldscrollrightaxis = false; //reset 
int oldscrollleftaxis = false; //reset 
int oldscrollupaxis = false; //reset 
int oldscrolldownaxis = false; //reset 
bool Apressed = false;
bool Bpressed = false;
bool Xpressed = false;
bool Ypressed = false;
bool leftPressedold;
bool rightPressedold;
bool oldA = false; 
bool oldB = false;
bool oldX = false;
bool oldY = false;
bool oldC = false;
bool oldD = false;
bool oldE = false;
bool oldF = false;
bool oldup = false;
bool olddown = false;
bool oldleft = false;
bool oldright = false;

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
int scanoption = 0;

int Atype = 0;
int Btype = 0;
int Xtype = 0;
int Ytype = 0;
int Ctype = 0;
int Dtype = 0;
int Etype = 0;
int Ftype = 0;

POINT PointA;
POINT PointB;
POINT PointX;
POINT PointY;
int scantick = 0;

int bmpAtype = 0;
int bmpBtype = 0;
int bmpXtype = 0;
int bmpYtype = 0;
int bmpCtype = 0;
int bmpDtype = 0;
int bmpEtype = 0;
int bmpFtype = 0;

int uptype = 0;
int downtype = 0;
int lefttype = 0;
int righttype = 0;


int x = 0;

HBITMAP hbm;

std::vector<BYTE> largePixels, smallPixels;
SIZE screenSize;
int strideLarge, strideSmall;
int smallW, smallH;

int mode = 0;
//int sovetid = 16;
int knappsovetid = 100;

int samekey = 0;
int samekeyA = 0;

HCURSOR WINAPI HookedSetCursor(HCURSOR hcursor) {
	    EnterCriticalSection(&critical);
		hCursor = hcursor; // Store the cursor handle  

        hcursor = fpSetCursor(hcursor);
		LeaveCriticalSection(&critical);
        return hcursor;
}




////SetRect_t)(LPRECT lprc, int xLeft, int yTop, int xRight, int yBottom);
BOOL WINAPI HookedSetRect(LPRECT lprc, int xLeft, int yTop, int xRight, int yBottom) {
	xLeft = leftrect; // Set the left coordinate to Xrect  
    yTop = toprect; // Set the top coordinate to Yrect  
    
    xRight = rightrect; // Set the right coordinate to Xrect + 10 
	yBottom = bottomrect; // Set the bottom coordinate to Yrect + 10    
 
    
	bool result = fpSetRect(lprc, xLeft, yTop, xRight, yBottom);
    return result;
}

BOOL WINAPI HookedAdjustWindowRect(LPRECT lprc, DWORD  dwStyle, BOOL bMenu) {
    lprc->top = toprect; // Set the left coordinate to Xrect  
    lprc->bottom = bottomrect; // Set the left coordinate to Xrect  
    lprc->left = leftrect; // Set the left coordinate to Xrect  
    lprc->right = rightrect; // Set the left coordinate to Xrect  

    bool result = fpAdjustWindowRect(lprc, dwStyle, bMenu);
    return result;
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
        SHORT result = fpGetAsyncKeyState(vKey);
        return result;
    }
}

// Hooked GetKeyState
SHORT WINAPI HookedGetKeyState(int nVirtKey) {
    if (samekey == nVirtKey) {
        return 8001;
        //8001 on hold key. but this may not work
    }
    else samekey = 0;

    if (nVirtKey == keystatesend)
    {
        samekey = nVirtKey;
        return 8000; //8001 ?
    }
    else
    {
        SHORT result = fpGetKeyState(nVirtKey);
        return result;
    }
}

BOOL WINAPI MyGetCursorPos(PPOINT lpPoint) {
    if (lpPoint) 
    {
        POINT mpos;
        if (scrollmap == false)
        { 

            if (ignorerect == 1) {
                mpos.x = Xf + rectignore.x; //hwnd coordinates 0-800 on a 800x600 window
                mpos.y = Yf + rectignore.y;//hwnd coordinate s0-600 on a 800x600 window
				lpPoint->x = mpos.x; 
				lpPoint->y = mpos.y;    
            }
            else {
                mpos.x = Xf; //hwnd coordinates 0-800 on a 800x600 window
                mpos.y = Yf;//hwnd coordinate s0-600 on a 800x600 window
                ClientToScreen(hwnd, &mpos);
		
                lpPoint->x = mpos.x; //desktop coordinates
                lpPoint->y = mpos.y;
                ScreenToClient(hwnd, &mpos); //revert so i am sure its done
            }
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
POINT mpos;
BOOL WINAPI MySetCursorPos(int X, int Y) {
    POINT point;
    point.x = X;
    point.y = Y;
    char buffer[256];
    

    ScreenToClient(hwnd, &point);
    sprintf_s(buffer, "X: %d Y: %d", point.x, point.y);
    Xf = point.x; // Update the global X coordinate
    Yf = point.y; // Update the global Y coordinate

	//MessageBoxA(NULL, buffer, "Info", MB_OK | MB_ICONINFORMATION);
   // movedmouse = true;
    //crash fixme!
  //  Sleep(20);
    return true; //fpSetCursorPos(lpPoint); // Call the original SetCursorPos function
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

void SetupHook() {
    if (MH_Initialize() != MH_OK) {
        MessageBox(NULL, "Failed to initialize MinHook", "Error", MB_OK | MB_ICONERROR);
        return;
    }

    //each of there hooks have a high chance of crashing the game

    if (getcursorposhook == 1) {
        MH_CreateHookApi(L"user32", "GetCursorPos", &MyGetCursorPos, reinterpret_cast<LPVOID*>(&fpGetCursorPos));
        MH_EnableHook(&GetCursorPos);
    }
    if (setcursorposhook == 1) {
        MH_CreateHook(&SetCursorPos, &MySetCursorPos, reinterpret_cast<LPVOID*>(&fpSetCursorPos));
        MH_EnableHook(&SetCursorPos);
    }
    if (getkeystatehook == 1) {
        MH_CreateHook(&GetAsyncKeyState, &HookedGetAsyncKeyState, reinterpret_cast<LPVOID*>(&fpGetAsyncKeyState));
        MH_EnableHook(&GetAsyncKeyState);
    }
    if (getasynckeystatehook == 1) {
        MH_CreateHook(&GetKeyState, &HookedGetKeyState, reinterpret_cast<LPVOID*>(&fpGetKeyState));
        MH_EnableHook(&GetKeyState);
    }
    if (clipcursorhook == 1) {
        MH_CreateHook(&ClipCursor, &HookedClipCursor, reinterpret_cast<LPVOID*>(&fpClipCursor));
        MH_EnableHook(&ClipCursor);
    }
    if (setrecthook == 1) {
        MH_CreateHook(&SetRect, &HookedSetRect, reinterpret_cast<LPVOID*>(&fpSetRect));
        MH_EnableHook(&SetRect);

        MH_CreateHook(&AdjustWindowRect, &HookedAdjustWindowRect, reinterpret_cast<LPVOID*>(&fpAdjustWindowRect));
        MH_EnableHook(&AdjustWindowRect);
    }
    if (setcursorhook == 1)
    {
        MH_CreateHook(&SetCursor, &HookedSetCursor, reinterpret_cast<LPVOID*>(&fpSetCursor));
        MH_EnableHook(&SetCursor);
    }
    //MessageBox(NULL, "Bmp + last setcursor. done", "other search", MB_OK | MB_ICONINFORMATION);
    hooksinited = true;
    //MH_EnableHook(MH_ALL_HOOKS);
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
        //
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
        if (z == 4)
        {
            PostMessage(hwnd, WM_LBUTTONUP, 0, clickPos);

        }
        if (z == 5) {
            PostMessage(hwnd, WM_RBUTTONDOWN, MK_RBUTTON, clickPos);
            keystatesend = VK_RIGHT;
        }
        if (z == 6)
        {
            PostMessage(hwnd, WM_RBUTTONUP, 0, clickPos);

        }
        if (z == 20 || z == 21) //WM_mousewheel need desktop coordinates
        {

            ClientToScreen(hwnd, &heer);
            LPARAM clickPos = MAKELPARAM(heer.x, heer.y);
            WPARAM wParam = 0;
            if (z == 20)
                wParam = MAKEWPARAM(0, -120); 
            if (z == 21)
                wParam = MAKEWPARAM(0, 120);
            PostMessage(hwnd, WM_MOUSEWHEEL, wParam, clickPos);
        }
        if (z == 30) //WM_LBUTTONDBLCLK
        {
            PostMessage(hwnd, WM_LBUTTONDBLCLK, 0, clickPos);
        }
        else if (z == 8 || z == 10 || z == 11) //only mousemove
        {
            PostMessage(hwnd, WM_MOUSEMOVE, 0, clickPos);
            //PostMessage(hwnd, WM_SETCURSOR, (WPARAM)hwnd, MAKELPARAM(HTCLIENT, WM_MOUSEMOVE));

        }
        return true;
    }
    else if (z == 1 || z == 2 || z == 3 || z == 6 || z == 10)
    {
       // ClientToScreen(hwnd, &heer); //need desktop
        //will fix later
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

bool IsTriggerPressed(BYTE triggerValue) {
    BYTE threshold = 175;
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

   
    
    HBITMAP hbm24  = CreateDIBSection(hdcWindow, &bmi, DIB_RGB_COLORS, (void**)&pBits, NULL, 0);
    if (hbm24 != 0)   
    { 
   
        HGDIOBJ oldBmp = SelectObject(hdcMem, hbm24);
 
        // Copy window contents to memory DC
        BitBlt(hdcMem, 0, 0, width, height, hdcWindow, 0, 0, SRCCOPY);

        if (draw) {
                    RECT rect = { 0, 0, 32, 32 }; //need bmp width height
                    FillRect(hdcMem, &rect, (HBRUSH)(COLOR_WINDOW + 1));

                    if (showmessage == 1)
                    {
                        TextOut(hdcWindow, Xf, Yf, TEXT("BMP MODE"), 8);
                        TextOut(hdcWindow, Xf, Yf + 17, TEXT("only mapping searches"), 21);
                    }
                    else if (showmessage == 2)
                    {
                        TextOut(hdcWindow, Xf, Yf, TEXT("CURSOR MODE"), 11);
                        TextOut(hdcWindow, Xf, Yf + 17, TEXT("mapping searches + cursor"), 25);
                    }
                    else if (showmessage == 3)
                    {
                        TextOut(hdcWindow, Xf, Yf, TEXT("EDIT MODE"), 9);
                        TextOut(hdcWindow, Xf, Yf + 15, TEXT("tap a button to bind it to coordinate"), 37);
                        TextOut(hdcWindow, Xf, Yf + 30, TEXT("A,B,X,Y,R2,R3,L2,L3 can be mapped"), 32);
                    }
                    else if (showmessage == 10)
                    {
                        TextOut(hdcWindow, Xf, Yf, TEXT("BUTTON MAPPED"), 13);
                    }
                    else if (showmessage == 11)
                    {
                        TextOut(hdcWindow, Xf, Yf, TEXT("WAIT FOR MESSAGE EXPIRE!"), 24);
                    }
                    else if (showmessage == 12)
                    {
                        TextOut(hdcWindow, 20, 20, TEXT("DISCONNECTED!"), 14); //14
                    }
                    else if (showmessage == 69)
                    {
                        TextOut(hdcWindow, Xf, Yf, TEXT("SHUTTING DOWN"), 13);
                    }
                    else if (showmessage == 70)
                    {
                        TextOut(hdcWindow, Xf, Yf, TEXT("STARTING!"), 10);
                    }
                    else if (hCursor != 0 && onoroff == true)
                    { 
                        gotcursoryet = true;
                        if (Xf - Xoffset < 0 || Yf - Yoffset < 0)
                            DrawIconEx(hdcWindow, 0 + Xf, 0 + Yf, hCursor, 32, 32, 0, NULL, DI_NORMAL);//need bmp width height
                        else 
							DrawIconEx(hdcWindow, Xf - Xoffset, Yf - Yoffset, hCursor, 32, 32, 0, NULL, DI_NORMAL);//need bmp width height
                         
                    }
                    else if ( onoroff == true && (alwaysdrawcursor == 1 || gotcursoryet == false))
                    {
                        for (int y = 0; y < 20; y++)
                        {
                            for (int x = 0; x < 20; x++)
                            {
                                int val = colorfulSword[y][x];
                                if (val != 0)
                                {
                                    HBRUSH hBrush = CreateSolidBrush(colors[val]);
                                    RECT rect = { Xf + x , Yf + y , Xf + x + 1, Yf + y + 1 };
                                    FillRect(hdcWindow, &rect, hBrush);
                                    DeleteObject(hBrush);
                                }
                            }
                        }
                    }


                GetDIBits(hdcMem, hbm24, 0, height, pixels.data(), &bmi, DIB_RGB_COLORS);
                SelectObject(hdcMem, oldBmp);;
                if (hdcMem) DeleteDC(hdcMem);
                if (hdcWindow) ReleaseDC(hwnd, hdcWindow);
                if (hbm24) DeleteObject(hbm24);

                return 0;
            }
        
        // Copy bits out
        GetDIBits(hdcMem, hbm24, 0, height, pixels.data(), &bmi, DIB_RGB_COLORS);
        SelectObject(hdcMem, oldBmp);
        if (hdcMem) DeleteDC(hdcMem);
        if (hdcWindow) ReleaseDC(hwnd, hdcWindow);
        if (hbm24) DeleteObject(hbm24);
        return hbm24 ? hbm24 : 0;
        } //hbm24 not null
	return 0; // Failed to create bitmap    
    } //function end
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
// #define DEADZONE 8000
// #define MAX_SPEED 30.0f        // Maximum pixels per poll
// #define ACCELERATION 2.0f      // Controls non-linear ramp (higher = more acceleration)
float radial_deadzone = 0.10f; // Circular/Radial Deadzone (0.0 to 0.3)
float axial_deadzone = 0.00f; // Square/Axial Deadzone (0.0 to 0.3)
const float max_threshold = 0.03f; // Max Input Threshold, an "outer deadzone" (0.0 to 0.15)
const float curve_slope = 0.16f; // The linear portion of the response curve (0.0 to 1.0)
const float curve_exponent = 5.00f; // The exponential portion of the curve (1.0 to 10.0)
float sensitivity = 20.00f; // Base sensitivity / max speed (1.0 to 30.0)
float accel_multiplier = 1.90f; // Look Acceleration Multiplier (1.0 to 3.0)

POINT CalculateUltimateCursorMove(
    SHORT stickX, SHORT stickY,
    float c_deadzone,
    float s_deadzone,
    float max_threshold,
    float curve_slope,
    float curve_exponent,
    float sensitivity,
    float accel_multiplier
) {
    static double mouseDeltaAccumulatorX = 0.0;
    static double mouseDeltaAccumulatorY = 0.0;

    double normX = static_cast<double>(stickX) / 32767.0;
    double normY = static_cast<double>(stickY) / 32767.0;

    double magnitude = std::sqrt(normX * normX + normY * normY);
    if (magnitude < c_deadzone) {
        return { 0, 0 }; // Inside circular deadzone
    }
    if (std::abs(normX) < s_deadzone) {
        normX = 0.0; // Inside axial deadzone for X
    }
    if (std::abs(normY) < s_deadzone) {
        normY = 0.0; // Inside axial deadzone for Y
    }
    magnitude = std::sqrt(normX * normX + normY * normY);
    if (magnitude < 1e-6) {
        return { 0, 0 };
    }

    double effectiveRange = 1.0 - max_threshold - c_deadzone;
    if (effectiveRange < 1e-6) effectiveRange = 1.0;

    double remappedMagnitude = (magnitude - c_deadzone) / effectiveRange;
    remappedMagnitude = (std::max)(0.0, (std::min)(1.0, remappedMagnitude));

    double curvedMagnitude = curve_slope * remappedMagnitude + (1.0 - curve_slope) * std::pow(remappedMagnitude, curve_exponent);

    double finalSpeed = sensitivity * accel_multiplier;

    double dirX = normX / magnitude;
    double dirY = normY / magnitude;
    double finalMouseDeltaX = dirX * curvedMagnitude * finalSpeed;
    double finalMouseDeltaY = dirY * curvedMagnitude * finalSpeed;

    mouseDeltaAccumulatorX += finalMouseDeltaX;
    mouseDeltaAccumulatorY += finalMouseDeltaY;
    LONG integerDeltaX = static_cast<LONG>(mouseDeltaAccumulatorX);
    LONG integerDeltaY = static_cast<LONG>(mouseDeltaAccumulatorY);

    mouseDeltaAccumulatorX -= integerDeltaX;
    mouseDeltaAccumulatorY -= integerDeltaY;

    return { integerDeltaX, -integerDeltaY };
}

void PostKeyFunction(HWND hwnd, int keytype, bool press) {
    DWORD mykey = 0;
	DWORD presskey = WM_KEYDOWN;

    UINT scanCode = MapVirtualKey(VK_LEFT, MAPVK_VK_TO_VSC);
    LPARAM lParam = (0x00000001 | (scanCode << 16));
    
    if (!press){
		presskey = WM_KEYUP; // Key up event 
    }

    //standard keys for dpad
    if (keytype == -1)
        mykey = VK_UP;
    if (keytype == -2)
        mykey = VK_DOWN;
    if (keytype == -3)
        mykey = VK_LEFT;
    if (keytype == -4)
        mykey = VK_RIGHT;

    if (keytype == 3)
        mykey = VK_ESCAPE;
    if (keytype == 4)
        mykey = VK_RETURN;
    if (keytype == 5)
        mykey = VK_TAB;
    if (keytype == 6)
        mykey = VK_SHIFT;
    if (keytype == 7)
        mykey = VK_CONTROL;
    if (keytype == 8)
        mykey = VK_SPACE;

    if (keytype == 9)
        mykey = 0x4D; //M

    if (keytype == 10)
        mykey = 0x57; //W

    if (keytype == 11)
        mykey = 0x53; //S

    if (keytype == 12)
        mykey = 0x41; //A

    if (keytype == 13)
        mykey = 0x44; //D

    if (keytype == 14)
        mykey = 0x45; //E

    if (keytype == 15)
        mykey = 0x46; //F

    if (keytype == 16)
        mykey = 0x47; //G

    if (keytype == 17)
        mykey = 0x48; //H

    if (keytype == 18)
        mykey = 0x49; //I

    if (keytype == 19)
        mykey = 0x51; //Q

    if (keytype == 20)
        mykey = VK_OEM_PERIOD;

    if (keytype == 21)
        mykey = 0x52; //R

    if (keytype == 22)
        mykey = 0x54; //T

    if (keytype == 23)
        mykey = 0x42; //B

    if (keytype == 24)
        mykey = 0x43; //C

    if (keytype == 25)
        mykey = 0x4B; //K

    if (keytype == 26)
        mykey = 0x55; //U

    if (keytype == 27)
        mykey = 0x56; //V

    if (keytype == 28)
        mykey = 0x57; //W

    if (keytype == 30)
        mykey = 0x30; //0

    if (keytype == 31)
        mykey = 0x31; //1

    if (keytype == 32)
        mykey = 0x32; //2

    if (keytype == 33)
        mykey = 0x33; //3

    if (keytype == 34)
        mykey = 0x34; //4

    if (keytype == 35)
        mykey = 0x35; //5

    if (keytype == 36)
        mykey = 0x36; //6

    if (keytype == 37)
        mykey = 0x37; //7

    if (keytype == 38)
        mykey = 0x38; //8

    if (keytype == 39)
        mykey = 0x39; //9

    if (keytype == 40)
        mykey = VK_UP; 

    if (keytype == 41)
        mykey = VK_DOWN; 

    if (keytype == 42)
        mykey = VK_LEFT; 

    if (keytype == 43)
        mykey = VK_RIGHT; 

    if (keytype == 44)
        mykey = 0x58; //X

    if (keytype == 45)
        mykey = 0x5A; //Z

    if (keytype == 20)
        mykey = VK_OEM_PERIOD;



    if (keytype == 51)
        mykey = VK_F1;

    if (keytype == 52)
        mykey = VK_F2;

    if (keytype == 53)
        mykey = VK_F3;

    if (keytype == 54)
        mykey = VK_F4;

    if (keytype == 55)
        mykey = VK_F5;

    if (keytype == 56)
        mykey = VK_F6;

    if (keytype == 57)
        mykey = VK_F7;

    if (keytype == 58)
        mykey = VK_F8;
    if (keytype == 59)
        mykey = VK_F9;

    if (keytype == 60)
        mykey = VK_F10;

    if (keytype == 61)
        mykey = VK_F11;

    if (keytype == 62)
        mykey = VK_F12;

    if (keytype == 63){ //control+C
        mykey = VK_CONTROL;
    }


    if (keytype == 70)
        mykey = VK_NUMPAD0;

    if (keytype == 71)
        mykey = VK_NUMPAD1;

    if (keytype == 72)
        mykey = VK_NUMPAD2;

    if (keytype == 73)
        mykey = VK_NUMPAD3;

    if (keytype == 74)
        mykey = VK_NUMPAD4;

    if (keytype == 75)
        mykey = VK_NUMPAD5;

    if (keytype == 76)
        mykey = VK_NUMPAD6;

    if (keytype == 77)
        mykey = VK_NUMPAD7;

    if (keytype == 78)
        mykey = VK_NUMPAD8;

    if (keytype == 79)
        mykey = VK_NUMPAD9;

    if (keytype == 80)
        mykey = VK_SUBTRACT;

    if (keytype == 81)
        mykey = VK_ADD;

    keystatesend = mykey;
    PostMessage(hwnd, presskey, mykey, lParam);
    PostMessage(hwnd, WM_INPUT, VK_RIGHT, lParam);
    if (keytype == 63) {
        PostMessage(hwnd, presskey, 0x43, lParam);
    }
    return;

}
void Buttonaction(const char key[3], int mode, int serchnum, int startsearch, bool onlysearch)
{
    POINT coords = {0,0};

    //criticals
    EnterCriticalSection(&critical);
    bool found = foundit;
    LeaveCriticalSection(&critical);
    //mode // onlysearch

    if (mode != 2 || onlysearch)
    {
        
        pausedraw = true;
        Sleep(25);
        bool movenotclick = false;
        bool clicknotmove = false;
        //int i = startsearch;
        //RECT rect;
        HBITMAP hbbmdsktop;
        //GetWindowRect(hwnd, &rect);
        //HDC tomakebmp = GetDC(hwnd);
        //HBITMAP hbmdsktop = CreateCompatibleBitmap(tomakebmp, rect.right, rect.left);
        //ReleaseDC(hwnd, tomakebmp);

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
                if (hbbmdsktop = CaptureWindow24Bit(hwnd, screenSize, largePixels, strideLarge, false))
                {
                    // MessageBox(NULL, "some kind of error", "captured desktop", MB_OK | MB_ICONINFORMATION);

                    POINT pt;
                    if (FindSubImage24(largePixels.data(), screenSize.cx, screenSize.cy, strideLarge, smallPixels.data(), smallW, smallH, strideSmall, pt, 0, 0))
                    {
                        Sleep(50);
                      //  vibrateController(controllerID, 15000);
                        if (strcmp(key, "\\A") == 0) {
                            if (bmpAtype == 1)
                            {
                                movenotclick = true;
                            }
                            if (bmpAtype == 2)
                            {
                                clicknotmove = true;
                            }
                            if (!onlysearch)
                                startsearchA = i + 1;
                            else
                            {
                                EnterCriticalSection(&critical);
                                startsearchA = i;
                                PointA.x = pt.x;
                                PointA.y = pt.y;
                                LeaveCriticalSection(&critical);
                            }
                        }
                        else if (strcmp(key, "\\B") == 0) {
                            if (bmpBtype == 1)
                            {
                                movenotclick = true;
                            }
                            if (bmpBtype == 2)
                            {
                                clicknotmove = true;
                            }
                            if (!onlysearch)
                                startsearchB = i + 1;
                            else
                            {
                                EnterCriticalSection(&critical);
                                startsearchB = i;
                                PointB.x = pt.x;
                                PointB.y = pt.y;
                                LeaveCriticalSection(&critical);
                            }
                        }
                        else if (strcmp(key, "\\X") == 0) {
                            if (bmpXtype == 1)
                            {
                                movenotclick = true;
                            }
                            if (bmpXtype == 2)
                            {
                                clicknotmove = true;
                            }
                            if (!onlysearch)
                                startsearchX = i + 1;
                            else
                            {
                                EnterCriticalSection(&critical);
                                startsearchX = i;
                                PointX.x = pt.x;
                                PointX.y = pt.y;
                                LeaveCriticalSection(&critical);
                            }
                        }
                        else if (strcmp(key, "\\Y") == 0) {
                            if (bmpYtype == 1)
                            {
                                movenotclick = true;
                            }
                            if (bmpYtype == 2)
                            {
                                clicknotmove = true;
                            }
                            if (!onlysearch)
                                startsearchY = i + 1;
                            else
                            {
                                EnterCriticalSection(&critical);
                                startsearchY = i;
                                PointY.x = pt.x;
                                PointY.y = pt.y;
                                LeaveCriticalSection(&critical);
                            }
                        }
                        else if (strcmp(key, "\\C") == 0) {
                            if (bmpCtype == 1)
                            {
                                movenotclick = true;
                            }
                            if (bmpCtype == 2)
                            {
                                clicknotmove = true;
                            }
                            startsearchC = i + 1;
                        }
                        else if (strcmp(key, "\\D") == 0) {
                            if (bmpDtype == 1)
                            {
                                movenotclick = true;
                            }
                            if (bmpDtype == 2)
                            {
                                clicknotmove = true;
                            }
                            startsearchD = i + 1;
                        }
                        else if (strcmp(key, "\\E") == 0) {
                            if (bmpEtype == 1)
                            {
                                movenotclick = true;
                            }
                            if (bmpEtype == 2)
                            {
                                clicknotmove = true;
                            }
                            startsearchE = i + 1;
                        }
                        else if (strcmp(key, "\\F") == 0) {
                            if (bmpFtype == 1)
                            {
                                movenotclick = true;
                            }
                            if (bmpFtype == 2)
                            {
                                clicknotmove = true;
                            }
                            startsearchF = i + 1;
                        }
                        // else return false;
                        if ( clicknotmove == false && !onlysearch)
                        { 
                            Xf = pt.x;
                            Yf = pt.y;
                        }
                        if (movenotclick == false && !onlysearch)
                        {
                            Xf = pt.x;
                            Yf = pt.y;
                           
                            SendMouseClick(pt.x, pt.y, 8, 1);
                            Sleep(10);
                            SendMouseClick(pt.x, pt.y, 3, 2);
                            Sleep(10);
                            SendMouseClick(pt.x, pt.y, 4, 2);
                         
                        }
                        else if (!onlysearch)
                        {
                           
                            SendMouseClick(pt.x, pt.y, 8, 1);
                           
                        }
                        if (clicknotmove == true && !onlysearch)
                        {
                         //   Sleep(10); //crashed game
                         // fixme!
                         //   X = fakecursor.x;
                         //   Y = fakecursor.y;
                           // SendMouseClick(fakecursor.x, fakecursor.y, 8, 1);

                        }
                        if (onlysearch) {
                            coords.x = pt.x;
                            coords.y = pt.y;
                        }
                        found = true;
                        break;
                    }

                    // else  return false; 
                    

                    DeleteObject(hbbmdsktop);
                }
                
                // else  

            }

            // else  
        }
        if (found == false)
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
                    if (hbbmdsktop = CaptureWindow24Bit(hwnd, screenSize, largePixels, strideLarge, false))
                    {
                       
                        POINT pt;
                       // wsprintf(buffer, "second A is %d", i);
                       // MessageBoxA(NULL, buffer, "Info", MB_OK);
                        // MessageBox(NULL, "some kind of error", "captured desktop", MB_OK | MB_ICONINFORMATION);
                        if (FindSubImage24(largePixels.data(), screenSize.cx, screenSize.cy, strideLarge, smallPixels.data(), smallW, smallH, strideSmall, pt, 0, 0))
                        {
                             //char buffer[100];       if (bmpAtype == 1 coords.x = pt.x;
                            Sleep(50);//testing
                            //vibrateController(controllerID, 15000);
                            if (strcmp(key, "\\A") == 0) 
                            {
                                if (bmpAtype == 1)
                                {
                                    movenotclick = true;
                                }
                                if (bmpAtype == 2)
                                {
                                    clicknotmove = true;
                                }
                                if (!onlysearch)
                                    startsearchA = i + 1;
                                else
                                {
                                    EnterCriticalSection(&critical);
                                    startsearchA = i;
                                    PointA.x = pt.x;
                                    PointA.y = pt.y;
                                    LeaveCriticalSection(&critical);
                                }
                            }
                            else if (strcmp(key, "\\B") == 0) {
                                if (bmpBtype == 1)
                                {
                                    movenotclick = true;
                                }
                                if (bmpBtype == 2)
                                {
                                    clicknotmove = true;
                                }
                                if (!onlysearch)
                                    startsearchB = i + 1;
                                else {
                                    EnterCriticalSection(&critical);
                                    startsearchB = i;
                                    PointB.x = pt.x;
                                    PointB.y = pt.y;
                                    LeaveCriticalSection(&critical);
                                    }
                            }
                            else if (strcmp(key, "\\X") == 0 && scanoption) {
                                if (bmpXtype == 1)
                                {
                                    movenotclick = true;
                                }
                                if (bmpXtype == 2)
                                {
                                    clicknotmove = true;
                                }
                                if (!onlysearch)
                                    startsearchX = i + 1;
                                else{
                                    EnterCriticalSection(&critical);
                                    startsearchX = i;
                                    PointX.x = pt.x;
                                    PointX.y = pt.y;
                                    LeaveCriticalSection(&critical);
                                }
                            }
                            else if (strcmp(key, "\\Y") == 0)
                            {
                                if (bmpYtype == 1)
                                {
                                    movenotclick = true;
                                }
                                if (bmpYtype == 2)
                                {
                                    clicknotmove = true;
                                }
                                if (!onlysearch)
                                    startsearchY = i + 1;
                                else {
                                    EnterCriticalSection(&critical);
                                    startsearchY = i;
                                    PointY.x = pt.x;
                                    PointY.y = pt.y;
                                    LeaveCriticalSection(&critical);
                                }
                            }
                            else if (strcmp(key, "\\C") == 0) {
                                if (bmpCtype == 1)
                                {
                                    movenotclick = true;
                                }
                                if (bmpCtype == 2)
                                {
                                    clicknotmove = true;
                                }
                                startsearchC = i + 1;
                            }
                            else if (strcmp(key, "\\D") == 0) {
                                if (bmpDtype == 1)
                                {
                                    movenotclick = true;
                                }
                                if (bmpDtype == 2)
                                {
                                    clicknotmove = true;
                                }
                                startsearchD = i + 1;
                            }
                            else if (strcmp(key, "\\E") == 0) {
                                if (bmpEtype == 1)
                                {
                                    movenotclick = true;
                                }
                                if (bmpEtype == 2)
                                {
                                    clicknotmove = true;
                                }
                                startsearchE = i + 1;
                            }
                            else if (strcmp(key, "\\F") == 0) {
                                if (bmpFtype == 1)
                                {
                                    movenotclick = true;
                                }
                                if (bmpFtype == 2)
                                {
                                    clicknotmove = true;
                                }
                                startsearchF = i + 1;
                            }
                           // X = pt.x;
                           // Y = pt.y;
                            if (clicknotmove == false)
                            {
                                Xf = pt.x;
                                Yf = pt.y;
                               // MessageBox(NULL, "some kind of error", "found image", MB_OK | MB_ICONINFORMATION);
                            }
                            if (movenotclick == false && !onlysearch)
                            {
                                Xf = pt.x;
                                Yf = pt.y;
                        
                                SendMouseClick(pt.x, pt.y, 8, 1);
                                Sleep(10);
                                SendMouseClick(pt.x, pt.y, 3, 2);
                                Sleep(10);
                                SendMouseClick(pt.x, pt.y, 4, 2);
                           
                            }
                            else if (!onlysearch)
                            {
                         
                                SendMouseClick(pt.x, pt.y, 8, 1);
                              
                            }
                            else if (clicknotmove == true && !onlysearch)
                            { //found the problem. point fakecursor is not good
                                //fixme!!
                                //Sleep(2);  
                               // X = fakecursor.x;
                               // Y = fakecursor.y;
                               // SendMouseClick(fakecursor.x, fakecursor.y, 8, 1);

                            }
                            else if (onlysearch)
                            { 
                                coords.x = pt.x;
                                coords.y = pt.y;
                            }
                            found = true;
                            break;
                        }
                        else if (onlysearch)//not found
                        {
                             if (strcmp(key, "\\X") == 0) 
                             {
                                   EnterCriticalSection(&critical);
                                   startsearchX = 0;
                                   PointX.x = 0;
                                   PointX.y = 0;
                                   LeaveCriticalSection(&critical);
                             }
                             if (strcmp(key, "\\Y") == 0)
                             {
                                 EnterCriticalSection(&critical);
                                 startsearchY = 0;
                                 PointY.x = 0;
                                 PointY.y = 0;
                                 LeaveCriticalSection(&critical);
                             }
                             if (strcmp(key, "\\B") == 0)
                             {
                                 EnterCriticalSection(&critical);
                                 startsearchB = 0;
                                 PointB.x = 0;
                                 PointB.y = 0;
                                 LeaveCriticalSection(&critical);
                             }
                             if (strcmp(key, "\\A") == 0)
                             {
                                 EnterCriticalSection(&critical);
                                 startsearchA = 0;
                                 PointA.x = 0;
                                 PointA.y = 0;
                                 LeaveCriticalSection(&critical);
                             }
                        }
                        
                       // DeleteObject(hbm24);
                        DeleteObject(hbmdsktop);
                    }


                    

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
        SaveWindow10x10BMP(hwnd, wpath.c_str(), Xf, Yf);
       // MessageBox(NULL, "Mapped spot!", key, MB_OK | MB_ICONINFORMATION);
        showmessage = 10;
        return;
    }
    else showmessage = 11;
    EnterCriticalSection(&critical);
    bool foundit = found;
    LeaveCriticalSection(&critical);
    if (!onlysearch)
    { 
        Sleep(50); //to avoid double press
        pausedraw = false; //used?
        return;
    }
    else return;
     //return not caught here, hopefully no problem
}
//int akkumulator = 0;    

#define WM_MOVE_pointerWindow (WM_APP + 1)

void GetWindowDimensions(HWND pointerWindow)
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

        SetWindowPos(pointerWindow, HWND_TOPMOST,
            topLeft.x,
            topLeft.y,
            cRect.right - cRect.left,
            cRect.bottom - cRect.top,
            SWP_NOACTIVATE);
        ShowWindow(pointerWindow, SW_SHOW);
    }
}
int cursoroffsetx, cursoroffsety;
int offsetSET; //0:sizing 1:offset 2:done
int cursorWidth = 40;
int cursorHeight = 40;
HWND pointerWindow = nullptr;
bool DrawFakeCursorFix = false;
static constexpr auto transparencyKey = RGB(0, 0, 1);
HBRUSH transparencyBrush;
HCURSOR oldhCursor = NULL;
HCURSOR hCursorW = NULL;
bool nochange = false;
int WoldX, WoldY;
bool oldHadShowCursor = true;
void DrawCursor(int X, int Y, HCURSOR hcursor)
{
    HDC hdccw = GetDC(pointerWindow);

    POINT pos = { X, Y };

    DrawIcon(hdccw, pos.x, pos.y, hcursor);

    ReleaseDC(pointerWindow, hdccw);
    
    //DeleteObject(hcursor);
}
// Window procedure
bool neederase = false;
LRESULT CALLBACK FakeCursorWndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg)
    {
    case WM_MOVE_pointerWindow:
    {
        GetWindowDimensions(hWnd);
        break;
    }
    case WM_PAINT:
    {
        PAINTSTRUCT ps;
        HDC hdcc = BeginPaint(hWnd, &ps);

        //criticals for windowthread
        EnterCriticalSection(&critical);
        POINT pos = { fakecursorW.x, fakecursorW.y };
        POINT Apos = { PointA.x, PointA.y };
        POINT Bpos = { PointB.x, PointB.y };
        POINT Xpos = { PointX.x, PointX.y };
        POINT Ypos = { PointY.x, PointY.y };
        hCursorW = hCursor;
        int showmessageWW = showmessageW;
        LeaveCriticalSection(&critical);
        RECT fill{ WoldX, WoldY, WoldX + cursorWidth, WoldY + cursorHeight };
        if (showmessageWW == 1)
        {
            TextOut(hdcc, 20, 20, TEXT("BMP MODE"), 8);
            TextOut(hdcc, 20, 20 + 17, TEXT("only mapping searches"), 21);
            neederase = true;
        }
        if (Apos.x != 0 && Apos.y != 0)
        { 
            TextOut(hdcc, Apos.x, Apos.y, TEXT("A"), 1);

        }
        if (Bpos.x != 0 && Bpos.y != 0)
            TextOut(hdcc, Bpos.x, Bpos.y, TEXT("B"), 1);
        if (Xpos.x != 0 && Xpos.y != 0)
            TextOut(hdcc, Xpos.x, Xpos.y, TEXT("X"), 1);
        if (Ypos.x != 0 && Ypos.y != 0)
            TextOut(hdcc, Ypos.x, Ypos.y, TEXT("Y"), 1);
        else if (showmessageWW == 2)
        {
            TextOut(hdcc, 20, 20, TEXT("CURSOR MODE"), 11);
            TextOut(hdcc, 20, 20 + 17, TEXT("mapping searches + cursor"), 25);
            neederase = true;
        }
        else if (showmessageWW == 3)
        {
            TextOut(hdcc, 20, 20, TEXT("EDIT MODE"), 9);
            TextOut(hdcc, 20, 20 + 15, TEXT("tap a button to bind it to coordinate"), 37);
            TextOut(hdcc, 20, 20 + 30, TEXT("A,B,X,Y,R2,R3,L2,L3 can be mapped"), 32);
            neederase = true;
        }
        else if (showmessageWW == 10)
        {
            TextOut(hdcc, 20, 20, TEXT("BUTTON MAPPED"), 13);
            neederase = true;
        }
        else if (showmessageWW == 11)
        {
            TextOut(hdcc, 20, 20, TEXT("WAIT FOR MESSAGE EXPIRE!"), 24);
            neederase = true;
        }
        else if (showmessageWW == 12)
        {
            TextOut(hdcc, 20, 20, TEXT("DISCONNECTED!"), 14); //14
            neederase = true;
        }
        else if (showmessageWW == 69)
        {
            TextOut(hdcc, 20, 20, TEXT("SHUTTING DOWN"), 13);
            neederase = true;
        }
        else if (showmessageWW == 70)
        {
            TextOut(hdcc, 20, 20, TEXT("STARTING!"), 10);
            neederase = true;
        }
        else if (neederase)
        {
            GetWindowRect(pointerWindow, &fill);
            neederase = false;
        }
        FillRect(hdcc, &fill, transparencyBrush); // Note: window, not screen coordinates! 


        if (!DrawFakeCursorFix)
        {
            if (hCursorW != NULL)
            {
                gotcursoryet = true;
                DrawIcon(hdcc, pos.x, pos.y, hCursorW);
            }
            else if (onoroff == true && (alwaysdrawcursor == 1 || gotcursoryet == false))
            {
                for (int y = 0; y < 20; y++)
                {
                    for (int x = 0; x < 20; x++)
                    {
                        int val = colorfulSword[y][x];
                        if (val != 0)
                        {
                            HBRUSH hBrush = CreateSolidBrush(colors[val]);
                            RECT rect = { Xf + x , Yf + y , Xf + x + 1, Yf + y + 1 };
                            FillRect(hdcc, &rect, hBrush);
                            DeleteObject(hBrush);
                        }
                    }
                }
            }
        }
        else if (DrawFakeCursorFix)
        {
            pos.x -= cursoroffsetx;
            pos.y -= cursoroffsety;
            RECT fill{ pos.x, pos.y, pos.x + cursorWidth, pos.y + cursorHeight };
            FillRect(hdcc, &fill, transparencyBrush); // Note: window, not screen coordinates! 
            if (pos.x < 0) pos.x = 0;
            if (pos.y < 0) pos.y = 0;

            if (hCursorW != NULL)// && hdc && hCursor
            {
                if (DrawIconEx(hdcc, pos.x, pos.y, hCursorW, cursorWidth, cursorHeight, 0, transparencyBrush, DI_NORMAL))
                {
                    if (hCursorW != oldhCursor && offsetSET > 1 && nochange == false)
                    {
                        offsetSET = 0;
                    }
                    if (offsetSET == 1 && hCursorW != LoadCursor(NULL, IDC_ARROW) && IsWindowVisible(pointerWindow)) //offset setting
                    {
                        HDC hdcMem = CreateCompatibleDC(hdcc);
                        HBITMAP hbmScreen = CreateCompatibleBitmap(hdcc, cursorWidth, cursorHeight);
                        SelectObject(hdcMem, hbmScreen);
                        BitBlt(hdcMem, 0, 0, cursorWidth, cursorHeight, hdcc, pos.x, pos.y, SRCCOPY);
                        cursoroffsetx = -1;
                        cursoroffsety = -1;
                        int leftcursoroffsetx = 0;
                        int leftcursoroffsety = -1;
                        int rightcursoroffsetx = 0;
                        // Scanning
                        for (int y = 0; y < cursorHeight; y++)
                        {
                            for (int x = 0; x < cursorWidth; x++)
                            {
                                COLORREF pixelColor = GetPixel(hdcMem, x, y); // scan from left and find common y to use in next scan also
                                if (pixelColor != transparencyKey)
                                {
                                    leftcursoroffsetx = x;
                                    leftcursoroffsety = y;
                                    break;
                                }
                            }
                            if (leftcursoroffsetx != -1) break;
                        }


                        for (int x = cursorWidth - 1; x >= 0; x--)
                        {
                            COLORREF pixelColor = GetPixel(hdcMem, x, leftcursoroffsety); // scan from right
                            if (pixelColor != transparencyKey)
                            {
                                rightcursoroffsetx = cursorWidth - x;
                                break;
                            }
                        }
                        //Adjusting possible here if symmetric cursor is not found
                        if (leftcursoroffsetx == rightcursoroffsetx - 1 || leftcursoroffsetx == rightcursoroffsetx + 1 || leftcursoroffsetx == rightcursoroffsetx) //check for symmetric first only if Y offset is small
                        {
                            cursoroffsety = cursorHeight / 2;
                            cursoroffsetx = cursorWidth / 2;
                        }
                        else if (leftcursoroffsety > 2 || leftcursoroffsetx > 2) //is there any other offsets?
                        {
                            cursoroffsetx = leftcursoroffsetx;
                            cursoroffsety = leftcursoroffsety;
                            nochange = true;
                        }

                        else { //no offsets
                            cursoroffsetx = 0;
                            cursoroffsety = 0;
                        }
                        offsetSET++; //offset set to 2 should do drawing only now
                        DeleteDC(hdcMem);
                        DeleteObject(hbmScreen);



                    }
                    else if (offsetSET == 0 && hCursorW != NULL && IsWindowVisible(pointerWindow)) //size setting
                    {
                        ICONINFO ii;
                        BITMAP bitmap;
                        cursoroffsetx = 0;
                        cursoroffsety = 0;
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
                    oldhCursor = hCursorW;
                }
            }
            else if (onoroff == true && (alwaysdrawcursor == 1 || gotcursoryet == false))
            {
                for (int y = 0; y < 20; y++)
                {
                    for (int x = 0; x < 20; x++)
                    {
                        int val = colorfulSword[y][x];
                        if (val != 0)
                        {
                            HBRUSH hBrush = CreateSolidBrush(colors[val]);
                            EnterCriticalSection(&critical);
                            RECT rect = { Xf + x , Yf + y , Xf + x + 1, Yf + y + 1 };
                            LeaveCriticalSection(&critical);
                            FillRect(hdcc, &rect, hBrush);
                            DeleteObject(hBrush);
                        }
                    }
                }
            }
        }
        WoldX = pos.x;
        WoldY = pos.y;
        

        EndPaint(hWnd, &ps);
        return 0;

    }
    case WM_DESTROY:
    {
        PostQuitMessage(0);
        return 0;
    }
    default:
        break;
    }

    return DefWindowProc(hWnd, msg, wParam, lParam);
}

bool scanloop = true;
DWORD WINAPI ScanThread(LPVOID) 
{
   // EnterCriticalSection(&critical);
   // LeaveCriticalSection(&critical);
    while (scanloop)
    { 
        if (scantick < 21)
            scantick++;
        else scantick = 0;
        if (scanoption == 1)
        {
            if (scantick == 5)
            {
                
                Buttonaction("\\A", mode, numphotoA, startsearchA, true);
                
                InvalidateRect(pointerWindow, NULL, true); //invalidate all
            }
            if (scantick == 10)
            {
                
                Buttonaction("\\B", mode, numphotoB, startsearchB, true);
                
                InvalidateRect(pointerWindow, NULL, true); //invalidate all
            }
            if (scantick == 15)
            {
               
                Buttonaction("\\X", mode, numphotoX, startsearchX, true);
                
                InvalidateRect(pointerWindow, NULL, true); //invalidate all
            }
            if (scantick == 20)
            {
                
                Buttonaction("\\Y", mode, numphotoY, startsearchY, true);
                
                InvalidateRect(pointerWindow, NULL, true); //invalidate all
            }
            Sleep(1);
        }
        //else Sleep(1000);
    }
    return 0;
}
//TODO: width/height probably needs to change


// DrawFakeCursorFix. cursor offset scan and cursor size fix
int scanAtype = 0;
int scanBtype = 0;
int scanXtype = 0;
int scanYtype = 0;

// Thread to create and run the window
DWORD WINAPI WindowThreadProc(LPVOID) {
    EnterCriticalSection(&critical);
    transparencyBrush = (HBRUSH)CreateSolidBrush(transparencyKey);
    WNDCLASSW wc = { };
    wc.lpfnWndProc = FakeCursorWndProc;
    wc.hInstance = GetModuleHandleW(0);
    wc.hbrBackground = transparencyBrush;

    std::srand(static_cast<unsigned int>(std::time(nullptr)));
    const std::wstring classNameStr = std::wstring{ L"GtoMnK_Pointer_Window" } + std::to_wstring(std::rand());

    wc.lpszClassName = classNameStr.c_str();
    wc.style = CS_OWNDC | CS_NOCLOSE;

    RegisterClassW(&wc);

    pointerWindow = CreateWindowExW(WS_EX_NOACTIVATE | WS_EX_NOINHERITLAYOUT | WS_EX_NOPARENTNOTIFY |
        WS_EX_TOPMOST | WS_EX_TRANSPARENT | WS_EX_TOOLWINDOW | WS_EX_LAYERED,
        wc.lpszClassName, L"classNameStr.c_str()",
        WS_POPUP | WS_VISIBLE,
        0, 0, 800, 600,
        nullptr, nullptr, GetModuleHandle(NULL), nullptr);




    if (!pointerWindow) {
        MessageBoxA(NULL, "Failed to create pointer window", "Error", MB_OK | MB_ICONERROR);
    }
    SetLayeredWindowAttributes(pointerWindow, transparencyKey, 0, LWA_COLORKEY);
    ShowWindow(pointerWindow, SW_SHOW);

    MSG msg;
    ZeroMemory(&msg, sizeof(msg));
	LeaveCriticalSection(&critical);
    while (GetMessage(&msg, nullptr, 0, 0))
    {
        if (msg.message != WM_QUIT)
        {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
		}

    }
    return 0;
}

RECT oldrect;
POINT oldposcheck;
DWORD WINAPI ThreadFunction(LPVOID lpParam)
{
    Sleep(2000);
    char buffer[256];
    
    // settings reporting
    std::string iniPath = UGetExecutableFolder() + "\\Xinput.ini";
    std::string iniSettings = "Settings";

    //controller settings
    controllerID = GetPrivateProfileInt(iniSettings.c_str(), "Controllerid", -9999, iniPath.c_str()); //simple test if settings read but write it wont work.
    int AxisLeftsens = GetPrivateProfileInt(iniSettings.c_str(), "AxisLeftsens", -7849, iniPath.c_str());
    int AxisRightsens = GetPrivateProfileInt(iniSettings.c_str(), "AxisRightsens", 12000, iniPath.c_str());
    int AxisUpsens = GetPrivateProfileInt(iniSettings.c_str(), "AxisUpsens", 0, iniPath.c_str());
    int AxisDownsens = GetPrivateProfileInt(iniSettings.c_str(), "AxisDownsens", -16049, iniPath.c_str());
    int scrollspeed3 = GetPrivateProfileInt(iniSettings.c_str(), "Scrollspeed", 150, iniPath.c_str());
    righthanded = GetPrivateProfileInt(iniSettings.c_str(), "Righthanded", 0, iniPath.c_str());
    scanoption = GetPrivateProfileInt(iniSettings.c_str(), "Scan", 0, iniPath.c_str());
    GetPrivateProfileString(iniSettings.c_str(), "Radial_Deadzone", "0.2", buffer, sizeof(buffer), iniPath.c_str());
    radial_deadzone = std::stof(buffer); //sensitivity

    GetPrivateProfileString(iniSettings.c_str(), "Axial_Deadzone", "0.0", buffer, sizeof(buffer), iniPath.c_str());
    axial_deadzone = std::stof(buffer); //sensitivity

    GetPrivateProfileString(iniSettings.c_str(), "Sensitivity", "20.0", buffer, sizeof(buffer), iniPath.c_str());
    sensitivity = std::stof(buffer); //sensitivity //accel_multiplier

    GetPrivateProfileString(iniSettings.c_str(), "Accel_Multiplier", "1.7", buffer, sizeof(buffer), iniPath.c_str());
    accel_multiplier = std::stof(buffer);

    Xoffset = GetPrivateProfileInt(iniSettings.c_str(), "Xoffset", 0, iniPath.c_str());
    Yoffset = GetPrivateProfileInt(iniSettings.c_str(), "Yoffset", 0, iniPath.c_str());

    //mode
    int InitialMode = GetPrivateProfileInt(iniSettings.c_str(), "Initial Mode", 1, iniPath.c_str());
    int Modechange = GetPrivateProfileInt(iniSettings.c_str(), "Allow modechange", 1, iniPath.c_str());

    int sendfocus = GetPrivateProfileInt(iniSettings.c_str(), "Sendfocus", 0, iniPath.c_str());
    int responsetime = GetPrivateProfileInt(iniSettings.c_str(), "Responsetime", 0, iniPath.c_str());
    int doubleclicks = GetPrivateProfileInt(iniSettings.c_str(), "Doubleclicks", 0, iniPath.c_str());
    int scrollenddelay = GetPrivateProfileInt(iniSettings.c_str(), "DelayEndScroll", 50, iniPath.c_str());
    int quickMW = GetPrivateProfileInt(iniSettings.c_str(), "MouseWheelContinous", 0, iniPath.c_str());

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

    bmpAtype = GetPrivateProfileInt(iniSettings.c_str(), "AbmpAction", 0, iniPath.c_str());
    bmpBtype = GetPrivateProfileInt(iniSettings.c_str(), "BbmpAction", 0, iniPath.c_str());
    bmpXtype = GetPrivateProfileInt(iniSettings.c_str(), "XbmpAction", 0, iniPath.c_str());
    bmpYtype = GetPrivateProfileInt(iniSettings.c_str(), "YbmpAction", 0, iniPath.c_str());
    bmpCtype = GetPrivateProfileInt(iniSettings.c_str(), "CbmpAction", 0, iniPath.c_str());
    bmpDtype = GetPrivateProfileInt(iniSettings.c_str(), "DbmpAction", 0, iniPath.c_str());
    bmpEtype = GetPrivateProfileInt(iniSettings.c_str(), "EbmpAction", 0, iniPath.c_str());
    bmpFtype = GetPrivateProfileInt(iniSettings.c_str(), "FbmpAction", 0, iniPath.c_str());
    //need a copy to be read if scanoption
    if (scanoption)
    {
        scanAtype = GetPrivateProfileInt(iniSettings.c_str(), "AbmpAction", 0, iniPath.c_str());
        scanBtype = GetPrivateProfileInt(iniSettings.c_str(), "BbmpAction", 0, iniPath.c_str());
        scanXtype = GetPrivateProfileInt(iniSettings.c_str(), "XbmpAction", 0, iniPath.c_str());
        scanYtype = GetPrivateProfileInt(iniSettings.c_str(), "YbmpAction", 0, iniPath.c_str());
    }
    uptype = GetPrivateProfileInt(iniSettings.c_str(), "Upkey", -1, iniPath.c_str());
    downtype = GetPrivateProfileInt(iniSettings.c_str(), "Downkey", -2, iniPath.c_str());
    lefttype = GetPrivateProfileInt(iniSettings.c_str(), "Leftkey", -3, iniPath.c_str());
    righttype = GetPrivateProfileInt(iniSettings.c_str(), "Rightkey", -4, iniPath.c_str());
    int ShoulderNextbmp = GetPrivateProfileInt(iniSettings.c_str(), "ShouldersNextBMP", 0, iniPath.c_str());

    //hooks
    drawfakecursor = GetPrivateProfileInt(iniSettings.c_str(), "DrawFakeCursor", 1, iniPath.c_str());
    alwaysdrawcursor = GetPrivateProfileInt(iniSettings.c_str(), "DrawFakeCursorAlways", 1, iniPath.c_str());
	userealmouse = GetPrivateProfileInt(iniSettings.c_str(), "UseRealMouse", 0, iniPath.c_str());   //scrolloutsidewindow
    ignorerect = GetPrivateProfileInt(iniSettings.c_str(), "IgnoreRect", 0, iniPath.c_str());

    int scrolloutsidewindow = GetPrivateProfileInt(iniSettings.c_str(), "Scrollmapfix", 1, iniPath.c_str());   //scrolloutsidewindow
    
    if (drawfakecursor == 3)
		DrawFakeCursorFix = true;

    Sleep(1000);
    hwnd = GetMainWindowHandle(GetCurrentProcessId());
    int mode = InitialMode;


    if (controllerID == -9999)
    {
        MessageBoxA(NULL, "Warning. Settings file Xinput.ini not read. All settings standard. Or maybe ControllerID is missing from ini", "Error", MB_OK | MB_ICONERROR);
		controllerID = 0; //default controller  

	}

    HBITMAP hbmdsktop = NULL; //memory corruption?

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

    if (scanoption == 1)
    {
        HANDLE two = CreateThread(nullptr, 0,
            (LPTHREAD_START_ROUTINE)ScanThread, g_hModule, 0, 0); //GetModuleHandle(0)
        CloseHandle(two);
       // InitializeCriticalSection(&criticalA);
    }
 
	bool window = false;
    while (loop == true)
    {
        bool movedmouse = false; //reset
        keystatesend = 0; //reset keystate
		foundit = false; //reset foundit the bmp search found or not
        if (hwnd == NULL)
        {
            hwnd = GetMainWindowHandle(GetCurrentProcessId());
        }
        else
        {
			//   
            RECT rect;
			POINT poscheck;
            GetClientRect(hwnd, &rect);
			poscheck.x = rect.left;
            poscheck.y = rect.top;
			ClientToScreen(hwnd, &poscheck);

            if (drawfakecursor == 2 || drawfakecursor == 3)
            {//fake cursor window creation
                
                if (!window) 
                { 
                    HANDLE two = CreateThread(nullptr, 0,
                        (LPTHREAD_START_ROUTINE)WindowThreadProc, g_hModule, 0, 0); //GetModuleHandle(0)
                    CloseHandle(two);
					Sleep(100); //give time to create window
                    EnterCriticalSection(&critical);
                    while (!pointerWindow) Sleep(10);
					
					SendMessage(pointerWindow, WM_MOVE_pointerWindow, 0, 0); //focus to avoid alt tab issues
                    LeaveCriticalSection(&critical);

				    window = true;
                }
                else if (oldrect.left != rect.left || oldrect.right != rect.right || oldrect.top != rect.top || oldrect.bottom != rect.bottom || oldposcheck.x != poscheck.x || oldposcheck.y != poscheck.y)
                {
                    EnterCriticalSection(&critical);
                    SendMessage(pointerWindow, WM_MOVE_pointerWindow, 0, 0); //focus to avoid alt tab issues
					//MessageBoxA(NULL, "Resized/moved window - adjust fake cursor window", "Info", MB_OK);
                    //ShowWindow(pointerWindow, SW_SHOW);
                    LeaveCriticalSection(&critical);
                }
                
			}
			
			oldposcheck.x = poscheck.x;
            oldposcheck.y = poscheck.y;
            oldrect.left = rect.left;
			oldrect.right = rect.right;
			oldrect.top = rect.top;
			oldrect.bottom = rect.bottom;


            if (ignorerect == 1)
            {
                RECT frameBounds;
                HRESULT hr = DwmGetWindowAttribute(hwnd, DWMWA_EXTENDED_FRAME_BOUNDS, &frameBounds, sizeof(frameBounds));
                if (SUCCEEDED(hr)) {
                    // These are the actual visible edges of the window in client coordinates
                    POINT upper;
                    upper.x = frameBounds.left;
                    upper.y = frameBounds.top;

                    //used in getcursrorpos
                    rectignore.x = upper.x;
                    rectignore.y = upper.y;

                    rect.right = frameBounds.right - frameBounds.left;
                    rect.bottom = frameBounds.bottom - frameBounds.top;
                    rect.left = 0;
                    rect.top = 0;

                }
            }

            XINPUT_STATE state;
            ZeroMemory(&state, sizeof(XINPUT_STATE));
            // Check controller 0
            DWORD dwResult = XInputGetState(controllerID, &state);
            bool leftPressed = IsTriggerPressed(state.Gamepad.bLeftTrigger);
            bool rightPressed = IsTriggerPressed(state.Gamepad.bRightTrigger);
            if (drawfakecursor == 2 || drawfakecursor == 3)
            {
                EnterCriticalSection(&critical);
                InvalidateRect(pointerWindow, NULL, false);
                LeaveCriticalSection(&critical);
            }
			



            if (dwResult == ERROR_SUCCESS)
            {
                
                //fakecursor.x = Xf;
               // fakecursor.y = Yf;
               // //fakecursor obsolete. use Xf and Yf
               // ClientToScreen(hwnd, &fakecursor);
               // Screen
                //criticals for windowthread
                EnterCriticalSection(&critical);
                fakecursorW.x = Xf;
                fakecursorW.y = Yf;
				showmessageW = showmessage;
                LeaveCriticalSection(&critical);

                
				
                // Controller is connected
                WORD buttons = state.Gamepad.wButtons;
                bool currA = (buttons & XINPUT_GAMEPAD_A) != 0;
                bool Apressed = (buttons & XINPUT_GAMEPAD_A);

                if (showmessage == 12) //was disconnected?
                {
                    showmessage = 0;
				}


                if (oldA == true)
                {
                    if (buttons & XINPUT_GAMEPAD_A && onoroff == true)
                    {
                        // keep posting?
                    }
                    else {
                        oldA = false;
                        PostKeyFunction(hwnd, Atype, false);
                    }
                }
                else if (buttons & XINPUT_GAMEPAD_A && onoroff == true)
                {
                    oldA = true;
                    
                    if (!scanoption) {
                        startsearch = startsearchA;
                        Buttonaction("\\A", mode, numphotoA, startsearch, false); //buttonacton will be occupied by scanthread
                    }
                    else
                    {
                        EnterCriticalSection(&critical);
                        POINT Cpoint;
                        Cpoint.x = PointA.x;
                        Cpoint.y = PointA.y;
                        
                        LeaveCriticalSection(&critical);
                        if (Cpoint.x != 0 && Cpoint.y != 0)
                        {
                            EnterCriticalSection(&critical);
                            startsearchA++; //dont want it to update before input done
                            
                           
                            movedmouse = true;
                            if (scanAtype == 0) //click and move
                            {
                                Xf = Cpoint.x;
                                Yf = Cpoint.y;
                                SendMouseClick(Cpoint.x, Cpoint.y, 8, 1);
                                Sleep(5);
                                SendMouseClick(Cpoint.x, Cpoint.y, 3, 2);
                                Sleep(5);
                                SendMouseClick(Cpoint.x, Cpoint.y, 4, 2);
                            }
                            else if (scanAtype == 1) //only move
                            {
                                //SendMouseClick(Cpoint.x, Cpoint.y, 8, 1);
                                //MessageBoxA(NULL, "Mousemove", "Info", MB_OK);
                                Xf = Cpoint.x;
                                Yf = Cpoint.y;
                            }
                            else if (scanAtype == 2) //only click
                            {
                                SendMouseClick(Cpoint.x, Cpoint.y, 8, 1);
                                Sleep(5);
                                SendMouseClick(Cpoint.x, Cpoint.y, 3, 2);
                                Sleep(5);
                                SendMouseClick(Cpoint.x, Cpoint.y, 4, 2);
                                Sleep(5);
                                SendMouseClick(Xf, Yf, 8, 1);
                            }
                            LeaveCriticalSection(&critical);
                        }
                    }
                    
                    
                    if (foundit == false)
                    {
                        PostKeyFunction(hwnd, Atype, true);
                    }
                    if (mode == 2 && showmessage != 11)
                    {
                       numphotoA++;
                       Sleep(500);
                    }
                }



                if (oldB == true)
                {
                    if (buttons & XINPUT_GAMEPAD_B && onoroff == true)
                    {
                        // keep posting?
                    }
                    else {
                        oldB = false;
                        PostKeyFunction(hwnd, Btype, false);
                    }
                }
                else if (buttons & XINPUT_GAMEPAD_B && onoroff == true)
                {
                    

                    oldB = true;
                    if (!scanoption) {
                        startsearch = startsearchB;
                        Buttonaction("\\B", mode, numphotoB, startsearch, false);
                    }
                    else
                    {
                        EnterCriticalSection(&critical);
                        POINT Cpoint;
                        Cpoint.x = PointB.x;
                        Cpoint.y = PointB.y;
                        LeaveCriticalSection(&critical);
                        
                        if (Cpoint.x != 0 && Cpoint.y != 0)
                        {
                            EnterCriticalSection(&critical);
                            startsearchB++;
                            
                            movedmouse = true;
                    
                            if (scanBtype == 0) //click and move
                            {
                                Xf = Cpoint.x;
                                Yf = Cpoint.y;
                                SendMouseClick(Cpoint.x, Cpoint.y, 8, 1);
                                Sleep(5);
                                SendMouseClick(Cpoint.x, Cpoint.y, 3, 2);
                                Sleep(5);
                                SendMouseClick(Cpoint.x, Cpoint.y, 4, 2);
                            }
                            else if (scanBtype == 1) //only move
                            {
                                Xf = Cpoint.x;
                                Yf = Cpoint.y;
                                //(NULL, "Resized/moved window - adjust fake cursor window", "Info", MB_OK);
                                //SendMouseClick(Cpoint.x, Cpoint.y, 8, 1);
                            }
                            else if (scanBtype == 2) //only click
                            {
                                SendMouseClick(Cpoint.x, Cpoint.y, 8, 1);
                                Sleep(5);
                                SendMouseClick(Cpoint.x, Cpoint.y, 3, 2);
                                Sleep(5);
                                SendMouseClick(Cpoint.x, Cpoint.y, 4, 2);
                                Sleep(5);
                                SendMouseClick(Xf, Yf, 8, 1);
                            }
                            LeaveCriticalSection(&critical);
                        }
                        
                    }
                
                    if (foundit == false)
                    {
                        PostKeyFunction(hwnd, Btype, true);
                    }
                    if (mode == 2 && showmessage != 11)
                    {
                        numphotoB++;
                        Sleep(500);
                    }
                    
                }



                if (oldX == true)
                {
                    if (buttons & XINPUT_GAMEPAD_X && onoroff == true)
                    {
                        // keep posting?
                    }
                    else {
                        oldX = false;
                        PostKeyFunction(hwnd, Xtype, false);
                    }
                }
                else if (buttons & XINPUT_GAMEPAD_X && onoroff == true)
                {
                    oldX = true;
                    
                    
                    if (!scanoption)
                    { 
                        startsearch = startsearchX;
                        Buttonaction("\\X", mode, numphotoX, startsearch, false);
                    }
                    else
                    {
                        EnterCriticalSection(&critical);
                        POINT Cpoint;
                        Cpoint.x = PointX.x;
                        Cpoint.y = PointX.y;
                        LeaveCriticalSection(&critical);
                        movedmouse = true;
                        if (Cpoint.x != 0 && Cpoint.y != 0)
                        {
                            EnterCriticalSection(&critical);
                            startsearchX++;
                            
        
                            if (scanXtype == 0) //click and move
                            {
                                Xf = Cpoint.x;
                                Yf = Cpoint.y;
                                SendMouseClick(Cpoint.x, Cpoint.y, 8, 1);
                                Sleep(5);
                                SendMouseClick(Cpoint.x, Cpoint.y, 3, 2);
                                Sleep(5);
                                SendMouseClick(Cpoint.x, Cpoint.y, 4, 2);
                            }
                            else if (scanXtype == 1) //only move
                            {
                                Xf = Cpoint.x;
                                Yf = Cpoint.y;
                                movedmouse = true;
                               // MessageBoxA(NULL, "Resized/moved window - adjust fake cursor window", "Info", MB_OK);
                                //SendMouseClick(Cpoint.x, Cpoint.y, 8, 1);
                            }
                            else if (scanXtype == 2) //only click
                            {
                                SendMouseClick(Cpoint.x, Cpoint.y, 8, 1);
                                Sleep(5);
                                SendMouseClick(Cpoint.x, Cpoint.y, 3, 2);
                                Sleep(5);
                                SendMouseClick(Cpoint.x, Cpoint.y, 4, 2);
                                Sleep(5);
                                SendMouseClick(Xf, Yf, 8, 1);
                            }
                            LeaveCriticalSection(&critical);
                        }

                    }
                    if (foundit == false)
                    {
                        PostKeyFunction(hwnd, Xtype, true);
                    }
                    if (mode == 2 && showmessage != 11)
                    {
                        numphotoX++;
                        Sleep(500);
                    }
                }



                if (oldY == true)
                {
                    if (buttons & XINPUT_GAMEPAD_Y && onoroff == true)
                    {
                        // keep posting?
                    }
                    else {
                        oldY = false;
                        PostKeyFunction(hwnd, Ytype, false);
                    }
                }
                else if (buttons & XINPUT_GAMEPAD_Y && onoroff == true)
                {
                    oldY = true;
                    
                    
                    if (!scanoption) {
                        startsearch = startsearchY;
                        Buttonaction("\\Y", mode, numphotoY, startsearch, false);
                    }
                    else
                    {
                        EnterCriticalSection(&critical);
                        POINT Cpoint;
                        Cpoint.x = PointY.x;
                        Cpoint.y = PointY.y;
                        LeaveCriticalSection(&critical);
                        if (Cpoint.x != 0 && Cpoint.y != 0)
                        {
                            EnterCriticalSection(&critical);
                            startsearchY++;
                            
                            movedmouse = true;
        
                            if (scanYtype == 0 ) //click and move
                            { 
                                Xf = Cpoint.x;
                                Yf = Cpoint.y;
                                SendMouseClick(Cpoint.x, Cpoint.y, 8, 1);
                                Sleep(5);
                                SendMouseClick(Cpoint.x, Cpoint.y, 3, 2);
                                Sleep(5);
                                SendMouseClick(Cpoint.x, Cpoint.y, 4, 2);
                            }
                            else if (scanYtype == 1) //only move
                            {
                                Xf = Cpoint.x;
                                Yf = Cpoint.y;
                                //MessageBoxA(NULL, "Resized/moved window - adjust fake cursor window", "Info", MB_OK);
                                //SendMouseClick(Cpoint.x, Cpoint.y, 8, 1);
                            }
                            else if (scanYtype == 2) //only click
                            {
                                SendMouseClick(Cpoint.x, Cpoint.y, 8, 1);
                                Sleep(5);
                                SendMouseClick(Cpoint.x, Cpoint.y, 3, 2);
                                Sleep(5);
                                SendMouseClick(Cpoint.x, Cpoint.y, 4, 2);
                                Sleep(5);
                                SendMouseClick(Xf, Yf, 8, 1);
                            }
                            LeaveCriticalSection(&critical);
                        }
                    }
                    if (mode == 2 && showmessage != 11)
                    {
                        numphotoY++;
                        Sleep(500);
                    }
                    if (foundit == false)
                    {
                        PostKeyFunction(hwnd, Ytype, true);
                    }
                }



                if (oldC == true)
                {
                    if (buttons & XINPUT_GAMEPAD_RIGHT_SHOULDER && onoroff == true)
                    {
                        // keep posting?
                    }
                    else {
                        oldC = false;
                        PostKeyFunction(hwnd, Ctype, false);
                    }
                }
                else if (buttons & XINPUT_GAMEPAD_RIGHT_SHOULDER && onoroff == true)
                {
                    oldC = true;
                    if (ShoulderNextbmp == 1)
                    { 
                        EnterCriticalSection(&critical);
                        //if (startsearchA < NumPhotoA)
                            startsearchA++;
                        //if (startsearchB < NumPhotoB)
                            startsearchB++;
                        //if (startsearchX < NumPhotoX)
                            startsearchX++;
                        //if (startsearchY < NumPhotoY)
                            startsearchY++;
                        LeaveCriticalSection(&critical);
                    }
                    startsearch = startsearchC;
                    if (!scanoption)
                        Buttonaction("\\C", mode, numphotoC, startsearch, false);
                    if (foundit == false)
                    {
                        PostKeyFunction(hwnd, Ctype, true);
                    }
                    if (mode == 2 && showmessage == 0)
                    {
                        numphotoC++;
                        Sleep(500);
                    }
                }



                if (oldD == true)
                {
                    if (buttons & XINPUT_GAMEPAD_LEFT_SHOULDER && onoroff == true)
                    {
                        // keep posting?
                    }
                    else {
                        oldD = false;
                        PostKeyFunction(hwnd, Dtype, false);
                    }
                }
                else if (buttons & XINPUT_GAMEPAD_LEFT_SHOULDER && onoroff == true)
                {
                    oldD = true;
                    
                    startsearch = startsearchD;
                    if (!scanoption)
                        Buttonaction("\\D", mode, numphotoD, startsearch, false);
                    if (ShoulderNextbmp)
                    { 
                        EnterCriticalSection(&critical);
                        if (startsearchA > 0)
                        startsearchA--;
                        if (startsearchB > 0)
                        startsearchB--;
                        if (startsearchX > 0)
                        startsearchX--;
                        if (startsearchY > 0)
                        startsearchY--;
                        LeaveCriticalSection(&critical);
                    }
                    if (foundit == false)
                    {
                        PostKeyFunction(hwnd, Dtype, true);
                    }
                    if (mode == 2 && showmessage != 11)
                    {
                        numphotoD++;
                        Sleep(500);
                    }
                }




                if (oldE == true)
                {
                    if (buttons & XINPUT_GAMEPAD_RIGHT_THUMB && onoroff == true)
                    {
                        // keep posting?
                    }
                    else {
                        oldE = false;
                        PostKeyFunction(hwnd, Etype, false);
                    }
                }
                else if (buttons & XINPUT_GAMEPAD_RIGHT_THUMB && onoroff == true)
                {
                    oldE = true;
                    
                    startsearch = startsearchE;
                    if (!scanoption)
                        Buttonaction("\\E", mode, numphotoE, startsearch, false);
                    if (foundit == false)
                    {
                        PostKeyFunction(hwnd, Etype, true);
                    }
                    if (mode == 2 && showmessage != 11)
                    {
                        numphotoE++;
                        Sleep(500);
                    }
                }



                if (oldF == true)
                {
                    if (buttons & XINPUT_GAMEPAD_LEFT_THUMB && onoroff == true)
                    {
                        // keep posting?
                    }
                    else {
                        oldF = false;
                        PostKeyFunction(hwnd, Ftype, false);
                    }
                }
                else if (buttons & XINPUT_GAMEPAD_LEFT_THUMB && onoroff == true)
                {
                    oldF = true;
                    
                    startsearch = startsearchF;
                    if (!scanoption)
                        Buttonaction("\\F", mode, numphotoF, startsearch, false);
                    if (foundit == false)
                    {
                        PostKeyFunction(hwnd, Ftype, true);
                    }
                    if (mode == 2 && showmessage != 11)
                    {
                        numphotoF++;
                        Sleep(500);
                    }
                }
 



                if (oldup)
                {
                    if (buttons & XINPUT_GAMEPAD_DPAD_UP && onoroff == true)
                    {
                        //post keep?
                        if (scrolloutsidewindow >= 3 && quickMW == 1) {
                        
                            SendMouseClick(Xf, Yf, 20, 1);
                        
                        }
                    }
                    else {
                        oldup = false;
                        PostKeyFunction(hwnd, uptype, false);
                    }
                }
                else if (buttons & XINPUT_GAMEPAD_DPAD_UP && onoroff == true)
                {
                    
                    scroll.x = rect.left + (rect.right - rect.left) / 2;
                    if (scrolloutsidewindow == 0)
                        scroll.y = rect.top + 1;
                    if (scrolloutsidewindow == 1)
                        scroll.y = rect.top - 1;
                    scrollmap = true;
                    if (scrolloutsidewindow == 2){
                        oldup = true;
                        PostKeyFunction(hwnd, uptype, true);
                    }
                    if (scrolloutsidewindow >= 3) {
                        oldup = true;
                        scrollmap = false;
                    
                        SendMouseClick(Xf, Yf, 20, 1);
                     
                        
                    }
                }




                else if (olddown)
                {
                    if (buttons & XINPUT_GAMEPAD_DPAD_DOWN && onoroff == true)
                    {
                        //post keep?
                        if (scrolloutsidewindow >= 3 && quickMW == 1) {
                        
                            SendMouseClick(Xf, Yf, 21, 1);
                       
                        }
                    }
                    else {
                        olddown = false;
                        PostKeyFunction(hwnd, downtype, false);
                    }
                }
               else if (buttons & XINPUT_GAMEPAD_DPAD_DOWN && onoroff == true)
                {
                    
                    scroll.x = rect.left + (rect.right - rect.left) / 2;
                    if (scrolloutsidewindow == 0)
                        scroll.y = rect.bottom - 1;
                    if (scrolloutsidewindow == 1)
                        scroll.y = rect.bottom + 1;
                    scrollmap = true;
                    if (scrolloutsidewindow == 2) {
                        olddown = true;
                        PostKeyFunction(hwnd, downtype, true);
                    }
                    if (scrolloutsidewindow >= 3) {
                        olddown = true;
                        scrollmap = false;
               
                        SendMouseClick(Xf, Yf, 21, 1);
                   
                        
                    }
                }





               else if (oldleft)
               {
                   if (buttons & XINPUT_GAMEPAD_DPAD_LEFT && onoroff == true)
                   {
                       //post keep?
                   }
                   else {
                       oldleft = false;
                       PostKeyFunction(hwnd, lefttype, false);
                   }
               }
               else if (buttons & XINPUT_GAMEPAD_DPAD_LEFT && onoroff == true)
                {
                    if (scrolloutsidewindow == 0)
                        scroll.x = rect.left + 1;
                    if (scrolloutsidewindow == 1)
                        scroll.x = rect.left - 1;
                    scroll.y = rect.top + (rect.bottom - rect.top) / 2;

                    scrollmap = true;
                    if (scrolloutsidewindow == 2 || scrolloutsidewindow == 4) {
                        oldleft = true;
                        PostKeyFunction(hwnd, lefttype, true);
                    }

                }





                else if (oldright)
                {
                    if (buttons & XINPUT_GAMEPAD_DPAD_RIGHT && onoroff == true)
                    {
                        //post keep?
                    }
                    else {
                        oldright = false;
                        PostKeyFunction(hwnd, righttype, false);
                    }
                }
                else if (buttons & XINPUT_GAMEPAD_DPAD_RIGHT && onoroff == true)
                {
                    if (scrolloutsidewindow == 0)
                        scroll.x = rect.right - 1;
                    if (scrolloutsidewindow == 1)
                        scroll.x = rect.right + 1;
                    scroll.y = rect.top + (rect.bottom - rect.top) / 2;
                    scrollmap = true;
                    if (scrolloutsidewindow == 2 || scrolloutsidewindow == 4) {
                        oldright = true;
                        PostKeyFunction(hwnd, righttype, true);
                    }
                }
                else
                {
                    scrollmap = false;
                }




                if (buttons & XINPUT_GAMEPAD_START && showmessage == 0)
                {
                    Sleep(100);
                    if (onoroff == true && buttons & XINPUT_GAMEPAD_LEFT_SHOULDER && buttons & XINPUT_GAMEPAD_RIGHT_SHOULDER)
                    {
                         //MessageBox(NULL, "Bmp mode", "shutdown", MB_OK | MB_ICONINFORMATION);
                        showmessage = 69;
                    }
                    else if (onoroff == false && buttons & XINPUT_GAMEPAD_LEFT_SHOULDER && buttons & XINPUT_GAMEPAD_RIGHT_SHOULDER)
                    {
                         //MessageBox(NULL, "Bmp mode", "starting", MB_OK | MB_ICONINFORMATION);
                        showmessage = 70;
                    }
                    else if (mode == 0 && Modechange == 1 && onoroff == true)
                    {
                        mode = 1;

                       // MessageBox(NULL, "Bmp + Emulated cursor mode", "Move the flickering red dot and use right trigger for left click. left trigger for right click", MB_OK | MB_ICONINFORMATION);
                        showmessage = 2;
                    }
                    else if (mode == 1 && Modechange == 1 && onoroff == true)
                    {
                        mode = 2;
                        //MessageBox(NULL, "Edit Mode", "Button mapping. will map buttons you click with the flickering red dot as an input coordinate", MB_OK | MB_ICONINFORMATION);
                        showmessage = 3;
                        

                    }
                    else if (mode == 2 && Modechange == 1 && onoroff == true)
                    {
                       // mode = 0;
                       // MessageBox(NULL, "Bmp mode", "only send input on bmp match", MB_OK | MB_ICONINFORMATION);
                        showmessage = 1;
                    }

                    else if (onoroff == true) { //assume modechange not allowed. send escape key instead
                        keystatesend = VK_ESCAPE;
                    }
                   // Sleep(1000); //have time to release button. this is no hurry anyway
                    
                }
                if (mode > 0 && onoroff == true)
                { 
                    //fake cursor poll
                    int Xaxis = 0;
                    int Yaxis = 0;
					int scrollXaxis = 0;
					int scrollYaxis = 0;    
                    int width = rect.right - rect.left;
                    int height = rect.bottom - rect.top;
                    int Yscroll = 0;
                    int Xscroll = 0;
                    bool didscroll = false;


					
					 
                    if (righthanded == 1) {
                        Xaxis = state.Gamepad.sThumbRX;
                        Yaxis = state.Gamepad.sThumbRY;
                        scrollXaxis = state.Gamepad.sThumbLX;
						scrollYaxis = state.Gamepad.sThumbLY;   
                    }
                    else
                    {
                        Xaxis = state.Gamepad.sThumbLX;
                        Yaxis = state.Gamepad.sThumbLY;
						scrollXaxis = state.Gamepad.sThumbRX;   
						scrollYaxis = state.Gamepad.sThumbRY;   
                    }


                    if (scrolloutsidewindow == 2 || scrolloutsidewindow == 3 || scrolloutsidewindow == 4)
                    {
                        
                        if (oldscrollleftaxis) 
                        {
                            if (scrollXaxis < AxisLeftsens) //left
                            {
                                if (scrolloutsidewindow == 3)
                                { //keep
									scrollXaxis = scrollXaxis - AxisLeftsens; //zero input
                                    doscrollyes = true; 
                                    Xscroll = + scrollXaxis / scrollspeed3;
                                    didscroll = true;
                                }
                               // PostKeyFunction(hwnd, 42, true);
                            }
                            else 
                            { //stop
                                oldscrollleftaxis = false;
                                if (scrolloutsidewindow == 2)
                                    PostKeyFunction(hwnd, 42, false);
                                if (scrolloutsidewindow == 4)
                                    PostKeyFunction(hwnd, 12, false);
                            }
                        }
                        else if (scrollXaxis < AxisLeftsens) //left
                        {
                            if (scrolloutsidewindow == 2)
                                PostKeyFunction(hwnd, 42, true);
                            if (scrolloutsidewindow == 4)
                                PostKeyFunction(hwnd, 12, true);
                            if (scrolloutsidewindow == 3 && doscrollyes == false)
                            {//start
                                tick = 0;
                                SendMouseClick(Xf, Yf, 8, 1); 
                                SendMouseClick(Xf, Yf, 5, 2); //4 skal vere 3 
                            }
                            oldscrollleftaxis = true;
                            //keystatesend = VK_LEFT;
                        }



                        if (oldscrollrightaxis)
                        {
                            if (scrollXaxis > AxisRightsens) //right
                            {
                                if (scrolloutsidewindow == 3)
                                { //keep
                                    doscrollyes = true;
									scrollXaxis = scrollXaxis - AxisRightsens; //zero input   
                                    Xscroll = scrollXaxis / scrollspeed3;
                                    didscroll = true;
                                }
                            }
                            else {
                                oldscrollrightaxis = false;
                                if (scrolloutsidewindow == 2)
                                    PostKeyFunction(hwnd, 43, false);
                                if (scrolloutsidewindow == 4)
                                    PostKeyFunction(hwnd, 13, false);
                            }
                        }
                        else if (scrollXaxis > AxisRightsens) //right
                        {
                            if (scrolloutsidewindow == 2)
                                PostKeyFunction(hwnd, 43, true);
                            if (scrolloutsidewindow == 4)
                                PostKeyFunction(hwnd, 13, true);
                            if (scrolloutsidewindow == 3 && doscrollyes == false)
                            {//start
                                tick = 0;
                                SendMouseClick(Xf, Yf, 8, 1); 
                                SendMouseClick(Xf, Yf, 5, 2); //4 skal vere 3
                            }
                            oldscrollrightaxis = true;
                            //keystatesend = VK_RIGHT;

                        }



                        if (oldscrolldownaxis)
                        {
                            if (scrollYaxis < AxisDownsens)
                            {
                              //  PostKeyFunction(hwnd, 41, true);
                                if (scrolloutsidewindow == 3)
                                { //keep
                                    scrollYaxis = scrollYaxis - AxisDownsens; //zero input
                                    doscrollyes = true;
                                    Yscroll = scrollYaxis / scrollspeed3;
                                    didscroll = true;
                                }
                            }
                            else {
                                oldscrolldownaxis = false;
                                if (scrolloutsidewindow == 2)
                                    PostKeyFunction(hwnd, 41, false);
                                if (scrolloutsidewindow == 4)
                                    PostKeyFunction(hwnd, 11, false);
                            }
                        }
                        else if (scrollYaxis < AxisDownsens) //down
                        { //start
                            if (scrolloutsidewindow == 2)
                                PostKeyFunction(hwnd, 41, true);
                            if (scrolloutsidewindow == 4)
                                PostKeyFunction(hwnd, 11, true);
                            if (scrolloutsidewindow == 3 && doscrollyes == false)
                            {//start
                                tick = 0;
                                SendMouseClick(Xf, Yf, 8, 1); 
                                SendMouseClick(Xf, Yf, 5, 2); //4 skal vere 3
                            }
                            oldscrolldownaxis = true;
                        }





                        if (oldscrollupaxis)
                        {
                            if (scrollYaxis > AxisUpsens)
                            {
                               // PostKeyFunction(hwnd, 40, true);
                                if (scrolloutsidewindow == 3)
                                { //keep
									scrollYaxis = scrollYaxis - AxisUpsens; //zero input
                                    doscrollyes = true;
                                    
                                    Yscroll = scrollYaxis / scrollspeed3; //150
                                    didscroll = true;
                                }
                            }
                            else {
                                oldscrollupaxis = false;
                                if (scrolloutsidewindow == 2)
                                    PostKeyFunction(hwnd, 40, false);
                                if (scrolloutsidewindow == 4)
                                    PostKeyFunction(hwnd, 10, false);
                            }
                        }
                        else if (scrollYaxis > AxisUpsens) //up
                        {
                            if (scrolloutsidewindow == 2)
                                PostKeyFunction(hwnd, 40, true);
                            if (scrolloutsidewindow == 4)
                                PostKeyFunction(hwnd, 10, true);
                            if (scrolloutsidewindow == 3 && doscrollyes == false)
                            {//start
                                tick = 0;
                                SendMouseClick(Xf, Yf, 8, 1); 
                                SendMouseClick(Xf, Yf, 5, 2); //4 skal vere 3
                            }
                            oldscrollupaxis = true;
                        }   
                    }


                    //mouse left click and drag scrollfunction //scrolltype 3

                    if (doscrollyes) {
                        SendMouseClick(Xf + Xscroll, Yf - Yscroll, 8, 1); //4 skal vere 3
                        if (!didscroll && tick == scrollenddelay)
                        { 
							//MessageBox(NULL, "Scroll stopped", "Info", MB_OK | MB_ICONINFORMATION);
                            doscrollyes = false;
                            SendMouseClick(Xf, Yf, 6, 2); //4 skal vere 3
                        }
                    }



                    if (scrolloutsidewindow < 2 && scrollmap == false) //was 2
                    {
                        if (scrollXaxis < AxisLeftsens - 10000) //left
                        {
                            if (scrolloutsidewindow == 0)
                                scroll.x = rect.left + 1;
                            if (scrolloutsidewindow == 1)
                                scroll.x = rect.left - 1;
                            if (scrolloutsidewindow == 3)
                                scroll.x = (rect.left + (rect.right - rect.left) / 2) - 100;
                            scroll.y = rect.top + (rect.bottom - rect.top) / 2;

                            scrollmap = true;

                        }
                        else if (scrollXaxis > AxisRightsens + 10000) //right
                        {
                            if (scrolloutsidewindow == 0)
                                scroll.x = rect.right - 1;
                            if (scrolloutsidewindow == 1)
                                scroll.x = rect.right + 1;
                            if (scrolloutsidewindow == 3)
                                scroll.x = (rect.left + (rect.right - rect.left) / 2) + 100;
                            scroll.y = rect.top + (rect.bottom - rect.top) / 2;

                            scrollmap = true;

                        }
                        else if (scrollYaxis < AxisDownsens - 10000) //down
                        {
                            scroll.x = rect.left + (rect.right - rect.left) / 2;
                            if (scrolloutsidewindow == 0)
                                scroll.y = rect.bottom - 1;
                            if (scrolloutsidewindow == 1)
                                scroll.y = rect.bottom + 1;
                            if (scrolloutsidewindow == 3)
                                scroll.y = (rect.top + (rect.bottom - rect.top) / 2) + 100;
                            scrollmap = true;


                        }




                        else if (scrollYaxis > AxisUpsens + 10000) //up
                        {
                            scroll.x = rect.left + (rect.right - rect.left) / 2;
                            if (scrolloutsidewindow == 0)
                                scroll.y = rect.top + 1;
                            if (scrolloutsidewindow == 1)
                                scroll.y = rect.top - 1;
                            if (scrolloutsidewindow == 3)
                                scroll.y = (rect.top + (rect.bottom - rect.top) / 2) - 100;
                            scrollmap = true;
                        }   

                        else {
                            scrollmap = false;
                        }
                    }

                    POINT delta = CalculateUltimateCursorMove(
                        Xaxis, Yaxis,
                        radial_deadzone, axial_deadzone, max_threshold,
                        curve_slope, curve_exponent,
                        sensitivity, accel_multiplier
                    );


                    if (delta.x != 0 || delta.y != 0) {
                        Xf += delta.x;
                        Yf += delta.y;
                        movedmouse = true;
                    }
                    if (Xf < rect.left) Xf = rect.left;
                    if (Xf > rect.right) Xf = rect.right;
                    if (Yf < rect.top) Yf = rect.top;
                    if (Yf > rect.bottom) Yf = rect.bottom;

                    if (movedmouse == true) //fake cursor move message
                    {
                        if (userealmouse == 0)
                        {
                          //  if ( !leftPressed && !rightPressed)
                                //fakecursor.x = Xf;
                                //fakecursor.y = Yf;
                                SendMouseClick(Xf, Yf, 8, 1);
                         //   else if (leftPressed && !rightPressed)
                          //      SendMouseClick(fakecursor.x, fakecursor.y, 8, 1);
                        }
                    }
              
                if (leftPressed)
                { 

                    if (leftPressedold == false)
                    {
                     //save coordinates
                     startdrag.x = Xf;
                     startdrag.y = Yf;
                     leftPressedold = true;
                     if (userealmouse == 0 && scrolloutsidewindow == 3)
                     {
                         SendMouseClick(Xf, Yf, 5, 2); //4 skal vere 3
                         SendMouseClick(Xf, Yf, 6, 2); //double click
                     }
                     else if (userealmouse == 0)
                         SendMouseClick(Xf, Yf, 5, 2); //4 skal vere 3
                    }
                }
                if (leftPressedold)
                {
                    if (!leftPressed)
                    {
                        if (userealmouse == 0) {
                            

                                SendMouseClick(Xf, Yf, 6, 2); //double click
                           
                        }
                        else
                        {

                            if (abs(startdrag.x - Xf) <= 5)
                            {
                               
                                SendMouseClick(startdrag.x, startdrag.y, 2, 3); //4 4 move //5 release
                           
                            }
                            else
                            {
                               
                                SendMouseClick(startdrag.x, startdrag.y, 5, 2); //4 4 move //5 release
                                Sleep(30);
                                SendMouseClick(Xf, Yf, 8, 1);
                                Sleep(20);
                                SendMouseClick(Xf, Yf, 6, 2);
                            
                                
                            }
                        }
                        leftPressedold = false;
                    }   
                }
                if (rightPressed)
                {
                    if (rightPressedold == false)
                    {
                        //save coordinates
                        //start
                        if (hooksinited == false)
                            SetupHook();
                        startdrag.x = Xf;
                        startdrag.y = Yf;
                        rightPressedold = true;
                        if (userealmouse == 0)
                        {
                            DWORD currentTime = GetTickCount64();
                            if (currentTime - lastClickTime < GetDoubleClickTime() && movedmouse == false && doubleclicks == 1)
                            {
                                SendMouseClick(Xf, Yf, 30, 2); //4 skal vere 3
                                
                            }
                            else 
                            {
                                SendMouseClick(Xf, Yf, 3, 2); //4 skal vere 3
                                
                            }
                            lastClickTime = currentTime;
                            
						}
                    }

                        
                }
                if (rightPressedold)
                {
                    if (!rightPressed)
                    {
                        if (userealmouse == 0) 
                            SendMouseClick(Xf, Yf, 4, 2);
                        else
                        { 
                            if (abs(startdrag.x - Xf) <= 5)
                            {
                             
                                SendMouseClick(startdrag.x, startdrag.y, 1, 3); //4 4 move //5 release
                            
                            }
                            else
                            {
                         
                                SendMouseClick(startdrag.x, startdrag.y, 3, 2); //4 4 move //5 release
                                Sleep(30);
                                SendMouseClick(Xf, Yf, 8, 1); //4 skal vere 3
                                Sleep(20);
                                SendMouseClick(Xf, Yf, 4, 2);
                           
                            }
                        }
                        rightPressedold = false;
                    }
                } //rightpress
                } // mode above 0
            } //no controller
            else {
                showmessage = 12;
				//MessageBoxA(NULL, "Controller not connected", "Error", MB_OK | MB_ICONERROR);
               // CaptureWindow24Bit(hwnd, screenSize, largePixels, strideLarge, true); //draw message
            }
            if (drawfakecursor == 1 || showmessage != 0)
                CaptureWindow24Bit(hwnd, screenSize, largePixels, strideLarge, true); //draw fake cursor


        } // no hwnd
        if (knappsovetid > 20)
        {
          //  sovetid = 20;
          //  knappsovetid = 100;
            
		}

        if (showmessage != 0 && showmessage != 12)
        {
            counter++;
            if (counter > 500)
            {
                if (showmessage == 1) {
                    mode = 0;
                }
                if (showmessage == 69) { //disabling dll
                    onoroff = false;
                    MH_DisableHook(MH_ALL_HOOKS);
                }
                if (showmessage == 70) { //enabling dll
                    onoroff = true;
                    MH_EnableHook(MH_ALL_HOOKS);
                }
                showmessage = 0;
                counter = 0;

            }
        }
        //ticks for scroll end delay
        if (tick < scrollenddelay)
            tick++;
        
        if (mode == 0)
            Sleep(10);
        if (mode > 0) {
           // Sleep(sovetid); //15-80 //ini value
            if (movedmouse == true)
                Sleep(1 + responsetime ); //max 3. 0-2 on slow movement - calcsleep
            else Sleep(2); //max 3. 0-2 on slow movement
        }


    } //loop end but endless
    return 0;
}
//DWORD WINAPI ThreadFunction(LPVOID lpParam)
void RemoveHook() {
    MH_DisableHook(MH_ALL_HOOKS);
    MH_Uninitialize();
}



BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved) 
{
    switch (ul_reason_for_call) 
    {
        case DLL_PROCESS_ATTACH:
        {
            g_hModule = hModule;

            std::string iniPath = UGetExecutableFolder() + "\\Xinput.ini";
            std::string iniSettings = "Hooks";
            // std::string controllerID = getIniString(iniSettings.c_str(), "Controller ID", "0", iniPath);

             //hook settings
            clipcursorhook = GetPrivateProfileInt(iniSettings.c_str(), "ClipCursorHook", 0, iniPath.c_str());
            getkeystatehook = GetPrivateProfileInt(iniSettings.c_str(), "GetKeystateHook", 0, iniPath.c_str());
            getasynckeystatehook = GetPrivateProfileInt(iniSettings.c_str(), "GetAsynckeystateHook", 0, iniPath.c_str());
            getcursorposhook = GetPrivateProfileInt(iniSettings.c_str(), "GetCursorposHook", 0, iniPath.c_str());
            setcursorposhook = GetPrivateProfileInt(iniSettings.c_str(), "SetCursorposHook", 0, iniPath.c_str());
            setcursorhook = GetPrivateProfileInt(iniSettings.c_str(), "SetCursorHook", 0, iniPath.c_str()); 

            setrecthook = GetPrivateProfileInt(iniSettings.c_str(), "SetRectHook", 0, iniPath.c_str()); 
            leftrect = GetPrivateProfileInt(iniSettings.c_str(), "SetRectLeft", 0, iniPath.c_str());
            toprect = GetPrivateProfileInt(iniSettings.c_str(), "SetRectTop", 0, iniPath.c_str());
            rightrect = GetPrivateProfileInt(iniSettings.c_str(), "SetRectRight", 800, iniPath.c_str());
            bottomrect = GetPrivateProfileInt(iniSettings.c_str(), "SetRectBottom", 600, iniPath.c_str());

            int hooksoninit = GetPrivateProfileInt(iniSettings.c_str(), "hooksoninit", 1, iniPath.c_str());
            if (hooksoninit)
                {
               // WaitForInputIdle(GetCurrentProcess(), 1000); //wait for input to be ready
               // DisableThreadLibraryCalls(hModule);
                SetupHook();
			}
           InitializeCriticalSection(&critical);
           HANDLE one = CreateThread(nullptr, 0,
                (LPTHREAD_START_ROUTINE)ThreadFunction, g_hModule, 0, 0); //GetModuleHandle(0)
			CloseHandle(one);
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