#include "pch.h"
#include "Hooks.h"
#include "Logging.h"
#include "Input.h"
#include "Mouse.h"
#include "Keyboard.h"
#include "RawInput.h"
#include "RawInputHooks.h"

// External global variables from dllmain.cpp
extern int leftrect, toprect, rightrect, bottomrect;
extern int getCursorPosHook, setCursorPosHook, clipCursorHook, getKeyStateHook, getAsyncKeyStateHook, getKeyboardStateHook, setCursorHook, setRectHook;

extern GtoMnK::InputMethod g_InputMethod;

// For RawInput
HOOK_TRACE_INFO g_getRawInputDataHook = { NULL };
HOOK_TRACE_INFO g_registerRawInputDevicesHook = { NULL };

// General hooks
HOOK_TRACE_INFO g_getCursorPosHookHandle = { NULL };
HOOK_TRACE_INFO g_setCursorPosHookHandle = { NULL };
HOOK_TRACE_INFO g_clipCursorHookHandle = { NULL };
HOOK_TRACE_INFO g_getKeyStateHookHandle = { NULL };
HOOK_TRACE_INFO g_getAsyncKeyStateHookHandle = { NULL };
HOOK_TRACE_INFO g_getKeyboardStateHookHandle = { NULL };
HOOK_TRACE_INFO g_setCursorHookHandle = { NULL };
HOOK_TRACE_INFO g_setRectHookHandle = { NULL };
HOOK_TRACE_INFO g_adjustWindowRectHookHandle = { NULL };

namespace GtoMnK {

