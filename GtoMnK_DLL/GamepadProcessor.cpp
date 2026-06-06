/*
  This file serves as the core translation engine for controller input. This class bridges the
  gap between raw gamepad states (from XInput/SDL2) and actionable in-game mechanics.
*/

#include "pch.h"
#include "GamepadProcessor.h"
#include "GamepadInputIDs.h"
#include "Mouse.h"
#include "FakeCursor.h"
#include "RawInput.h"
#include "INISettings.h"
#include <algorithm>
#include <cmath>

GamepadProcessor::GamepadProcessor() : m_hwnd(nullptr), m_lastMoveTime(0) {
    m_clientRect = { 0, 0, 800, 600 };
}

void GamepadProcessor::UpdateWindowInfo(HWND newHwnd, RECT newRect) {
    m_hwnd = newHwnd;
    m_clientRect = newRect;
}

void GamepadProcessor::ProcessInput(const CustomControllerState& state) {
    ProcessStandardButtons(state);

    int activeThumbStickToMouse = GetActiveThumbStickToMouse();
    bool useLeftStickForMouse = (activeThumbStickToMouse == 1);
    bool useRightStickForMouse = (activeThumbStickToMouse == 2);

    ProcessThumbsticksAsButtons(state, useLeftStickForMouse, useRightStickForMouse);
    ProcessMouseMovement(state, useLeftStickForMouse, useRightStickForMouse);
}

void GamepadProcessor::ProcessStandardButtons(const CustomControllerState& state) {
    // Face Buttons
    ProcessButton(GAMEPAD_ID_A, state.buttons[GAMEPAD_ID_A]);
    ProcessButton(GAMEPAD_ID_B, state.buttons[GAMEPAD_ID_B]);
    ProcessButton(GAMEPAD_ID_X, state.buttons[GAMEPAD_ID_X]);
    ProcessButton(GAMEPAD_ID_Y, state.buttons[GAMEPAD_ID_Y]);

    // D-Pad
    ProcessButton(GAMEPAD_ID_DPAD_UP, state.buttons[GAMEPAD_ID_DPAD_UP]);
    ProcessButton(GAMEPAD_ID_DPAD_DOWN, state.buttons[GAMEPAD_ID_DPAD_DOWN]);
    ProcessButton(GAMEPAD_ID_DPAD_LEFT, state.buttons[GAMEPAD_ID_DPAD_LEFT]);
    ProcessButton(GAMEPAD_ID_DPAD_RIGHT, state.buttons[GAMEPAD_ID_DPAD_RIGHT]);

    // Start & Back
    ProcessButton(GAMEPAD_ID_START, state.buttons[GAMEPAD_ID_START]);
    ProcessButton(GAMEPAD_ID_BACK, state.buttons[GAMEPAD_ID_BACK]);

    // Extended Buttons
    ProcessButton(GAMEPAD_ID_GUIDE, state.buttons[GAMEPAD_ID_GUIDE]);
    ProcessButton(GAMEPAD_ID_MISC1, state.buttons[GAMEPAD_ID_MISC1]);
    ProcessButton(GAMEPAD_ID_PADDLE1, state.buttons[GAMEPAD_ID_PADDLE1]);
    ProcessButton(GAMEPAD_ID_PADDLE2, state.buttons[GAMEPAD_ID_PADDLE2]);
    ProcessButton(GAMEPAD_ID_PADDLE3, state.buttons[GAMEPAD_ID_PADDLE3]);
    ProcessButton(GAMEPAD_ID_PADDLE4, state.buttons[GAMEPAD_ID_PADDLE4]);

    // Touchpad Button
    ProcessButton(GAMEPAD_ID_TOUCHPAD_BUTTON, state.buttons[GAMEPAD_ID_TOUCHPAD_BUTTON]);

    // Stick Buttons
    ProcessButton(GAMEPAD_ID_RSB, state.buttons[GAMEPAD_ID_RSB]);
    ProcessButton(GAMEPAD_ID_LSB, state.buttons[GAMEPAD_ID_LSB]);

    // Shoulder Buttons
    ProcessButton(GAMEPAD_ID_RB, state.buttons[GAMEPAD_ID_RB]);
    ProcessButton(GAMEPAD_ID_LB, state.buttons[GAMEPAD_ID_LB]);

    // Triggers
    ProcessTrigger(GAMEPAD_ID_LT, state.LeftTrigger);
    ProcessTrigger(GAMEPAD_ID_RT, state.RightTrigger);
}

