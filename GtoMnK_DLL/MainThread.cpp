#include "pch.h"
#include "Logging.h"
#include "Hooks.h"
#include "Mouse.h"
#include "Keyboard.h"
#include "InputState.h"
#include "RawInput.h"
#include "MainThread.h"
#include "FakeCursor.h"
#include "OverlayMenu.h"

using namespace GtoMnK;

//#pragma comment(lib, "Xinput9_1_0.lib")

extern "C" BOOL WINAPI OpenXinputDllMain(HINSTANCE hInstDll, DWORD fdwReason, LPVOID lpvReserved);

// For Initialization and Thread state
HMODULE g_hModule = nullptr;
bool hooksinited = false;
bool loop = true;
int startUpDelay = 0;
bool recheckHWND = true;
bool disableOverlayOptions = false;

// For Controller Input
std::string A_Action, B_Action, X_Action, Y_Action;
std::string RB_Action, LB_Action, RSB_Action, LSB_Action;
std::string D_UP_Action, D_DOWN_Action, D_LEFT_Action, D_RIGHT_Action;
std::string Start_Action, Back_Action;
std::string LT_Action, RT_Action;

enum CustomInputID {
    CUSTOM_ID_LT = 0x10000,
    CUSTOM_ID_RT,
    CUSTOM_ID_LSU,
    CUSTOM_ID_LSD,
    CUSTOM_ID_LSL,
    CUSTOM_ID_LSR,
    CUSTOM_ID_RSU,
    CUSTOM_ID_RSD,
    CUSTOM_ID_RSL,
    CUSTOM_ID_RSR
};

// For Controller Button States
const size_t NO_ACTION = static_cast<size_t>(-1);
std::map<UINT, ButtonState> buttonStates;
bool g_EnableMouseDoubleClick = false;
int righthanded, Xoffset, Yoffset;
int keystatesend = 0;

// For Controller Setting
int controllerID = 0;
float radial_deadzone, axial_deadzone, sensitivity, max_threshold, curve_slope, curve_exponent, sensitivity_multiplier, horizontal_sensitivity, vertical_sensitivity, look_accel_multiplier;
float stick_as_button_deadzone;
float g_TriggerThreshold = 175;

// Others
HWND hwnd = nullptr;
HANDLE hMutex = nullptr;
int responsetime = 4;

// Drawing & cursor state
int drawfakecursor = 0;
int drawProtoFakeCursor = 0; // From ProtoInput also for the Cursor visibility hooks
int createdWindowIsOwned = 1;
int ShowProtoCursorWhenImageUpdated = 1; // From ProtoInput
int mode = 1;
bool onoroff = true;
int showmessage = 0;
int counter = 0;
bool pausedraw = false;

// For the Hooks
InputMethod g_InputMethod = InputMethod::PostMessage;
int getCursorPosHook, setCursorPosHook, clipCursorHook, getKeyStateHook, getAsyncKeyStateHook, getKeyboardStateHook, setCursorHook, setRectHook;

// For MessageFilterHook
bool g_filterRawInput = false;
bool g_filterMouseMove = false;
bool g_filterMouseActivate = false;
bool g_filterWindowActivate = false;
bool g_filterWindowActivateApp = false;
bool g_filterMouseWheel = false;
bool g_filterMouseButton = false;
bool g_filterKeyboardButton = false;

// For SetRectHook and AdjustWindowRectHook
int leftrect, toprect, rightrect, bottomrect;

// For finding the game window
char iniWindowName[256] = { 0 };
char iniClassName[256] = { 0 };

// General
//bool gotcursoryet = false;
//POINT startdrag = { 0, 0 };
//DWORD lastClickTime = 0;
//int userealmouse;
//int alwaysdrawcursor;
//int doubleclicks;
//int scrolloutsidewindow;
//int quickMW;
//int scrollenddelay;
//int tick = 0;
//bool doscrollyes = false;
//POINT scroll = { 0, 0 };
//bool scrollmap = false;
//POINT rectignore = { 0, 0 };
//int ignorerect;


HWND GetMainWindowHandle(DWORD targetPID, const char* requiredName, const char* requiredClass, DWORD timeoutMS) {
    struct HandleData {
        DWORD pid;
        HWND bestHandle;
        const char* reqName;
        const char* reqClass;
    };

    ULONGLONG startTime = GetTickCount64();

    while (true) {
        HandleData data = { targetPID, nullptr, requiredName, requiredClass };

        auto EnumWindowsCallback = [](HWND hWnd, LPARAM lParam) -> BOOL {
            HandleData* pData = reinterpret_cast<HandleData*>(lParam);
            DWORD windowPID = 0;
            GetWindowThreadProcessId(hWnd, &windowPID);

            if (windowPID != pData->pid) return TRUE;

            if (!IsWindowVisible(hWnd)) return TRUE;
            if (hWnd == FakeCursor::GetPointerWindow()) return TRUE;

            bool usingStrictFilters = (pData->reqName && pData->reqName[0]) ||
                (pData->reqClass && pData->reqClass[0]);

            if (usingStrictFilters) {

                if (pData->reqClass && pData->reqClass[0] != '\0') {
                    char className[256];
                    GetClassNameA(hWnd, className, sizeof(className));
                    if (strcmp(className, pData->reqClass) != 0) return TRUE;
                }

                if (pData->reqName && pData->reqName[0] != '\0') {
                    char windowName[256];
                    GetWindowTextA(hWnd, windowName, sizeof(windowName));
                    if (strcmp(windowName, pData->reqName) != 0) return TRUE;
                }
            }
            else {
                if (GetWindowLongPtr(hWnd, GWL_EXSTYLE) & WS_EX_TOOLWINDOW) return TRUE;
                if (GetWindow(hWnd, GW_OWNER) != (HWND)0) return TRUE;
            }

            pData->bestHandle = hWnd;
            return FALSE;
            };

        EnumWindows(EnumWindowsCallback, reinterpret_cast<LPARAM>(&data));

        if (data.bestHandle != nullptr) {
            return data.bestHandle;
        }

        if ((GetTickCount64() - startTime) >= timeoutMS) {
            break;
        }
        Sleep(500);
    }

    return nullptr;
}

