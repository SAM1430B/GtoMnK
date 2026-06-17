#include "pch.h"
#include "GamepadState.h"
#include "INISettings.h"
#include "FakeInput.h"
#include <array>

#include <cmath>
#include <algorithm>

// For Controller Button States
const size_t NO_ACTION = static_cast<size_t>(-1);

int g_Fn1_ButtonID = -1;
int g_Fn2_ButtonID = -1;
bool g_Fn1_State = false;
bool g_Fn2_State = false;

static std::array<int, 256> g_ButtonLayer = { 0 };
static std::array<bool, 256> g_ButtonLastState = { false };

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

    if (g_UseLegacyMouseMovement) {
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
            if (magnitude < 1e-6) {smoothX = 0.0;smoothY = 0.0;}
            return { 0, 0 };
        }

        if (magnitude > 1.0) magnitude = 1.0;

        double dirX = normX / magnitude;
        double dirY = normY / magnitude;

        if (std::abs(dirX) < axial_deadzone) {dirX = 0.0;dirY = (dirY > 0) ? 1.0 : -1.0;}
        else if (std::abs(dirY) < axial_deadzone) {dirY = 0.0;dirX = (dirX > 0) ? 1.0 : -1.0;}

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
            if (rampWidth <= 1e-6) {finalCurveValue = look_accel_multiplier;}
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
            mouseDeltaAccumulatorX = 0; mouseDeltaAccumulatorY = 0; return { 0, 0 };
        }

        mouseDeltaAccumulatorX += targetDeltaX;
        mouseDeltaAccumulatorY += targetDeltaY;
        LONG integerDeltaX = static_cast<LONG>(mouseDeltaAccumulatorX);
        LONG integerDeltaY = static_cast<LONG>(mouseDeltaAccumulatorY);
        mouseDeltaAccumulatorX -= integerDeltaX;
        mouseDeltaAccumulatorY -= integerDeltaY;

        return { integerDeltaX, -integerDeltaY };
    }
    else {
        static LARGE_INTEGER frequency;
        static LARGE_INTEGER lastTime;
        static bool timerInit = false;

        if (!timerInit) {
            QueryPerformanceFrequency(&frequency);
            QueryPerformanceCounter(&lastTime);
            timerInit = true;
        }

        LARGE_INTEGER currentTime;
        QueryPerformanceCounter(&currentTime);

        double deltaTime = static_cast<double>(currentTime.QuadPart - lastTime.QuadPart) / frequency.QuadPart;
        lastTime = currentTime;

        if (deltaTime > 0.05) deltaTime = 0.05;
        double timeScale = deltaTime * 250.0;

        const double base_smoothing_factor = 0.35;
        double frameIndependentSmoothing = 1.0 - std::pow(1.0 - base_smoothing_factor, timeScale);
        frameIndependentSmoothing = (std::max)(0.0, (std::min)(1.0, frameIndependentSmoothing));

        static double smoothX = 0.0;
        static double smoothY = 0.0;
        static double mouseDeltaAccumulatorX = 0.0;
        static double mouseDeltaAccumulatorY = 0.0;
        static bool firstRun = true;

        double safe_multiplier = (sensitivity_multiplier <= 0.001) ? 1.0 : sensitivity_multiplier;

        double rawNormX = static_cast<double>(stickX) / 32768.0;
        double rawNormY = static_cast<double>(stickY) / 32768.0;

        if (firstRun) {
            smoothX = rawNormX;
            smoothY = rawNormY;
            firstRun = false;
        }
        else {
            smoothX = smoothX + frameIndependentSmoothing * (rawNormX - smoothX);
            smoothY = smoothY + frameIndependentSmoothing * (rawNormY - smoothY);
        }

        double normX = smoothX;
        double normY = smoothY;
        double magnitude = std::sqrt(normX * normX + normY * normY);

        if (magnitude < 1e-6 || magnitude < radial_deadzone) {
            if (magnitude < 1e-6) { smoothX = 0.0; smoothY = 0.0; }
            return { 0, 0 };
        }

        if (magnitude > 1.0) magnitude = 1.0;
        double dirX = normX / magnitude;
        double dirY = normY / magnitude;

        if (std::abs(dirX) < axial_deadzone) { dirX = 0.0; dirY = (dirY > 0) ? 1.0 : -1.0; }
        else if (std::abs(dirY) < axial_deadzone) { dirY = 0.0; dirX = (dirX > 0) ? 1.0 : -1.0; }

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
            if (rampWidth <= 1e-6) { finalCurveValue = look_accel_multiplier; }
            else {
                double rampProgress = (x - accelBoundary) / rampWidth;
                finalCurveValue = kneeValue + (look_accel_multiplier - kneeValue) * rampProgress;
            }
        }

        double finalSpeedX = sensitivity * (1.0 + horizontal_sensitivity) * safe_multiplier;
        double finalSpeedY = sensitivity * (1.0 + vertical_sensitivity) * safe_multiplier;

        double targetDeltaX = dirX * finalCurveValue * finalSpeedX * timeScale;
        double targetDeltaY = dirY * finalCurveValue * finalSpeedY * timeScale;

        if (std::isnan(targetDeltaX) || std::isnan(targetDeltaY)) {
            mouseDeltaAccumulatorX = 0; mouseDeltaAccumulatorY = 0; return { 0, 0 };
        }

        mouseDeltaAccumulatorX += targetDeltaX;
        mouseDeltaAccumulatorY += targetDeltaY;

        LONG integerDeltaX = static_cast<LONG>(mouseDeltaAccumulatorX);
        LONG integerDeltaY = static_cast<LONG>(mouseDeltaAccumulatorY);

        mouseDeltaAccumulatorX -= integerDeltaX;
        mouseDeltaAccumulatorY -= integerDeltaY;

        return { integerDeltaX, -integerDeltaY };
    }
}

