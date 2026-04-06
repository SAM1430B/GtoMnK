#include "pch.h"
#include <SDL.h>
#include "SDL2_Gamepad.h"
#include "GamepadState.h"
#include "INISettings.h"

extern SDL_GameController* g_GameController;

Uint8 SDLCALL Hook_SDL_GameControllerGetButtonMask(SDL_GameController* gamecontroller, SDL_GameControllerButton button) {
    g_GameController = gamecontroller;

    UINT customID = 0;
    switch (button) {
    case SDL_CONTROLLER_BUTTON_A: customID = CUSTOM_ID_A; break;
    case SDL_CONTROLLER_BUTTON_B: customID = CUSTOM_ID_B; break;
    case SDL_CONTROLLER_BUTTON_X: customID = CUSTOM_ID_X; break;
    case SDL_CONTROLLER_BUTTON_Y: customID = CUSTOM_ID_Y; break;
    case SDL_CONTROLLER_BUTTON_LEFTSHOULDER: customID = CUSTOM_ID_LB; break;
    case SDL_CONTROLLER_BUTTON_RIGHTSHOULDER: customID = CUSTOM_ID_RB; break;
    case SDL_CONTROLLER_BUTTON_LEFTSTICK: customID = CUSTOM_ID_LSB; break;
    case SDL_CONTROLLER_BUTTON_RIGHTSTICK: customID = CUSTOM_ID_RSB; break;
    case SDL_CONTROLLER_BUTTON_START: customID = CUSTOM_ID_START; break;
    case SDL_CONTROLLER_BUTTON_BACK: customID = CUSTOM_ID_BACK; break;
    case SDL_CONTROLLER_BUTTON_DPAD_UP: customID = CUSTOM_ID_DPAD_UP; break;
    case SDL_CONTROLLER_BUTTON_DPAD_DOWN: customID = CUSTOM_ID_DPAD_DOWN; break;
    case SDL_CONTROLLER_BUTTON_DPAD_LEFT: customID = CUSTOM_ID_DPAD_LEFT; break;
    case SDL_CONTROLLER_BUTTON_DPAD_RIGHT: customID = CUSTOM_ID_DPAD_RIGHT; break;
    }

    if (enableSDL2Mask && customID != 0 && IsButtonMapped(customID)) {
        return 0;
    }

    if (TrueSDLGetButton) return TrueSDLGetButton(gamecontroller, button);
    return 0;
}

Sint16 SDLCALL Hook_SDL_GameControllerGetAxisMask(SDL_GameController* gamecontroller, SDL_GameControllerAxis axis) {
    g_GameController = gamecontroller;

    bool mapped = false;

    if (mode == 1) {
        if (righthanded == 1 && (axis == SDL_CONTROLLER_AXIS_RIGHTX || axis == SDL_CONTROLLER_AXIS_RIGHTY)) {
            mapped = true; // Hide Right Stick
        }
        else if (righthanded == 0 && (axis == SDL_CONTROLLER_AXIS_LEFTX || axis == SDL_CONTROLLER_AXIS_LEFTY)) {
            mapped = true; // Hide Left Stick
        }
    }

    if (axis == SDL_CONTROLLER_AXIS_TRIGGERLEFT && IsButtonMapped(CUSTOM_ID_LT)) mapped = true;
    if (axis == SDL_CONTROLLER_AXIS_TRIGGERRIGHT && IsButtonMapped(CUSTOM_ID_RT)) mapped = true;
    if (axis == SDL_CONTROLLER_AXIS_LEFTX && (IsButtonMapped(CUSTOM_ID_LSL) || IsButtonMapped(CUSTOM_ID_LSR))) mapped = true;
    if (axis == SDL_CONTROLLER_AXIS_LEFTY && (IsButtonMapped(CUSTOM_ID_LSU) || IsButtonMapped(CUSTOM_ID_LSD))) mapped = true;
    if (axis == SDL_CONTROLLER_AXIS_RIGHTX && (IsButtonMapped(CUSTOM_ID_RSL) || IsButtonMapped(CUSTOM_ID_RSR))) mapped = true;
    if (axis == SDL_CONTROLLER_AXIS_RIGHTY && (IsButtonMapped(CUSTOM_ID_RSU) || IsButtonMapped(CUSTOM_ID_RSD))) mapped = true;

    if (enableSDL2Mask && mapped) {
        return 0;
    }

    if (TrueSDLGetAxis) return TrueSDLGetAxis(gamecontroller, axis);
    return 0;
}