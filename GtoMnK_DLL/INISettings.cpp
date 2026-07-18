#include "pch.h"
#include "INISettings.h"
#include "GamepadInputIDs.h"
#include "Logging.h"
#include "FakeInput.h"
#include "GamepadState.h"
#include "InstallHooks.h"

extern HMODULE g_hModule;

std::string g_iniPath = "";

// For Controller Button States
std::array<ButtonState, 256> buttonStates;
bool g_EnableMouseDoubleClick = false;
int righthanded;
int global_thumbStickToMouse, g_thumbStickToMouse[3];
bool g_UseLegacyMouseMovement = false;

// For Controller Setting
int controllerID = 0;
float radial_deadzone, axial_deadzone, sensitivity, max_threshold, curve_slope, curve_exponent, sensitivity_multiplier, horizontal_sensitivity, vertical_sensitivity, look_accel_multiplier;
float stick_as_button_deadzone;
float stick_as_button_axial_deadzone;
float inner_ring_threshold;
float outer_ring_threshold;
float g_TriggerThreshold = 40;
float touchpad_sensitivity, touchpad_horizontal_sensitivity, touchpad_vertical_sensitivity, touchpad_deadzone, touchpad_smoothing;
int global_touchPadToMouse, g_touchPadToMouse[3];

// For Initialization and Thread state
int startUpDelay = 0;
char waitForDllName[256] = { 0 };
bool recheckHWND = true;
bool enableDev = false;
bool disableOverlayOptions = false;
bool g_EnableOpenXinput = true;

// Drawing & cursor state
int drawProtoFakeCursor = 0; // From ProtoInput also for the Cursor visibility hooks
int createdWindowIsOwned = 1;
int ShowProtoCursorWhenImageUpdated = 1; // From ProtoInput
int DontShowCursorWhenImageUpdated = 0; // From ProtoInput, used as a way to enable/disable the ShowProtoCursorWhenImageUpdated behavior.
int mode;
int responsetime = 4;

// For the Hooks
FakeInputMethod g_FakeInputMethod = FakeInputMethod::Hybrid;
int getCursorPosHook, setCursorPosHook, clipCursorHook, getKeyStateHook, getAsyncKeyStateHook, getKeyboardStateHook, setRectHook;

int gamepadMaskHook = 0;