POINT TouchpadMouseMove(float touchX, float touchY, bool isActive) {
    static float lastTouchX = 0.0f;
    static float lastTouchY = 0.0f;
    static bool wasActive = false;

    static double smoothedDeltaX = 0.0;
    static double smoothedDeltaY = 0.0;

    static double mouseDeltaAccumulatorX = 0.0;
    static double mouseDeltaAccumulatorY = 0.0;

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

    double rawDeltaX = static_cast<double>(touchX - lastTouchX);
    double rawDeltaY = static_cast<double>(touchY - lastTouchY);

    lastTouchX = touchX;
    lastTouchY = touchY;

    double magnitude = std::sqrt((rawDeltaX * rawDeltaX) + (rawDeltaY * rawDeltaY));
    bool inDeadzone = magnitude < touchpad_deadzone;

    if (inDeadzone) {
        rawDeltaX = 0.0;
        rawDeltaY = 0.0;
    }

    double clamped_ini_smoothing = std::max(0.0, std::min(0.99, static_cast<double>(touchpad_smoothing)));
    double alpha = 1.0 - clamped_ini_smoothing;

    smoothedDeltaX = smoothedDeltaX + alpha * (rawDeltaX - smoothedDeltaX);
    smoothedDeltaY = smoothedDeltaY + alpha * (rawDeltaY - smoothedDeltaY);

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
    if (buttonFlag >= 256) return;

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
            FakeInput::SendAction(bs.pendingReleaseAction, false);
            bs.pendingReleaseAction.clear();
        }
    }

    // On Press
    if (!bs.isPhysicallyPressed && isCurrentlyPressed) {
        bs.isPhysicallyPressed = true;
        bs.pressTime = GetTickCount64();
        bs.activeActionIndex = static_cast<size_t>(-1);
        bs.pressActionFired = false;
        bs.heldAction.clear();

        if (!bs.actions.empty() && bs.actions[0].holdDurationMs == 0) {
            bs.activeActionIndex = 0;
            const FakeInputAction& tapAction = bs.actions[0];

            if (!tapAction.onRelease) {
                FakeInput::SendAction(tapAction.keycodes, true);
                bs.heldAction = tapAction.keycodes;
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
            if (!bs.heldAction.empty()) {
                FakeInput::SendAction(bs.heldAction, false);
            }
            bs.activeActionIndex = newActionIndex;
            bs.pressActionFired = false;
            bs.heldAction.clear();
            const FakeInputAction& newActiveAction = bs.actions[bs.activeActionIndex];
            if (!newActiveAction.onRelease) {
                FakeInput::SendAction(newActiveAction.keycodes, true);
                bs.heldAction = newActiveAction.keycodes;
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
            const FakeInputAction& finalAction = bs.actions[finalActionIndex];

            FakeInput::SendAction(finalAction.keycodes, true);
            bs.pendingReleaseAction = finalAction.keycodes;
            bs.pendingReleaseTime = GetTickCount64() + 20;
        }

        if (!bs.heldAction.empty()) {
            FakeInput::SendAction(bs.heldAction, false);
        }

        bs.activeActionIndex = static_cast<size_t>(-1);
        bs.pressActionFired = false;
        bs.heldAction.clear();
        return;
    }
}

void ProcessTrigger(UINT triggerID, BYTE triggerValue) {
    bool isPressed = IsTriggerPressed(triggerValue);
    ProcessButton(triggerID, isPressed);
}