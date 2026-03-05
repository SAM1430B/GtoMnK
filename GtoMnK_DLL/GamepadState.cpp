#include "pch.h"
#include "GamepadState.h"
#include "INISettings.h"
#include "InputState.h"
#include "Input.h"

// For Controller Button States
const size_t NO_ACTION = static_cast<size_t>(-1);

int g_Fn1_ButtonID = -1;
int g_Fn2_ButtonID = -1;
bool g_Fn1_State = false;
bool g_Fn2_State = false;

static int g_ButtonLayer[256] = { 0 };
static bool g_ButtonLastState[256] = { false };

    double safe_multiplier = (sensitivity_multiplier <= 0.001) ? 1.0 : sensitivity_multiplier;

    static double mouseDeltaAccumulatorX = 0.0;
    static double mouseDeltaAccumulatorY = 0.0;

    double normX = static_cast<double>(stickX) / 32767.0;
    double normY = static_cast<double>(stickY) / 32767.0;

    if (std::abs(normX) < s) normX = 0.0;
    if (std::abs(normY) < s) normY = 0.0;

    double magnitude = std::sqrt(normX * normX + normY * normY);
    if (magnitude <= c) return { 0, 0 };
    if (magnitude > 1.0) magnitude = 1.0;

    double effectiveRange = 1.0 - c;
    if (effectiveRange < 1e-6) effectiveRange = 1.0;

    double x = (magnitude - c) / effectiveRange;

    double finalCurveValue = 0.0;
    double accelBoundary = 1.0 - m;
    accelBoundary = (std::max)(0.0, (std::min)(1.0, accelBoundary));

    if (x <= accelBoundary) {
        finalCurveValue = b * x + (1.0 - b) * std::pow(x, a);
    }
    else {
        double kneeValue = b * accelBoundary + (1.0 - b) * std::pow(accelBoundary, a);

        double rampWidth = 1.0 - accelBoundary;

        if (rampWidth <= 1e-6) {
            finalCurveValue = t;
        }
        else {
            double rampProgress = (x - accelBoundary) / rampWidth;
            finalCurveValue = kneeValue + (t - kneeValue) * rampProgress;
        }
    }

    double finalSpeedX = base_sensitivity * safe_multiplier * (1.0f + horizontal_sensitivity);
    double finalSpeedY = base_sensitivity * safe_multiplier * (1.0f + vertical_sensitivity);

    double dirX = normX / magnitude;
    double dirY = normY / magnitude;

    double finalMouseDeltaX = dirX * finalCurveValue * finalSpeedX;
    double finalMouseDeltaY = dirY * finalCurveValue * finalSpeedY;

    mouseDeltaAccumulatorX += finalMouseDeltaX;
    mouseDeltaAccumulatorY += finalMouseDeltaY;

    LONG integerDeltaX = static_cast<LONG>(mouseDeltaAccumulatorX);
    LONG integerDeltaY = static_cast<LONG>(mouseDeltaAccumulatorY);

    mouseDeltaAccumulatorX -= integerDeltaX;
    mouseDeltaAccumulatorY -= integerDeltaY;

    return { integerDeltaX, -integerDeltaY };
}

// 0-255 lower value is more sensitive
bool IsTriggerPressed(BYTE triggerValue) {
    return triggerValue > g_TriggerThreshold;
}

void ProcessButton(UINT buttonFlag, bool isCurrentlyPressed) {
    if (buttonFlag == g_Fn1_ButtonID) {
        g_Fn1_State = isCurrentlyPressed;
        return;
    }
    if (buttonFlag == g_Fn2_ButtonID) {
        g_Fn2_State = isCurrentlyPressed;
        return;
    }

    if (isCurrentlyPressed && !g_ButtonLastState[buttonFlag]) {
        if (g_Fn2_State) g_ButtonLayer[buttonFlag] = 200;
        else if (g_Fn1_State) g_ButtonLayer[buttonFlag] = 100;
        else g_ButtonLayer[buttonFlag] = 0;
    }

    g_ButtonLastState[buttonFlag] = isCurrentlyPressed;

    UINT effectiveFlag = buttonFlag + g_ButtonLayer[buttonFlag];

    ButtonState& bs = buttonStates[effectiveFlag];

    // On Press
    if (!bs.isPhysicallyPressed && isCurrentlyPressed) {
        bs.isPhysicallyPressed = true;
        bs.pressTime = GetTickCount64();
        bs.activeActionIndex = NO_ACTION;
        bs.pressActionFired = false;
        bs.heldActionString = "0";

        if (!bs.actions.empty() && bs.actions[0].holdDurationMs == 0) {
            bs.activeActionIndex = 0;
            const Action& tapAction = bs.actions[0];

            if (!tapAction.onRelease) {
                Input::SendAction(tapAction.actionString, true);
                bs.heldActionString = tapAction.actionString;
                bs.pressActionFired = true;
            }
        }
        return;
    }

    // While Held
    if (bs.isPhysicallyPressed && isCurrentlyPressed) {
        ULONGLONG holdTime = GetTickCount64() - bs.pressTime;
        size_t newActionIndex = NO_ACTION;
        for (size_t i = 0; i < bs.actions.size(); ++i) {
            if (holdTime >= bs.actions[i].holdDurationMs) {
                newActionIndex = i;
            }
        }
        if (newActionIndex != NO_ACTION && newActionIndex != bs.activeActionIndex) {
            if (!bs.heldActionString.empty() && bs.heldActionString != "0") {
                Input::SendAction(bs.heldActionString, false);
            }
            bs.activeActionIndex = newActionIndex;
            bs.pressActionFired = false;
            bs.heldActionString = "0";
            const Action& newActiveAction = bs.actions[bs.activeActionIndex];
            if (!newActiveAction.onRelease) {
                Input::SendAction(newActiveAction.actionString, true);
                bs.heldActionString = newActiveAction.actionString;
                bs.pressActionFired = true;
            }
        }
        return;
    }

    // On Release
    if (bs.isPhysicallyPressed && !isCurrentlyPressed) {
        bs.isPhysicallyPressed = false;

        size_t finalActionIndex = NO_ACTION;
        ULONGLONG finalHoldTime = GetTickCount64() - bs.pressTime;
        for (size_t i = 0; i < bs.actions.size(); ++i) {
            if (finalHoldTime >= bs.actions[i].holdDurationMs) {
                finalActionIndex = i;
            }
        }

        if (finalActionIndex != NO_ACTION && bs.actions[finalActionIndex].onRelease) {
            const Action& finalAction = bs.actions[finalActionIndex];

            Input::SendAction(finalAction.actionString, true);
            Sleep(20);
            Input::SendAction(finalAction.actionString, false);
        }

        if (!bs.heldActionString.empty() && bs.heldActionString != "0") {
            Input::SendAction(bs.heldActionString, false);
        }

        bs.activeActionIndex = NO_ACTION;
        bs.pressActionFired = false;
        bs.heldActionString = "0";
        return;
    }
}

void ProcessTrigger(UINT triggerID, BYTE triggerValue) {
    bool isPressed = IsTriggerPressed(triggerValue);
    ProcessButton(triggerID, isPressed);
}