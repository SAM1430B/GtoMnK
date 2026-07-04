#pragma once
#include "pch.h"
#include "GamepadState.h"

void SDL3_Initialize();
bool SDL3_GetState(CustomControllerState& outState);
void SDL3_Cleanup();