// UGetExecutableFolder_main is for getting the executable folder path
std::string UGetExecutableFolder_main() {
    char path[MAX_PATH];
    GetModuleFileNameA(NULL, path, MAX_PATH);
    std::string exePath(path);
    size_t lastSlash = exePath.find_last_of("\\/");
    return exePath.substr(0, lastSlash);
}

// UGetDllFolder_main is for getting the DLL folder path
std::string UGetDllFolder_main() {
    char path[MAX_PATH];
    GetModuleFileNameA(g_hModule, path, MAX_PATH);
    std::string dllPath(path);
    size_t lastSlash = dllPath.find_last_of("\\/");
    return dllPath.substr(0, lastSlash);
}

// getShortenedPath_Manual is for shortening the path
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

    std::string iniPath = "";

    std::string exeIniPath = UGetExecutableFolder_main() + "\\GtoMnK.ini";

    if (GetFileAttributesA(exeIniPath.c_str()) != INVALID_FILE_ATTRIBUTES) {
        iniPath = exeIniPath;
        LOG("INI file found next the EXE at: %s", getShortenedPath_Manual(exeIniPath).c_str());
    }
    else {
        std::string dllIniPath = UGetDllFolder_main() + "\\GtoMnK.ini";

        if (GetFileAttributesA(dllIniPath.c_str()) != INVALID_FILE_ATTRIBUTES) {
            iniPath = dllIniPath;
            LOG("INI file found next the DLL at: %s", getShortenedPath_Manual(dllIniPath).c_str());
        }
        else {
            LOG("WARNING: No INI file found in next the DLL or the EXE files. Using default values...");
        }
    }

    char buffer[256];

	// [FindWindow]
    GetPrivateProfileStringA("FindWindow", "WindowName", NULL, iniWindowName, sizeof(iniWindowName), iniPath.c_str());
    GetPrivateProfileStringA("FindWindow", "ClassName", NULL, iniClassName, sizeof(iniClassName), iniPath.c_str());

    // [API]
    int method = GetPrivateProfileIntA("API", "InputMethod", 0, iniPath.c_str());
    if (method == 1) {
        g_InputMethod = InputMethod::RawInput;
    }
    else {
        g_InputMethod = InputMethod::PostMessage;
    }
    LOG("Using Input Method: %s", (g_InputMethod == InputMethod::RawInput) ? "RawInput" : "PostMessage");

    // [Hooks]
    getCursorPosHook = GetPrivateProfileIntA("Hooks", "GetCursorposHook", 1, iniPath.c_str());
    setCursorPosHook = GetPrivateProfileIntA("Hooks", "SetCursorposHook", 1, iniPath.c_str());
    clipCursorHook = GetPrivateProfileIntA("Hooks", "ClipCursorHook", 0, iniPath.c_str());
    getKeyStateHook = GetPrivateProfileIntA("Hooks", "GetKeystateHook", 1, iniPath.c_str());
    getAsyncKeyStateHook = GetPrivateProfileIntA("Hooks", "GetAsynckeystateHook", 1, iniPath.c_str());
    getKeyboardStateHook = GetPrivateProfileIntA("Hooks", "GetKeyboardstateHook", 1, iniPath.c_str());
    setCursorHook = GetPrivateProfileIntA("Hooks", "SetCursorHook", 0, iniPath.c_str()); // DrawProtoFakeCursor doewn't use this.
	setRectHook = GetPrivateProfileIntA("Hooks", "SetRectHook", 0, iniPath.c_str());

	// [MessageFilter]
    g_filterRawInput = GetPrivateProfileIntA("MessageFilter", "RawInputFilter", 0, iniPath.c_str()) == 1;
    g_filterMouseMove = GetPrivateProfileIntA("MessageFilter", "MouseMoveFilter", 0, iniPath.c_str()) == 1;
    g_filterMouseActivate = GetPrivateProfileIntA("MessageFilter", "MouseActivateFilter", 0, iniPath.c_str()) == 1;
    g_filterWindowActivate = GetPrivateProfileIntA("MessageFilter", "WindowActivateFilter", 0, iniPath.c_str()) == 1;
    g_filterWindowActivateApp = GetPrivateProfileIntA("MessageFilter", "WindowActivateAppFilter", 0, iniPath.c_str()) == 1;
    g_filterMouseWheel = GetPrivateProfileIntA("MessageFilter", "MouseWheelFilter", 0, iniPath.c_str()) == 1;
    g_filterMouseButton = GetPrivateProfileIntA("MessageFilter", "MouseButtonFilter", 0, iniPath.c_str()) == 1;
    g_filterKeyboardButton = GetPrivateProfileIntA("MessageFilter", "KeyboardButtonFilter", 0, iniPath.c_str()) == 1;

    // [Settings]
    startUpDelay = GetPrivateProfileIntA("Settings", "StartUpDelay", 0, iniPath.c_str());
    recheckHWND = GetPrivateProfileIntA("Settings", "RecheckHWND", 1, iniPath.c_str()) == 1;
    disableOverlayOptions = (GetPrivateProfileIntA("Settings", "DisableOverlayOptions", 0, iniPath.c_str()) != 0);
    controllerID = GetPrivateProfileIntA("Settings", "Controllerid", 0, iniPath.c_str());
    mode = GetPrivateProfileIntA("Settings", "Mode", 1, iniPath.c_str());
    drawfakecursor = GetPrivateProfileIntA("Settings", "drawfakecursor", 0, iniPath.c_str());
	drawProtoFakeCursor = GetPrivateProfileIntA("Settings", "DrawProtoFakeCursor", 0, iniPath.c_str()); //From ProtoInput
	ShowProtoCursorWhenImageUpdated = GetPrivateProfileIntA("Settings", "ShowCursorWhenImageUpdated", 1, iniPath.c_str()); //From ProtoInput
    if (drawProtoFakeCursor == 1) {
        if (drawfakecursor == 1) {
            LOG("INI Info: Both 'drawfakecursor' and 'drawProtoFakeCursor' are enabled. Prioritizing the ProtoInput cursor.");
            drawfakecursor = 0; // Force the old cursor to be disabled.
        }
    }
    //Let the PointerWindow and RawInput window owned by the main window (game).
    createdWindowIsOwned = GetPrivateProfileIntA("Settings", "CreatedWindowIsOwned", 1, iniPath.c_str());
    responsetime = GetPrivateProfileIntA("Settings", "Responsetime", 4, iniPath.c_str());

    // [StickToMouse]
    righthanded = GetPrivateProfileIntA("StickToMouse", "Righthanded", 2, iniPath.c_str());
    GetPrivateProfileStringA("StickToMouse", "Sensitivity", "5.00", buffer, sizeof(buffer), iniPath.c_str()); sensitivity = std::stof(buffer);
    GetPrivateProfileStringA("StickToMouse", "Sensitivity_Multiplier", "2.40", buffer, sizeof(buffer), iniPath.c_str()); sensitivity_multiplier = std::stof(buffer);
    GetPrivateProfileStringA("StickToMouse", "Horizontal_Sensitivity", "0.0", buffer, sizeof(buffer), iniPath.c_str()); horizontal_sensitivity = std::stof(buffer);
    GetPrivateProfileStringA("StickToMouse", "Vertical_Sensitivity", "0.58", buffer, sizeof(buffer), iniPath.c_str()); vertical_sensitivity = std::stof(buffer);
    GetPrivateProfileStringA("StickToMouse", "Max_Threshold", "0.045", buffer, sizeof(buffer), iniPath.c_str()); max_threshold = std::stof(buffer);
    GetPrivateProfileStringA("StickToMouse", "Radial_Deadzone", "0.1", buffer, sizeof(buffer), iniPath.c_str()); radial_deadzone = std::stof(buffer);
    GetPrivateProfileStringA("StickToMouse", "Axial_Deadzone", "0.0", buffer, sizeof(buffer), iniPath.c_str()); axial_deadzone = std::stof(buffer);
    GetPrivateProfileStringA("StickToMouse", "Look_Accel_Multiplier", "1.40", buffer, sizeof(buffer), iniPath.c_str()); look_accel_multiplier = std::stof(buffer);
    GetPrivateProfileStringA("StickToMouse", "Curve_Slope", "0.16", buffer, sizeof(buffer), iniPath.c_str()); curve_slope = std::stof(buffer);
    GetPrivateProfileStringA("StickToMouse", "Curve_Exponent", "1.85", buffer, sizeof(buffer), iniPath.c_str()); curve_exponent = std::stof(buffer);

    // [KeyMapping] Section
    g_EnableMouseDoubleClick = GetPrivateProfileIntA("KeyMapping", "EnableMouseDoubleClick", 0, iniPath.c_str()) == 1;
    GetPrivateProfileStringA("KeyMapping", "TriggerThreshold", "40", buffer, sizeof(buffer), iniPath.c_str()); g_TriggerThreshold = std::stof(buffer);
    GetPrivateProfileStringA("KeyMapping", "StickAsButtonDeadzone", "0.25", buffer, sizeof(buffer), iniPath.c_str()); stick_as_button_deadzone = std::stof(buffer);

    GetPrivateProfileStringA("KeyMapping", "A", "13", buffer, sizeof(buffer), iniPath.c_str());
    buttonStates[XINPUT_GAMEPAD_A].actions = Input::ParseActionString(buffer);
    GetPrivateProfileStringA("KeyMapping", "B", "0", buffer, sizeof(buffer), iniPath.c_str());
    buttonStates[XINPUT_GAMEPAD_B].actions = Input::ParseActionString(buffer);
    GetPrivateProfileStringA("KeyMapping", "X", "42", buffer, sizeof(buffer), iniPath.c_str());
    buttonStates[XINPUT_GAMEPAD_X].actions = Input::ParseActionString(buffer);
    GetPrivateProfileStringA("KeyMapping", "Y", "0", buffer, sizeof(buffer), iniPath.c_str());
    buttonStates[XINPUT_GAMEPAD_Y].actions = Input::ParseActionString(buffer);

    // Shoulder Buttons & Thumbs
    GetPrivateProfileStringA("KeyMapping", "RB", "0", buffer, sizeof(buffer), iniPath.c_str());
    buttonStates[XINPUT_GAMEPAD_RIGHT_SHOULDER].actions = Input::ParseActionString(buffer);
    GetPrivateProfileStringA("KeyMapping", "LB", "0", buffer, sizeof(buffer), iniPath.c_str());
    buttonStates[XINPUT_GAMEPAD_LEFT_SHOULDER].actions = Input::ParseActionString(buffer);
    GetPrivateProfileStringA("KeyMapping", "RSB", "0", buffer, sizeof(buffer), iniPath.c_str());
    buttonStates[XINPUT_GAMEPAD_RIGHT_THUMB].actions = Input::ParseActionString(buffer);
    GetPrivateProfileStringA("KeyMapping", "LSB", "4", buffer, sizeof(buffer), iniPath.c_str());
    buttonStates[XINPUT_GAMEPAD_LEFT_THUMB].actions = Input::ParseActionString(buffer);

    // D-Pad
    GetPrivateProfileStringA("KeyMapping", "D_UP", "14", buffer, sizeof(buffer), iniPath.c_str());
    buttonStates[XINPUT_GAMEPAD_DPAD_UP].actions = Input::ParseActionString(buffer);
    GetPrivateProfileStringA("KeyMapping", "D_DOWN", "15", buffer, sizeof(buffer), iniPath.c_str());
    buttonStates[XINPUT_GAMEPAD_DPAD_DOWN].actions = Input::ParseActionString(buffer);
    GetPrivateProfileStringA("KeyMapping", "D_LEFT", "16", buffer, sizeof(buffer), iniPath.c_str());
    buttonStates[XINPUT_GAMEPAD_DPAD_LEFT].actions = Input::ParseActionString(buffer);
    GetPrivateProfileStringA("KeyMapping", "D_RIGHT", "17", buffer, sizeof(buffer), iniPath.c_str());
    buttonStates[XINPUT_GAMEPAD_DPAD_RIGHT].actions = Input::ParseActionString(buffer);

    // Start & Back
    GetPrivateProfileStringA("KeyMapping", "Start", "1", buffer, sizeof(buffer), iniPath.c_str());
    buttonStates[XINPUT_GAMEPAD_START].actions = Input::ParseActionString(buffer);
    GetPrivateProfileStringA("KeyMapping", "Back", "3", buffer, sizeof(buffer), iniPath.c_str());
    buttonStates[XINPUT_GAMEPAD_BACK].actions = Input::ParseActionString(buffer);

    // Left Stick
    GetPrivateProfileStringA("KeyMapping", "LSU", "47", buffer, sizeof(buffer), iniPath.c_str());
    buttonStates[CUSTOM_ID_LSU].actions = Input::ParseActionString(buffer);
    GetPrivateProfileStringA("KeyMapping", "LSD", "43", buffer, sizeof(buffer), iniPath.c_str());
    buttonStates[CUSTOM_ID_LSD].actions = Input::ParseActionString(buffer);
    GetPrivateProfileStringA("KeyMapping", "LSL", "25", buffer, sizeof(buffer), iniPath.c_str());
    buttonStates[CUSTOM_ID_LSL].actions = Input::ParseActionString(buffer);
    GetPrivateProfileStringA("KeyMapping", "LSR", "28", buffer, sizeof(buffer), iniPath.c_str());
    buttonStates[CUSTOM_ID_LSR].actions = Input::ParseActionString(buffer);

    // Right Stick
    GetPrivateProfileStringA("KeyMapping", "RSU", "0", buffer, sizeof(buffer), iniPath.c_str());
    buttonStates[CUSTOM_ID_RSU].actions = Input::ParseActionString(buffer);
    GetPrivateProfileStringA("KeyMapping", "RSD", "0", buffer, sizeof(buffer), iniPath.c_str());
    buttonStates[CUSTOM_ID_RSD].actions = Input::ParseActionString(buffer);
    GetPrivateProfileStringA("KeyMapping", "RSL", "0", buffer, sizeof(buffer), iniPath.c_str());
    buttonStates[CUSTOM_ID_RSL].actions = Input::ParseActionString(buffer);
    GetPrivateProfileStringA("KeyMapping", "RSR", "0", buffer, sizeof(buffer), iniPath.c_str());
    buttonStates[CUSTOM_ID_RSR].actions = Input::ParseActionString(buffer);

    // Triggers
    GetPrivateProfileStringA("KeyMapping", "LT", "-2", buffer, sizeof(buffer), iniPath.c_str());
    buttonStates[CUSTOM_ID_LT].actions = Input::ParseActionString(buffer);
    GetPrivateProfileStringA("KeyMapping", "RT", "-1", buffer, sizeof(buffer), iniPath.c_str());
    buttonStates[CUSTOM_ID_RT].actions = Input::ParseActionString(buffer);
}

