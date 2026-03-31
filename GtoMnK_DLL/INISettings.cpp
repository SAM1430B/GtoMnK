#include "pch.h"
#include "INISettings.h"
#include "Logging.h"
#include "InputState.h" 
#include "Input.h"
#include "GamepadState.h"

extern HMODULE g_hModule;

// For Controller Button States
std::map<UINT, ButtonState> buttonStates;
bool g_EnableMouseDoubleClick = false;
int righthanded;

// For Controller Setting
int controllerID = 0;
GamepadMethod g_GamepadMethod = GamepadMethod::SDL2;
float radial_deadzone, axial_deadzone, sensitivity, max_threshold, curve_slope, curve_exponent, sensitivity_multiplier, horizontal_sensitivity, vertical_sensitivity, look_accel_multiplier;
float stick_as_button_deadzone;
float g_TriggerThreshold = 40;

// For Initialization and Thread state
int startUpDelay = 0;
bool recheckHWND = true;
bool disableOverlayOptions = false;
bool g_EnableOpenXinput = false;

// Drawing & cursor state
int drawfakecursor = 0;
int drawProtoFakeCursor = 0; // From ProtoInput also for the Cursor visibility hooks
int createdWindowIsOwned = 1;
int ShowProtoCursorWhenImageUpdated = 1; // From ProtoInput
int mode = 1;
int responsetime = 4;

// For the Hooks
InputMethod g_InputMethod = InputMethod::Hybrid;
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

// For Gamepad Masking
bool enableXInputMask = false;
bool enableSDL2Mask = false;

// For finding the game window
char iniWindowName[256] = { 0 };
char iniClassName[256] = { 0 };

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

