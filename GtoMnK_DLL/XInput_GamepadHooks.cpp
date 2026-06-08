#include "pch.h"
#include "XInput_GamepadHooks.h"
#include "GamepadMasking.h"
#include "GamepadInputIDs.h"
#include "EnableOpenXinput.h"
#include "OverlayMenu.h"

namespace GtoMnK {
    namespace XInputHooks {

        XInputGetState_t TrueXInputGetState = nullptr;
        XInputGetState_t TrueXInputGetStateEx = nullptr;

        void XInputMaskState(XINPUT_STATE* pState) {
            if (!pState) return;

            // Suppress the real input when overlay is open
            if (GtoMnK::OverlayMenu::state.isMenuOpen) {
                pState->Gamepad.wButtons = 0;
                pState->Gamepad.bLeftTrigger = 0;
                pState->Gamepad.bRightTrigger = 0;
                pState->Gamepad.sThumbLX = 0;
                pState->Gamepad.sThumbLY = 0;
                pState->Gamepad.sThumbRX = 0;
                pState->Gamepad.sThumbRY = 0;
                return;
            }

            if (IsButtonMapped(GAMEPAD_ID_A)) pState->Gamepad.wButtons &= ~XINPUT_GAMEPAD_A;
            if (IsButtonMapped(GAMEPAD_ID_B)) pState->Gamepad.wButtons &= ~XINPUT_GAMEPAD_B;
            if (IsButtonMapped(GAMEPAD_ID_X)) pState->Gamepad.wButtons &= ~XINPUT_GAMEPAD_X;
            if (IsButtonMapped(GAMEPAD_ID_Y)) pState->Gamepad.wButtons &= ~XINPUT_GAMEPAD_Y;
            if (IsButtonMapped(GAMEPAD_ID_DPAD_UP)) pState->Gamepad.wButtons &= ~XINPUT_GAMEPAD_DPAD_UP;
            if (IsButtonMapped(GAMEPAD_ID_DPAD_DOWN)) pState->Gamepad.wButtons &= ~XINPUT_GAMEPAD_DPAD_DOWN;
            if (IsButtonMapped(GAMEPAD_ID_DPAD_LEFT)) pState->Gamepad.wButtons &= ~XINPUT_GAMEPAD_DPAD_LEFT;
            if (IsButtonMapped(GAMEPAD_ID_DPAD_RIGHT)) pState->Gamepad.wButtons &= ~XINPUT_GAMEPAD_DPAD_RIGHT;
            if (IsButtonMapped(GAMEPAD_ID_START)) pState->Gamepad.wButtons &= ~XINPUT_GAMEPAD_START;
            if (IsButtonMapped(GAMEPAD_ID_BACK)) pState->Gamepad.wButtons &= ~XINPUT_GAMEPAD_BACK;
            if (IsButtonMapped(GAMEPAD_ID_LSB)) pState->Gamepad.wButtons &= ~XINPUT_GAMEPAD_LEFT_THUMB;
            if (IsButtonMapped(GAMEPAD_ID_RSB)) pState->Gamepad.wButtons &= ~XINPUT_GAMEPAD_RIGHT_THUMB;
            if (IsButtonMapped(GAMEPAD_ID_LB)) pState->Gamepad.wButtons &= ~XINPUT_GAMEPAD_LEFT_SHOULDER;
            if (IsButtonMapped(GAMEPAD_ID_RB)) pState->Gamepad.wButtons &= ~XINPUT_GAMEPAD_RIGHT_SHOULDER;
            if (IsButtonMapped(GAMEPAD_ID_GUIDE)) pState->Gamepad.wButtons &= ~0x0400;

            if (IsTriggerMapped(GAMEPAD_ID_LT)) pState->Gamepad.bLeftTrigger = 0;
            if (IsTriggerMapped(GAMEPAD_ID_RT)) pState->Gamepad.bRightTrigger = 0;

            if (IsLeftStickMapped()) { pState->Gamepad.sThumbLX = 0; pState->Gamepad.sThumbLY = 0; }
            if (IsRightStickMapped()) { pState->Gamepad.sThumbRX = 0; pState->Gamepad.sThumbRY = 0; }
        }

        DWORD WINAPI MaskHook_XInputGetState(DWORD dwUserIndex, XINPUT_STATE* pState) {
            DWORD res;

            if (g_EnableOpenXinput && pXInputGetState != nullptr) {
                res = pXInputGetState(dwUserIndex, pState);
            }
            else {
                res = TrueXInputGetState(dwUserIndex, pState);
            }

            if (res == ERROR_SUCCESS) XInputMaskState(pState);
            return res;
        }

        DWORD WINAPI MaskHook_XInputGetStateEx(DWORD dwUserIndex, XINPUT_STATE* pState) {
            DWORD res;

            if (g_EnableOpenXinput && pXInputGetState != nullptr) {
                res = pXInputGetState(dwUserIndex, pState);
            }
            else {
                res = TrueXInputGetStateEx(dwUserIndex, pState);
            }

            if (res == ERROR_SUCCESS) XInputMaskState(pState);
            return res;
        }
    }
}