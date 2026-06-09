/*
 * This file acts as the dynamic masking bridge between the custom input configuration
 * (INISettings) and the API interception hooks (XInput/SDL2).
 * * These functions evaluate the controller's current state against the
 * user's custom mappings in real-time, accounting for base layers,
 * active modifiers (Fn1/Fn2), and global Stick/Touch-to-Mouse settings.
 * They return boolean flags used by the hooking system to determine
 * whether a native game input should be suppressed (masked) or allowed
 * to pass through to the game.
 */

#include "pch.h"
#include "GamepadMasking.h"
#include "GamepadInputIDs.h"
#include "GamepadState.h"
#include "INISettings.h"

extern bool g_Fn1_State;
extern bool g_Fn2_State;

bool IsButtonMapped(UINT buttonFlag) {
    if (buttonFlag == g_Fn1_ButtonID || buttonFlag == g_Fn2_ButtonID) {
        return true;
    }

    int currentOffset = 0;
    if (g_Fn2_State) currentOffset = 200;
    else if (g_Fn1_State) currentOffset = 100;

    UINT index = buttonFlag + currentOffset;
    if (index < 256) {
        const ButtonState& state = buttonStates[index];
        if (!state.actions.empty() && !state.actions[0].keycodes.empty()) {
            return true;
        }
    }

    return false;
}

bool IsTriggerMapped(UINT triggerID) {
    return IsButtonMapped(triggerID);
}

bool IsLeftStickMapped() {
    int activeThumbStick = GetActiveThumbStickToMouse();
    if (activeThumbStick == 1) return true;

    if (IsButtonMapped(GAMEPAD_ID_LSU) || IsButtonMapped(GAMEPAD_ID_LSD) ||
        IsButtonMapped(GAMEPAD_ID_LSL) || IsButtonMapped(GAMEPAD_ID_LSR)) {
        return true;
    }
    return false;
}

bool IsRightStickMapped() {
    int activeThumbStick = GetActiveThumbStickToMouse();
    if (activeThumbStick == 2) return true;

    if (IsButtonMapped(GAMEPAD_ID_RSU) || IsButtonMapped(GAMEPAD_ID_RSD) ||
        IsButtonMapped(GAMEPAD_ID_RSL) || IsButtonMapped(GAMEPAD_ID_RSR)) {
        return true;
    }
    return false;
}

bool IsTouchpadMapped() {
    if (GetActiveTouchPadToMouse() == 1) return true;
    if (IsButtonMapped(GAMEPAD_ID_TOUCHPAD_BUTTON)) return true;
    return false;
}