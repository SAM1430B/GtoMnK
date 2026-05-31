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

int GetActiveThumbStickToMouse() {
    if (g_Fn2_State) return g_thumbStickToMouse[2];
    if (g_Fn1_State) return g_thumbStickToMouse[1];
    return g_thumbStickToMouse[0];
}

int GetActiveTouchPadToMouse() {
    if (g_Fn2_State) return g_touchPadToMouse[2];
    if (g_Fn1_State) return g_touchPadToMouse[1];
    return g_touchPadToMouse[0];
}

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

POINT TouchpadMouseMove(float touchX, float touchY, bool isActive) {
    static float lastTouchX = 0.0f;
    static float lastTouchY = 0.0f;
    static bool wasActive = false;

    static double smoothedDeltaX = 0.0;
    static double smoothedDeltaY = 0.0;

    static double mouseDeltaAccumulatorX = 0.0;
    static double mouseDeltaAccumulatorY = 0.0;

    // Smoothing Factor
    // 0.0 to 1.0. Lower = smoother but more delayed. Higher = more responsive but jittery.
    // 0.35 to 0.5 is usually the "sweet spot" for touchpads.
    const double smoothing_factor = 0.4;

    // Reset smoothing when finger lifts
    if (!isActive) {
        wasActive = false;
        smoothedDeltaX = 0.0;
        smoothedDeltaY = 0.0;
        return { 0, 0 };
    }

    if (!wasActive) {
        lastTouchX = touchX;
        lastTouchY = touchY;
        smoothedDeltaX = 0.0;
        smoothedDeltaY = 0.0;
        wasActive = true;
        return { 0, 0 };
    }

    float rawDeltaX = touchX - lastTouchX;
    float rawDeltaY = touchY - lastTouchY;

    lastTouchX = touchX;
    lastTouchY = touchY;

    // Micro-Deadzone
    // Ignore hardware capacitive noise (usually < 0.0001) when resting your finger.
    if (std::abs(rawDeltaX) < touchpad_deadzone) rawDeltaX = 0.0f;
    if (std::abs(rawDeltaY) < touchpad_deadzone) rawDeltaY = 0.0f;

    // EMA Smoothing
    // Blend the new raw delta into the existing smoothed delta
    double safe_smoothing = std::max(0.0f, std::min(0.99f, touchpad_smoothing));
    smoothedDeltaX = smoothedDeltaX + safe_smoothing * (rawDeltaX - smoothedDeltaX);
    smoothedDeltaY = smoothedDeltaY + safe_smoothing * (rawDeltaY - smoothedDeltaY);

    double finalSpeedX = touchpad_sensitivity * (1.0 + touchpad_horizontal_sensitivity);
    double finalSpeedY = touchpad_sensitivity * (1.0 + touchpad_vertical_sensitivity);

    double targetDeltaX = smoothedDeltaX * finalSpeedX;
    double targetDeltaY = smoothedDeltaY * finalSpeedY;

    mouseDeltaAccumulatorX += targetDeltaX;
    mouseDeltaAccumulatorY += targetDeltaY;

    LONG integerDeltaX = static_cast<LONG>(mouseDeltaAccumulatorX);
    LONG integerDeltaY = static_cast<LONG>(mouseDeltaAccumulatorY);

    mouseDeltaAccumulatorX -= integerDeltaX;
    mouseDeltaAccumulatorY -= integerDeltaY;

    return { integerDeltaX, integerDeltaY };
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

    if (!bs.pendingReleaseAction.empty()) {
        if (GetTickCount64() >= bs.pendingReleaseTime) {
            Input::SendAction(bs.pendingReleaseAction, false);
            bs.pendingReleaseAction.clear();
        }
    }

    // On Press
    if (!bs.isPhysicallyPressed && isCurrentlyPressed) {
        bs.isPhysicallyPressed = true;
        bs.pressTime = GetTickCount64();
        bs.activeActionIndex = static_cast<size_t>(-1);
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
        size_t newActionIndex = static_cast<size_t>(-1);
        for (size_t i = 0; i < bs.actions.size(); ++i) {
            if (holdTime >= bs.actions[i].holdDurationMs) {
                newActionIndex = i;
            }
        }
        if (newActionIndex != static_cast<size_t>(-1) && newActionIndex != bs.activeActionIndex) {
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

        size_t finalActionIndex = static_cast<size_t>(-1);
        ULONGLONG finalHoldTime = GetTickCount64() - bs.pressTime;
        for (size_t i = 0; i < bs.actions.size(); ++i) {
            if (finalHoldTime >= bs.actions[i].holdDurationMs) {
                finalActionIndex = i;
            }
        }

        if (finalActionIndex != static_cast<size_t>(-1) && bs.actions[finalActionIndex].onRelease) {
            const Action& finalAction = bs.actions[finalActionIndex];

            Input::SendAction(finalAction.actionString, true);
            bs.pendingReleaseAction = finalAction.actionString;
            bs.pendingReleaseTime = GetTickCount64() + 20;
        }

        if (!bs.heldActionString.empty() && bs.heldActionString != "0") {
            Input::SendAction(bs.heldActionString, false);
        }

        bs.activeActionIndex = static_cast<size_t>(-1);
        bs.pressActionFired = false;
        bs.heldActionString = "0";
        return;
    }
}

void ProcessTrigger(UINT triggerID, BYTE triggerValue) {
    bool isPressed = IsTriggerPressed(triggerValue);
    ProcessButton(triggerID, isPressed);
}