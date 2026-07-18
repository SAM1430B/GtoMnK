#include "pch.h"

#if defined(USE_SDL2)

#include "SDL2_GamepadHooks.h"
#include "GamepadMasking.h"
#include "GamepadInputIDs.h"
#include "OverlayMenu.h"
#include "INISettings.h"

namespace GtoMnK {
    namespace SDL2Hooks {

        SDL_PollEvent_t TrueSDL_PollEvent = nullptr;
        SDL_GameController* SnatchedGameController = nullptr;

        SDL_GameControllerGetButton_t TrueSDL_GameControllerGetButton = nullptr;
        SDL_GameControllerGetAxis_t TrueSDL_GameControllerGetAxis = nullptr;
        SDL_GameControllerGetNumTouchpads_t TrueSDL_GameControllerGetNumTouchpads = nullptr;
        SDL_GameControllerGetTouchpadFinger_t TrueSDL_GameControllerGetTouchpadFinger = nullptr;

        bool IsLivePointer(SDL_GameController* gc) {
            typedef SDL_bool(CDECL* SDL_GameControllerGetAttached_t)(SDL_GameController*);
            static SDL_GameControllerGetAttached_t TrueSDL_GetAttached = nullptr;
            static bool attempted = false;

            if (!attempted) {
                HMODULE hSDL2 = GetModuleHandleA("SDL2.dll");
                if (hSDL2) TrueSDL_GetAttached = (SDL_GameControllerGetAttached_t)GetProcAddress(hSDL2, "SDL_GameControllerGetAttached");
                attempted = true;
            }

            if (TrueSDL_GetAttached) {
                return TrueSDL_GetAttached(gc) == SDL_TRUE;
            }
            return true;
        }

        UINT SDLButtonToGAMEPAD_ID(Uint8 button) {
            switch (button) {
            case SDL_CONTROLLER_BUTTON_A: return GAMEPAD_ID_A;
            case SDL_CONTROLLER_BUTTON_B: return GAMEPAD_ID_B;
            case SDL_CONTROLLER_BUTTON_X: return GAMEPAD_ID_X;
            case SDL_CONTROLLER_BUTTON_Y: return GAMEPAD_ID_Y;
            case SDL_CONTROLLER_BUTTON_BACK: return GAMEPAD_ID_BACK;
            case SDL_CONTROLLER_BUTTON_GUIDE: return GAMEPAD_ID_GUIDE;
            case SDL_CONTROLLER_BUTTON_START: return GAMEPAD_ID_START;
            case SDL_CONTROLLER_BUTTON_LEFTSTICK: return GAMEPAD_ID_LSB;
            case SDL_CONTROLLER_BUTTON_RIGHTSTICK: return GAMEPAD_ID_RSB;
            case SDL_CONTROLLER_BUTTON_LEFTSHOULDER: return GAMEPAD_ID_LB;
            case SDL_CONTROLLER_BUTTON_RIGHTSHOULDER: return GAMEPAD_ID_RB;
            case SDL_CONTROLLER_BUTTON_DPAD_UP: return GAMEPAD_ID_DPAD_UP;
            case SDL_CONTROLLER_BUTTON_DPAD_DOWN: return GAMEPAD_ID_DPAD_DOWN;
            case SDL_CONTROLLER_BUTTON_DPAD_LEFT: return GAMEPAD_ID_DPAD_LEFT;
            case SDL_CONTROLLER_BUTTON_DPAD_RIGHT: return GAMEPAD_ID_DPAD_RIGHT;
            case SDL_CONTROLLER_BUTTON_TOUCHPAD: return GAMEPAD_ID_TOUCHPAD_BUTTON;
            default: return (UINT)-1;
            }
        }

        bool IsSDLEventMasked(const SDL_Event* event) {
            if (SnatchedGameController == nullptr) return false;

            // Suppress the real input when overlay is open
            bool isControllerEvent = (event->type >= SDL_CONTROLLERAXISMOTION && event->type <= SDL_CONTROLLERTOUCHPADMOTION);
            if (GtoMnK::OverlayMenu::state.isMenuOpen && isControllerEvent) {
                return true;
            }

            if (event->type == SDL_CONTROLLERBUTTONDOWN || event->type == SDL_CONTROLLERBUTTONUP) {
                UINT cid = SDLButtonToGAMEPAD_ID(event->cbutton.button);
                if (cid != (UINT)-1 && IsButtonMapped(cid)) return true;
            }
            else if (event->type == SDL_CONTROLLERAXISMOTION) {
                auto axis = event->caxis.axis;
                if ((axis == SDL_CONTROLLER_AXIS_LEFTX || axis == SDL_CONTROLLER_AXIS_LEFTY) && IsLeftStickMapped()) return true;
                if ((axis == SDL_CONTROLLER_AXIS_RIGHTX || axis == SDL_CONTROLLER_AXIS_RIGHTY) && IsRightStickMapped()) return true;
                if (axis == SDL_CONTROLLER_AXIS_TRIGGERLEFT && IsTriggerMapped(GAMEPAD_ID_LT)) return true;
                if (axis == SDL_CONTROLLER_AXIS_TRIGGERRIGHT && IsTriggerMapped(GAMEPAD_ID_RT)) return true;
            }
            return false;
        }

