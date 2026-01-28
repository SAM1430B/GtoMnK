#include "pch.h"
#include "Logging.h"
#include "Hooks.h"
#include "Mouse.h"
#include "Keyboard.h"
#include "InputState.h"
#include "RawInput.h"
#include "MainThread.h"
#include "FakeCursor.h"
#include "OverlayMenu.h"
#include "EnableOpenXinput.h"
#include "INISettings.h"
#include "HandleMainWindow.h"
#include "GamepadState.h"

using namespace GtoMnK;

XInputGetStateFunc pXInputGetState = nullptr;
extern "C" BOOL WINAPI OpenXinputDllMain(HINSTANCE hInstDll, DWORD fdwReason, LPVOID lpvReserved);

// For Initialization and Thread state
HMODULE g_hModule = nullptr;
bool hooksinited = false;
bool loop = true;

// For Keyboard States
int keystatesend = 0;

// Others
HWND hwnd = nullptr;
HANDLE hMutex = nullptr;

// Drawing & cursor state
bool onoroff = true;
int showmessage = 0;
int counter = 0;
bool pausedraw = false;

// For SetRectHook and AdjustWindowRectHook
int leftrect, toprect, rightrect, bottomrect;

// General
//bool gotcursoryet = false;
//POINT startdrag = { 0, 0 };
//DWORD lastClickTime = 0;
//int userealmouse;
//int alwaysdrawcursor;
//int doubleclicks;
//int scrolloutsidewindow;
//int quickMW;
//int scrollenddelay;
//int tick = 0;
//bool doscrollyes = false;
//POINT scroll = { 0, 0 };
//bool scrollmap = false;
//POINT rectignore = { 0, 0 };
//int ignorerect;


void DrawOverlay() {
    if (showmessage == 0 && !(drawfakecursor && mode == 1 && onoroff)) {
        return;
    }
    HDC hdcWindow = GetDC(hwnd);
    if (!hdcWindow) {
        return;
    }
    if (showmessage == 1) TextOutA(hdcWindow, Mouse::Xf, Mouse::Yf, "KEYBOARD MODE", 13);
    else if (showmessage == 2) TextOutA(hdcWindow, Mouse::Xf, Mouse::Yf, "CURSOR MODE", 11);
    else if (showmessage == 69) TextOutA(hdcWindow, Mouse::Xf, Mouse::Yf, "DISABLED", 8);
    else if (showmessage == 70) TextOutA(hdcWindow, Mouse::Xf, Mouse::Yf, "ENABLED", 7);
    else if (showmessage == 12) TextOutA(hdcWindow, 20, 20, "CONTROLLER DISCONNECTED", 23);
    else if (drawfakecursor && mode == 1 && onoroff) {
        Mouse::DrawBeautifulCursor(hdcWindow);
    }
    ReleaseDC(hwnd, hdcWindow);
}