    void Hooks::SetupHooks() {
        LOG("--- Setting up API hooks for PostMessage mode ---");
        HMODULE hUser32 = GetModuleHandleA("user32");
        if (!hUser32) {
            LOG("FATAL: Could not get a handle to user32.dll! Hooks will not be installed.");
            return;
        }

        NTSTATUS result;

		// RawInput hooks
        if (g_InputMethod == InputMethod::RawInput) {
            LOG("Installing hooks for RawInput mode...");
            result = LhInstallHook(GetProcAddress(hUser32, "GetRawInputData"), GtoMnK::RawInputHooks::GetRawInputDataHook, NULL, &g_getRawInputDataHook);
            if (FAILED(result)) LOG("Failed to install hook for GetRawInputData: %S", RtlGetLastErrorString());

            result = LhInstallHook(GetProcAddress(hUser32, "RegisterRawInputDevices"), GtoMnK::RawInputHooks::RegisterRawInputDevicesHook, NULL, &g_registerRawInputDevicesHook);
            if (FAILED(result)) LOG("Failed to install hook for RegisterRawInputDevices: %S", RtlGetLastErrorString());
        }
		// General hooks
        if (getCursorPosHook) {
            result = LhInstallHook(GetProcAddress(hUser32, "GetCursorPos"), Mouse::GetCursorPosHook, NULL, &g_getCursorPosHookHandle);
            if (FAILED(result)) LOG("Failed to install hook for GetCursorPos: %S", RtlGetLastErrorString());
        }
        if (setCursorPosHook) {
            result = LhInstallHook(GetProcAddress(hUser32, "SetCursorPos"), Mouse::SetCursorPosHook, NULL, &g_setCursorPosHookHandle);
            if (FAILED(result)) LOG("Failed to install hook for SetCursorPos: %S", RtlGetLastErrorString());
        }
        if (clipCursorHook) {
            result = LhInstallHook(GetProcAddress(hUser32, "ClipCursor"), ClipCursorHook, NULL, &g_clipCursorHookHandle);
            if (FAILED(result)) LOG("Failed to install hook for ClipCursor: %S", RtlGetLastErrorString());
        }
        if (getKeyStateHook) {
            result = LhInstallHook(GetProcAddress(hUser32, "GetKeyState"), Keyboard::GetKeyStateHook, NULL, &g_getKeyStateHookHandle);
            if (FAILED(result)) LOG("Failed to install hook for GetKeyState: %S", RtlGetLastErrorString());
        }
        if (getAsyncKeyStateHook) {
            result = LhInstallHook(GetProcAddress(hUser32, "GetAsyncKeyState"), Keyboard::GetAsyncKeyStateHook, NULL, &g_getAsyncKeyStateHookHandle);
            if (FAILED(result)) LOG("Failed to install hook for GetAsyncKeyState: %S", RtlGetLastErrorString());
        }
        if (getKeyboardStateHook) {
            result = LhInstallHook(GetProcAddress(hUser32, "GetKeyboardState"), Keyboard::GetKeyboardStateHook, NULL, &g_getKeyboardStateHookHandle);
            if (FAILED(result)) LOG("Failed to install hook for GetKeyboardState: %S", RtlGetLastErrorString());
        }
        if (setCursorHook) {
            result = LhInstallHook(GetProcAddress(hUser32, "SetCursor"), SetCursorHook, NULL, &g_setCursorHookHandle);
            if (FAILED(result)) LOG("Failed to install hook for SetCursor: %S", RtlGetLastErrorString());
        }
        if (setRectHook) {
            result = LhInstallHook(GetProcAddress(hUser32, "SetRect"), SetRectHook, NULL, &g_setRectHookHandle);
            if (FAILED(result)) LOG("Failed to install hook for SetRect: %S", RtlGetLastErrorString());
            result = LhInstallHook(GetProcAddress(hUser32, "AdjustWindowRect"), AdjustWindowRectHook, NULL, &g_adjustWindowRectHookHandle);
            if (FAILED(result)) LOG("Failed to install hook for AdjustWindowRect: %S", RtlGetLastErrorString());
        }

        LOG("Activating installed hooks...");
        ULONG threadIdList[] = { 0 };

        if (g_InputMethod == InputMethod::RawInput) {
            LhSetExclusiveACL(threadIdList, 1, &g_getRawInputDataHook);
            LhSetExclusiveACL(threadIdList, 1, &g_registerRawInputDevicesHook);
        }
        if (getCursorPosHook) LhSetExclusiveACL(threadIdList, 1, &g_getCursorPosHookHandle);
        if (setCursorPosHook) LhSetExclusiveACL(threadIdList, 1, &g_setCursorPosHookHandle);
		if (clipCursorHook) LhSetExclusiveACL(threadIdList, 1, &g_clipCursorHookHandle);
        if (getKeyStateHook) LhSetExclusiveACL(threadIdList, 1, &g_getKeyStateHookHandle);
        if (getAsyncKeyStateHook) LhSetExclusiveACL(threadIdList, 1, &g_getAsyncKeyStateHookHandle);
        if (getKeyboardStateHook) LhSetExclusiveACL(threadIdList, 1, &g_getKeyboardStateHookHandle);
        if (setCursorHook) LhSetExclusiveACL(threadIdList, 1, &g_setCursorHookHandle);
        if (setRectHook) {
            LhSetExclusiveACL(threadIdList, 1, &g_setRectHookHandle);
            LhSetExclusiveACL(threadIdList, 1, &g_adjustWindowRectHookHandle);
        }

        LOG("All selected hooks are now enabled.");
    }

    void Hooks::RemoveHooks() {
        LhUninstallAllHooks();
        LOG("All hooks have been uninstalled.");
    }

    HCURSOR WINAPI Hooks::SetCursorHook(HCURSOR hcursor) {
        Mouse::hCursor = hcursor;
        return SetCursor(hcursor);
    }

    BOOL WINAPI Hooks::ClipCursorHook(const RECT* lpRect) {
        return TRUE;
    }

    BOOL WINAPI Hooks::SetRectHook(LPRECT lprc, int xLeft, int yTop, int xRight, int yBottom) {
        if (lprc) *lprc = { leftrect, toprect, rightrect, bottomrect };
        return TRUE;
    }

    BOOL WINAPI Hooks::AdjustWindowRectHook(LPRECT lprc, DWORD dwStyle, BOOL bMenu) {
        if (lprc) *lprc = { leftrect, toprect, rightrect, bottomrect };
        return TRUE;
    }

}