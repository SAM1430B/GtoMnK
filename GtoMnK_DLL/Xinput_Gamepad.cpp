#include "pch.h"
#include "XInput_Gamepad.h"
#include "INISettings.h"
#include "EnableOpenXinput.h"
#include "Logging.h"

XInputGetStateFunc pXInputGetState = nullptr;
XInputGetState_t TrueXInputGetState = nullptr;
extern "C" BOOL WINAPI OpenXinputDllMain(HINSTANCE hInstDll, DWORD fdwReason, LPVOID lpvReserved);

extern int mode;
extern int righthanded;

void XInput_Initialize() {
    if (g_EnableOpenXinput) {
        LOG("Initializing OpenXinput...");
        OpenXinputDllMain(GetModuleHandle(NULL), DLL_PROCESS_ATTACH, NULL);
        pXInputGetState = (XInputGetStateFunc)OXI::OpenXInputGetState;
    }
    else {
        LOG("Using Standard XInput.");
        pXInputGetState = (XInputGetStateFunc)XInputGetState;
    }
}

// Helper local to this file
bool IsXInputBtnPressed(const XINPUT_STATE& state, WORD flag) {
    return (state.Gamepad.wButtons & flag) != 0;
}

bool XInput_GetState(CustomControllerState& outState) {
    if (controllerID < 0) return false;

    XINPUT_STATE state;
    ZeroMemory(&state, sizeof(XINPUT_STATE));

    DWORD result;

    if (enableXInputMask && TrueXInputGetState != nullptr && !g_EnableOpenXinput) {
        result = TrueXInputGetState(controllerID, &state);
    }
    else {
        result = pXInputGetState(controllerID, &state);
    }

    if (result != ERROR_SUCCESS) {
        return false;
    }

    // Buttons
    outState.buttons[CUSTOM_ID_A] = IsXInputBtnPressed(state, XINPUT_GAMEPAD_A);
    outState.buttons[CUSTOM_ID_B] = IsXInputBtnPressed(state, XINPUT_GAMEPAD_B);
    outState.buttons[CUSTOM_ID_X] = IsXInputBtnPressed(state, XINPUT_GAMEPAD_X);
    outState.buttons[CUSTOM_ID_Y] = IsXInputBtnPressed(state, XINPUT_GAMEPAD_Y);

    outState.buttons[CUSTOM_ID_LB] = IsXInputBtnPressed(state, XINPUT_GAMEPAD_LEFT_SHOULDER);
    outState.buttons[CUSTOM_ID_RB] = IsXInputBtnPressed(state, XINPUT_GAMEPAD_RIGHT_SHOULDER);
    outState.buttons[CUSTOM_ID_LSB] = IsXInputBtnPressed(state, XINPUT_GAMEPAD_LEFT_THUMB);
    outState.buttons[CUSTOM_ID_RSB] = IsXInputBtnPressed(state, XINPUT_GAMEPAD_RIGHT_THUMB);

    outState.buttons[CUSTOM_ID_START] = IsXInputBtnPressed(state, XINPUT_GAMEPAD_START);
    outState.buttons[CUSTOM_ID_BACK] = IsXInputBtnPressed(state, XINPUT_GAMEPAD_BACK);

    outState.buttons[CUSTOM_ID_DPAD_UP] = IsXInputBtnPressed(state, XINPUT_GAMEPAD_DPAD_UP);
    outState.buttons[CUSTOM_ID_DPAD_DOWN] = IsXInputBtnPressed(state, XINPUT_GAMEPAD_DPAD_DOWN);
    outState.buttons[CUSTOM_ID_DPAD_LEFT] = IsXInputBtnPressed(state, XINPUT_GAMEPAD_DPAD_LEFT);
    outState.buttons[CUSTOM_ID_DPAD_RIGHT] = IsXInputBtnPressed(state, XINPUT_GAMEPAD_DPAD_RIGHT);

    // Triggers
    outState.LeftTrigger = state.Gamepad.bLeftTrigger;
    outState.RightTrigger = state.Gamepad.bRightTrigger;

    // Left Thumbsticks
    outState.ThumbLX = state.Gamepad.sThumbLX;
    outState.ThumbLY = state.Gamepad.sThumbLY;

    // Right Thumbsticks
    outState.ThumbRX = state.Gamepad.sThumbRX;
    outState.ThumbRY = state.Gamepad.sThumbRY;

    return true;
}

