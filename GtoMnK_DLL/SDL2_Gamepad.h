#pragma once
#include "pch.h"
#include "GamepadState.h"

void SDL2_Initialize();
bool SDL2_GetState(CustomControllerState& outState);
void SDL2_Cleanup();

// Used for SDL2 masking hooks
Uint8 SDLCALL Hook_SDL_GameControllerGetButton(SDL_GameController* gamecontroller, SDL_GameControllerButton button);
Sint16 SDLCALL Hook_SDL_GameControllerGetAxis(SDL_GameController* gamecontroller, SDL_GameControllerAxis axis);

typedef Uint8(SDLCALL* SDL_GameControllerGetButton_t)(SDL_GameController*, SDL_GameControllerButton);
extern SDL_GameControllerGetButton_t TrueSDLGetButton;

typedef Sint16(SDLCALL* SDL_GameControllerGetAxis_t)(SDL_GameController*, SDL_GameControllerAxis);
extern SDL_GameControllerGetAxis_t TrueSDLGetAxis;