// For MessageFilterHook
bool g_filterRawInput = false;
bool g_filterMouseMove = false;
bool g_filterMouseActivate = false;
bool g_filterWindowActivate = false;
bool g_filterWindowActivateApp = false;
bool g_filterMouseWheel = false;
bool g_filterMouseButton = false;
bool g_filterKeyboardButton = false;

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
void LegacyMouseOption();

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

    g_iniPath = iniPath;

    char buffer[256];

    // [FindWindow]
    GetPrivateProfileStringA("FindWindow", "WindowName", NULL, iniWindowName, sizeof(iniWindowName), iniPath.c_str());
    GetPrivateProfileStringA("FindWindow", "ClassName", NULL, iniClassName, sizeof(iniClassName), iniPath.c_str());

    // [API]
    int fakeInputMethod = GetPrivateProfileIntA("API", "InputMethod", 2, iniPath.c_str());
    if (fakeInputMethod == 0) {
		g_FakeInputMethod = FakeInputMethod::PostMessage;
    }
    else if (fakeInputMethod == 1) {
        g_FakeInputMethod = FakeInputMethod::RawInput;
    }
    else {
        g_FakeInputMethod = FakeInputMethod::Hybrid;
    }


    g_EnableOpenXinput = GetPrivateProfileIntA("API", "EnableOpenXinput", 1, iniPath.c_str()) == 1;

    gamepadMaskHook = GetPrivateProfileIntA("API", "GamepadMaskHook", 0, iniPath.c_str());

    // [Hooks]
    getCursorPosHook = GetPrivateProfileIntA("Hooks", "GetCursorposHook", 1, iniPath.c_str());
    setCursorPosHook = GetPrivateProfileIntA("Hooks", "SetCursorposHook", 1, iniPath.c_str());
    clipCursorHook = GetPrivateProfileIntA("Hooks", "ClipCursorHook", 1, iniPath.c_str());
    getKeyStateHook = GetPrivateProfileIntA("Hooks", "GetKeystateHook", 1, iniPath.c_str());
    getAsyncKeyStateHook = GetPrivateProfileIntA("Hooks", "GetAsynckeystateHook", 1, iniPath.c_str());
    getKeyboardStateHook = GetPrivateProfileIntA("Hooks", "GetKeyboardstateHook", 1, iniPath.c_str());
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
    GetPrivateProfileStringA("Settings", "WaitForDllName", "", waitForDllName, sizeof(waitForDllName), iniPath.c_str());
    recheckHWND = GetPrivateProfileIntA("Settings", "RecheckHWND", 1, iniPath.c_str()) == 1;
    enableDev = GetPrivateProfileIntA("Settings", "EnableDev", 0, iniPath.c_str()) == 1;
    disableOverlayOptions = (GetPrivateProfileIntA("Settings", "DisableOverlayOptions", 0, iniPath.c_str()) != 0);
    controllerID = GetPrivateProfileIntA("Settings", "Controllerid", 0, iniPath.c_str());
    mode = GetPrivateProfileIntA("Settings", "Mode", -1, iniPath.c_str()); // This is replaced with ThumbStickToMouse in v1.4.0
    drawProtoFakeCursor = GetPrivateProfileIntA("Settings", "DrawProtoFakeCursor", 0, iniPath.c_str());
    ShowProtoCursorWhenImageUpdated = GetPrivateProfileIntA("Settings", "ShowCursorWhenImageUpdated", 1, iniPath.c_str());
    DontShowCursorWhenImageUpdated = GetPrivateProfileIntA("Settings", "DontShowCursorWhenImageUpdated", 0, iniPath.c_str());
    
    // Let the PointerWindow and RawInput window owned by the main window (game).
    createdWindowIsOwned = GetPrivateProfileIntA("Settings", "CreatedWindowIsOwned", 1, iniPath.c_str());
    responsetime = GetPrivateProfileIntA("Settings", "Responsetime", 4, iniPath.c_str());

    // [StickToMouse]
    righthanded = GetPrivateProfileIntA("StickToMouse", "Righthanded", -1, iniPath.c_str()); // This is replaced with ThumbStickToMouse in v1.4.0
    global_thumbStickToMouse = GetPrivateProfileIntA("StickToMouse", "ThumbStickToMouse", -1, iniPath.c_str());
    g_UseLegacyMouseMovement = GetPrivateProfileIntA("StickToMouse", "UseLegacyMouseMovement", 0, iniPath.c_str()) == 1;
    LegacyMouseOption();
    GetPrivateProfileStringA("StickToMouse", "Sensitivity", "2.60", buffer, sizeof(buffer), iniPath.c_str()); sensitivity = std::stof(buffer);
    GetPrivateProfileStringA("StickToMouse", "Sensitivity_Multiplier", "2.40", buffer, sizeof(buffer), iniPath.c_str()); sensitivity_multiplier = std::stof(buffer);
    GetPrivateProfileStringA("StickToMouse", "Horizontal_Sensitivity", "0.00", buffer, sizeof(buffer), iniPath.c_str()); horizontal_sensitivity = std::stof(buffer);
    GetPrivateProfileStringA("StickToMouse", "Vertical_Sensitivity", "0.00", buffer, sizeof(buffer), iniPath.c_str()); vertical_sensitivity = std::stof(buffer);
    GetPrivateProfileStringA("StickToMouse", "Max_Threshold", "0.150", buffer, sizeof(buffer), iniPath.c_str()); max_threshold = std::stof(buffer);
    GetPrivateProfileStringA("StickToMouse", "Radial_Deadzone", "0.10", buffer, sizeof(buffer), iniPath.c_str()); radial_deadzone = std::stof(buffer);
    GetPrivateProfileStringA("StickToMouse", "Axial_Deadzone", "0.0", buffer, sizeof(buffer), iniPath.c_str()); axial_deadzone = std::stof(buffer);
    GetPrivateProfileStringA("StickToMouse", "Look_Accel_Multiplier", "1.380", buffer, sizeof(buffer), iniPath.c_str()); look_accel_multiplier = std::stof(buffer);
    GetPrivateProfileStringA("StickToMouse", "Curve_Slope", "0.145", buffer, sizeof(buffer), iniPath.c_str()); curve_slope = std::stof(buffer);
    GetPrivateProfileStringA("StickToMouse", "Curve_Exponent", "1.660", buffer, sizeof(buffer), iniPath.c_str()); curve_exponent = std::stof(buffer);

    // [TouchToMouse]
    global_touchPadToMouse = GetPrivateProfileIntA("TouchToMouse", "TouchpadToMouse", 1, iniPath.c_str()) == 1;
    GetPrivateProfileStringA("TouchToMouse", "Sensitivity", "1500.00", buffer, sizeof(buffer), iniPath.c_str()); touchpad_sensitivity = std::stof(buffer);
    GetPrivateProfileStringA("TouchToMouse", "Horizontal_Sensitivity", "0.00", buffer, sizeof(buffer), iniPath.c_str()); touchpad_horizontal_sensitivity = std::stof(buffer);
    GetPrivateProfileStringA("TouchToMouse", "Vertical_Sensitivity", "0.00", buffer, sizeof(buffer), iniPath.c_str()); touchpad_vertical_sensitivity = std::stof(buffer);
    GetPrivateProfileStringA("TouchToMouse", "Deadzone", "0.0015", buffer, sizeof(buffer), iniPath.c_str()); touchpad_deadzone = std::stof(buffer);
    GetPrivateProfileStringA("TouchToMouse", "Smoothing", "0.65", buffer, sizeof(buffer), iniPath.c_str()); touchpad_smoothing = std::stof(buffer);

    // [KeyMapping]
    g_EnableMouseDoubleClick = GetPrivateProfileIntA("KeyMapping", "EnableMouseDoubleClick", 0, iniPath.c_str()) == 1;
    GetPrivateProfileStringA("KeyMapping", "TriggerThreshold", "40", buffer, sizeof(buffer), iniPath.c_str()); g_TriggerThreshold = std::stof(buffer);
    GetPrivateProfileStringA("KeyMapping", "StickAsButtonDeadzone", "0.25", buffer, sizeof(buffer), iniPath.c_str()); stick_as_button_deadzone = std::stof(buffer);
	GetPrivateProfileStringA("KeyMapping", "StickAsButtonAxialDeadzone", "0.00", buffer, sizeof(buffer), iniPath.c_str()); stick_as_button_axial_deadzone = std::stof(buffer);

    GetPrivateProfileStringA("KeyMapping", "InnerRingThreshold", "0.10", buffer, sizeof(buffer), iniPath.c_str()); inner_ring_threshold = std::stof(buffer);
    GetPrivateProfileStringA("KeyMapping", "OuterRingThreshold", "0.10", buffer, sizeof(buffer), iniPath.c_str()); outer_ring_threshold = std::stof(buffer);
    
    g_Fn1_ButtonID = -1; g_Fn2_ButtonID = -1;

    LoadButtonLayer("KeyMapping", 0, true, iniPath.c_str());
    g_thumbStickToMouse[0] = GetPrivateProfileIntA("KeyMapping", "ThumbstickToMouse", global_thumbStickToMouse, iniPath.c_str());
    g_touchPadToMouse[0] = GetPrivateProfileIntA("KeyMapping", "TouchpadToMouse", global_touchPadToMouse ? 1 : 0, iniPath.c_str()) == 1;

    // [Fn1]
    LoadButtonLayer("Fn1", 100, false, iniPath.c_str());
    g_thumbStickToMouse[1] = GetPrivateProfileIntA("Fn1", "ThumbstickToMouse", global_thumbStickToMouse, iniPath.c_str());
    g_touchPadToMouse[1] = GetPrivateProfileIntA("Fn1", "TouchpadToMouse", global_touchPadToMouse ? 1 : 0, iniPath.c_str()) == 1;
    // [Fn2]
    LoadButtonLayer("Fn2", 200, false, iniPath.c_str());
    g_thumbStickToMouse[2] = GetPrivateProfileIntA("Fn2", "ThumbstickToMouse", global_thumbStickToMouse, iniPath.c_str());
    g_touchPadToMouse[2] = GetPrivateProfileIntA("Fn2", "TouchpadToMouse", global_touchPadToMouse ? 1 : 0, iniPath.c_str()) == 1;

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

    // Safely cast to size_t to satisfy std::array's operator[]
    size_t index = static_cast<size_t>(baseId) + static_cast<size_t>(offset);

    // Safety bounds check to make the compiler happy
    if (index < 256) {
        buttonStates[index].actions = FakeInput::ParseActionString(val);
    }
}