void MaskXInputStateForGame(XINPUT_STATE* pState) {
    if (!pState) return;

    if (mode == 1) {
        if (righthanded == 1) {
            pState->Gamepad.sThumbRX = 0;
            pState->Gamepad.sThumbRY = 0;
        }
        else if (righthanded == 0) {
            pState->Gamepad.sThumbLX = 0;
            pState->Gamepad.sThumbLY = 0;
        }
    }

    // Mask Face Buttons
    if (IsButtonMapped(CUSTOM_ID_A)) pState->Gamepad.wButtons &= ~XINPUT_GAMEPAD_A;
    if (IsButtonMapped(CUSTOM_ID_B)) pState->Gamepad.wButtons &= ~XINPUT_GAMEPAD_B;
    if (IsButtonMapped(CUSTOM_ID_X)) pState->Gamepad.wButtons &= ~XINPUT_GAMEPAD_X;
    if (IsButtonMapped(CUSTOM_ID_Y)) pState->Gamepad.wButtons &= ~XINPUT_GAMEPAD_Y;

    // Mask Shoulder Buttons & Thumbs
    if (IsButtonMapped(CUSTOM_ID_LB)) pState->Gamepad.wButtons &= ~XINPUT_GAMEPAD_LEFT_SHOULDER;
    if (IsButtonMapped(CUSTOM_ID_RB)) pState->Gamepad.wButtons &= ~XINPUT_GAMEPAD_RIGHT_SHOULDER;
    if (IsButtonMapped(CUSTOM_ID_LSB)) pState->Gamepad.wButtons &= ~XINPUT_GAMEPAD_LEFT_THUMB;
    if (IsButtonMapped(CUSTOM_ID_RSB)) pState->Gamepad.wButtons &= ~XINPUT_GAMEPAD_RIGHT_THUMB;

    // Mask Start / Back
    if (IsButtonMapped(CUSTOM_ID_START)) pState->Gamepad.wButtons &= ~XINPUT_GAMEPAD_START;
    if (IsButtonMapped(CUSTOM_ID_BACK)) pState->Gamepad.wButtons &= ~XINPUT_GAMEPAD_BACK;

    // Mask D-Pad
    if (IsButtonMapped(CUSTOM_ID_DPAD_UP)) pState->Gamepad.wButtons &= ~XINPUT_GAMEPAD_DPAD_UP;
    if (IsButtonMapped(CUSTOM_ID_DPAD_DOWN)) pState->Gamepad.wButtons &= ~XINPUT_GAMEPAD_DPAD_DOWN;
    if (IsButtonMapped(CUSTOM_ID_DPAD_LEFT)) pState->Gamepad.wButtons &= ~XINPUT_GAMEPAD_DPAD_LEFT;
    if (IsButtonMapped(CUSTOM_ID_DPAD_RIGHT)) pState->Gamepad.wButtons &= ~XINPUT_GAMEPAD_DPAD_RIGHT;

    // Mask Triggers (set to 0 if mapped)
    if (IsButtonMapped(CUSTOM_ID_LT)) pState->Gamepad.bLeftTrigger = 0;
    if (IsButtonMapped(CUSTOM_ID_RT)) pState->Gamepad.bRightTrigger = 0;

    // Mask Left Stick Axis if directions are mapped
    if (IsButtonMapped(CUSTOM_ID_LSU) || IsButtonMapped(CUSTOM_ID_LSD)) pState->Gamepad.sThumbLY = 0;
    if (IsButtonMapped(CUSTOM_ID_LSL) || IsButtonMapped(CUSTOM_ID_LSR)) pState->Gamepad.sThumbLX = 0;

    // Mask Right Stick Axis if directions are mapped
    if (IsButtonMapped(CUSTOM_ID_RSU) || IsButtonMapped(CUSTOM_ID_RSD)) pState->Gamepad.sThumbRY = 0;
    if (IsButtonMapped(CUSTOM_ID_RSL) || IsButtonMapped(CUSTOM_ID_RSR)) pState->Gamepad.sThumbRX = 0;
}

DWORD WINAPI Hook_XInputGetState(DWORD dwUserIndex, XINPUT_STATE* pState) {
    // Call the original function to get the real hardware state
    DWORD result = TrueXInputGetState(dwUserIndex, pState);

    if (result == ERROR_SUCCESS) {
        MaskXInputStateForGame(pState);
    }
    return result;
}