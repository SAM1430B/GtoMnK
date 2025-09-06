#include "pch.h"
#include "Logging.h"
#include "Hooks.h"
#include "Mouse.h"
#include "Keyboard.h"
#include "ScreenshotInteract.h"

using namespace ScreenshotInput;

#pragma comment(lib, "Xinput9_1_0.lib")

// Global State Variables
HMODULE g_hModule = nullptr;
HWND hwnd = nullptr;
HANDLE hMutex = nullptr;
bool loop = true;
int showmessage = 0;
int counter = 0;
bool onoroff = true;
int mode = 0;
int controllerID = 0;

// Input state tracking
bool oldA = false, oldB = false, oldX = false, oldY = false;
bool oldC = false, oldD = false, oldE = false, oldF = false;
bool oldup = false, olddown = false, oldleft = false, oldright = false;
bool oldStart = false;
bool leftPressedold = false, rightPressedold = false;
bool oldscrollrightaxis = false, oldscrollleftaxis = false, oldscrollupaxis = false, oldscrolldownaxis = false;

// Image search state
bool foundit = false;
int startsearchA = 0, startsearchB = 0, startsearchX = 0, startsearchY = 0;
int startsearchC = 0, startsearchD = 0, startsearchE = 0, startsearchF = 0;

// Drawing & cursor state
bool pausedraw = false;
bool gotcursoryet = false;
bool scrollmap = false;
POINT scroll = { 0, 0 };
POINT startdrag = { 0, 0 };
POINT rectignore = { 0, 0 };
DWORD lastClickTime = 0;

// Data buffers for image processing
std::vector<BYTE> largePixels, smallPixels;
SIZE screenSize;
int strideLarge, strideSmall;
int smallW, smallH;

// INI Settings Variables
// Hooks
int clipcursorhook, getkeystatehook, getasynckeystatehook, getcursorposhook, setcursorposhook, setcursorhook, setrecthook;
int leftrect, toprect, rightrect, bottomrect;
// Controller
int righthanded, Xoffset, Yoffset;
float radial_deadzone, axial_deadzone, sensitivity, max_threshold, curve_slope, curve_exponent, accel_multiplier;
// Button Actions
int Atype, Btype, Xtype, Ytype, Ctype, Dtype, Etype, Ftype;
int bmpAtype, bmpBtype, bmpXtype, bmpYtype, bmpCtype, bmpDtype, bmpEtype, bmpFtype;
int uptype, downtype, lefttype, righttype, startbuttontype;
// General
int userealmouse, ignorerect, drawfakecursor, alwaysdrawcursor, doubleclicks, scrolloutsidewindow, responsetime, quickMW, scrollenddelay;
bool hooksinited = false;
int tick = 0;
bool doscrollyes = false;

// Helper Functions

HWND GetMainWindowHandle(DWORD targetPID) {
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
            return FALSE;
        }
        return TRUE;
        };

    EnumWindows(EnumWindowsCallback, reinterpret_cast<LPARAM>(&data));
    return data.hwnd;
}

std::string UGetExecutableFolder_main() {
    char path[MAX_PATH];
    GetModuleFileNameA(NULL, path, MAX_PATH);
    std::string exePath(path);
    size_t lastSlash = exePath.find_last_of("\\/");
    return exePath.substr(0, lastSlash);
}

std::string getShortenedPath_Manual(const std::string& fullPath)
{
    const char separator = '\\';
    size_t last_separator_pos = fullPath.find_last_of(separator);
    if (last_separator_pos == std::string::npos) {
        return fullPath;
    }
    size_t second_last_separator_pos = fullPath.find_last_of(separator, last_separator_pos - 1);

    if (second_last_separator_pos == std::string::npos) {
        return fullPath;
    }
    std::string shortened = fullPath.substr(second_last_separator_pos + 1);

    return "...\\" + shortened;
}

