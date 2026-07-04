#include "pch.h"

#if defined(USE_XINPUT)
#include <Xinput.h>

namespace OXI {
#define OPENXINPUT_XUSER_MAX_COUNT 64
#include "OpenXinput.h"
}

#include "XInput_Gamepad.h"
#include "GamepadInputIDs.h"
#include "INISettings.h"
#include "EnableOpenXinput.h"
#include "XInput_GamepadHooks.h"
#include "Logging.h"

XInputGetStateFunc pXInputGetState = nullptr;
extern "C" BOOL WINAPI OpenXinputDllMain(HINSTANCE hInstDll, DWORD fdwReason, LPVOID lpvReserved);

void XInput_Initialize() {
    if (g_EnableOpenXinput) {
        LOG("Initializing OpenXinput...");
        OpenXinputDllMain(GetModuleHandle(NULL), DLL_PROCESS_ATTACH, NULL);
        pXInputGetState = (XInputGetStateFunc)OXI::OpenXInputGetState;
    }
    else {
        pXInputGetState = nullptr;
    }
}

void XInput_Cleanup() {
    // OpenXinput doesn't strictly require a teardown call, so we leave this empty 
    // to satisfy the GamepadBackend wrapper API.
}

// Helper grabs a pure, unhooked pointer straight from Windows to bypass the MaskHook
XInputGetStateFunc GetSystemXInputGetState() {
    static XInputGetStateFunc RealSystemGetState = nullptr;

    if (!RealSystemGetState) {
        HMODULE hXInput = LoadLibraryA("xinput1_4.dll");
        if (!hXInput) hXInput = LoadLibraryA("xinput1_3.dll");
        if (!hXInput) hXInput = LoadLibraryA("xinput9_1_0.dll");

        if (hXInput) {
            RealSystemGetState = (XInputGetStateFunc)GetProcAddress(hXInput, "XInputGetState");
        }
    }

    return RealSystemGetState;
}

// Helper local to this file
bool IsXInputBtnPressed(const XINPUT_STATE& state, WORD flag) {
    return (state.Gamepad.wButtons & flag) != 0;
}

bool XInput_GetState(CustomControllerState& outState) {
    if (controllerID < 0) return false;

    XInputGetStateFunc GetStateFunc = pXInputGetState;
    if (!g_EnableOpenXinput) {
        GetStateFunc = GetSystemXInputGetState();
    }

    if (!GetStateFunc) return false;

    XINPUT_STATE state;
    ZeroMemory(&state, sizeof(XINPUT_STATE));

    if (GetStateFunc(controllerID, &state) != ERROR_SUCCESS) {
        return false;
    }

    // Face Buttons
    outState.buttons[GAMEPAD_ID_A] = IsXInputBtnPressed(state, XINPUT_GAMEPAD_A);
    outState.buttons[GAMEPAD_ID_B] = IsXInputBtnPressed(state, XINPUT_GAMEPAD_B);
    outState.buttons[GAMEPAD_ID_X] = IsXInputBtnPressed(state, XINPUT_GAMEPAD_X);
    outState.buttons[GAMEPAD_ID_Y] = IsXInputBtnPressed(state, XINPUT_GAMEPAD_Y);

    // D-Pad
    outState.buttons[GAMEPAD_ID_DPAD_UP] = IsXInputBtnPressed(state, XINPUT_GAMEPAD_DPAD_UP);
    outState.buttons[GAMEPAD_ID_DPAD_DOWN] = IsXInputBtnPressed(state, XINPUT_GAMEPAD_DPAD_DOWN);
    outState.buttons[GAMEPAD_ID_DPAD_LEFT] = IsXInputBtnPressed(state, XINPUT_GAMEPAD_DPAD_LEFT);
    outState.buttons[GAMEPAD_ID_DPAD_RIGHT] = IsXInputBtnPressed(state, XINPUT_GAMEPAD_DPAD_RIGHT);

    // Start & Back
    outState.buttons[GAMEPAD_ID_START] = IsXInputBtnPressed(state, XINPUT_GAMEPAD_START);
    outState.buttons[GAMEPAD_ID_BACK] = IsXInputBtnPressed(state, XINPUT_GAMEPAD_BACK);

    // Extended Buttons
    // Extended buttons are not supported in XInput.
    outState.buttons[GAMEPAD_ID_GUIDE] = false;
    outState.buttons[GAMEPAD_ID_MISC1] = false;
    outState.buttons[GAMEPAD_ID_PADDLE1] = false;
    outState.buttons[GAMEPAD_ID_PADDLE2] = false;
    outState.buttons[GAMEPAD_ID_PADDLE3] = false;
    outState.buttons[GAMEPAD_ID_PADDLE4] = false;

    // Touchpad Button
	// Touchpad is not supported in XInput.
    outState.buttons[GAMEPAD_ID_TOUCHPAD_BUTTON] = false;

    // Stick Buttons
    outState.buttons[GAMEPAD_ID_LSB] = IsXInputBtnPressed(state, XINPUT_GAMEPAD_LEFT_THUMB);
    outState.buttons[GAMEPAD_ID_RSB] = IsXInputBtnPressed(state, XINPUT_GAMEPAD_RIGHT_THUMB);

    // Shoulder Buttons
    outState.buttons[GAMEPAD_ID_LB] = IsXInputBtnPressed(state, XINPUT_GAMEPAD_LEFT_SHOULDER);
    outState.buttons[GAMEPAD_ID_RB] = IsXInputBtnPressed(state, XINPUT_GAMEPAD_RIGHT_SHOULDER);

    // Triggers
    outState.LeftTrigger = state.Gamepad.bLeftTrigger;
    outState.RightTrigger = state.Gamepad.bRightTrigger;

    // Left Thumbsticks
    outState.ThumbLX = state.Gamepad.sThumbLX;
    outState.ThumbLY = state.Gamepad.sThumbLY;

    // Right Thumbsticks
    outState.ThumbRX = state.Gamepad.sThumbRX;
    outState.ThumbRY = state.Gamepad.sThumbRY;

    // Touchpad
    // Touchpad is not supported in XInput.
    outState.TouchpadActive = false;
    outState.TouchpadX = 0.0f;
    outState.TouchpadY = 0.0f;
    outState.TouchpadPressure = 0.0f;

    return true;
}

#endif // USE_XINPUT