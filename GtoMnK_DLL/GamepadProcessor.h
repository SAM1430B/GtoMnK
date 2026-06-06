#pragma once
#include <windows.h>
#include "GamepadState.h"

class GamepadProcessor {
public:
    GamepadProcessor();

    void UpdateWindowInfo(HWND newHwnd, RECT newRect);

    // This function is used in the MainThread input loop.
    void ProcessInput(const CustomControllerState& state);

private:
    HWND m_hwnd;
    RECT m_clientRect;
    ULONGLONG m_lastMoveTime;
    const DWORD m_moveUpdateInterval = 8;

	// The main gamepad processing functions
    void ProcessStandardButtons(const CustomControllerState& state);
    void ProcessThumbsticksAsButtons(const CustomControllerState& state, bool useLeftStick, bool useRightStick);
    void ProcessMouseMovement(const CustomControllerState& state, bool useLeftStick, bool useRightStick);
};