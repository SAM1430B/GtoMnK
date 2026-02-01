#pragma once
#include "pch.h"
#include "GamepadState.h"

void SDL2_Initialize();
bool SDL2_GetState(CustomControllerState& outState);
void SDL2_Cleanup();