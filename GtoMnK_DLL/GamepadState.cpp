#include "pch.h"
#include "GamepadState.h"
#include "INISettings.h"
#include "InputState.h"
#include "Input.h"
#include <unordered_map>

// For Controller Button States
const size_t NO_ACTION = static_cast<size_t>(-1);

int g_Fn1_ButtonID = -1;
int g_Fn2_ButtonID = -1;
bool g_Fn1_State = false;
bool g_Fn2_State = false;

static std::unordered_map<UINT, int> g_ButtonLayer;
static std::unordered_map<UINT, bool> g_ButtonLastState;

POINT ThumbstickMouseMove(SHORT stickX, SHORT stickY) {
    const double smoothing_factor = 0.35;

    static double smoothX = 0.0;
    static double smoothY = 0.0;
    static double mouseDeltaAccumulatorX = 0.0;
    static double mouseDeltaAccumulatorY = 0.0;
    static bool firstRun = true;

    double safe_multiplier = (sensitivity_multiplier <= 0.001) ? 1.0 : sensitivity_multiplier;

    double rawNormX = static_cast<double>(stickX) / 32767.0;
    double rawNormY = static_cast<double>(stickY) / 32767.0;

    if (firstRun) {
        smoothX = rawNormX;
        smoothY = rawNormY;
        firstRun = false;
    }
    else {
        smoothX = smoothX + smoothing_factor * (rawNormX - smoothX);
        smoothY = smoothY + smoothing_factor * (rawNormY - smoothY);
    }

    double normX = smoothX;
    double normY = smoothY;

    double magnitude = std::sqrt(normX * normX + normY * normY);

    if (magnitude < 1e-6 || magnitude < radial_deadzone) {
        if (magnitude < 1e-6) {
            smoothX = 0.0;
            smoothY = 0.0;
        }
        return { 0, 0 };
    }

    if (magnitude > 1.0) magnitude = 1.0;

    double dirX = normX / magnitude;
    double dirY = normY / magnitude;

    if (std::abs(dirX) < axial_deadzone) {
        dirX = 0.0;
        dirY = (dirY > 0) ? 1.0 : -1.0;
    }
    else if (std::abs(dirY) < axial_deadzone) {
        dirY = 0.0;
        dirX = (dirX > 0) ? 1.0 : -1.0;
    }

    double effectiveRange = 1.0 - radial_deadzone;
    if (effectiveRange < 1e-6) effectiveRange = 1.0;

    double x = (magnitude - radial_deadzone) / effectiveRange;
    x = (std::max)(0.0, (std::min)(1.0, x));

    double finalCurveValue = 0.0;
    double accelBoundary = 1.0 - max_threshold;
    accelBoundary = (std::max)(0.0, (std::min)(1.0, accelBoundary));

    if (x <= accelBoundary) {
        finalCurveValue = curve_slope * x + (1.0 - curve_slope) * std::pow(x, curve_exponent);
    }
    else {
        double kneeValue = curve_slope * accelBoundary + (1.0 - curve_slope) * std::pow(accelBoundary, curve_exponent);
        double rampWidth = 1.0 - accelBoundary;

        if (rampWidth <= 1e-6) {
            finalCurveValue = look_accel_multiplier;
        }
        else {
            double rampProgress = (x - accelBoundary) / rampWidth;
            finalCurveValue = kneeValue + (look_accel_multiplier - kneeValue) * rampProgress;
        }
    }

    double finalSpeedX = sensitivity * (1.0 + horizontal_sensitivity) * safe_multiplier;
    double finalSpeedY = sensitivity * (1.0 + vertical_sensitivity) * safe_multiplier;

    double targetDeltaX = dirX * finalCurveValue * finalSpeedX;
    double targetDeltaY = dirY * finalCurveValue * finalSpeedY;

    if (std::isnan(targetDeltaX) || std::isnan(targetDeltaY)) {
        mouseDeltaAccumulatorX = 0;
        mouseDeltaAccumulatorY = 0;
        return { 0, 0 };
    }

    mouseDeltaAccumulatorX += targetDeltaX;
    mouseDeltaAccumulatorY += targetDeltaY;

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