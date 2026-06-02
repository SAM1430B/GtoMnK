#pragma once
#include "pch.h"
#include <string>
#include <map>
#include <vector>
#include "MainThread.h"

using namespace GtoMnK;

// Gamepad Input Method
enum class GamepadMethod {
    XInput, // 0
    SDL2 // 1
};

// Find Window
extern char iniWindowName[256];
extern char iniClassName[256];

// API
extern InputMethod g_InputMethod;
extern GamepadMethod g_GamepadMethod;

// Hooks
extern int getCursorPosHook, setCursorPosHook, clipCursorHook, getKeyStateHook, getAsyncKeyStateHook, getKeyboardStateHook, setCursorHook, setRectHook;

// MessageFilter
extern bool g_filterRawInput;
extern bool g_filterMouseMove;
extern bool g_filterMouseActivate;
extern bool g_filterWindowActivate;
extern bool g_filterWindowActivateApp;
extern bool g_filterMouseWheel;
extern bool g_filterMouseButton;
extern bool g_filterKeyboardButton;

// General Settings
extern int startUpDelay;
extern bool recheckHWND;
extern bool disableOverlayOptions;
extern int controllerID;
extern bool g_EnableOpenXinput;
extern int mode;
extern int drawfakecursor;
extern int drawProtoFakeCursor;
extern int ShowProtoCursorWhenImageUpdated;
extern int createdWindowIsOwned;
extern int responsetime;

// StickToMouse
extern int righthanded;
extern int g_thumbStickToMouse[3];
extern float radial_deadzone, axial_deadzone, sensitivity, max_threshold, curve_slope, curve_exponent, sensitivity_multiplier, horizontal_sensitivity, vertical_sensitivity, look_accel_multiplier;

// TouchToMouse
extern float touchpad_sensitivity, touchpad_horizontal_sensitivity, touchpad_vertical_sensitivity, touchpad_deadzone, touchpad_smoothing;
extern int g_touchPadToMouse[3];

// KeyMapping
extern bool g_EnableMouseDoubleClick;
extern float stick_as_button_deadzone;
extern float g_TriggerThreshold;
extern std::map<UINT, ButtonState> buttonStates;

void LoadIniSettings();