        int CDECL Hook_SDL_PollEvent(SDL_Event* event) {
            int result;
            do {
                result = TrueSDL_PollEvent(event);
                if (result && event) {

                    // Parasitic Disconnect
                    if (event->type == SDL_CONTROLLERDEVICEREMOVED) {
                        SnatchedGameController = nullptr;
                    }

                    // Filter strictly for physical input events, rejecting DEVICEADDED/REMOVED
                    bool isInputEvent = (
                        event->type == SDL_CONTROLLERAXISMOTION ||
                        event->type == SDL_CONTROLLERBUTTONDOWN ||
                        event->type == SDL_CONTROLLERBUTTONUP ||
                        event->type == SDL_CONTROLLERTOUCHPADDOWN ||
                        event->type == SDL_CONTROLLERTOUCHPADMOTION ||
                        event->type == SDL_CONTROLLERTOUCHPADUP
                        );

                    if (SnatchedGameController == nullptr && isInputEvent) {

                        static SDL_GameControllerFromInstanceID_t TrueSDL_FromInstanceID = nullptr;
                        static bool attemptedResolve = false;

                        if (!attemptedResolve) {
                            HMODULE hSDL2 = GetModuleHandleA("SDL2.dll");
                            if (hSDL2) {
                                TrueSDL_FromInstanceID = (SDL_GameControllerFromInstanceID_t)GetProcAddress(hSDL2, "SDL_GameControllerFromInstanceID");
                            }
                            attemptedResolve = true;
                        }

                        if (TrueSDL_FromInstanceID) {
                            SDL_GameController* newGc = TrueSDL_FromInstanceID(event->cdevice.which);
                            if (newGc) {
                                SnatchedGameController = newGc;
                            }
                        }
                    }
                    if (IsSDLEventMasked(event)) continue;
                }
                break;
            } while (result);
            return result;
        }

        Uint8 CDECL Hook_SDL_GameControllerGetButton(SDL_GameController* gamecontroller, SDL_GameControllerButton button) {
            if (!IsLivePointer(gamecontroller)) {
                if (SnatchedGameController == gamecontroller) SnatchedGameController = nullptr;
                return TrueSDL_GameControllerGetButton(gamecontroller, button);
            }

            SnatchedGameController = gamecontroller;

            // Suppress the real input when overlay is open
            if (GtoMnK::OverlayMenu::state.isMenuOpen) return 0;
            UINT cid = SDLButtonToGAMEPAD_ID(button);
            if (cid != (UINT)-1 && IsButtonMapped(cid)) return 0;

            return TrueSDL_GameControllerGetButton(gamecontroller, button);
        }

        Sint16 CDECL Hook_SDL_GameControllerGetAxis(SDL_GameController* gamecontroller, SDL_GameControllerAxis axis) {
            if (!IsLivePointer(gamecontroller)) {
                if (SnatchedGameController == gamecontroller) SnatchedGameController = nullptr;
                return TrueSDL_GameControllerGetAxis(gamecontroller, axis);
            }

            SnatchedGameController = gamecontroller;

            // Suppress the real input when overlay is open
            if (GtoMnK::OverlayMenu::state.isMenuOpen) return 0;

            if ((axis == SDL_CONTROLLER_AXIS_LEFTX || axis == SDL_CONTROLLER_AXIS_LEFTY) && IsLeftStickMapped()) return 0;
            if ((axis == SDL_CONTROLLER_AXIS_RIGHTX || axis == SDL_CONTROLLER_AXIS_RIGHTY) && IsRightStickMapped()) return 0;
            if (axis == SDL_CONTROLLER_AXIS_TRIGGERLEFT && IsTriggerMapped(GAMEPAD_ID_LT)) return 0;
            if (axis == SDL_CONTROLLER_AXIS_TRIGGERRIGHT && IsTriggerMapped(GAMEPAD_ID_RT)) return 0;

            return TrueSDL_GameControllerGetAxis(gamecontroller, axis);
        }

        int CDECL Hook_SDL_GameControllerGetNumTouchpads(SDL_GameController* gamecontroller) {
            if (!IsLivePointer(gamecontroller)) {
                if (SnatchedGameController == gamecontroller) SnatchedGameController = nullptr;
                return TrueSDL_GameControllerGetNumTouchpads(gamecontroller);
            }

            SnatchedGameController = gamecontroller;
            return TrueSDL_GameControllerGetNumTouchpads(gamecontroller);
        }

        int CDECL Hook_SDL_GameControllerGetTouchpadFinger(SDL_GameController* gamecontroller, int touchpad, int finger, Uint8* state, float* x, float* y, float* pressure) {
            if (!IsLivePointer(gamecontroller)) {
                if (SnatchedGameController == gamecontroller) SnatchedGameController = nullptr;
                return TrueSDL_GameControllerGetTouchpadFinger(gamecontroller, touchpad, finger, state, x, y, pressure);
            }

            SnatchedGameController = gamecontroller;

            // Suppress the real input when overlay is open
            if (GtoMnK::OverlayMenu::state.isMenuOpen || IsTouchpadMapped()) {
                if (state) *state = 0;
                if (x) *x = 0.0f;
                if (y) *y = 0.0f;
                if (pressure) *pressure = 0.0f;
                return 0;
            }

            return TrueSDL_GameControllerGetTouchpadFinger(gamecontroller, touchpad, finger, state, x, y, pressure);
        }
    }
}

#endif // USE_SDL2