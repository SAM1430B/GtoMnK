/*
  This file serves as the core translation engine for controller input. This class bridges the
  gap between raw gamepad states (from XInput/SDL2) and actionable in-game mechanics.
*/

#include "pch.h"
#include "GamepadProcessor.h"
#include "GamepadInputIDs.h"
#include "FakeMouse.h"
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
    const float k = 0.41421356f;
    const float deadzoneSq = stick_as_button_deadzone * stick_as_button_deadzone;

    // Left Thumbstick
    if (!useLeftStickForMouse) {
        float normLX = static_cast<float>(state.ThumbLX) / 32767.0f;
        float normLY = static_cast<float>(state.ThumbLY) / 32767.0f;

        if (std::abs(normLX) < stick_as_button_axial_deadzone) normLX = 0.0f;
        if (std::abs(normLY) < stick_as_button_axial_deadzone) normLY = 0.0f;

        bool isLSU = false, isLSD = false, isLSL = false, isLSR = false;

        if ((normLX * normLX + normLY * normLY) > deadzoneSq) {
            isLSU = normLY > k * std::abs(normLX);
            isLSD = normLY < -k * std::abs(normLX);
            isLSR = normLX > k * std::abs(normLY);
            isLSL = normLX < -k * std::abs(normLY);
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

        if (std::abs(normRX) < stick_as_button_axial_deadzone) normRX = 0.0f;
        if (std::abs(normRY) < stick_as_button_axial_deadzone) normRY = 0.0f;

        bool isRSU = false, isRSD = false, isRSL = false, isRSR = false;

        if ((normRX * normRX + normRY * normRY) > deadzoneSq) {
            isRSU = normRY > k * std::abs(normRX);
            isRSD = normRY < -k * std::abs(normRX);
            isRSR = normRX > k * std::abs(normRY);
            isRSL = normRX < -k * std::abs(normRY);
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
    if (g_UseLegacyMouseMovement) {
        if (totalDelta.x != 0 || totalDelta.y != 0) {
            FakeMouse::Xf += totalDelta.x;
            FakeMouse::Yf += totalDelta.y;

            FakeMouse::Xf = (std::max)((LONG)m_clientRect.left, (std::min)((LONG)FakeMouse::Xf, (LONG)m_clientRect.right - 1));
            FakeMouse::Yf = (std::max)((LONG)m_clientRect.top, (std::min)((LONG)FakeMouse::Yf, (LONG)m_clientRect.bottom - 1));
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
                    POINT screenPos = { (LONG)FakeMouse::Xf, (LONG)FakeMouse::Yf };
                    ClientToScreen(m_hwnd, &screenPos);
                    FakeInput::SendMouseMoveAbsolute(screenPos.x, screenPos.y);
                }
            }
        }
    }
    else {
        static LONG unsentRawX = 0;
        static LONG unsentRawY = 0;

        if (totalDelta.x != 0 || totalDelta.y != 0) {
            FakeMouse::Xf += totalDelta.x;
            FakeMouse::Yf += totalDelta.y;

            FakeMouse::Xf = (std::max)((LONG)m_clientRect.left, (std::min)((LONG)FakeMouse::Xf, (LONG)m_clientRect.right - 1));
            FakeMouse::Yf = (std::max)((LONG)m_clientRect.top, (std::min)((LONG)FakeMouse::Yf, (LONG)m_clientRect.bottom - 1));

            if (drawProtoFakeCursor == 1) {
                FakeCursor::NotifyUpdatedCursorPosition();
            }

            unsentRawX += totalDelta.x;
            unsentRawY += totalDelta.y;
        }

        static LARGE_INTEGER frequency;
        static LARGE_INTEGER lastSendTime;
        static bool timerInit = false;

        if (!timerInit) {
            QueryPerformanceFrequency(&frequency);
            QueryPerformanceCounter(&lastSendTime);
            timerInit = true;
        }

        LARGE_INTEGER currentTime;
        QueryPerformanceCounter(&currentTime);

        double elapsedMs = static_cast<double>(currentTime.QuadPart - lastSendTime.QuadPart) * 1000.0 / frequency.QuadPart;

        if (elapsedMs >= m_moveUpdateInterval) {

            LONGLONG intervalTicks = static_cast<LONGLONG>(m_moveUpdateInterval * frequency.QuadPart / 1000.0);
            lastSendTime.QuadPart += intervalTicks;

            if (elapsedMs > m_moveUpdateInterval * 5.0) {
                lastSendTime = currentTime;
            }

            if (g_FakeInputMethod == FakeInputMethod::RawInput || g_FakeInputMethod == FakeInputMethod::Hybrid) {
                if (unsentRawX != 0 || unsentRawY != 0) {
                    FakeInput::SendMouseMoveDelta(unsentRawX, unsentRawY);
                    unsentRawX = 0;
                    unsentRawY = 0;
                }
            }

            if (g_FakeInputMethod == FakeInputMethod::PostMessage || g_FakeInputMethod == FakeInputMethod::Hybrid) {
                static LONG lastSentAbsX = -1;
                static LONG lastSentAbsY = -1;

                if (FakeMouse::Xf != lastSentAbsX || FakeMouse::Yf != lastSentAbsY) {
                    POINT screenPos = { (LONG)FakeMouse::Xf, (LONG)FakeMouse::Yf };
                    ClientToScreen(m_hwnd, &screenPos);
                    FakeInput::SendMouseMoveAbsolute(screenPos.x, screenPos.y);

                    lastSentAbsX = FakeMouse::Xf;
                    lastSentAbsY = FakeMouse::Yf;
                }
            }
        }
    }
}