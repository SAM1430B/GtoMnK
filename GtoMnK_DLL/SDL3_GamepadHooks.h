#pragma once
#include <windows.h>
#include <SDL3/SDL.h>

namespace GtoMnK {
    namespace SDL3Hooks {
        typedef bool (CDECL* SDL_PollEvent_t)(SDL_Event*); // Returns bool in SDL3
        typedef SDL_Gamepad* (CDECL* SDL_GetGamepadFromID_t)(SDL_JoystickID joyid);

        typedef bool (CDECL* SDL_GetGamepadButton_t)(SDL_Gamepad*, SDL_GamepadButton);
        typedef Sint16(CDECL* SDL_GetGamepadAxis_t)(SDL_Gamepad*, SDL_GamepadAxis);
        typedef int (CDECL* SDL_GetNumGamepadTouchpads_t)(SDL_Gamepad*);
        // Signature completely changed from (Uint8* state) to (bool* down)
        typedef bool (CDECL* SDL_GetGamepadTouchpadFinger_t)(SDL_Gamepad*, int, int, bool*, float*, float*, float*);

        extern SDL_PollEvent_t TrueSDL_PollEvent;
        extern SDL_Gamepad* SnatchedGamepad;

        extern SDL_GetGamepadButton_t TrueSDL_GetGamepadButton;
        extern SDL_GetGamepadAxis_t TrueSDL_GetGamepadAxis;
        extern SDL_GetNumGamepadTouchpads_t TrueSDL_GetNumGamepadTouchpads;
        extern SDL_GetGamepadTouchpadFinger_t TrueSDL_GetGamepadTouchpadFinger;

        bool CDECL Hook_SDL_PollEvent(SDL_Event* event);

        bool CDECL Hook_SDL_GetGamepadButton(SDL_Gamepad* gamepad, SDL_GamepadButton button);
        Sint16 CDECL Hook_SDL_GetGamepadAxis(SDL_Gamepad* gamepad, SDL_GamepadAxis axis);
        int CDECL Hook_SDL_GetNumGamepadTouchpads(SDL_Gamepad* gamepad);
        bool CDECL Hook_SDL_GetGamepadTouchpadFinger(SDL_Gamepad* gamepad, int touchpad, int finger, bool* down, float* x, float* y, float* pressure);

        bool IsSDLEventMasked(const SDL_Event* event);
        UINT SDLButtonToGAMEPAD_ID(SDL_GamepadButton button);
    }
}