void LoadIniSettings() {
    std::string iniPath = UGetExecutableFolder_main() + "\\Xinput.ini";
    LOG("Reading settings from: %s", getShortenedPath_Manual(iniPath).c_str());

    // Hooks Section
    clipcursorhook = GetPrivateProfileIntA("Hooks", "ClipCursorHook", 0, iniPath.c_str());
    getkeystatehook = GetPrivateProfileIntA("Hooks", "GetKeystateHook", 0, iniPath.c_str());
    getasynckeystatehook = GetPrivateProfileIntA("Hooks", "GetAsynckeystateHook", 0, iniPath.c_str());
    getcursorposhook = GetPrivateProfileIntA("Hooks", "GetCursorposHook", 1, iniPath.c_str());
    setcursorposhook = GetPrivateProfileIntA("Hooks", "SetCursorposHook", 1, iniPath.c_str());
    setcursorhook = GetPrivateProfileIntA("Hooks", "SetCursorHook", 1, iniPath.c_str());
    setrecthook = GetPrivateProfileIntA("Hooks", "SetRectHook", 0, iniPath.c_str());
    leftrect = GetPrivateProfileIntA("Hooks", "SetRectLeft", 0, iniPath.c_str());
    toprect = GetPrivateProfileIntA("Hooks", "SetRectTop", 0, iniPath.c_str());
    rightrect = GetPrivateProfileIntA("Hooks", "SetRectRight", 800, iniPath.c_str());
    bottomrect = GetPrivateProfileIntA("Hooks", "SetRectBottom", 600, iniPath.c_str());

    // Settings Section
    controllerID = GetPrivateProfileIntA("Settings", "Controllerid", 0, iniPath.c_str());
    righthanded = GetPrivateProfileIntA("Settings", "Righthanded", 0, iniPath.c_str());
    Xoffset = GetPrivateProfileIntA("Settings", "Xoffset", 0, iniPath.c_str());
    Yoffset = GetPrivateProfileIntA("Settings", "Yoffset", 0, iniPath.c_str());
    mode = GetPrivateProfileIntA("Settings", "Initial Mode", 1, iniPath.c_str());
    userealmouse = GetPrivateProfileIntA("Settings", "UseRealMouse", 0, iniPath.c_str());
    ignorerect = GetPrivateProfileIntA("Settings", "IgnoreRect", 0, iniPath.c_str());
    drawfakecursor = GetPrivateProfileIntA("Settings", "DrawFakeCursor", 1, iniPath.c_str());
    alwaysdrawcursor = GetPrivateProfileIntA("Settings", "DrawFakeCursorAlways", 1, iniPath.c_str());
    doubleclicks = GetPrivateProfileIntA("Settings", "Doubleclicks", 0, iniPath.c_str());
    scrolloutsidewindow = GetPrivateProfileIntA("Settings", "Scrollmapfix", 1, iniPath.c_str());
    responsetime = GetPrivateProfileIntA("Settings", "Responsetime", 0, iniPath.c_str());
    quickMW = GetPrivateProfileIntA("Settings", "MouseWheelContinous", 0, iniPath.c_str());
    scrollenddelay = GetPrivateProfileIntA("Settings", "DelayEndScroll", 50, iniPath.c_str());

    char buffer[256];
    GetPrivateProfileStringA("Settings", "Radial_Deadzone", "0.1", buffer, sizeof(buffer), iniPath.c_str()); radial_deadzone = std::stof(buffer);
    GetPrivateProfileStringA("Settings", "Axial_Deadzone", "0.0", buffer, sizeof(buffer), iniPath.c_str()); axial_deadzone = std::stof(buffer);
    GetPrivateProfileStringA("Settings", "Sensitivity", "15.0", buffer, sizeof(buffer), iniPath.c_str()); sensitivity = std::stof(buffer);
    GetPrivateProfileStringA("Settings", "Max_Threshold", "0.03", buffer, sizeof(buffer), iniPath.c_str()); max_threshold = std::stof(buffer);
	GetPrivateProfileStringA("Settings", "Curve_Slope", "0.16", buffer, sizeof(buffer), iniPath.c_str()); curve_slope = std::stof(buffer);
	GetPrivateProfileStringA("Settings", "Curve_Exponent", "5.0", buffer, sizeof(buffer), iniPath.c_str()); curve_exponent = std::stof(buffer);
    GetPrivateProfileStringA("Settings", "Accel_Multiplier", "1.7", buffer, sizeof(buffer), iniPath.c_str()); accel_multiplier = std::stof(buffer);

    // --- UPDATED BUTTON MAPPINGS ---
    // All defaults are now 0, meaning "unassigned".
    Atype = GetPrivateProfileIntA("Settings", "Ainputtype", 0, iniPath.c_str());
    Btype = GetPrivateProfileIntA("Settings", "Binputtype", 0, iniPath.c_str());
    Xtype = GetPrivateProfileIntA("Settings", "Xinputtype", 0, iniPath.c_str());
    Ytype = GetPrivateProfileIntA("Settings", "Yinputtype", 0, iniPath.c_str());
    Ctype = GetPrivateProfileIntA("Settings", "Cinputtype", 0, iniPath.c_str()); // Right Shoulder
    Dtype = GetPrivateProfileIntA("Settings", "Dinputtype", 0, iniPath.c_str()); // Left Shoulder
    Etype = GetPrivateProfileIntA("Settings", "Einputtype", 0, iniPath.c_str()); // Right Thumb
    Ftype = GetPrivateProfileIntA("Settings", "Finputtype", 0, iniPath.c_str()); // Left Thumb

    uptype = GetPrivateProfileIntA("Settings", "Upkey", 0, iniPath.c_str());
    downtype = GetPrivateProfileIntA("Settings", "Downkey", 0, iniPath.c_str());
    lefttype = GetPrivateProfileIntA("Settings", "Leftkey", 0, iniPath.c_str());
    righttype = GetPrivateProfileIntA("Settings", "Rightkey", 0, iniPath.c_str());
    startbuttontype = GetPrivateProfileIntA("Settings", "Startbuttonkey", 3, iniPath.c_str()); // Defaults to ESC, can be set to 0 to disable

    // BMP action types (move only, click only, etc.)
    bmpAtype = GetPrivateProfileIntA("Settings", "AbmpAction", 0, iniPath.c_str());
    bmpBtype = GetPrivateProfileIntA("Settings", "BbmpAction", 0, iniPath.c_str());
    bmpXtype = GetPrivateProfileIntA("Settings", "XbmpAction", 0, iniPath.c_str());
    bmpYtype = GetPrivateProfileIntA("Settings", "YbmpAction", 0, iniPath.c_str());
    bmpCtype = GetPrivateProfileIntA("Settings", "CbmpAction", 0, iniPath.c_str());
    bmpDtype = GetPrivateProfileIntA("Settings", "DbmpAction", 0, iniPath.c_str());
    bmpEtype = GetPrivateProfileIntA("Settings", "EbmpAction", 0, iniPath.c_str());
    bmpFtype = GetPrivateProfileIntA("Settings", "FbmpAction", 0, iniPath.c_str());
}

