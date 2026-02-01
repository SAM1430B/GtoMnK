#include "pch.h"
#include "XInput_Gamepad.h"
#include "INISettings.h"
#include "EnableOpenXinput.h" 
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

    if (pXInputGetState(controllerID, &state) != ERROR_SUCCESS) {
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