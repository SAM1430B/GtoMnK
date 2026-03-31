#pragma once
#include "pch.h"
#include "GamepadState.h"

void XInput_Initialize();
bool XInput_GetState(CustomControllerState& outState);

// Used for XInput masking hooks
DWORD WINAPI Hook_XInputGetState(DWORD dwUserIndex, XINPUT_STATE* pState);

typedef DWORD(WINAPI* XInputGetState_t)(DWORD, XINPUT_STATE*);
extern XInputGetState_t TrueXInputGetState;