#pragma once
#include "pch.h"
#include "GamepadState.h"

void SDL2_Initialize();
bool SDL2_GetState(CustomControllerState& outState);
void SDL2_Cleanup();

// Used for SDL2 masking hooks
Uint8 SDLCALL Hook_SDL_GameControllerGetButtonMask(SDL_GameController* gamecontroller, SDL_GameControllerButton button);
Sint16 SDLCALL Hook_SDL_GameControllerGetAxisMask(SDL_GameController* gamecontroller, SDL_GameControllerAxis axis);
int SDLCALL Hook_SDL_GameControllerGetNumTouchpads(SDL_GameController* gamecontroller);

typedef Uint8(SDLCALL* SDL_GameControllerGetButton_t)(SDL_GameController*, SDL_GameControllerButton);
extern SDL_GameControllerGetButton_t TrueSDLGetButton;

typedef Sint16(SDLCALL* SDL_GameControllerGetAxis_t)(SDL_GameController*, SDL_GameControllerAxis);
extern SDL_GameControllerGetAxis_t TrueSDLGetAxis;

typedef int(SDLCALL* SDL_GameControllerGetNumTouchpads_t)(SDL_GameController*);
extern SDL_GameControllerGetNumTouchpads_t TrueSDLGetNumTouchpads;

typedef int(SDLCALL* SDL_GameControllerGetTouchpadFinger_t)(SDL_GameController*, int, int, Uint8*, float*, float*, float*);
extern SDL_GameControllerGetTouchpadFinger_t TrueSDLGetTouchpadFinger;