POINT ThumbstickMouseMove(SHORT stickX, SHORT stickY) {
    // Make sure the deadzone is not 0
    double c = (radial_deadzone < 0.01) ? 0.01 : radial_deadzone;
    const double s = axial_deadzone;
    const double m = max_threshold;
    const double b = curve_slope;
    const double a = curve_exponent;
    const double t = look_accel_multiplier;

    double base_sensitivity = sensitivity;

    double safe_multiplier = (sensitivity_multiplier <= 0.001) ? 1.0 : sensitivity_multiplier;

    static double mouseDeltaAccumulatorX = 0.0;
    static double mouseDeltaAccumulatorY = 0.0;

    double normX = static_cast<double>(stickX) / 32767.0;
    double normY = static_cast<double>(stickY) / 32767.0;

    if (std::abs(normX) < s) normX = 0.0;
    if (std::abs(normY) < s) normY = 0.0;

    double magnitude = std::sqrt(normX * normX + normY * normY);
    if (magnitude <= c) return { 0, 0 };
    if (magnitude > 1.0) magnitude = 1.0;

    double effectiveRange = 1.0 - c;
    if (effectiveRange < 1e-6) effectiveRange = 1.0;

    double x = (magnitude - c) / effectiveRange;

    double finalCurveValue = 0.0;
    double accelBoundary = 1.0 - m;
    accelBoundary = (std::max)(0.0, (std::min)(1.0, accelBoundary));

    if (x <= accelBoundary) {
        finalCurveValue = b * x + (1.0 - b) * std::pow(x, a);
    }
    else {
        double kneeValue = b * accelBoundary + (1.0 - b) * std::pow(accelBoundary, a);

        double rampWidth = 1.0 - accelBoundary;

        if (rampWidth <= 1e-6) {
            finalCurveValue = t;
        }
        else {
            double rampProgress = (x - accelBoundary) / rampWidth;
            finalCurveValue = kneeValue + (t - kneeValue) * rampProgress;
        }
    }

    double finalSpeedX = base_sensitivity * safe_multiplier * (1.0f + horizontal_sensitivity);
    double finalSpeedY = base_sensitivity * safe_multiplier * (1.0f + vertical_sensitivity);

    double dirX = normX / magnitude;
    double dirY = normY / magnitude;

    double finalMouseDeltaX = dirX * finalCurveValue * finalSpeedX;
    double finalMouseDeltaY = dirY * finalCurveValue * finalSpeedY;

    mouseDeltaAccumulatorX += finalMouseDeltaX;
    mouseDeltaAccumulatorY += finalMouseDeltaY;

    LONG integerDeltaX = static_cast<LONG>(mouseDeltaAccumulatorX);
    LONG integerDeltaY = static_cast<LONG>(mouseDeltaAccumulatorY);

    mouseDeltaAccumulatorX -= integerDeltaX;
    mouseDeltaAccumulatorY -= integerDeltaY;

    return { integerDeltaX, -integerDeltaY };
}

