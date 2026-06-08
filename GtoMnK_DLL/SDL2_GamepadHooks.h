#pragma once
#include <windows.h>
#include <SDL.h>

namespace GtoMnK {
    namespace SDL2Hooks {
        typedef int(CDECL* SDL_PollEvent_t)(SDL_Event*);
        typedef SDL_GameController* (CDECL* SDL_GameControllerFromInstanceID_t)(SDL_JoystickID joyid);

        typedef Uint8(CDECL* SDL_GameControllerGetButton_t)(SDL_GameController*, SDL_GameControllerButton);
        typedef Sint16(CDECL* SDL_GameControllerGetAxis_t)(SDL_GameController*, SDL_GameControllerAxis);
        typedef int(CDECL* SDL_GameControllerGetNumTouchpads_t)(SDL_GameController*);
        typedef int(CDECL* SDL_GameControllerGetTouchpadFinger_t)(SDL_GameController*, int, int, Uint8*, float*, float*, float*);
        

        extern SDL_PollEvent_t TrueSDL_PollEvent;
        extern SDL_GameController* SnatchedGameController;

        extern SDL_GameControllerGetButton_t TrueSDL_GameControllerGetButton;
        extern SDL_GameControllerGetAxis_t TrueSDL_GameControllerGetAxis;
        extern SDL_GameControllerGetNumTouchpads_t TrueSDL_GameControllerGetNumTouchpads;
        extern SDL_GameControllerGetTouchpadFinger_t TrueSDL_GameControllerGetTouchpadFinger;

        int CDECL Hook_SDL_PollEvent(SDL_Event* event);

        Uint8 CDECL Hook_SDL_GameControllerGetButton(SDL_GameController* gamecontroller, SDL_GameControllerButton button);
        Sint16 CDECL Hook_SDL_GameControllerGetAxis(SDL_GameController* gamecontroller, SDL_GameControllerAxis axis);
        int CDECL Hook_SDL_GameControllerGetNumTouchpads(SDL_GameController* gamecontroller);
        int CDECL Hook_SDL_GameControllerGetTouchpadFinger(SDL_GameController* gamecontroller, int touchpad, int finger, Uint8* state, float* x, float* y, float* pressure);

        bool IsSDLEventMasked(const SDL_Event* event);
        UINT SDLButtonToGAMEPAD_ID(Uint8 button);
    }
}