#pragma once
#include "pch.h"

POINT ThumbstickMouseMove(SHORT stickX, SHORT stickY);

bool IsTriggerPressed(BYTE triggerValue);

void ProcessButton(UINT buttonFlag, bool isCurrentlyPressed);

void ProcessTrigger(UINT triggerID, BYTE triggerValue);