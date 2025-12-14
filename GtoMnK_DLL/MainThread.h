#pragma once
#include <map>
#include "Input.h"

struct ButtonState {
    bool isPhysicallyPressed = false;
    ULONGLONG pressTime = 0;
    size_t activeActionIndex = static_cast<size_t>(-1);
    bool pressActionFired = false;
    std::vector<Action> actions;
    std::string heldActionString = "0";
};

// Used by dllmain.cpp
extern volatile bool g_isInitialized;
extern HMODULE g_hModule;
extern bool loop;
extern bool hooksinited;
extern std::map<UINT, ButtonState> buttonStates;

// Used by Input.cpp, Hooks.cpp, dllmain.cpp, etc.
extern GtoMnK::InputMethod g_InputMethod;
extern HWND hwnd;
extern int keystatesend;
extern bool g_EnableMouseDoubleClick;

DWORD WINAPI ThreadFunction(LPVOID lpParam);