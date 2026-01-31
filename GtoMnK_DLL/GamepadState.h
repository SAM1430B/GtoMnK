#pragma once
#include "pch.h"
#include <map>

struct CustomControllerState {
    // Use a map or fixed array to store the state of every CUSTOM_ID
    // Simple boolean state: Is CUSTOM_ID_X pressed?
    bool buttons[30];

    // Analog values for sticks/triggers
    SHORT ThumbLX, ThumbLY, ThumbRX, ThumbRY;
    BYTE LeftTrigger, RightTrigger;
};

POINT ThumbstickMouseMove(SHORT stickX, SHORT stickY);
bool IsTriggerPressed(BYTE triggerValue);
void ProcessButton(UINT buttonFlag, bool isCurrentlyPressed);
void ProcessTrigger(UINT triggerID, BYTE triggerValue);