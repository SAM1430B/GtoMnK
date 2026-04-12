#include "pch.h"
#include <Xinput.h>
#include "XInput_Gamepad.h"
#include "GamepadState.h"
#include "INISettings.h"
#include "OverlayMenu.h"

void MaskXInputStateForGame(XINPUT_STATE* pState) {
    if (!pState) return;

	// Disable the Native XInput buttons when the overlay menu is open.
    if (!disableOverlayOptions && OverlayMenu::state.isMenuOpen) {
        pState->Gamepad.wButtons = 0;
        pState->Gamepad.bLeftTrigger = 0;
        pState->Gamepad.bRightTrigger = 0;
        pState->Gamepad.sThumbLX = 0;
        pState->Gamepad.sThumbLY = 0;
        pState->Gamepad.sThumbRX = 0;
        pState->Gamepad.sThumbRY = 0;
        return;
    }

    if (mode == 1 && !IsMouseDisabledForCurrentLayer()) {
        if (righthanded == 1) {
            pState->Gamepad.sThumbRX = 0;
            pState->Gamepad.sThumbRY = 0;
        }
        else if (righthanded == 0) {
            pState->Gamepad.sThumbLX = 0;
            pState->Gamepad.sThumbLY = 0;
        }
    }

        if (IsButtonMapped(CUSTOM_ID_A)) pState->Gamepad.wButtons &= ~XINPUT_GAMEPAD_A;
        if (IsButtonMapped(CUSTOM_ID_B)) pState->Gamepad.wButtons &= ~XINPUT_GAMEPAD_B;
        if (IsButtonMapped(CUSTOM_ID_X)) pState->Gamepad.wButtons &= ~XINPUT_GAMEPAD_X;
        if (IsButtonMapped(CUSTOM_ID_Y)) pState->Gamepad.wButtons &= ~XINPUT_GAMEPAD_Y;
        if (IsButtonMapped(CUSTOM_ID_LB)) pState->Gamepad.wButtons &= ~XINPUT_GAMEPAD_LEFT_SHOULDER;
        if (IsButtonMapped(CUSTOM_ID_RB)) pState->Gamepad.wButtons &= ~XINPUT_GAMEPAD_RIGHT_SHOULDER;
        if (IsButtonMapped(CUSTOM_ID_LSB)) pState->Gamepad.wButtons &= ~XINPUT_GAMEPAD_LEFT_THUMB;
        if (IsButtonMapped(CUSTOM_ID_RSB)) pState->Gamepad.wButtons &= ~XINPUT_GAMEPAD_RIGHT_THUMB;
        if (IsButtonMapped(CUSTOM_ID_START)) pState->Gamepad.wButtons &= ~XINPUT_GAMEPAD_START;
        if (IsButtonMapped(CUSTOM_ID_BACK)) pState->Gamepad.wButtons &= ~XINPUT_GAMEPAD_BACK;
        if (IsButtonMapped(CUSTOM_ID_DPAD_UP)) pState->Gamepad.wButtons &= ~XINPUT_GAMEPAD_DPAD_UP;
        if (IsButtonMapped(CUSTOM_ID_DPAD_DOWN)) pState->Gamepad.wButtons &= ~XINPUT_GAMEPAD_DPAD_DOWN;
        if (IsButtonMapped(CUSTOM_ID_DPAD_LEFT)) pState->Gamepad.wButtons &= ~XINPUT_GAMEPAD_DPAD_LEFT;
        if (IsButtonMapped(CUSTOM_ID_DPAD_RIGHT)) pState->Gamepad.wButtons &= ~XINPUT_GAMEPAD_DPAD_RIGHT;
        if (IsButtonMapped(CUSTOM_ID_LT)) pState->Gamepad.bLeftTrigger = 0;
        if (IsButtonMapped(CUSTOM_ID_RT)) pState->Gamepad.bRightTrigger = 0;
        if (IsButtonMapped(CUSTOM_ID_LSU) || IsButtonMapped(CUSTOM_ID_LSD)) pState->Gamepad.sThumbLY = 0;
        if (IsButtonMapped(CUSTOM_ID_LSL) || IsButtonMapped(CUSTOM_ID_LSR)) pState->Gamepad.sThumbLX = 0;
        if (IsButtonMapped(CUSTOM_ID_RSU) || IsButtonMapped(CUSTOM_ID_RSD)) pState->Gamepad.sThumbRY = 0;
        if (IsButtonMapped(CUSTOM_ID_RSL) || IsButtonMapped(CUSTOM_ID_RSR)) pState->Gamepad.sThumbRX = 0;
    }

DWORD WINAPI Hook_XInputGetStateMask(DWORD dwUserIndex, XINPUT_STATE* pState) {
    // Call the original function to get the real hardware state
    DWORD result = TrueXInputGetState(dwUserIndex, pState);

    if (result == ERROR_SUCCESS) {
        MaskXInputStateForGame(pState);
    }
    return result;
}