POINT CalculateUltimateCursorMove(SHORT stickX, SHORT stickY) {
    static double mouseDeltaAccumulatorX = 0.0;
    static double mouseDeltaAccumulatorY = 0.0;

    double normX = static_cast<double>(stickX) / 32767.0;
    double normY = static_cast<double>(stickY) / 32767.0;
    double magnitude = std::sqrt(normX * normX + normY * normY);

    if (magnitude < radial_deadzone) return { 0, 0 };
    if (std::abs(normX) < axial_deadzone) normX = 0.0;
    if (std::abs(normY) < axial_deadzone) normY = 0.0;

    magnitude = std::sqrt(normX * normX + normY * normY);
    if (magnitude < 1e-6) return { 0, 0 };

    double effectiveRange = 1.0 - max_threshold - radial_deadzone;
    if (effectiveRange < 1e-6) effectiveRange = 1.0;

    double remappedMagnitude = (magnitude - radial_deadzone) / effectiveRange;
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

bool IsTriggerPressed(BYTE triggerValue) {
    return triggerValue > 175;
}

int CountExistingBmps(const std::string& buttonPrefix) {
    std::string folderPath = UGetExecutableFolder_main();
    for (int i = 0; i < 50; ++i) { // Check for up to 50 files
        std::string filePathStr = folderPath + "\\" + buttonPrefix + std::to_string(i) + ".bmp";
        std::wstring filePathWstr(filePathStr.begin(), filePathStr.end());

        HBITMAP hbm = (HBITMAP)LoadImageW(NULL, filePathWstr.c_str(), IMAGE_BITMAP, 0, 0, LR_LOADFROMFILE | LR_CREATEDIBSECTION);

        if (hbm) {
            DeleteObject(hbm);
        }
        else {
            LOG("Found %d BMPs for button %s.", i, buttonPrefix.c_str());
            return i;
        }
    }
    LOG("Found maximum (50) BMPs for button %s.", buttonPrefix.c_str());
    return 50; // Return max if all 50 exist
}

DWORD WINAPI ThreadFunction(LPVOID lpParam) {
    LOG("ThreadFunction started.");
    Sleep(2000);

    LoadIniSettings();

    Hooks::SetupHooks();
    hooksinited = true;

    hwnd = GetMainWindowHandle(GetCurrentProcessId());

    int numphotoA = 0, numphotoB = 0, numphotoX = 0, numphotoY = 0, numphotoC = 0, numphotoD = 0, numphotoE = 0, numphotoF = 0;

    LOG("Checking for existing BMP map files...");
    numphotoA = CountExistingBmps("A");
    numphotoB = CountExistingBmps("B");
    numphotoX = CountExistingBmps("X");
    numphotoY = CountExistingBmps("Y");
    numphotoC = CountExistingBmps("C");
    numphotoD = CountExistingBmps("D");
    numphotoE = CountExistingBmps("E");
    numphotoF = CountExistingBmps("F");

	// Loop to read controller state and perform actions
    while (loop) {
        bool movedmouse = false;
        foundit = false;
        Keyboard::keystatesend = 0;

        if (!hwnd) {
            hwnd = GetMainWindowHandle(GetCurrentProcessId());
            Sleep(1000);
            continue;
        }

        RECT rect;
        GetClientRect(hwnd, &rect);

        XINPUT_STATE state;
        if (XInputGetState(controllerID, &state) == ERROR_SUCCESS) {
            if (showmessage == 12) showmessage = 0; // Clear "disconnected" message

            WORD buttons = state.Gamepad.wButtons;

            //Button Logic (A, B, X, Y, etc.)
            if (!oldA && (buttons & XINPUT_GAMEPAD_A) && onoroff) {
                oldA = true;
                if (!ScreenshotInteract::Buttonaction("\\A", mode, numphotoA, startsearchA)) {
                    Keyboard::PostKeyFunction(Atype, true);
                }
                if (mode == 2 && showmessage != 11) { numphotoA++; Sleep(500); }
            }
            else if (oldA && !(buttons & XINPUT_GAMEPAD_A)) {
                oldA = false;
                Keyboard::PostKeyFunction(Atype, false);
            }

            if (!oldB && (buttons & XINPUT_GAMEPAD_B) && onoroff) {
                oldB = true;
                if (!ScreenshotInteract::Buttonaction("\\B", mode, numphotoB, startsearchB)) {
                    Keyboard::PostKeyFunction(Btype, true);
                }
                if (mode == 2 && showmessage != 11) { numphotoB++; Sleep(500); }
            }
            else if (oldB && !(buttons & XINPUT_GAMEPAD_B)) {
                oldB = false;
                Keyboard::PostKeyFunction(Btype, false);
            }

            if (!oldX && (buttons & XINPUT_GAMEPAD_X) && onoroff) {
                oldX = true;
                if (!ScreenshotInteract::Buttonaction("\\X", mode, numphotoX, startsearchX)) {
                    Keyboard::PostKeyFunction(Xtype, true);
                }
                if (mode == 2 && showmessage != 11) { numphotoX++; Sleep(500); }
            }
            else if (oldX && !(buttons & XINPUT_GAMEPAD_X)) {
                oldX = false;
                Keyboard::PostKeyFunction(Xtype, false);
			}

            if (!oldY && (buttons & XINPUT_GAMEPAD_Y) && onoroff) {
                oldY = true;
                if (!ScreenshotInteract::Buttonaction("\\Y", mode, numphotoY, startsearchY)) {
                    Keyboard::PostKeyFunction(Ytype, true);
                }
                if (mode == 2 && showmessage != 11) { numphotoY++; Sleep(500); }
            }
            else if (oldY && !(buttons & XINPUT_GAMEPAD_Y)) {
                oldY = false;
                Keyboard::PostKeyFunction(Ytype, false);
			}

            if (!oldC && (buttons & XINPUT_GAMEPAD_RIGHT_SHOULDER) && onoroff) {
                oldC = true;
                if (!ScreenshotInteract::Buttonaction("\\C", mode, numphotoC, startsearchC)) {
                    Keyboard::PostKeyFunction(Ctype, true);
                }
                if (mode == 2 && showmessage != 11) { numphotoC++; Sleep(500); }
            }
            else if (oldC && !(buttons & XINPUT_GAMEPAD_RIGHT_SHOULDER)) {
                oldC = false;
                Keyboard::PostKeyFunction(Ctype, false);
            }

            if (!oldD && (buttons & XINPUT_GAMEPAD_LEFT_SHOULDER) && onoroff) {
                oldD = true;
                if (!ScreenshotInteract::Buttonaction("\\D", mode, numphotoD, startsearchD)) {
                    Keyboard::PostKeyFunction(Dtype, true);
                }
                if (mode == 2 && showmessage != 11) { numphotoD++; Sleep(500); }
            }
            else if (oldD && !(buttons & XINPUT_GAMEPAD_LEFT_SHOULDER)) {
                oldD = false;
                Keyboard::PostKeyFunction(Dtype, false);
			}

            if (!oldE && (buttons & XINPUT_GAMEPAD_RIGHT_THUMB) && onoroff) {
                oldE = true;
                if (!ScreenshotInteract::Buttonaction("\\E", mode, numphotoE, startsearchE)) {
                    Keyboard::PostKeyFunction(Etype, true);
                }
                if (mode == 2 && showmessage != 11) { numphotoE++; Sleep(500); }
            }
            else if (oldE && !(buttons & XINPUT_GAMEPAD_RIGHT_THUMB)) {
                oldE = false;
                Keyboard::PostKeyFunction(Etype, false);
            }

            if (!oldF && (buttons & XINPUT_GAMEPAD_LEFT_THUMB) && onoroff) {
                oldF = true;
                if (!ScreenshotInteract::Buttonaction("\\F", mode, numphotoF, startsearchF)) {
                    Keyboard::PostKeyFunction(Ftype, true);
                }
                if (mode == 2 && showmessage != 11) { numphotoF++; Sleep(500); }
            }
            else if (oldF && !(buttons & XINPUT_GAMEPAD_LEFT_THUMB)) {
                oldF = false;
                Keyboard::PostKeyFunction(Ftype, false);
			}

            //DPAD Logic

            if (!oldup && (buttons & XINPUT_GAMEPAD_DPAD_UP) && onoroff)
            {
                oldup = true;
                if (scrolloutsidewindow == 2 || scrolloutsidewindow == 4) {
                    Keyboard::PostKeyFunction(uptype, true);
                }
                else if (scrolloutsidewindow < 2) {
                    scroll.x = rect.left + (rect.right - rect.left) / 2;
                    scroll.y = (scrolloutsidewindow == 0) ? rect.top + 1 : rect.top - 1;
                    scrollmap = true;
                }
            }
            else if (oldup && !(buttons & XINPUT_GAMEPAD_DPAD_UP))
            {
                oldup = false;
                scrollmap = false;
                if (scrolloutsidewindow == 2 || scrolloutsidewindow == 4) {
                    Keyboard::PostKeyFunction(uptype, false);
                }
            }

            if (!olddown && (buttons & XINPUT_GAMEPAD_DPAD_DOWN) && onoroff)
            {
                olddown = true;
                if (scrolloutsidewindow == 2 || scrolloutsidewindow == 4) {
                    Keyboard::PostKeyFunction(downtype, true);
                }
                else if (scrolloutsidewindow < 2) {
                    scroll.x = rect.left + (rect.right - rect.left) / 2;
                    scroll.y = (scrolloutsidewindow == 0) ? rect.bottom - 1 : rect.bottom + 1;
                    scrollmap = true;
                }
            }
            else if (olddown && !(buttons & XINPUT_GAMEPAD_DPAD_DOWN))
            {
                olddown = false;
                scrollmap = false;
                if (scrolloutsidewindow == 2 || scrolloutsidewindow == 4) {
                    Keyboard::PostKeyFunction(downtype, false);
                }
			}

            if (!oldleft && (buttons & XINPUT_GAMEPAD_DPAD_LEFT) && onoroff)
            {
                oldleft = true;
                if (scrolloutsidewindow == 2 || scrolloutsidewindow == 4) {
                    Keyboard::PostKeyFunction(lefttype, true);
                }
                else if (scrolloutsidewindow < 2) {
                    scroll.x = (scrolloutsidewindow == 0) ? rect.left + 1 : rect.left - 1;
                    scroll.y = rect.top + (rect.bottom - rect.top) / 2;
                    scrollmap = true;
                }
            }
            else if (oldleft && !(buttons & XINPUT_GAMEPAD_DPAD_LEFT))
            {
                oldleft = false;
                scrollmap = false;
                if (scrolloutsidewindow == 2 || scrolloutsidewindow == 4) {
                    Keyboard::PostKeyFunction(lefttype, false);
                }
            }

            if (!oldright && (buttons & XINPUT_GAMEPAD_DPAD_RIGHT) && onoroff)
            {
                oldright = true;
                if (scrolloutsidewindow == 2 || scrolloutsidewindow == 4) {
                    Keyboard::PostKeyFunction(righttype, true);
                }
                else if (scrolloutsidewindow < 2) {
                    scroll.x = (scrolloutsidewindow == 0) ? rect.right - 1 : rect.right + 1;
                    scroll.y = rect.top + (rect.bottom - rect.top) / 2;
                    scrollmap = true;
                }
            }
            else if (oldright && !(buttons & XINPUT_GAMEPAD_DPAD_RIGHT))
            {
                oldright = false;
                scrollmap = false;
                if (scrolloutsidewindow == 2 || scrolloutsidewindow == 4) {
                    Keyboard::PostKeyFunction(righttype, false);
                }
            }

            // START Button Logic
            if (!oldStart && (buttons & XINPUT_GAMEPAD_START) && onoroff)
            {
                oldStart = true;

                if (showmessage == 0) {
                    int Modechange = GetPrivateProfileIntA("Settings", "Allow modechange", 1, (UGetExecutableFolder_main() + "\\Xinput.ini").c_str());

                    if ((buttons & XINPUT_GAMEPAD_LEFT_SHOULDER) && (buttons & XINPUT_GAMEPAD_RIGHT_SHOULDER)) {
                        showmessage = onoroff ? 69 : 70; // Toggle On/Off
                    }
                    else if (Modechange == 1) {
                        if (mode == 0) { mode = 1; showmessage = 2; }
                        else if (mode == 1) { mode = 2; showmessage = 3; }
                        else if (mode == 2) { mode = 0; showmessage = 1; }
                    }
                    else {
                        Keyboard::PostKeyFunction(startbuttontype, true);
                        Keyboard::PostKeyFunction(startbuttontype, false);
                    }
                }
            }
            else if (oldStart && !(buttons & XINPUT_GAMEPAD_START))
            {
                oldStart = false;
            }

            // Cursor Movement Logic
            if (mode > 0 && onoroff) {
                SHORT thumbX = (righthanded) ? state.Gamepad.sThumbRX : state.Gamepad.sThumbLX;
                SHORT thumbY = (righthanded) ? state.Gamepad.sThumbRY : state.Gamepad.sThumbLY;

                POINT delta = CalculateUltimateCursorMove(thumbX, thumbY);
                if (delta.x != 0 || delta.y != 0) {
                    Mouse::Xf += delta.x;
                    Mouse::Yf += delta.y;
                    Mouse::Xf = std::max((LONG)rect.left, std::min((LONG)Mouse::Xf, (LONG)rect.right));
                    Mouse::Yf = std::max((LONG)rect.top, std::min((LONG)Mouse::Yf, (LONG)rect.bottom));

                    movedmouse = true;
                    POINT screenPos = { (LONG)Mouse::Xf, (LONG)Mouse::Yf };
                    ClientToScreen(hwnd, &screenPos);
                    Mouse::SendMouseClick(screenPos.x, screenPos.y, 8, 1); // WM_MOUSEMOVE
                }

                //Trigger (Mouse Click) Logic
                bool rightPressed = IsTriggerPressed(state.Gamepad.bRightTrigger);
                if (rightPressed && !rightPressedold) {
                    rightPressedold = true;
                    startdrag = { Mouse::Xf, Mouse::Yf };
                    DWORD currentTime = GetTickCount64();
                    ClientToScreen(hwnd, &startdrag);

                    if (doubleclicks && (currentTime - lastClickTime < GetDoubleClickTime()) && !movedmouse) {
                        Mouse::SendMouseClick(startdrag.x, startdrag.y, 30, 2); // Double click
                    }
                    else {
                        Mouse::SendMouseClick(startdrag.x, startdrag.y, 3, 2); // Left down
                    }
                    lastClickTime = currentTime;
                }
                else if (!rightPressed && rightPressedold) {
                    rightPressedold = false;
                    POINT screenPos = { Mouse::Xf, Mouse::Yf }; ClientToScreen(hwnd, &screenPos);
                    Mouse::SendMouseClick(screenPos.x, screenPos.y, 4, 2); // Left up
                }

                bool leftPressed = IsTriggerPressed(state.Gamepad.bLeftTrigger);
                if (leftPressed && !leftPressedold) {
                    leftPressedold = true;
                    POINT currentPos = { Mouse::Xf, Mouse::Yf }; ClientToScreen(hwnd, &currentPos);
                    Mouse::SendMouseClick(currentPos.x, currentPos.y, 5, 2); // Right down
                }
                else if (!leftPressed && leftPressedold) {
                    leftPressedold = false;
                    POINT currentPos = { Mouse::Xf, Mouse::Yf }; ClientToScreen(hwnd, &currentPos);
                    Mouse::SendMouseClick(currentPos.x, currentPos.y, 6, 2); // Right up
                }
            }

        }
        else {
            showmessage = 12; // Controller disconnected
        }

        // Drawing and Message Handling
        if ((drawfakecursor == 1 && onoroff) || showmessage != 0) {
            if (!pausedraw) {
                HBITMAP hbm = ScreenshotInteract::CaptureWindow(true);
                if (hbm) DeleteObject(hbm);
            }
        }

        if (showmessage != 0 && showmessage != 12) {
            if (++counter > 300) {
                if (showmessage == 1) mode = 0;
                if (showmessage == 69) { onoroff = false; MH_DisableHook(MH_ALL_HOOKS); }
                if (showmessage == 70) { onoroff = true; MH_EnableHook(MH_ALL_HOOKS); }
                showmessage = 0;
                counter = 0;
            }
        }

        if (mode == 0) Sleep(20);
        else Sleep(responsetime + 1);

    } 
    return 0;
}

DWORD WINAPI SafeThreadLauncher(LPVOID hModule) {
    HANDLE hThread = CreateThread(nullptr, 0, ThreadFunction, hModule, 0, nullptr);
    if (hThread) {
        LOG("Main worker thread created successfully.");
        CloseHandle(hThread);
    }
    else {
        LOG("FATAL: Failed to create main worker thread! GetLastError() = %lu", GetLastError());
    }
    return 0;
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved) {
    switch (ul_reason_for_call) {
    case DLL_PROCESS_ATTACH: {
        g_hModule = hModule;
        DisableThreadLibraryCalls(hModule);

        INIT_LOGGER();
        LOG("================== DLL Attached (PID: %lu) ==================", GetCurrentProcessId());

        HANDLE hLauncher = CreateThread(nullptr, 0, SafeThreadLauncher, hModule, 0, nullptr);
        if (hLauncher) {
            CloseHandle(hLauncher);
        }
        else {
            LOG("FATAL: Failed to create the launcher thread! GetLastError() = %lu", GetLastError());
        }
        break;
    }

    case DLL_PROCESS_DETACH: {
        if (lpReserved == nullptr) {
            LOG("DLL Detaching gracefully.");
            loop = false;
        }
        else {
            LOG("DLL detaching due to process termination.");
        }

        if (hooksinited) {
            Hooks::RemoveHooks();
            LOG("Hooks removed.");
        }
        SHUTDOWN_LOGGER();
        break;
    }
    }
    return TRUE;
}