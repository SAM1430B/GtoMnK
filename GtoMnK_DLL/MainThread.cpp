#include "pch.h"
#include "MainThread.h"
#include "INISettings.h"
#include "Logging.h"

#include "HandleMainWindow.h"
#include "InstallHooks.h"

#include "RawInput.h"
#include "FakeCursor.h"

#include "OverlayMenu.h"
#include "OverlayNotification.h"

#include "GamepadProcessor.h"
#include "GamepadBackend.h"

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
    InstallHooks::SetupHooks();
    hooksinited = true;

    if (g_FakeInputMethod == FakeInputMethod::RawInput || g_FakeInputMethod == FakeInputMethod::Hybrid) {
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

    // Initialize Gamepad Input Backend
    GamepadBackend::Initialize();
    
    CustomControllerState state; // Reused every frame
    GamepadProcessor gamepadProcessor; // Handles translation of controller input to mouse/button actions

    while (loop) {

        if (!RecoverMainWindow(hwnd, recheckHWND, iniWindowName, iniClassName)) {
            loop = false;
            break;
        }

        RECT rect;
        if (!GetClientRect(hwnd, &rect)) {
            rect.left = 0; rect.top = 0; rect.right = 800; rect.bottom = 600;
        }

        gamepadProcessor.UpdateWindowInfo(hwnd, rect);

        bool isConnected = GamepadBackend::GetState(state);

        if (isConnected) {

			// Handle Overlay Menu Toggle (Back + D-Pad Down)
            if (OverlayMenu::state.HandleOverlayMenuToggle(state)) {
                continue;
            }

            if (showmessage == 12) showmessage = 0; // "disconnected" message on reconnect

            // Route input to the overlay menu if it's open; otherwise, process the game input.
            if (OverlayMenu::state.isMenuOpen) {
                OverlayMenu::state.ProcessInput(state);
            }
            else {
                gamepadProcessor.ProcessInput(state);
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
    GamepadBackend::Cleanup();

    if (IsOverlayNotificationEnabled) {
        OverlayNotification::state.CleanupGDIOverlay();
		LOG("Overlay Notification cleaned up.");
    }

    LOG("ThreadFunction gracefully exiting.");
    return 0;
}