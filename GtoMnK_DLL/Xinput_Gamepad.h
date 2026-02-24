#pragma once
#include "pch.h"
#include "GamepadState.h"

void XInput_Initialize();
bool XInput_GetState(CustomControllerState& outState);