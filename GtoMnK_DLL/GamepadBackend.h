/*
  This header file serves as the Gamepad API Wrapper.
  Handles gamepad setup, polling, and cleanup.
*/

#pragma once
#include "INISettings.h"

#if defined(USE_XINPUT)
#include "Xinput_Gamepad.h"
#elif defined(USE_SDL2)
#include "SDL2_Gamepad.h"
#elif defined(USE_SDL3)
 #include "SDL3_Gamepad.h"
#endif

namespace GamepadBackend {

#if defined(USE_XINPUT)
    inline void Backend_Initialize() { XInput_Initialize(); }
    inline bool Backend_GetState(CustomControllerState& state) { return XInput_GetState(state); }
    inline void Backend_Cleanup() { XInput_Cleanup(); }

#elif defined(USE_SDL2)
    inline void Backend_Initialize() { SDL2_Initialize(); }
    inline bool Backend_GetState(CustomControllerState& state) { return SDL2_GetState(state); }
    inline void Backend_Cleanup() { SDL2_Cleanup(); }

#elif defined(USE_SDL3)
    inline void Backend_Initialize() { SDL3_Initialize(); }
    inline bool Backend_GetState(CustomControllerState& state) { return SDL3_GetState(state); }
    inline void Backend_Cleanup() { SDL3_Cleanup(); }
#endif

    inline void Initialize() {
        Backend_Initialize();
    }

    inline bool GetState(CustomControllerState& state) {
        return Backend_GetState(state);
    }

    inline void Cleanup() {
        Backend_Cleanup();
    }
}