// 0-255 lower value is more sensitive
bool IsTriggerPressed(BYTE triggerValue) {
    return triggerValue > g_TriggerThreshold;
}

void DrawOverlay() {
    if (showmessage == 0 && !(drawfakecursor && mode == 1 && onoroff)) {
        return;
    }
    HDC hdcWindow = GetDC(hwnd);
    if (!hdcWindow) {
        return;
    }
    if (showmessage == 1) TextOutA(hdcWindow, Mouse::Xf, Mouse::Yf, "KEYBOARD MODE", 13);
    else if (showmessage == 2) TextOutA(hdcWindow, Mouse::Xf, Mouse::Yf, "CURSOR MODE", 11);
    else if (showmessage == 69) TextOutA(hdcWindow, Mouse::Xf, Mouse::Yf, "DISABLED", 8);
    else if (showmessage == 70) TextOutA(hdcWindow, Mouse::Xf, Mouse::Yf, "ENABLED", 7);
    else if (showmessage == 12) TextOutA(hdcWindow, 20, 20, "CONTROLLER DISCONNECTED", 23);
    else if (drawfakecursor && mode == 1 && onoroff) {
        Mouse::DrawBeautifulCursor(hdcWindow);
    }
    ReleaseDC(hwnd, hdcWindow);
}