void ParseKey(const char* section, const char* key, const char* defaultVal, UINT baseId, int offset, const char* iniPath);
void LoadButtonLayer(const char* section, int offset, bool isBaseLayer, const char* iniPath);

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
    int inputMethod = GetPrivateProfileIntA("API", "InputMethod", 2, iniPath.c_str());
    if (inputMethod == 0) {
		g_InputMethod = InputMethod::PostMessage;
    }
    else if (inputMethod == 1) {
        g_InputMethod = InputMethod::RawInput;
    }
    else {
        g_InputMethod = InputMethod::Hybrid;
    }

    const char* methodName = "Hybrid";
    if (g_InputMethod == InputMethod::RawInput) {
        methodName = "RawInput";
    }
    else if (g_InputMethod == InputMethod::PostMessage) {
        methodName = "PostMessage";
    }
    LOG("Using Input Method: %s", methodName);

    int gamepadMethod = GetPrivateProfileIntA("API", "GamepadMethod", 0, iniPath.c_str());
    if (gamepadMethod == 1) {
        g_GamepadMethod = GamepadMethod::SDL2;
    }
    else {
        g_GamepadMethod = GamepadMethod::XInput;
    }
	LOG("Using Gamepad Method: %s", (g_GamepadMethod == GamepadMethod::SDL2) ? "SDL2" : "XInput");

    g_EnableOpenXinput = GetPrivateProfileIntA("API", "EnableOpenXinput", 0, iniPath.c_str()) == 1;
    enableXInputMask = GetPrivateProfileIntA("API", "EnableXInputMask", 0, iniPath.c_str()) == 1;
    enableSDL2Mask = GetPrivateProfileIntA("API", "EnableSDL2Mask", 0, iniPath.c_str()) == 1;

    // [Hooks]
    getCursorPosHook = GetPrivateProfileIntA("Hooks", "GetCursorposHook", 1, iniPath.c_str());
    setCursorPosHook = GetPrivateProfileIntA("Hooks", "SetCursorposHook", 1, iniPath.c_str());
    clipCursorHook = GetPrivateProfileIntA("Hooks", "ClipCursorHook", 0, iniPath.c_str());
    getKeyStateHook = GetPrivateProfileIntA("Hooks", "GetKeystateHook", 1, iniPath.c_str());
    getAsyncKeyStateHook = GetPrivateProfileIntA("Hooks", "GetAsynckeystateHook", 1, iniPath.c_str());
    getKeyboardStateHook = GetPrivateProfileIntA("Hooks", "GetKeyboardstateHook", 1, iniPath.c_str());
    setCursorHook = GetPrivateProfileIntA("Hooks", "SetCursorHook", 0, iniPath.c_str());
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
    drawProtoFakeCursor = GetPrivateProfileIntA("Settings", "DrawProtoFakeCursor", 0, iniPath.c_str());
    ShowProtoCursorWhenImageUpdated = GetPrivateProfileIntA("Settings", "ShowCursorWhenImageUpdated", 1, iniPath.c_str());
    if (drawProtoFakeCursor == 1) {
        if (drawfakecursor == 1) {
            LOG("INI Info: Both 'drawfakecursor' and 'drawProtoFakeCursor' are enabled. Prioritizing the ProtoInput cursor.");
            drawfakecursor = 0; // Force the old cursor to be disabled.
        }
    }
    // Let the PointerWindow and RawInput window owned by the main window (game).
    createdWindowIsOwned = GetPrivateProfileIntA("Settings", "CreatedWindowIsOwned", 1, iniPath.c_str());
    responsetime = GetPrivateProfileIntA("Settings", "Responsetime", 4, iniPath.c_str());

    // [StickToMouse]
    righthanded = GetPrivateProfileIntA("StickToMouse", "Righthanded", 2, iniPath.c_str());
    GetPrivateProfileStringA("StickToMouse", "Sensitivity", "5.00", buffer, sizeof(buffer), iniPath.c_str()); sensitivity = std::stof(buffer);
    GetPrivateProfileStringA("StickToMouse", "Sensitivity_Multiplier", "2.40", buffer, sizeof(buffer), iniPath.c_str()); sensitivity_multiplier = std::stof(buffer);
    GetPrivateProfileStringA("StickToMouse", "Horizontal_Sensitivity", "0.00", buffer, sizeof(buffer), iniPath.c_str()); horizontal_sensitivity = std::stof(buffer);
    GetPrivateProfileStringA("StickToMouse", "Vertical_Sensitivity", "0.00", buffer, sizeof(buffer), iniPath.c_str()); vertical_sensitivity = std::stof(buffer);
    GetPrivateProfileStringA("StickToMouse", "Max_Threshold", "0.045", buffer, sizeof(buffer), iniPath.c_str()); max_threshold = std::stof(buffer);
    GetPrivateProfileStringA("StickToMouse", "Radial_Deadzone", "0.15", buffer, sizeof(buffer), iniPath.c_str()); radial_deadzone = std::stof(buffer);
    GetPrivateProfileStringA("StickToMouse", "Axial_Deadzone", "0.0", buffer, sizeof(buffer), iniPath.c_str()); axial_deadzone = std::stof(buffer);
    GetPrivateProfileStringA("StickToMouse", "Look_Accel_Multiplier", "1.15", buffer, sizeof(buffer), iniPath.c_str()); look_accel_multiplier = std::stof(buffer);
    GetPrivateProfileStringA("StickToMouse", "Curve_Slope", "0.16", buffer, sizeof(buffer), iniPath.c_str()); curve_slope = std::stof(buffer);
    GetPrivateProfileStringA("StickToMouse", "Curve_Exponent", "1.85", buffer, sizeof(buffer), iniPath.c_str()); curve_exponent = std::stof(buffer);

    // [KeyMapping]
    g_EnableMouseDoubleClick = GetPrivateProfileIntA("KeyMapping", "EnableMouseDoubleClick", 0, iniPath.c_str()) == 1;
    GetPrivateProfileStringA("KeyMapping", "TriggerThreshold", "40", buffer, sizeof(buffer), iniPath.c_str()); g_TriggerThreshold = std::stof(buffer);
    GetPrivateProfileStringA("KeyMapping", "StickAsButtonDeadzone", "0.25", buffer, sizeof(buffer), iniPath.c_str()); stick_as_button_deadzone = std::stof(buffer);
    
    g_Fn1_ButtonID = -1; g_Fn2_ButtonID = -1;

    LoadButtonLayer("KeyMapping", 0, true, iniPath.c_str());
    // [Fn1]
    LoadButtonLayer("Fn1", 100, false, iniPath.c_str());
    // [Fn2]
    LoadButtonLayer("Fn2", 200, false, iniPath.c_str());

}

