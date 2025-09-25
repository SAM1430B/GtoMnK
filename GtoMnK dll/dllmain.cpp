#include "pch.h"
#include "Logging.h"
#include "Hooks.h"
#include "Mouse.h"
#include "Keyboard.h"
#include "Input.h"
#include "InputState.h"
#include <map>
#include "RawInput.h"


using namespace GtoMnK;

#pragma comment(lib, "Xinput9_1_0.lib")

const size_t NO_ACTION = static_cast<size_t>(-1);

struct ButtonState {
    bool isPhysicallyPressed = false;
    ULONGLONG pressTime = 0;
    size_t activeActionIndex = NO_ACTION;
    bool pressActionFired = false;
    std::vector<Action> actions;
    std::string heldActionString = "0";
};

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

HMODULE g_hModule = nullptr;
HWND hwnd = nullptr;
HANDLE hMutex = nullptr;
bool loop = true;
int showmessage = 0;
int counter = 0;
bool onoroff = true;
int mode = 1; // Default to Cursor Mode
int controllerID = 0;
int keystatesend = 0;

std::map<UINT, ButtonState> buttonStates;

// Drawing & cursor state
bool pausedraw = false;
bool gotcursoryet = false;

InputMethod g_InputMethod = InputMethod::PostMessage;

bool g_EnableMouseDoubleClick = false;
BYTE g_TriggerThreshold = 175;
float stick_as_button_deadzone;

std::string A_Action, B_Action, X_Action, Y_Action;
std::string RB_Action, LB_Action, RSB_Action, LSB_Action;
std::string D_UP_Action, D_DOWN_Action, D_LEFT_Action, D_RIGHT_Action;

std::string Start_Action;
std::string Back_Action;
std::string LT_Action, RT_Action;

bool scrollmap = false;
POINT scroll = { 0, 0 };
POINT startdrag = { 0, 0 };
POINT rectignore = { 0, 0 };
DWORD lastClickTime = 0;

