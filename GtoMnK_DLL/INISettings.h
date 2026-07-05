#pragma once
#include "pch.h"
#include <string>
#include <array>
#include <vector>
#include "MainThread.h"

using namespace GtoMnK;

// Find Window
extern char iniWindowName[256];
extern char iniClassName[256];

// API
extern FakeInputMethod g_FakeInputMethod;

// Hooks
extern int getCursorPosHook, setCursorPosHook, clipCursorHook, getKeyStateHook, getAsyncKeyStateHook, getKeyboardStateHook, setRectHook;

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
extern char waitForDllName[256];
extern bool recheckHWND;
extern bool enableDev;
extern bool disableOverlayOptions;
extern int controllerID;
extern bool g_EnableOpenXinput;
extern int mode;
extern int drawProtoFakeCursor;
extern int ShowProtoCursorWhenImageUpdated;
extern int createdWindowIsOwned;
extern int responsetime;

// StickToMouse
extern int righthanded;
extern int g_thumbStickToMouse[3];
extern bool g_UseLegacyMouseMovement;
extern float radial_deadzone, axial_deadzone, sensitivity, max_threshold, curve_slope, curve_exponent, sensitivity_multiplier, horizontal_sensitivity, vertical_sensitivity, look_accel_multiplier;

// TouchToMouse
extern float touchpad_sensitivity, touchpad_horizontal_sensitivity, touchpad_vertical_sensitivity, touchpad_deadzone, touchpad_smoothing;
extern int g_touchPadToMouse[3];

// KeyMapping
extern bool g_EnableMouseDoubleClick;
extern float stick_as_button_deadzone;
extern float stick_as_button_axial_deadzone;
extern float g_TriggerThreshold;
extern std::array<ButtonState, 256> buttonStates;

void LoadEarlySettings();
void LoadIniSettings();