DWORD WINAPI ThreadFunction(LPVOID lpParam) {
    LOG("ThreadFunction started.");

    LoadIniSettings();
    LOG("INI settings is Loaded");

    if (g_EnableOpenXinput) {
        LOG("Initializing OpenXinput...");
        OpenXinputDllMain(GetModuleHandle(NULL), DLL_PROCESS_ATTACH, NULL);
        pXInputGetState = (XInputGetStateFunc)OXI::OpenXInputGetState;
    }
    else {
        LOG("Using Standard XInput.");
        pXInputGetState = (XInputGetStateFunc)XInputGetState;
    }

    if (controllerID < 0) {
        LOG("Controller is disabled by INI. Thread is exiting.");
        loop = false;
        return 0;
    }
    else {
        LOG("Using controller ID: %d", controllerID);
    }

    if (startUpDelay > 0) {
        LOG("Waiting for startup delay of %dSec before hooking...", startUpDelay);
        Sleep(startUpDelay * 1000);
    }

    LOG("Searching for Game Window...");
    if (iniWindowName[0] != '\0') { LOG("Filter Name: '%s'", iniWindowName); }
    if (iniClassName[0] != '\0') { LOG("Filter Class: '%s'", iniClassName); }
    hwnd = GetMainWindowHandle(GetCurrentProcessId(), iniWindowName, iniClassName, 300000);
    if (!hwnd) {
        LOG("CRITICAL: Game window not found after 5min. Thread exiting to prevent crash.");
        loop = false;
        return 0;
    }
    LOG("Initial window handle acquired: 0x%p", hwnd);

    if (drawProtoFakeCursor == 1) {
        LOG("ProtoInput Fake Cursor is enabled. Initializing...");
        FakeCursor::Initialise();
        if (!createdWindowIsOwned)
            FakeCursor::EnableDisableFakeCursor(true);
        // CursorVisibilityHook will be enabled automatically when `drawProtoFakeCursor` is enabled. 
    }
    else if (drawfakecursor) {
        LOG("Simple Fake Cursor is enabled.");
    }

    Hooks::SetupHooks();
    hooksinited = true;

    if (g_InputMethod == InputMethod::RawInput) {
        RawInput::Initialize();
    }
    
	// Overlay Options
    if (!disableOverlayOptions)
    {
        OverlayMenu::state.Initialise();
        LOG("Overlay Options Initialized.");
    }
    else {
        LOG("Overlay Options is disabled.");
    }
    ULONGLONG menuToggleTimer = 0;
    bool menuTogglePending = false;

    ULONGLONG lastMoveTime = 0;
    const DWORD MOVE_UPDATE_INTERVAL = 8;

    while (loop) {
        bool movedmouse = false;

        if (recheckHWND) {
            if (hwnd && !IsWindow(hwnd)) {
                LOG("Window handle became invalid. Resetting...");
                hwnd = nullptr;
            }
        }

        if (!hwnd) {
            hwnd = GetMainWindowHandle(GetCurrentProcessId(), iniWindowName, iniClassName, 300000);

            if (!hwnd) {
                LOG("CRITICAL: Game window missing for 5 minute.");
                loop = false;
                break;
            }
            LOG("Acquired new window handle: 0x%p", hwnd);
        }

        RECT rect;
        if (!GetClientRect(hwnd, &rect)) {
            rect.left = 0; rect.top = 0; rect.right = 800; rect.bottom = 600;
        }

        XINPUT_STATE state;
        ZeroMemory(&state, sizeof(XINPUT_STATE));
        if (pXInputGetState(controllerID, &state) == ERROR_SUCCESS) {

            if (!disableOverlayOptions)
            {
                // Buttons to open the overlay window
                bool backDown = (state.Gamepad.wButtons & XINPUT_GAMEPAD_BACK) != 0;
                bool dpadDown = (state.Gamepad.wButtons & XINPUT_GAMEPAD_DPAD_DOWN) != 0;

                if (backDown && dpadDown) {
                    ProcessButton(XINPUT_GAMEPAD_BACK, false);
                    ProcessButton(XINPUT_GAMEPAD_DPAD_DOWN, false);
                    if (!menuTogglePending) {
                        menuToggleTimer = GetTickCount64();
                        menuTogglePending = true;
                    }
                    else if (GetTickCount64() - menuToggleTimer > 50) {
                        // Toggle the State
                        bool newState = !OverlayMenu::state.isMenuOpen;
                        OverlayMenu::state.EnableDisableMenu(newState);

                        menuTogglePending = false;
                        Mouse::Xf = Mouse::Xf;
                        XINPUT_STATE waitState;
                        ZeroMemory(&waitState, sizeof(XINPUT_STATE));
                        do {
                            Sleep(100); // Important
                            if (pXInputGetState(controllerID, &waitState) != ERROR_SUCCESS) {
                                break;
                            }
                        } while ((waitState.Gamepad.wButtons & XINPUT_GAMEPAD_BACK) ||
                            (waitState.Gamepad.wButtons & XINPUT_GAMEPAD_DPAD_DOWN));
                        continue;
                    }
                }
                else {
                    menuTogglePending = false;
                }
            }

            if (showmessage == 12) showmessage = 0; // "disconnected" message on reconnect

            if (!disableOverlayOptions && OverlayMenu::state.isMenuOpen) {
                OverlayMenu::state.ProcessInput(state.Gamepad.wButtons);
            }

            else {
                // Buttons
                ProcessButton(XINPUT_GAMEPAD_A, (state.Gamepad.wButtons & XINPUT_GAMEPAD_A) != 0);
                ProcessButton(XINPUT_GAMEPAD_B, (state.Gamepad.wButtons & XINPUT_GAMEPAD_B) != 0);
                ProcessButton(XINPUT_GAMEPAD_X, (state.Gamepad.wButtons & XINPUT_GAMEPAD_X) != 0);
                ProcessButton(XINPUT_GAMEPAD_Y, (state.Gamepad.wButtons & XINPUT_GAMEPAD_Y) != 0);
                ProcessButton(XINPUT_GAMEPAD_RIGHT_SHOULDER, (state.Gamepad.wButtons & XINPUT_GAMEPAD_RIGHT_SHOULDER) != 0);
                ProcessButton(XINPUT_GAMEPAD_LEFT_SHOULDER, (state.Gamepad.wButtons & XINPUT_GAMEPAD_LEFT_SHOULDER) != 0);
                ProcessButton(XINPUT_GAMEPAD_RIGHT_THUMB, (state.Gamepad.wButtons & XINPUT_GAMEPAD_RIGHT_THUMB) != 0);
                ProcessButton(XINPUT_GAMEPAD_LEFT_THUMB, (state.Gamepad.wButtons & XINPUT_GAMEPAD_LEFT_THUMB) != 0);
                ProcessButton(XINPUT_GAMEPAD_DPAD_UP, (state.Gamepad.wButtons & XINPUT_GAMEPAD_DPAD_UP) != 0);
                ProcessButton(XINPUT_GAMEPAD_DPAD_DOWN, (state.Gamepad.wButtons & XINPUT_GAMEPAD_DPAD_DOWN) != 0);
                ProcessButton(XINPUT_GAMEPAD_DPAD_LEFT, (state.Gamepad.wButtons & XINPUT_GAMEPAD_DPAD_LEFT) != 0);
                ProcessButton(XINPUT_GAMEPAD_DPAD_RIGHT, (state.Gamepad.wButtons & XINPUT_GAMEPAD_DPAD_RIGHT) != 0);
                ProcessButton(XINPUT_GAMEPAD_START, (state.Gamepad.wButtons & XINPUT_GAMEPAD_START) != 0);
                ProcessButton(XINPUT_GAMEPAD_BACK, (state.Gamepad.wButtons & XINPUT_GAMEPAD_BACK) != 0);

                // Triggers
                ProcessTrigger(CUSTOM_ID_LT, state.Gamepad.bLeftTrigger);
                ProcessTrigger(CUSTOM_ID_RT, state.Gamepad.bRightTrigger);

                // Analog Sticks (as buttons)
                // Left Stick
                if (onoroff && (mode == 0 || (mode == 1 && righthanded == 1))) {
                    float normLX = static_cast<float>(state.Gamepad.sThumbLX) / 32767.0f;
                    float normLY = static_cast<float>(state.Gamepad.sThumbLY) / 32767.0f;
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
                    ProcessButton(CUSTOM_ID_LSU, isLSU);
                    ProcessButton(CUSTOM_ID_LSD, isLSD);
                    ProcessButton(CUSTOM_ID_LSL, isLSL);
                    ProcessButton(CUSTOM_ID_LSR, isLSR);
                }
                // Right Stick
                if (onoroff && (mode == 0 || (mode == 1 && righthanded == 0))) {
                    float normRX = static_cast<float>(state.Gamepad.sThumbRX) / 32767.0f;
                    float normRY = static_cast<float>(state.Gamepad.sThumbRY) / 32767.0f;
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
                    ProcessButton(CUSTOM_ID_RSU, isRSU);
                    ProcessButton(CUSTOM_ID_RSD, isRSD);
                    ProcessButton(CUSTOM_ID_RSL, isRSL);
                    ProcessButton(CUSTOM_ID_RSR, isRSR);
                }

                // Mouse MOVEMENT
                if (mode == 1 && onoroff) {
                    SHORT thumbX = (righthanded == 1) ? state.Gamepad.sThumbRX : state.Gamepad.sThumbLX;
                    SHORT thumbY = (righthanded == 1) ? state.Gamepad.sThumbRY : state.Gamepad.sThumbLY;
                    POINT delta = ThumbstickMouseMove(thumbX, thumbY);
                    if (delta.x != 0 || delta.y != 0) {
                        Mouse::Xf += delta.x; Mouse::Yf += delta.y;
                        Mouse::Xf = std::max((LONG)rect.left, std::min((LONG)Mouse::Xf, (LONG)rect.right - 1));
                        Mouse::Yf = std::max((LONG)rect.top, std::min((LONG)Mouse::Yf, (LONG)rect.bottom - 1));
                        movedmouse = true;

                        //ProtoInput Fake Cursor update
                        if (drawProtoFakeCursor == 1) {
                            FakeCursor::NotifyUpdatedCursorPosition();
                        }

                        ULONGLONG currentTime = GetTickCount64();
                        if (currentTime - lastMoveTime > MOVE_UPDATE_INTERVAL) {
                            lastMoveTime = currentTime;
                            if (g_InputMethod == InputMethod::RawInput) {
                                Input::SendActionDelta(delta.x, delta.y);
                            }
                            else {
                                POINT screenPos = { (LONG)Mouse::Xf, (LONG)Mouse::Yf };
                                ClientToScreen(hwnd, &screenPos);
                                Input::SendAction(screenPos.x, screenPos.y);
                            }
                        }
                    }
                }
            }
        }
        else {
            showmessage = 12; // Controller disconnected
        }

        // Drawing and Message
        DrawOverlay();
        if (showmessage != 0 && showmessage != 12) {
            if (++counter > 150) {
                showmessage = 0;
                counter = 0;
            }
        }
        
        if (responsetime <= 0)
            responsetime = 1;
        Sleep(responsetime);
    }

    LOG("ThreadFunction gracefully exiting.");
    return 0;
}