void GamepadProcessor::ProcessThumbsticksAsButtons(const CustomControllerState& state, bool useLeftStickForMouse, bool useRightStickForMouse) {
    // Left Thumbstick
    if (!useLeftStickForMouse) {
        float normLX = static_cast<float>(state.ThumbLX) / 32767.0f;
        float normLY = static_cast<float>(state.ThumbLY) / 32767.0f;
        float magnitudeL = static_cast<float>(sqrt(normLX * normLX + normLY * normLY));
        bool isLSU = false, isLSD = false, isLSL = false, isLSR = false;

        if (magnitudeL > stick_as_button_deadzone) {
            float angleDeg = atan2(normLY, normLX) * 180.0f / 3.1415926535f;
            if (angleDeg < 0) angleDeg += 360.0f;
            if (angleDeg > 22.5f && angleDeg < 157.5f) isLSU = true;
            if (angleDeg > 202.5f && angleDeg < 337.5f) isLSD = true;
            if (angleDeg > 112.5f && angleDeg < 247.5f) isLSL = true;
            if (angleDeg > 292.5f || angleDeg < 67.5f) isLSR = true;
        }
        ProcessButton(GAMEPAD_ID_LSU, isLSU);
        ProcessButton(GAMEPAD_ID_LSD, isLSD);
        ProcessButton(GAMEPAD_ID_LSL, isLSL);
        ProcessButton(GAMEPAD_ID_LSR, isLSR);
    }

    // Right Thumbstick
    if (!useRightStickForMouse) {
        float normRX = static_cast<float>(state.ThumbRX) / 32767.0f;
        float normRY = static_cast<float>(state.ThumbRY) / 32767.0f;
        float magnitudeR = static_cast<float>(sqrt(normRX * normRX + normRY * normRY));
        bool isRSU = false, isRSD = false, isRSL = false, isRSR = false;

        if (magnitudeR > stick_as_button_deadzone) {
            float angleDeg = atan2(normRY, normRX) * 180.0f / 3.1415926535f;
            if (angleDeg < 0) angleDeg += 360.0f;
            if (angleDeg > 22.5f && angleDeg < 157.5f) isRSU = true;
            if (angleDeg > 202.5f && angleDeg < 337.5f) isRSD = true;
            if (angleDeg > 112.5f && angleDeg < 247.5f) isRSL = true;
            if (angleDeg > 292.5f || angleDeg < 67.5f) isRSR = true;
        }
        ProcessButton(GAMEPAD_ID_RSU, isRSU);
        ProcessButton(GAMEPAD_ID_RSD, isRSD);
        ProcessButton(GAMEPAD_ID_RSL, isRSL);
        ProcessButton(GAMEPAD_ID_RSR, isRSR);
    }
}

void GamepadProcessor::ProcessMouseMovement(const CustomControllerState& state, bool useLeftStick, bool useRightStick) {
    POINT totalDelta = { 0, 0 };

    // Thumbstick As Mouse
    if (useLeftStick || useRightStick) {
        SHORT thumbX = useRightStick ? state.ThumbRX : state.ThumbLX;
        SHORT thumbY = useRightStick ? state.ThumbRY : state.ThumbLY;

        POINT thumbDelta = ThumbstickMouseMove(thumbX, thumbY);
        totalDelta.x += thumbDelta.x;
        totalDelta.y += thumbDelta.y;
    }

    // Touchpad As Mouse (SDL2 only)
    bool useTouchpadForMouse = (GetActiveTouchPadToMouse() == 1);
    if (g_GamepadMethod == GamepadMethod::SDL2 && useTouchpadForMouse) {
        POINT touchDelta = TouchpadMouseMove(state.TouchpadX, state.TouchpadY, state.TouchpadActive);
        totalDelta.x += touchDelta.x;
        totalDelta.y += touchDelta.y;
    }

    // Apply Movement
    if (totalDelta.x != 0 || totalDelta.y != 0) {
        Mouse::Xf += totalDelta.x;
        Mouse::Yf += totalDelta.y;

        Mouse::Xf = std::max((LONG)m_clientRect.left, std::min((LONG)Mouse::Xf, (LONG)m_clientRect.right - 1));
        Mouse::Yf = std::max((LONG)m_clientRect.top, std::min((LONG)Mouse::Yf, (LONG)m_clientRect.bottom - 1));

        // ProtoInput Fake Cursor update
        if (drawProtoFakeCursor == 1) {
            FakeCursor::NotifyUpdatedCursorPosition();
        }

        ULONGLONG currentTime = GetTickCount64();
        if (currentTime - m_lastMoveTime > m_moveUpdateInterval) {
            m_lastMoveTime = currentTime;

            if (g_FakeInputMethod == FakeInputMethod::RawInput || g_FakeInputMethod == FakeInputMethod::Hybrid) {
                FakeInput::SendMouseMoveDelta(totalDelta.x, totalDelta.y);
            }
            if (g_FakeInputMethod == FakeInputMethod::PostMessage || g_FakeInputMethod == FakeInputMethod::Hybrid) {
                POINT screenPos = { (LONG)Mouse::Xf, (LONG)Mouse::Yf };
                ClientToScreen(m_hwnd, &screenPos);
                FakeInput::SendMouseMoveAbsolute(screenPos.x, screenPos.y);
            }
        }
    }
}