void ProcessButton(UINT buttonFlag, bool isCurrentlyPressed) {
    ButtonState& bs = buttonStates[buttonFlag];

    // On Press
    if (!bs.isPhysicallyPressed && isCurrentlyPressed) {
        bs.isPhysicallyPressed = true;
        bs.pressTime = GetTickCount64();
        bs.activeActionIndex = NO_ACTION;
        bs.pressActionFired = false;
        bs.heldActionString = "0";

        if (!bs.actions.empty() && bs.actions[0].holdDurationMs == 0) {
            bs.activeActionIndex = 0;
            const Action& tapAction = bs.actions[0];

            if (!tapAction.onRelease) {
                Input::SendAction(tapAction.actionString, true);
                bs.heldActionString = tapAction.actionString;
                bs.pressActionFired = true;
            }
        }
        return;
    }

    // While Held
    if (bs.isPhysicallyPressed && isCurrentlyPressed) {
        ULONGLONG holdTime = GetTickCount64() - bs.pressTime;
        size_t newActionIndex = NO_ACTION;
        for (size_t i = 0; i < bs.actions.size(); ++i) {
            if (holdTime >= bs.actions[i].holdDurationMs) {
                newActionIndex = i;
            }
        }
        if (newActionIndex != NO_ACTION && newActionIndex != bs.activeActionIndex) {
            if (!bs.heldActionString.empty() && bs.heldActionString != "0") {
                Input::SendAction(bs.heldActionString, false);
            }
            bs.activeActionIndex = newActionIndex;
            bs.pressActionFired = false;
            bs.heldActionString = "0";
            const Action& newActiveAction = bs.actions[bs.activeActionIndex];
            if (!newActiveAction.onRelease) {
                Input::SendAction(newActiveAction.actionString, true);
                bs.heldActionString = newActiveAction.actionString;
                bs.pressActionFired = true;
            }
        }
        return;
    }

    // On Release
    if (bs.isPhysicallyPressed && !isCurrentlyPressed) {
        bs.isPhysicallyPressed = false;

        size_t finalActionIndex = NO_ACTION;
        ULONGLONG finalHoldTime = GetTickCount64() - bs.pressTime;
        for (size_t i = 0; i < bs.actions.size(); ++i) {
            if (finalHoldTime >= bs.actions[i].holdDurationMs) {
                finalActionIndex = i;
            }
        }

        if (finalActionIndex != NO_ACTION && bs.actions[finalActionIndex].onRelease) {
            const Action& finalAction = bs.actions[finalActionIndex];

            Input::SendAction(finalAction.actionString, true);
            Sleep(20);
            Input::SendAction(finalAction.actionString, false);
        }

        if (!bs.heldActionString.empty() && bs.heldActionString != "0") {
            Input::SendAction(bs.heldActionString, false);
        }

        bs.activeActionIndex = NO_ACTION;
        bs.pressActionFired = false;
        bs.heldActionString = "0";
        return;
    }
}