void ParseKey(const char* section, const char* key, const char* defaultVal, UINT baseId, int offset, const char* iniPath) {
    char buffer[256];
    GetPrivateProfileStringA(section, key, defaultVal, buffer, sizeof(buffer), iniPath);
    std::string val = buffer;

    // Check for Fn1 / Fn2 in the base layer only
    if (offset == 0) {
        if (val == "Fn1" || val == "fn1") { g_Fn1_ButtonID = baseId; return; }
        if (val == "Fn2" || val == "fn2") { g_Fn2_ButtonID = baseId; return; }
    }
    buttonStates[baseId + offset].actions = Input::ParseActionString(val);
}

void LoadButtonLayer(const char* section, int offset, bool isBaseLayer, const char* iniPath) {
	// Face Buttons
    ParseKey(section, "A", isBaseLayer ? "13" : "0", CUSTOM_ID_A, offset, iniPath);
    ParseKey(section, "B", isBaseLayer ? "0" : "0", CUSTOM_ID_B, offset, iniPath);
    ParseKey(section, "X", isBaseLayer ? "42" : "0", CUSTOM_ID_X, offset, iniPath);
    ParseKey(section, "Y", isBaseLayer ? "0" : "0", CUSTOM_ID_Y, offset, iniPath);

    // Triggers
    ParseKey(section, "LT", isBaseLayer ? "-2" : "0", CUSTOM_ID_LT, offset, iniPath);
    ParseKey(section, "RT", isBaseLayer ? "-1" : "0", CUSTOM_ID_RT, offset, iniPath);

    // Shoulder Buttons
    ParseKey(section, "RB", isBaseLayer ? "0" : "0", CUSTOM_ID_RB, offset, iniPath);
    ParseKey(section, "LB", isBaseLayer ? "0" : "0", CUSTOM_ID_LB, offset, iniPath);

    // D-Pad
    ParseKey(section, "D_UP", isBaseLayer ? "14" : "0", CUSTOM_ID_DPAD_UP, offset, iniPath);
    ParseKey(section, "D_DOWN", isBaseLayer ? "15" : "0", CUSTOM_ID_DPAD_DOWN, offset, iniPath);
    ParseKey(section, "D_LEFT", isBaseLayer ? "16" : "0", CUSTOM_ID_DPAD_LEFT, offset, iniPath);
    ParseKey(section, "D_RIGHT", isBaseLayer ? "17" : "0", CUSTOM_ID_DPAD_RIGHT, offset, iniPath);

    // Start & Back
    ParseKey(section, "Start", isBaseLayer ? "1" : "0", CUSTOM_ID_START, offset, iniPath);
    ParseKey(section, "Back", isBaseLayer ? "3" : "0", CUSTOM_ID_BACK, offset, iniPath);

    // Stick Buttons
    ParseKey(section, "RSB", isBaseLayer ? "0" : "0", CUSTOM_ID_RSB, offset, iniPath);
    ParseKey(section, "LSB", isBaseLayer ? "4" : "0", CUSTOM_ID_LSB, offset, iniPath);

    // Left Stick
    ParseKey(section, "LSU", isBaseLayer ? "47" : "0", CUSTOM_ID_LSU, offset, iniPath);
    ParseKey(section, "LSD", isBaseLayer ? "43" : "0", CUSTOM_ID_LSD, offset, iniPath);
    ParseKey(section, "LSL", isBaseLayer ? "25" : "0", CUSTOM_ID_LSL, offset, iniPath);
    ParseKey(section, "LSR", isBaseLayer ? "28" : "0", CUSTOM_ID_LSR, offset, iniPath);

    // Right Stick
    ParseKey(section, "RSU", isBaseLayer ? "0" : "0", CUSTOM_ID_RSU, offset, iniPath);
    ParseKey(section, "RSD", isBaseLayer ? "0" : "0", CUSTOM_ID_RSD, offset, iniPath);
    ParseKey(section, "RSL", isBaseLayer ? "0" : "0", CUSTOM_ID_RSL, offset, iniPath);
    ParseKey(section, "RSR", isBaseLayer ? "0" : "0", CUSTOM_ID_RSR, offset, iniPath);
}