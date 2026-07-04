#include "pch.h"

#if defined(USE_SDL3)

#include "SDL3_GamepadHooks.h"
#include "GamepadMasking.h"
#include "GamepadInputIDs.h"
#include "OverlayMenu.h"
#include "INISettings.h"

namespace GtoMnK {
    namespace SDL3Hooks {

        SDL_PollEvent_t TrueSDL_PollEvent = nullptr;
        SDL_Gamepad* SnatchedGamepad = nullptr;

        SDL_GetGamepadButton_t TrueSDL_GetGamepadButton = nullptr;
        SDL_GetGamepadAxis_t TrueSDL_GetGamepadAxis = nullptr;
        SDL_GetNumGamepadTouchpads_t TrueSDL_GetNumGamepadTouchpads = nullptr;
        SDL_GetGamepadTouchpadFinger_t TrueSDL_GetGamepadTouchpadFinger = nullptr;

        bool IsLivePointer(SDL_Gamepad* gc) {
            typedef bool(CDECL* SDL_GamepadConnected_t)(SDL_Gamepad*);
            static SDL_GamepadConnected_t TrueSDL_Connected = nullptr;
            static bool attempted = false;

            if (!attempted) {
                HMODULE hSDL3 = GetModuleHandleA("SDL3.dll");
                if (hSDL3) TrueSDL_Connected = (SDL_GamepadConnected_t)GetProcAddress(hSDL3, "SDL_GamepadConnected");
                attempted = true;
            }

            if (TrueSDL_Connected) {
                return TrueSDL_Connected(gc);
            }
            return true;
        }

        UINT SDLButtonToGAMEPAD_ID(SDL_GamepadButton button) {
            switch (button) {
            case SDL_GAMEPAD_BUTTON_SOUTH: return GAMEPAD_ID_A;
            case SDL_GAMEPAD_BUTTON_EAST: return GAMEPAD_ID_B;
            case SDL_GAMEPAD_BUTTON_WEST: return GAMEPAD_ID_X;
            case SDL_GAMEPAD_BUTTON_NORTH: return GAMEPAD_ID_Y;
            case SDL_GAMEPAD_BUTTON_BACK: return GAMEPAD_ID_BACK;
            case SDL_GAMEPAD_BUTTON_GUIDE: return GAMEPAD_ID_GUIDE;
            case SDL_GAMEPAD_BUTTON_START: return GAMEPAD_ID_START;
            case SDL_GAMEPAD_BUTTON_LEFT_STICK: return GAMEPAD_ID_LSB;
            case SDL_GAMEPAD_BUTTON_RIGHT_STICK: return GAMEPAD_ID_RSB;
            case SDL_GAMEPAD_BUTTON_LEFT_SHOULDER: return GAMEPAD_ID_LB;
            case SDL_GAMEPAD_BUTTON_RIGHT_SHOULDER: return GAMEPAD_ID_RB;
            case SDL_GAMEPAD_BUTTON_DPAD_UP: return GAMEPAD_ID_DPAD_UP;
            case SDL_GAMEPAD_BUTTON_DPAD_DOWN: return GAMEPAD_ID_DPAD_DOWN;
            case SDL_GAMEPAD_BUTTON_DPAD_LEFT: return GAMEPAD_ID_DPAD_LEFT;
            case SDL_GAMEPAD_BUTTON_DPAD_RIGHT: return GAMEPAD_ID_DPAD_RIGHT;
            case SDL_GAMEPAD_BUTTON_TOUCHPAD: return GAMEPAD_ID_TOUCHPAD_BUTTON;
            default: return (UINT)-1;
            }
        }

