#include "pch.h"
#include "MainThread.h"
#include "INISettings.h"
#include "GamepadInputIDs.h"
#include "Logging.h"
#include "Hooks.h"
#include "Mouse.h"
#include "RawInput.h"
#include "FakeCursor.h"
#include "OverlayMenu.h"
#include "Xinput_Gamepad.h"
#include "SDL2_Gamepad.h"
#include "OverlayNotification.h"

using namespace GtoMnK;

// For Initialization and Thread state
bool hooksinited = false;
bool loop = true;

// For Keyboard States
int keystatesend = 0;

// Others
HWND hwnd = nullptr;
HANDLE hMutex = nullptr;

// Drawing & cursor state
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

void WaitForOtherDll() {
    if (waitForDllName[0] == '\0') {
        return;
    }
    LOG("Waiting for DLL '%s' to be loaded into memory...", waitForDllName);

    while (loop) {
        if (GetModuleHandleA(waitForDllName) != NULL) {
            LOG("Found DLL '%s'. Proceeding with initialization...", waitForDllName);
            break;
        }
        Sleep(500);
    }
}

DWORD WINAPI ThreadFunction(LPVOID lpParam) {
    LOG("ThreadFunction started.");

    LoadIniSettings();
    LOG("INI settings is Loaded");

    // Check Controller ID
    if (controllerID < 0) {
        LOG("Controller is disabled by INI. Thread is exiting.");
        loop = false;
        return 0;
    }
    else {
        LOG("Using controller ID: %d", controllerID);
    }

    // Wait for external DLL
    WaitForOtherDll();

    // Startup Delay
    if (startUpDelay > 0) {
        LOG("Waiting for startup delay of %dsec before hooking...", startUpDelay);
        Sleep(startUpDelay * 1000);
    }

    // Find Main Window
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

    // Initialize Fake Cursor
    if (drawProtoFakeCursor == 1) {
        LOG("ProtoInput Fake Cursor is enabled. Initializing...");
        FakeCursor::Initialise();
        if (!createdWindowIsOwned)
            FakeCursor::EnableDisableFakeCursor(true);
    }

    // Setup Hooks
    Hooks::SetupHooks();
    hooksinited = true;

    if (g_InputMethod == InputMethod::RawInput || g_InputMethod == InputMethod::Hybrid) {
        RawInput::Initialize();
    }

    // Initialize Overlay
    if (!disableOverlayOptions)
    {
        OverlayMenu::state.Initialise();
        LOG("Overlay Options Initialized.");
        OverlayNotification::state.Initialise();
		LOG("Overlay Notification Initialized.");
    }
    else {
        LOG("Overlay Options is disabled.");
    }

    // Initialize Input API
    if (g_GamepadMethod == GamepadMethod::XInput) {
        XInput_Initialize();
    }
    else {
        SDL2_Initialize();
        
    }
    
    ULONGLONG menuToggleTimer = 0;
    bool menuTogglePending = false;
    ULONGLONG lastMoveTime = 0;
    const DWORD MOVE_UPDATE_INTERVAL = 8;
    CustomControllerState state; // Reused every frame

    while (loop) {

        // Window Handle Validation
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

            /*if (g_InputMethod == InputMethod::RawInput || g_InputMethod == InputMethod::Hyberd) {
                LOG("Window changed! Re-running RawInput recovery...");
                RawInput::RecoverMissedRegistration();
            }*/
        }

        RECT rect;
        if (!GetClientRect(hwnd, &rect)) {
            rect.left = 0; rect.top = 0; rect.right = 800; rect.bottom = 600;
        }

        bool isConnected = false;

        switch (g_GamepadMethod) {
        case GamepadMethod::XInput:
            isConnected = XInput_GetState(state);
            break;
        case GamepadMethod::SDL2:
            isConnected = SDL2_GetState(state);
            break;
        default:
            isConnected = XInput_GetState(state);
            break;
        }

        if (isConnected) {

            if (!disableOverlayOptions && OverlayMenu::state.IsReady())
            {
                bool backDown = state.buttons[GAMEPAD_ID_BACK];
                bool dpadDown = state.buttons[GAMEPAD_ID_DPAD_DOWN];

                if (backDown && dpadDown) {
                    ProcessButton(GAMEPAD_ID_BACK, false);
                    ProcessButton(GAMEPAD_ID_DPAD_DOWN, false);

                    if (!menuTogglePending) {
                        menuToggleTimer = GetTickCount64();
                        menuTogglePending = true;
                    }
                    else if (GetTickCount64() - menuToggleTimer > 50) {
                        // Toggle Menu
                        bool newState = !OverlayMenu::state.isMenuOpen;
                        OverlayMenu::state.EnableDisableMenu(newState);
                        menuTogglePending = false;
                        Mouse::Xf = Mouse::Xf;

                        CustomControllerState waitState;
                        do {
                            Sleep(100); // Important
                            bool waitConnected = false;
                            if (g_GamepadMethod == GamepadMethod::SDL2) {waitConnected = SDL2_GetState(waitState);}
                            else {waitConnected = XInput_GetState(waitState);}
                            if (!waitConnected) break;
                        } while (waitState.buttons[GAMEPAD_ID_BACK] || waitState.buttons[GAMEPAD_ID_DPAD_DOWN]);
                        continue;
                    }
                }
                else {
                    menuTogglePending = false;
                }
            }

            if (showmessage == 12) showmessage = 0; // "disconnected" message on reconnect

            if (!disableOverlayOptions && OverlayMenu::state.isMenuOpen) {
                OverlayMenu::state.ProcessInput(state);
            }
            else {
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

                // Analog Sticks (as buttons)
                int activeThumbStickToMouse = GetActiveThumbStickToMouse();

                // Left Thumbsticks
                bool useLeftStickForMouse = (activeThumbStickToMouse == 1);

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
                // Right Thumbsticks
                bool useRightStickForMouse = (activeThumbStickToMouse == 2);

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

                // Mouse Movement
                POINT totalDelta = { 0, 0 };

                // Thumbstick As Mouse
                if (useLeftStickForMouse || useRightStickForMouse) {
                    SHORT thumbX = useRightStickForMouse ? state.ThumbRX : state.ThumbLX;
                    SHORT thumbY = useRightStickForMouse ? state.ThumbRY : state.ThumbLY;

                    POINT thumbDelta = ThumbstickMouseMove(thumbX, thumbY);
                    totalDelta.x += thumbDelta.x;
                    totalDelta.y += thumbDelta.y;
                }

                // Touchpad As Mouse
                int activeTouchPadToMouse = GetActiveTouchPadToMouse();

                // used only in SDL2 mode since XInput doesn't support it.
                bool useTouchpadForMouse = (activeTouchPadToMouse == 1);
                if (g_GamepadMethod == GamepadMethod::SDL2 && useTouchpadForMouse) {
                    POINT touchDelta = TouchpadMouseMove(state.TouchpadX, state.TouchpadY, state.TouchpadActive);
                    totalDelta.x += touchDelta.x;
                    totalDelta.y += touchDelta.y;
                }

                if (totalDelta.x != 0 || totalDelta.y != 0) {
                    Mouse::Xf += totalDelta.x;
                    Mouse::Yf += totalDelta.y;

                    Mouse::Xf = std::max((LONG)rect.left, std::min((LONG)Mouse::Xf, (LONG)rect.right - 1));
                    Mouse::Yf = std::max((LONG)rect.top, std::min((LONG)Mouse::Yf, (LONG)rect.bottom - 1));

                    //ProtoInput Fake Cursor update
                    if (drawProtoFakeCursor == 1) {
                        FakeCursor::NotifyUpdatedCursorPosition();
                    }

                    ULONGLONG currentTime = GetTickCount64();
                    if (currentTime - lastMoveTime > MOVE_UPDATE_INTERVAL) {
                        lastMoveTime = currentTime;
                        if (g_InputMethod == InputMethod::RawInput || g_InputMethod == InputMethod::Hybrid) {
                            Input::SendMouseMoveDelta(totalDelta.x, totalDelta.y);
                        }
                        if (g_InputMethod == InputMethod::PostMessage || g_InputMethod == InputMethod::Hybrid) {
                            POINT screenPos = { (LONG)Mouse::Xf, (LONG)Mouse::Yf };
                            ClientToScreen(hwnd, &screenPos);
                            Input::SendMouseMoveAbsolute(screenPos.x, screenPos.y);
                        }
                    }
                }
            }
        }
        else {
            showmessage = 12; // Controller disconnected
        }

        // Drawing and Message
        if (IsOverlayNotificationEnabled) {
            OverlayNotification::state.DrawOverlay(showmessage);
        }

        if (responsetime <= 0)
            responsetime = 1;
        Sleep(responsetime);
    }

	// Cleanup
    if (g_GamepadMethod == GamepadMethod::SDL2) {
        SDL2_Cleanup();
    }

    if (IsOverlayNotificationEnabled) {
        OverlayNotification::state.CleanupGDIOverlay();
		LOG("Overlay Notification cleaned up.");
    }

    LOG("ThreadFunction gracefully exiting.");
    return 0;
}