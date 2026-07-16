#pragma once
#include "pch.h"
#include "GamepadInputIDs.h"
#include <map>

struct CustomControllerState {
    // Use a map or fixed array to store the state of every GAMEPAD_ID
    // Simple boolean state: Is GAMEPAD_ID_X pressed?
    bool buttons[GAMEPAD_ID_COUNT] = { false };

    // Analog values for sticks/triggers
    SHORT ThumbLX = 0, ThumbLY = 0, ThumbRX = 0, ThumbRY = 0;
    BYTE LeftTrigger = 0, RightTrigger = 0;

    // Touchpad state
    bool TouchpadActive = false;
    float TouchpadX = 0.0f, TouchpadY = 0.0f;
    float TouchpadPressure = 0.0f;
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