void LoadButtonLayer(const char* section, int offset, bool isBaseLayer, const char* iniPath) {
	// Face Buttons
    ParseKey(section, "A", isBaseLayer ? "0" : "0", GAMEPAD_ID_A, offset, iniPath);
    ParseKey(section, "B", isBaseLayer ? "0" : "0", GAMEPAD_ID_B, offset, iniPath);
    ParseKey(section, "X", isBaseLayer ? "0" : "0", GAMEPAD_ID_X, offset, iniPath);
    ParseKey(section, "Y", isBaseLayer ? "0" : "0", GAMEPAD_ID_Y, offset, iniPath);

    // D-Pad
    ParseKey(section, "D_UP", isBaseLayer ? "0" : "0", GAMEPAD_ID_DPAD_UP, offset, iniPath);
    ParseKey(section, "D_DOWN", isBaseLayer ? "0" : "0", GAMEPAD_ID_DPAD_DOWN, offset, iniPath);
    ParseKey(section, "D_LEFT", isBaseLayer ? "0" : "0", GAMEPAD_ID_DPAD_LEFT, offset, iniPath);
    ParseKey(section, "D_RIGHT", isBaseLayer ? "0" : "0", GAMEPAD_ID_DPAD_RIGHT, offset, iniPath);

    // Start & Back
    ParseKey(section, "Start", isBaseLayer ? "0" : "0", GAMEPAD_ID_START, offset, iniPath);
    ParseKey(section, "Back", isBaseLayer ? "0" : "0", GAMEPAD_ID_BACK, offset, iniPath);

    // Extended Buttons
    ParseKey(section, "Guide", isBaseLayer ? "0" : "0", GAMEPAD_ID_GUIDE, offset, iniPath);
    ParseKey(section, "Misc1", isBaseLayer ? "0" : "0", GAMEPAD_ID_MISC1, offset, iniPath);
    ParseKey(section, "Paddle1", isBaseLayer ? "0" : "0", GAMEPAD_ID_PADDLE1, offset, iniPath);
    ParseKey(section, "Paddle2", isBaseLayer ? "0" : "0", GAMEPAD_ID_PADDLE2, offset, iniPath);
    ParseKey(section, "Paddle3", isBaseLayer ? "0" : "0", GAMEPAD_ID_PADDLE3, offset, iniPath);
    ParseKey(section, "Paddle4", isBaseLayer ? "0" : "0", GAMEPAD_ID_PADDLE4, offset, iniPath);

	// Touchpad Button
	ParseKey(section, "Touchpad_Button", isBaseLayer ? "-1" : "0", GAMEPAD_ID_TOUCHPAD_BUTTON, offset, iniPath);

    // Stick Buttons
    ParseKey(section, "RSB", isBaseLayer ? "0" : "0", GAMEPAD_ID_RSB, offset, iniPath);
    ParseKey(section, "LSB", isBaseLayer ? "0" : "0", GAMEPAD_ID_LSB, offset, iniPath);

    // Shoulder Buttons
    ParseKey(section, "RB", isBaseLayer ? "0" : "0", GAMEPAD_ID_RB, offset, iniPath);
    ParseKey(section, "LB", isBaseLayer ? "0" : "0", GAMEPAD_ID_LB, offset, iniPath);

    // Triggers
    ParseKey(section, "LT", isBaseLayer ? "0" : "0", GAMEPAD_ID_LT, offset, iniPath);
    ParseKey(section, "RT", isBaseLayer ? "0" : "0", GAMEPAD_ID_RT, offset, iniPath);

    // Left Stick As Buttons
    ParseKey(section, "LSU", isBaseLayer ? "0" : "0", GAMEPAD_ID_LSU, offset, iniPath);
    ParseKey(section, "LSD", isBaseLayer ? "0" : "0", GAMEPAD_ID_LSD, offset, iniPath);
    ParseKey(section, "LSL", isBaseLayer ? "0" : "0", GAMEPAD_ID_LSL, offset, iniPath);
    ParseKey(section, "LSR", isBaseLayer ? "0" : "0", GAMEPAD_ID_LSR, offset, iniPath);

    // Left Stick Rings
    ParseKey(section, "LS_Inner", isBaseLayer ? "0" : "0", GAMEPAD_ID_LS_INNER, offset, iniPath);
    ParseKey(section, "LS_Outer", isBaseLayer ? "0" : "0", GAMEPAD_ID_LS_OUTER, offset, iniPath);

    // Right Stick As Buttons
    ParseKey(section, "RSU", isBaseLayer ? "0" : "0", GAMEPAD_ID_RSU, offset, iniPath);
    ParseKey(section, "RSD", isBaseLayer ? "0" : "0", GAMEPAD_ID_RSD, offset, iniPath);
    ParseKey(section, "RSL", isBaseLayer ? "0" : "0", GAMEPAD_ID_RSL, offset, iniPath);
    ParseKey(section, "RSR", isBaseLayer ? "0" : "0", GAMEPAD_ID_RSR, offset, iniPath);

    // Right Stick Rings
    ParseKey(section, "RS_Inner", isBaseLayer ? "0" : "0", GAMEPAD_ID_RS_INNER, offset, iniPath);
    ParseKey(section, "RS_Outer", isBaseLayer ? "0" : "0", GAMEPAD_ID_RS_OUTER, offset, iniPath);
}