// Hooks
int getCursorPosHook, setCursorPosHook, clipCursorHook, getKeyStateHook, getAsyncKeyStateHook, getKeyboardStateHook, setCursorHook, setRectHook;
int leftrect, toprect, rightrect, bottomrect;
// Controller
int righthanded, Xoffset, Yoffset;
float radial_deadzone, axial_deadzone, sensitivity, max_threshold, curve_slope, curve_exponent, accel_multiplier, horizontal_sensitivity, vertical_sensitivity;
// General
int userealmouse, ignorerect, drawfakecursor, alwaysdrawcursor, doubleclicks, scrolloutsidewindow, responsetime, quickMW, scrollenddelay;
bool hooksinited = false;
int tick = 0;
bool doscrollyes = false;

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

    std::string dllIniPath = UGetDllFolder_main() + "\\GtoMnK.ini";

    if (GetFileAttributesA(dllIniPath.c_str()) != INVALID_FILE_ATTRIBUTES) {
        iniPath = dllIniPath;
        LOG("INI file found next the DLL at: %s", getShortenedPath_Manual(dllIniPath).c_str());
    }
    else {
        std::string exeIniPath = UGetExecutableFolder_main() + "\\GtoMnK.ini";

        if (GetFileAttributesA(exeIniPath.c_str()) != INVALID_FILE_ATTRIBUTES) {
            iniPath = exeIniPath;
            LOG("INI file found next the EXE at: %s", getShortenedPath_Manual(exeIniPath).c_str());
        }
        else {
            LOG("WARNING: No INI file found in next the DLL or the EXE files. Using default values...");
        }
    }

    char buffer[256];

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
    setCursorHook = GetPrivateProfileIntA("Hooks", "SetCursorHook", 1, iniPath.c_str());
    setRectHook = GetPrivateProfileIntA("Hooks", "SetRectHook", 0, iniPath.c_str());
    
    // [Settings]
    controllerID = GetPrivateProfileIntA("Settings", "Controllerid", 0, iniPath.c_str());
    mode = GetPrivateProfileIntA("Settings", "Mode", 1, iniPath.c_str());
    drawfakecursor = GetPrivateProfileIntA("Settings", "DrawFakeCursor", 1, iniPath.c_str());
    alwaysdrawcursor = GetPrivateProfileIntA("Settings", "DrawFakeCursorAlways", 0, iniPath.c_str());
    responsetime = GetPrivateProfileIntA("Settings", "Responsetime", 0, iniPath.c_str());
    LOG("Using controller ID: %d", controllerID);

    // [StickToMouse]
    righthanded = GetPrivateProfileIntA("StickToMouse", "Righthanded", 2, iniPath.c_str());
    GetPrivateProfileStringA("StickToMouse", "Sensitivity", "1.45", buffer, sizeof(buffer), iniPath.c_str()); sensitivity = std::stof(buffer);
	GetPrivateProfileStringA("StickToMouse", "Horizontal_Sensitivity", "0.0", buffer, sizeof(buffer), iniPath.c_str()); horizontal_sensitivity = std::stof(buffer);
	GetPrivateProfileStringA("StickToMouse", "Vertical_Sensitivity", "0.0", buffer, sizeof(buffer), iniPath.c_str()); vertical_sensitivity = std::stof(buffer);
    GetPrivateProfileStringA("StickToMouse", "Accel_Multiplier", "3.0", buffer, sizeof(buffer), iniPath.c_str()); accel_multiplier = std::stof(buffer);
    GetPrivateProfileStringA("StickToMouse", "Radial_Deadzone", "0.1", buffer, sizeof(buffer), iniPath.c_str()); radial_deadzone = std::stof(buffer);
    GetPrivateProfileStringA("StickToMouse", "Axial_Deadzone", "0.0", buffer, sizeof(buffer), iniPath.c_str()); axial_deadzone = std::stof(buffer);
    GetPrivateProfileStringA("StickToMouse", "Max_Threshold", "0.01", buffer, sizeof(buffer), iniPath.c_str()); max_threshold = std::stof(buffer);
    GetPrivateProfileStringA("StickToMouse", "Curve_Slope", "0.30", buffer, sizeof(buffer), iniPath.c_str()); curve_slope = std::stof(buffer);
    GetPrivateProfileStringA("StickToMouse", "Curve_Exponent", "6.50", buffer, sizeof(buffer), iniPath.c_str()); curve_exponent = std::stof(buffer);

    // [KeyMapping] Section
    g_EnableMouseDoubleClick = GetPrivateProfileIntA("KeyMapping", "EnableMouseDoubleClick", 0, iniPath.c_str()) == 1;
    g_TriggerThreshold = (BYTE)GetPrivateProfileIntA("KeyMapping", "TriggerThreshold", 175, iniPath.c_str());
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
    double finalSpeedX = sensitivity * (1.0f + horizontal_sensitivity) * accel_multiplier;
    double finalSpeedY = sensitivity * (1.0f + vertical_sensitivity) * accel_multiplier;
    double dirX = normX / magnitude;
    double dirY = normY / magnitude;
    double finalMouseDeltaX = dirX * curvedMagnitude * finalSpeedX;
    double finalMouseDeltaY = dirY * curvedMagnitude * finalSpeedY;

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
    if (!hwnd || pausedraw || !drawfakecursor) return;
    HDC hdcWindow = GetDC(hwnd);
    if (!hdcWindow) return;
    if (showmessage == 1) TextOutA(hdcWindow, Mouse::Xf, Mouse::Yf, "KEYBOARD MODE", 13);
    else if (showmessage == 2) TextOutA(hdcWindow, Mouse::Xf, Mouse::Yf, "CURSOR MODE", 11);
    else if (showmessage == 69) TextOutA(hdcWindow, Mouse::Xf, Mouse::Yf, "DISABLED", 8);
    else if (showmessage == 70) TextOutA(hdcWindow, Mouse::Xf, Mouse::Yf, "ENABLED", 7);
    else if (showmessage == 12) TextOutA(hdcWindow, 20, 20, "CONTROLLER DISCONNECTED", 23);
    else if (mode == 1 && onoroff) { Mouse::DrawBeautifulCursor(hdcWindow); }
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
    Sleep(1000 * 2);

    LoadIniSettings();
    LOG("Settings loaded. Setting up hooks...");

    if (controllerID < 0) {
		LOG("Controller is disabled: terminating GtoMnK...");
        loop = false;
		return 0;
    }

    LOG("Settings loaded. Setting up hooks...");
    Hooks::SetupHooks();
    hooksinited = true;

    if (g_InputMethod == InputMethod::RawInput) {
        LOG("Initializing RawInput system...");
        RawInput::Initialize();
    }
    else {
        LOG("Initializing PostMessage system...");
    }

    hwnd = GetMainWindowHandle(GetCurrentProcessId());

    ULONGLONG lastMoveTime = 0;
    const DWORD MOVE_UPDATE_INTERVAL = 8;

    while (loop) {
        bool movedmouse = false;
        if (!hwnd) {
            hwnd = GetMainWindowHandle(GetCurrentProcessId());
            Sleep(1000);
            continue;
        }

        RECT rect;
        GetClientRect(hwnd, &rect);

        XINPUT_STATE state;
        if (XInputGetState(controllerID, &state) == ERROR_SUCCESS) {
            if (showmessage == 12) showmessage = 0; // "disconnected" message on reconnect

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
                    ULONGLONG currentTime = GetTickCount64();
                    if (currentTime - lastMoveTime > MOVE_UPDATE_INTERVAL) {
                        lastMoveTime = currentTime;
                        if (g_InputMethod == InputMethod::RawInput) {
                            Input::SendActionDelta(delta.x, delta.y);
                        } else {
                            POINT screenPos = { (LONG)Mouse::Xf, (LONG)Mouse::Yf };
                            ClientToScreen(hwnd, &screenPos);
                            Input::SendAction(screenPos.x, screenPos.y);
                        }
                    }
                }
            }
        } else {
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

        Sleep(responsetime + 1);
    }

    LOG("ThreadFunction gracefully exiting.");
    return 0;
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved) {
    switch (ul_reason_for_call) {
    case DLL_PROCESS_ATTACH: {
        g_hModule = hModule;
        DisableThreadLibraryCalls(hModule);

        INIT_LOGGER();
        LOG("================== DLL Attached (PID: %lu) ==================", GetCurrentProcessId());

        HANDLE hThread = CreateThread(nullptr, 0, ThreadFunction, hModule, 0, nullptr);
        if (hThread) {
            CloseHandle(hThread);
        }
        else {
            LOG("FATAL: Failed to create main worker thread! GetLastError() = %lu", GetLastError());
            return FALSE;
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