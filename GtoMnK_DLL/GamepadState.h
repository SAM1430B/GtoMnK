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

    // Touchpad state
    bool TouchpadActive;
    float TouchpadX, TouchpadY;
    float TouchpadPressure;
};

extern int g_Fn1_ButtonID;
extern int g_Fn2_ButtonID;

int GetActiveThumbStickToMouse();
int GetActiveTouchPadToMouse();

POINT ThumbstickMouseMove(SHORT stickX, SHORT stickY);
POINT TouchpadMouseMove(float touchX, float touchY, bool isActive);

bool IsTriggerPressed(BYTE triggerValue);
void ProcessButton(UINT buttonFlag, bool isCurrentlyPressed);
void ProcessTrigger(UINT triggerID, BYTE triggerValue);