// This function is to handle the legacy "Mode" and "Righthanded" options for the ThumbStickToMouse setting, to avoid breaking existing INI configurations.
void LegacyMouseOption() {
    if (global_thumbStickToMouse == -1) {
        if (mode != -1) {
            if (mode == 1) {
                global_thumbStickToMouse = (righthanded == 0) ? 1 : 2;
            }
            else {
                global_thumbStickToMouse = 0;
            }
        }
        else {
            global_thumbStickToMouse = 0;
        }
    }
}

// We need to initialize the logging at the very beginning of the DLL load by enabling the dev mode.
void LoadEarlySettings() {
    std::string iniPath = "";

    std::string exeIniPath = UGetExecutableFolder_main() + "\\GtoMnK.ini";

    if (GetFileAttributesA(exeIniPath.c_str()) != INVALID_FILE_ATTRIBUTES) {
        iniPath = exeIniPath;
    }
    else {
        std::string dllIniPath = UGetDllFolder_main() + "\\GtoMnK.ini";
        if (GetFileAttributesA(dllIniPath.c_str()) != INVALID_FILE_ATTRIBUTES) {
            iniPath = dllIniPath;
        }
    }

    if (!iniPath.empty()) {
        enableDev = GetPrivateProfileIntA("Settings", "EnableDev", 0, iniPath.c_str()) == 1;
    }
}

