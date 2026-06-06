/*
  This header file serves as the Gamepad API Wrapper.
  Handles gamepad setup, polling, and cleanup.
*/

#pragma once
#include "Xinput_Gamepad.h"
#include "SDL2_Gamepad.h"
#include "INISettings.h"

namespace GamepadBackend {

    inline void Initialize() {
        if (g_GamepadMethod == GamepadMethod::XInput) {
            XInput_Initialize();
        }
        else {
            SDL2_Initialize();
        }
    }

    inline bool GetState(CustomControllerState& state) {
        switch (g_GamepadMethod) {
        case GamepadMethod::XInput:
            return XInput_GetState(state);
        case GamepadMethod::SDL2:
            return SDL2_GetState(state);
        default:
            return XInput_GetState(state);
        }
    }

    inline void Cleanup() {
        if (g_GamepadMethod == GamepadMethod::SDL2) {
            SDL2_Cleanup();
        }
    }
}