        bool IsSDLEventMasked(const SDL_Event* event) {
            if (SnatchedGamepad == nullptr) return false;

            // Suppress the real input when overlay is open
            bool isInputEvent = (
                event->type == SDL_EVENT_GAMEPAD_AXIS_MOTION ||
                event->type == SDL_EVENT_GAMEPAD_BUTTON_DOWN ||
                event->type == SDL_EVENT_GAMEPAD_BUTTON_UP ||
                event->type == SDL_EVENT_GAMEPAD_TOUCHPAD_DOWN ||
                event->type == SDL_EVENT_GAMEPAD_TOUCHPAD_MOTION ||
                event->type == SDL_EVENT_GAMEPAD_TOUCHPAD_UP
                );

            if (GtoMnK::OverlayMenu::state.isMenuOpen && isInputEvent) {
                return true;
            }

            if (event->type == SDL_EVENT_GAMEPAD_BUTTON_DOWN || event->type == SDL_EVENT_GAMEPAD_BUTTON_UP) {
                UINT cid = SDLButtonToGAMEPAD_ID((SDL_GamepadButton)event->gbutton.button);
                if (cid != (UINT)-1 && IsButtonMapped(cid)) return true;
            }
            else if (event->type == SDL_EVENT_GAMEPAD_AXIS_MOTION) {
                auto axis = event->gaxis.axis;
                if ((axis == SDL_GAMEPAD_AXIS_LEFTX || axis == SDL_GAMEPAD_AXIS_LEFTY) && IsLeftStickMapped()) return true;
                if ((axis == SDL_GAMEPAD_AXIS_RIGHTX || axis == SDL_GAMEPAD_AXIS_RIGHTY) && IsRightStickMapped()) return true;
                if (axis == SDL_GAMEPAD_AXIS_LEFT_TRIGGER && IsTriggerMapped(GAMEPAD_ID_LT)) return true;
                if (axis == SDL_GAMEPAD_AXIS_RIGHT_TRIGGER && IsTriggerMapped(GAMEPAD_ID_RT)) return true;
            }
            return false;
        }

        bool CDECL Hook_SDL_PollEvent(SDL_Event* event) {
            bool result;
            do {
                result = TrueSDL_PollEvent(event);
                if (result && event) {

                    // Parasitic Disconnect
                    if (event->type == SDL_EVENT_GAMEPAD_REMOVED) {
                        SnatchedGamepad = nullptr;
                    }

                    // Filter strictly for physical input events, rejecting DEVICEADDED/REMOVED
                    bool isInputEvent = (
                        event->type == SDL_EVENT_GAMEPAD_AXIS_MOTION ||
                        event->type == SDL_EVENT_GAMEPAD_BUTTON_DOWN ||
                        event->type == SDL_EVENT_GAMEPAD_BUTTON_UP ||
                        event->type == SDL_EVENT_GAMEPAD_TOUCHPAD_DOWN ||
                        event->type == SDL_EVENT_GAMEPAD_TOUCHPAD_MOTION ||
                        event->type == SDL_EVENT_GAMEPAD_TOUCHPAD_UP
                        );

                    if (SnatchedGamepad == nullptr && isInputEvent) {

                        static SDL_GetGamepadFromID_t TrueSDL_FromID = nullptr;
                        static bool attemptedResolve = false;

                        if (!attemptedResolve) {
                            HMODULE hSDL3 = GetModuleHandleA("SDL3.dll");
                            if (hSDL3) {
                                TrueSDL_FromID = (SDL_GetGamepadFromID_t)GetProcAddress(hSDL3, "SDL_GetGamepadFromID");
                            }
                            attemptedResolve = true;
                        }

                        if (TrueSDL_FromID) {
                            SDL_JoystickID joyID = 0;

                            switch (event->type) {
                            case SDL_EVENT_GAMEPAD_AXIS_MOTION: joyID = event->gaxis.which; break;
                            case SDL_EVENT_GAMEPAD_BUTTON_DOWN:
                            case SDL_EVENT_GAMEPAD_BUTTON_UP: joyID = event->gbutton.which; break;
                            case SDL_EVENT_GAMEPAD_TOUCHPAD_DOWN:
                            case SDL_EVENT_GAMEPAD_TOUCHPAD_MOTION:
                            case SDL_EVENT_GAMEPAD_TOUCHPAD_UP: joyID = event->gtouchpad.which; break;
                            }

                            if (joyID != 0) {
                                SDL_Gamepad* newGc = TrueSDL_FromID(joyID);
                                if (newGc) {
                                    SnatchedGamepad = newGc;
                                }
                            }
                        }
                    }
                    if (IsSDLEventMasked(event)) continue;
                }
                break;
            } while (result);
            return result;
        }