void ReloadIniSettings()
{
    if (g_iniPath.empty()) return;

    std::string dllIniPath = UGetDllFolder_main() + "\\GtoMnK.ini";

	LOG("Reloading INI settings...");

    char buffer[256];

    // [API]
    int fakeInputMethod = GetPrivateProfileIntA("API", "InputMethod", 2, g_iniPath.c_str());
    if (fakeInputMethod == 0) { g_FakeInputMethod = FakeInputMethod::PostMessage;}
    else if (fakeInputMethod == 1) { g_FakeInputMethod = FakeInputMethod::RawInput;}
    else { g_FakeInputMethod = FakeInputMethod::Hybrid;}

    gamepadMaskHook = GetPrivateProfileIntA("API", "GamepadMaskHook", 0, g_iniPath.c_str());

    // [Hooks]
    getCursorPosHook = GetPrivateProfileIntA("Hooks", "GetCursorposHook", 1, g_iniPath.c_str());
    setCursorPosHook = GetPrivateProfileIntA("Hooks", "SetCursorposHook", 1, g_iniPath.c_str());
    clipCursorHook = GetPrivateProfileIntA("Hooks", "ClipCursorHook", 1, g_iniPath.c_str());
    getKeyStateHook = GetPrivateProfileIntA("Hooks", "GetKeystateHook", 1, g_iniPath.c_str());
    getAsyncKeyStateHook = GetPrivateProfileIntA("Hooks", "GetAsynckeystateHook", 1, g_iniPath.c_str());
    getKeyboardStateHook = GetPrivateProfileIntA("Hooks", "GetKeyboardstateHook", 1, g_iniPath.c_str());
    setRectHook = GetPrivateProfileIntA("Hooks", "SetRectHook", 0, g_iniPath.c_str());

    // [MessageFilter]
    g_filterRawInput = GetPrivateProfileIntA("MessageFilter", "RawInputFilter", 0, g_iniPath.c_str()) == 1;
    g_filterMouseMove = GetPrivateProfileIntA("MessageFilter", "MouseMoveFilter", 0, g_iniPath.c_str()) == 1;
    g_filterMouseActivate = GetPrivateProfileIntA("MessageFilter", "MouseActivateFilter", 0, g_iniPath.c_str()) == 1;
    g_filterWindowActivate = GetPrivateProfileIntA("MessageFilter", "WindowActivateFilter", 0, g_iniPath.c_str()) == 1;
    g_filterWindowActivateApp = GetPrivateProfileIntA("MessageFilter", "WindowActivateAppFilter", 0, g_iniPath.c_str()) == 1;
    g_filterMouseWheel = GetPrivateProfileIntA("MessageFilter", "MouseWheelFilter", 0, g_iniPath.c_str()) == 1;
    g_filterMouseButton = GetPrivateProfileIntA("MessageFilter", "MouseButtonFilter", 0, g_iniPath.c_str()) == 1;
    g_filterKeyboardButton = GetPrivateProfileIntA("MessageFilter", "KeyboardButtonFilter", 0, g_iniPath.c_str()) == 1;

    // [Settings]
    enableDev = GetPrivateProfileIntA("Settings", "EnableDev", 0, g_iniPath.c_str()) == 1;
    DontShowCursorWhenImageUpdated = GetPrivateProfileIntA("Settings", "DontShowCursorWhenImageUpdated", 0, g_iniPath.c_str());

    // [StickToMouse]
    righthanded = GetPrivateProfileIntA("StickToMouse", "Righthanded", -1, g_iniPath.c_str());
    global_thumbStickToMouse = GetPrivateProfileIntA("StickToMouse", "ThumbStickToMouse", -1, g_iniPath.c_str());
    LegacyMouseOption();

    GetPrivateProfileStringA("StickToMouse", "Sensitivity", "2.60", buffer, sizeof(buffer), g_iniPath.c_str()); sensitivity = std::stof(buffer);
    GetPrivateProfileStringA("StickToMouse", "Sensitivity_Multiplier", "2.40", buffer, sizeof(buffer), g_iniPath.c_str()); sensitivity_multiplier = std::stof(buffer);
    GetPrivateProfileStringA("StickToMouse", "Horizontal_Sensitivity", "0.00", buffer, sizeof(buffer), g_iniPath.c_str()); horizontal_sensitivity = std::stof(buffer);
    GetPrivateProfileStringA("StickToMouse", "Vertical_Sensitivity", "0.00", buffer, sizeof(buffer), g_iniPath.c_str()); vertical_sensitivity = std::stof(buffer);
    GetPrivateProfileStringA("StickToMouse", "Max_Threshold", "0.150", buffer, sizeof(buffer), g_iniPath.c_str()); max_threshold = std::stof(buffer);
    GetPrivateProfileStringA("StickToMouse", "Radial_Deadzone", "0.10", buffer, sizeof(buffer), g_iniPath.c_str()); radial_deadzone = std::stof(buffer);
    GetPrivateProfileStringA("StickToMouse", "Axial_Deadzone", "0.0", buffer, sizeof(buffer), g_iniPath.c_str()); axial_deadzone = std::stof(buffer);
    GetPrivateProfileStringA("StickToMouse", "Look_Accel_Multiplier", "1.380", buffer, sizeof(buffer), g_iniPath.c_str()); look_accel_multiplier = std::stof(buffer);
    GetPrivateProfileStringA("StickToMouse", "Curve_Slope", "0.145", buffer, sizeof(buffer), g_iniPath.c_str()); curve_slope = std::stof(buffer);
    GetPrivateProfileStringA("StickToMouse", "Curve_Exponent", "1.660", buffer, sizeof(buffer), g_iniPath.c_str()); curve_exponent = std::stof(buffer);

    // [TouchToMouse]
    global_touchPadToMouse = GetPrivateProfileIntA("TouchToMouse", "TouchpadToMouse", 1, g_iniPath.c_str()) == 1;
    GetPrivateProfileStringA("TouchToMouse", "Sensitivity", "1500.00", buffer, sizeof(buffer), g_iniPath.c_str()); touchpad_sensitivity = std::stof(buffer);
    GetPrivateProfileStringA("TouchToMouse", "Horizontal_Sensitivity", "0.00", buffer, sizeof(buffer), g_iniPath.c_str()); touchpad_horizontal_sensitivity = std::stof(buffer);
    GetPrivateProfileStringA("TouchToMouse", "Vertical_Sensitivity", "0.00", buffer, sizeof(buffer), g_iniPath.c_str()); touchpad_vertical_sensitivity = std::stof(buffer);
    GetPrivateProfileStringA("TouchToMouse", "Deadzone", "0.0015", buffer, sizeof(buffer), g_iniPath.c_str()); touchpad_deadzone = std::stof(buffer);
    GetPrivateProfileStringA("TouchToMouse", "Smoothing", "0.65", buffer, sizeof(buffer), g_iniPath.c_str()); touchpad_smoothing = std::stof(buffer);

    // [KeyMapping]
    g_EnableMouseDoubleClick = GetPrivateProfileIntA("KeyMapping", "EnableMouseDoubleClick", 0, g_iniPath.c_str()) == 1;
    GetPrivateProfileStringA("KeyMapping", "TriggerThreshold", "40", buffer, sizeof(buffer), g_iniPath.c_str()); g_TriggerThreshold = std::stof(buffer);
    GetPrivateProfileStringA("KeyMapping", "StickAsButtonDeadzone", "0.25", buffer, sizeof(buffer), g_iniPath.c_str()); stick_as_button_deadzone = std::stof(buffer);
    GetPrivateProfileStringA("KeyMapping", "StickAsButtonAxialDeadzone", "0.00", buffer, sizeof(buffer), g_iniPath.c_str()); stick_as_button_axial_deadzone = std::stof(buffer);
    GetPrivateProfileStringA("KeyMapping", "InnerRingThreshold", "0.10", buffer, sizeof(buffer), g_iniPath.c_str()); inner_ring_threshold = std::stof(buffer);
    GetPrivateProfileStringA("KeyMapping", "OuterRingThreshold", "0.10", buffer, sizeof(buffer), g_iniPath.c_str()); outer_ring_threshold = std::stof(buffer);

    g_Fn1_ButtonID = -1; g_Fn2_ButtonID = -1;

    LoadButtonLayer("KeyMapping", 0, true, g_iniPath.c_str());
    g_thumbStickToMouse[0] = GetPrivateProfileIntA("KeyMapping", "ThumbstickToMouse", global_thumbStickToMouse, g_iniPath.c_str());
    g_touchPadToMouse[0] = GetPrivateProfileIntA("KeyMapping", "TouchpadToMouse", global_touchPadToMouse ? 1 : 0, g_iniPath.c_str()) == 1;

    // [Fn1]
    LoadButtonLayer("Fn1", 100, false, g_iniPath.c_str());
    g_thumbStickToMouse[1] = GetPrivateProfileIntA("Fn1", "ThumbstickToMouse", global_thumbStickToMouse, g_iniPath.c_str());
    g_touchPadToMouse[1] = GetPrivateProfileIntA("Fn1", "TouchpadToMouse", global_touchPadToMouse ? 1 : 0, g_iniPath.c_str()) == 1;

    // [Fn2]
    LoadButtonLayer("Fn2", 200, false, g_iniPath.c_str());
    g_thumbStickToMouse[2] = GetPrivateProfileIntA("Fn2", "ThumbstickToMouse", global_thumbStickToMouse, g_iniPath.c_str());
    g_touchPadToMouse[2] = GetPrivateProfileIntA("Fn2", "TouchpadToMouse", global_touchPadToMouse ? 1 : 0, g_iniPath.c_str()) == 1;

    // Re-Apply Hooks
    GtoMnK::InstallHooks::RemoveHooks();
    GtoMnK::InstallHooks::SetupHooks();

    LOG("INI Settings successfully reloaded from: %s", getShortenedPath_Manual(dllIniPath).c_str());
}