void ProcessTrigger(UINT triggerID, BYTE triggerValue) {
    bool isPressed = IsTriggerPressed(triggerValue);
    ProcessButton(triggerID, isPressed);
}

DWORD WINAPI ThreadFunction(LPVOID lpParam) {
    LOG("ThreadFunction started.");

    OpenXinputDllMain(GetModuleHandle(NULL), DLL_PROCESS_ATTACH, NULL);

    LoadIniSettings();
    LOG("INI settings is Loaded");

    if (controllerID < 0) {
        LOG("Controller is disabled by INI. Thread is exiting.");
        loop = false;
        return 0;
    }
    else {
        LOG("Using controller ID: %d", controllerID);
    }

    if (startUpDelay > 0) {
        LOG("Waiting for startup delay of %dSec before hooking...", startUpDelay);
        Sleep(startUpDelay * 1000);
    }

    LOG("Searching for Game Window...");
    if (iniWindowName[0] != '\0') { LOG("Filter Name: '%s'", iniWindowName); }
    if (iniClassName[0] != '\0') { LOG("Filter Class: '%s'", iniClassName); }
    hwnd = GetMainWindowHandle(GetCurrentProcessId(), iniWindowName, iniClassName, 300000);
    if (!hwnd) {
        LOG("CRITICAL: Game window not found after 5min. Thread exiting to prevent crash.");
        loop = false;
        return 0;
    }
    LOG("Initial window handle acquired: 0x%p", hwnd);

    if (drawProtoFakeCursor == 1) {
        LOG("ProtoInput Fake Cursor is enabled. Initializing...");
        FakeCursor::Initialise();
        if (!createdWindowIsOwned)
            FakeCursor::EnableDisableFakeCursor(true);
        // CursorVisibilityHook will be enabled automatically when `drawProtoFakeCursor` is enabled. 
    }
    else if (drawfakecursor) {
        LOG("Simple Fake Cursor is enabled.");
    }

    Hooks::SetupHooks();
    hooksinited = true;

    if (g_InputMethod == InputMethod::RawInput) {
        RawInput::Initialize();
    }
    
	// Overlay Options
    if (!disableOverlayOptions)
    {
        OverlayMenu::state.Initialise();
        LOG("Overlay Options Initialized.");
    }
    else {
        LOG("Overlay Options is disabled.");
    }
    ULONGLONG menuToggleTimer = 0;
    bool menuTogglePending = false;

    ULONGLONG lastMoveTime = 0;
    const DWORD MOVE_UPDATE_INTERVAL = 8;

    while (loop) {
        bool movedmouse = false;

        if (recheckHWND) {
            if (hwnd && !IsWindow(hwnd)) {
                LOG("Window handle became invalid. Resetting...");
                hwnd = nullptr;
            }
        }

        if (!hwnd) {
            hwnd = GetMainWindowHandle(GetCurrentProcessId(), iniWindowName, iniClassName, 300000);

            if (!hwnd) {
                LOG("CRITICAL: Game window missing for 5 minute.");
                loop = false;
                break;
            }
            LOG("Acquired new window handle: 0x%p", hwnd);
        }

        RECT rect;
        if (!GetClientRect(hwnd, &rect)) {
            rect.left = 0; rect.top = 0; rect.right = 800; rect.bottom = 600;
        }

        XINPUT_STATE state;
        if (OpenXInputGetState(controllerID, &state) == ERROR_SUCCESS) {

            if (!disableOverlayOptions)
            {
                // Buttons to open the overlay window
                bool backDown = (state.Gamepad.wButtons & XINPUT_GAMEPAD_BACK) != 0;
                bool dpadDown = (state.Gamepad.wButtons & XINPUT_GAMEPAD_DPAD_DOWN) != 0;

                if (backDown && dpadDown) {
                    if (!menuTogglePending) {
                        menuToggleTimer = GetTickCount64();
                        menuTogglePending = true;
                    }
                    else if (GetTickCount64() - menuToggleTimer > 50) {
                        // Toggle the State
                        bool newState = !OverlayMenu::state.isMenuOpen;
                        OverlayMenu::state.EnableDisableMenu(newState);

                        menuTogglePending = false;
                        Mouse::Xf = Mouse::Xf;
                        XINPUT_STATE waitState;
                        do {
                            Sleep(100); // Important
                            if (OpenXInputGetState(controllerID, &waitState) != ERROR_SUCCESS) {
                                break;
                            }
                        } while ((waitState.Gamepad.wButtons & XINPUT_GAMEPAD_BACK) ||
                            (waitState.Gamepad.wButtons & XINPUT_GAMEPAD_DPAD_DOWN));
                        continue;
                    }
                }
                else {
                    menuTogglePending = false;
                }
            }

            if (showmessage == 12) showmessage = 0; // "disconnected" message on reconnect

            if (!disableOverlayOptions && OverlayMenu::state.isMenuOpen) {
                OverlayMenu::state.ProcessInput(state.Gamepad.wButtons);
            }

            else {
                // Buttons
                ProcessButton(XINPUT_GAMEPAD_A, (state.Gamepad.wButtons & XINPUT_GAMEPAD_A) != 0);
                ProcessButton(XINPUT_GAMEPAD_B, (state.Gamepad.wButtons & XINPUT_GAMEPAD_B) != 0);
                ProcessButton(XINPUT_GAMEPAD_X, (state.Gamepad.wButtons & XINPUT_GAMEPAD_X) != 0);
                ProcessButton(XINPUT_GAMEPAD_Y, (state.Gamepad.wButtons & XINPUT_GAMEPAD_Y) != 0);
                ProcessButton(XINPUT_GAMEPAD_RIGHT_SHOULDER, (state.Gamepad.wButtons & XINPUT_GAMEPAD_RIGHT_SHOULDER) != 0);
                ProcessButton(XINPUT_GAMEPAD_LEFT_SHOULDER, (state.Gamepad.wButtons & XINPUT_GAMEPAD_LEFT_SHOULDER) != 0);
                ProcessButton(XINPUT_GAMEPAD_RIGHT_THUMB, (state.Gamepad.wButtons & XINPUT_GAMEPAD_RIGHT_THUMB) != 0);
                ProcessButton(XINPUT_GAMEPAD_LEFT_THUMB, (state.Gamepad.wButtons & XINPUT_GAMEPAD_LEFT_THUMB) != 0);
                ProcessButton(XINPUT_GAMEPAD_DPAD_UP, (state.Gamepad.wButtons & XINPUT_GAMEPAD_DPAD_UP) != 0);
                ProcessButton(XINPUT_GAMEPAD_DPAD_DOWN, (state.Gamepad.wButtons & XINPUT_GAMEPAD_DPAD_DOWN) != 0);
                ProcessButton(XINPUT_GAMEPAD_DPAD_LEFT, (state.Gamepad.wButtons & XINPUT_GAMEPAD_DPAD_LEFT) != 0);
                ProcessButton(XINPUT_GAMEPAD_DPAD_RIGHT, (state.Gamepad.wButtons & XINPUT_GAMEPAD_DPAD_RIGHT) != 0);
                ProcessButton(XINPUT_GAMEPAD_START, (state.Gamepad.wButtons & XINPUT_GAMEPAD_START) != 0);
                ProcessButton(XINPUT_GAMEPAD_BACK, (state.Gamepad.wButtons & XINPUT_GAMEPAD_BACK) != 0);

                // Triggers
                ProcessTrigger(CUSTOM_ID_LT, state.Gamepad.bLeftTrigger);
                ProcessTrigger(CUSTOM_ID_RT, state.Gamepad.bRightTrigger);

                // Analog Sticks (as buttons)
                // Left Stick
                if (onoroff && (mode == 0 || (mode == 1 && righthanded == 1))) {
                    float normLX = static_cast<float>(state.Gamepad.sThumbLX) / 32767.0f;
                    float normLY = static_cast<float>(state.Gamepad.sThumbLY) / 32767.0f;
                    float magnitudeL = static_cast<float>(sqrt(normLX * normLX + normLY * normLY));

                    bool isLSU = false, isLSD = false, isLSL = false, isLSR = false;
                    if (magnitudeL > stick_as_button_deadzone) {
                        float angleDeg = atan2(normLY, normLX) * 180.0f / 3.1415926535f;
                        if (angleDeg < 0) angleDeg += 360.0f;
                        if (angleDeg > 22.5f && angleDeg < 157.5f) isLSU = true;
                        if (angleDeg > 202.5f && angleDeg < 337.5f) isLSD = true;
                        if (angleDeg > 112.5f && angleDeg < 247.5f) isLSL = true;
                        if (angleDeg > 292.5f || angleDeg < 67.5f) isLSR = true;
                    }
                    ProcessButton(CUSTOM_ID_LSU, isLSU);
                    ProcessButton(CUSTOM_ID_LSD, isLSD);
                    ProcessButton(CUSTOM_ID_LSL, isLSL);
                    ProcessButton(CUSTOM_ID_LSR, isLSR);
                }
                // Right Stick
                if (onoroff && (mode == 0 || (mode == 1 && righthanded == 0))) {
                    float normRX = static_cast<float>(state.Gamepad.sThumbRX) / 32767.0f;
                    float normRY = static_cast<float>(state.Gamepad.sThumbRY) / 32767.0f;
                    float magnitudeR = static_cast<float>(sqrt(normRX * normRX + normRY * normRY));
                    bool isRSU = false, isRSD = false, isRSL = false, isRSR = false;
                    if (magnitudeR > stick_as_button_deadzone) {
                        float angleDeg = atan2(normRY, normRX) * 180.0f / 3.1415926535f;
                        if (angleDeg < 0) angleDeg += 360.0f;
                        if (angleDeg > 22.5f && angleDeg < 157.5f) isRSU = true;
                        if (angleDeg > 202.5f && angleDeg < 337.5f) isRSD = true;
                        if (angleDeg > 112.5f && angleDeg < 247.5f) isRSL = true;
                        if (angleDeg > 292.5f || angleDeg < 67.5f) isRSR = true;
                    }
                    ProcessButton(CUSTOM_ID_RSU, isRSU);
                    ProcessButton(CUSTOM_ID_RSD, isRSD);
                    ProcessButton(CUSTOM_ID_RSL, isRSL);
                    ProcessButton(CUSTOM_ID_RSR, isRSR);
                }

                // Mouse MOVEMENT
                if (mode == 1 && onoroff) {
                    SHORT thumbX = (righthanded == 1) ? state.Gamepad.sThumbRX : state.Gamepad.sThumbLX;
                    SHORT thumbY = (righthanded == 1) ? state.Gamepad.sThumbRY : state.Gamepad.sThumbLY;
                    POINT delta = ThumbstickMouseMove(thumbX, thumbY);
                    if (delta.x != 0 || delta.y != 0) {
                        Mouse::Xf += delta.x; Mouse::Yf += delta.y;
                        Mouse::Xf = std::max((LONG)rect.left, std::min((LONG)Mouse::Xf, (LONG)rect.right));
                        Mouse::Yf = std::max((LONG)rect.top, std::min((LONG)Mouse::Yf, (LONG)rect.bottom));
                        movedmouse = true;

                        //ProtoInput Fake Cursor update
                        if (drawProtoFakeCursor == 1) {
                            FakeCursor::NotifyUpdatedCursorPosition();
                        }

                        ULONGLONG currentTime = GetTickCount64();
                        if (currentTime - lastMoveTime > MOVE_UPDATE_INTERVAL) {
                            lastMoveTime = currentTime;
                            if (g_InputMethod == InputMethod::RawInput) {
                                Input::SendActionDelta(delta.x, delta.y);
                            }
                            else {
                                POINT screenPos = { (LONG)Mouse::Xf, (LONG)Mouse::Yf };
                                ClientToScreen(hwnd, &screenPos);
                                Input::SendAction(screenPos.x, screenPos.y);
                            }
                        }
                    }
                }
            }
        }
        else {
            showmessage = 12; // Controller disconnected
        }

        // Drawing and Message
        DrawOverlay();
        if (showmessage != 0 && showmessage != 12) {
            if (++counter > 150) {
                showmessage = 0;
                counter = 0;
            }
        }
        
        if (responsetime <= 0)
            responsetime = 1;
        Sleep(responsetime);
    }

    LOG("ThreadFunction gracefully exiting.");
    return 0;
}