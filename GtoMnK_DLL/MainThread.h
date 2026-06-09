#pragma once
#include <map>
#include "FakeInput.h"

struct ButtonState {
    bool isPhysicallyPressed = false;
    ULONGLONG pressTime = 0;
    size_t activeActionIndex = static_cast<size_t>(-1);
    bool pressActionFired = false;
    std::vector<GtoMnK::FakeInputAction> actions;
    std::vector<int> heldAction;

    ULONGLONG pendingReleaseTime = 0;
    std::vector<int> pendingReleaseAction;
};

extern HMODULE g_hModule;
extern bool loop;
extern bool hooksinited;
extern bool IsOverlayNotificationEnabled;
extern std::array<ButtonState, 256> buttonStates;

// Used by Input.cpp, Hooks.cpp, dllmain.cpp, etc.
extern GtoMnK::FakeInputMethod g_FakeInputMethod;
extern HWND hwnd;

// For finding the game window
extern char iniWindowName[256];
extern char iniClassName[256];

extern int keystatesend;
extern bool g_EnableMouseDoubleClick;

DWORD WINAPI ThreadFunction(LPVOID lpParam);