#pragma once
#include "pch.h"
#include "GamepadState.h"

void XInput_Initialize();
void XInput_Cleanup();
bool XInput_GetState(CustomControllerState& outState);