void SaveIniSettings()
{
    if (g_iniPath.empty()) return;

    std::string dllIniPath = UGetDllFolder_main() + "\\GtoMnK.ini";

    LOG("Saving INI settings...");

    char buffer[64];

    // Check if a key actually exists in the INI
    auto KeyExists = [&](const char* section, const char* key) -> bool {
        char checkBuf[32];
        // "MissingOption" as the default. If the key doesn't exist, it returns this string.
        GetPrivateProfileStringA(section, key, "MissingOption", checkBuf, sizeof(checkBuf), g_iniPath.c_str());
        return strcmp(checkBuf, "MissingOption") != 0;
        };

    // Helper lambdas to format and save data
    auto SaveFloat = [&](const char* section, const char* key, float val, int precision) {
        if (!KeyExists(section, key)) return;

        if (precision == 4) sprintf_s(buffer, "%.4f", val);
        else if (precision == 3) sprintf_s(buffer, "%.3f", val);
        else if (precision == 2) sprintf_s(buffer, "%.2f", val);
        else sprintf_s(buffer, "%.0f", val);
        WritePrivateProfileStringA(section, key, buffer, g_iniPath.c_str());
        };

    auto SaveInt = [&](const char* section, const char* key, int val) {
        if (!KeyExists(section, key)) return;

        sprintf_s(buffer, "%d", val);
        WritePrivateProfileStringA(section, key, buffer, g_iniPath.c_str());
        };

    // [Settings]
    SaveInt("Settings", "EnableDev", enableDev ? 1 : 0);

    // [StickToMouse]
    SaveFloat("StickToMouse", "Sensitivity", sensitivity, 2);
    SaveFloat("StickToMouse", "Sensitivity_Multiplier", sensitivity_multiplier, 2);
    SaveFloat("StickToMouse", "Horizontal_Sensitivity", horizontal_sensitivity, 2);
    SaveFloat("StickToMouse", "Vertical_Sensitivity", vertical_sensitivity, 2);
    SaveFloat("StickToMouse", "Max_Threshold", max_threshold, 3);
    SaveFloat("StickToMouse", "Radial_Deadzone", radial_deadzone, 2);
    SaveFloat("StickToMouse", "Axial_Deadzone", axial_deadzone, 2);
    SaveFloat("StickToMouse", "Look_Accel_Multiplier", look_accel_multiplier, 3);
    SaveFloat("StickToMouse", "Curve_Slope", curve_slope, 3);
    SaveFloat("StickToMouse", "Curve_Exponent", curve_exponent, 3);

    // [KeyMapping]
    SaveFloat("KeyMapping", "TriggerThreshold", g_TriggerThreshold, 0);
    SaveFloat("KeyMapping", "StickAsButtonDeadzone", stick_as_button_deadzone, 2);
    SaveFloat("KeyMapping", "StickAsButtonAxialDeadzone", stick_as_button_axial_deadzone, 2);

    // [TouchToMouse]
    SaveFloat("TouchToMouse", "Sensitivity", touchpad_sensitivity, 2);
    SaveFloat("TouchToMouse", "Horizontal_Sensitivity", touchpad_horizontal_sensitivity, 2);
    SaveFloat("TouchToMouse", "Vertical_Sensitivity", touchpad_vertical_sensitivity, 2);
    SaveFloat("TouchToMouse", "Deadzone", touchpad_deadzone, 4);
    SaveFloat("TouchToMouse", "Smoothing", touchpad_smoothing, 2);

    LOG("INI Settings successfully overwritten at: %s", getShortenedPath_Manual(dllIniPath).c_str());
}