        bool CDECL Hook_SDL_GetGamepadButton(SDL_Gamepad* gamepad, SDL_GamepadButton button) {
            if (!IsLivePointer(gamepad)) {
                if (SnatchedGamepad == gamepad) SnatchedGamepad = nullptr;
                return TrueSDL_GetGamepadButton(gamepad, button);
            }

            SnatchedGamepad = gamepad;

            // Suppress the real input when overlay is open
            if (GtoMnK::OverlayMenu::state.isMenuOpen) return false;
            UINT cid = SDLButtonToGAMEPAD_ID(button);
            if (cid != (UINT)-1 && IsButtonMapped(cid)) return false;

            return TrueSDL_GetGamepadButton(gamepad, button);
        }

        Sint16 CDECL Hook_SDL_GetGamepadAxis(SDL_Gamepad* gamepad, SDL_GamepadAxis axis) {
            if (!IsLivePointer(gamepad)) {
                if (SnatchedGamepad == gamepad) SnatchedGamepad = nullptr;
                return TrueSDL_GetGamepadAxis(gamepad, axis);
            }

            SnatchedGamepad = gamepad;

            // Suppress the real input when overlay is open
            if (GtoMnK::OverlayMenu::state.isMenuOpen) return 0;

            if ((axis == SDL_GAMEPAD_AXIS_LEFTX || axis == SDL_GAMEPAD_AXIS_LEFTY) && IsLeftStickMapped()) return 0;
            if ((axis == SDL_GAMEPAD_AXIS_RIGHTX || axis == SDL_GAMEPAD_AXIS_RIGHTY) && IsRightStickMapped()) return 0;
            if (axis == SDL_GAMEPAD_AXIS_LEFT_TRIGGER && IsTriggerMapped(GAMEPAD_ID_LT)) return 0;
            if (axis == SDL_GAMEPAD_AXIS_RIGHT_TRIGGER && IsTriggerMapped(GAMEPAD_ID_RT)) return 0;

            return TrueSDL_GetGamepadAxis(gamepad, axis);
        }

        int CDECL Hook_SDL_GetNumGamepadTouchpads(SDL_Gamepad* gamepad) {
            if (!IsLivePointer(gamepad)) {
                if (SnatchedGamepad == gamepad) SnatchedGamepad = nullptr;
                return TrueSDL_GetNumGamepadTouchpads(gamepad);
            }

            SnatchedGamepad = gamepad;
            return TrueSDL_GetNumGamepadTouchpads(gamepad);
        }

        bool CDECL Hook_SDL_GetGamepadTouchpadFinger(SDL_Gamepad* gamepad, int touchpad, int finger, bool* down, float* x, float* y, float* pressure) {
            if (!IsLivePointer(gamepad)) {
                if (SnatchedGamepad == gamepad) SnatchedGamepad = nullptr;
                return TrueSDL_GetGamepadTouchpadFinger(gamepad, touchpad, finger, down, x, y, pressure);
            }

            SnatchedGamepad = gamepad;

            // Suppress the real input when overlay is open
            if (GtoMnK::OverlayMenu::state.isMenuOpen || IsTouchpadMapped()) {
                if (down) *down = false;
                if (x) *x = 0.0f;
                if (y) *y = 0.0f;
                if (pressure) *pressure = 0.0f;
                return true;
            }

            return TrueSDL_GetGamepadTouchpadFinger(gamepad, touchpad, finger, down, x, y, pressure);
        }
    }
}

#endif // USE_SDL3