#include "pch.h"
#include "Hooks.h"
#include "Logging.h"
#include "Input.h"
#include "Mouse.h"
#include "Keyboard.h"
#include "RawInput.h"
#include "RawInputHooks.h"
#include "CursorVisibilityHooks.h"
#include "MessageFilterHooks.h"

// External global variables from dllmain.cpp
extern int leftrect, toprect, rightrect, bottomrect;
extern int getCursorPosHook, setCursorPosHook, clipCursorHook, getKeyStateHook, getAsyncKeyStateHook, getKeyboardStateHook, setRectHook;

extern GtoMnK::InputMethod g_InputMethod;
extern int drawProtoFakeCursor;
extern bool g_filterRawInput, g_filterMouseMove, g_filterMouseActivate, g_filterWindowActivate, g_filterWindowActivateApp, g_filterMouseWheel, g_filterMouseButton, g_filterKeyboardButton;

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
HOOK_TRACE_INFO g_setRectHookHandle = { NULL };
HOOK_TRACE_INFO g_adjustWindowRectHookHandle = { NULL };

// From ProtoInput
HOOK_TRACE_INFO g_HookShowCursorHandle = { NULL };
HOOK_TRACE_INFO g_HookSetCursorHandle = { NULL };
HOOK_TRACE_INFO g_HookSetSystemCursorHandle = { NULL };

HOOK_TRACE_INFO g_HookGetMessageAHandle = { NULL };
HOOK_TRACE_INFO g_HookGetMessageWHandle = { NULL };
HOOK_TRACE_INFO g_HookPeekMessageAHandle = { NULL };
HOOK_TRACE_INFO g_HookPeekMessageWHandle = { NULL };

namespace GtoMnK {

    void Hooks::SetupHooks() {
        LOG("Setting up hooks...");
        HMODULE hUser32 = GetModuleHandleA("user32");
        if (!hUser32) {
            LOG("FATAL: Could not get a handle to user32.dll! Hooks will not be installed.");
            return;
        }

        NTSTATUS result;

		// RawInput hooks
        if (g_InputMethod == InputMethod::RawInput) {
            LOG("Installing hooks for RawInput mode...");

			LOG("Installing GetRawInputDataHook...");
            result = LhInstallHook(GetProcAddress(hUser32, "GetRawInputData"), GtoMnK::RawInputHooks::GetRawInputDataHook, NULL, &g_getRawInputDataHook);
            if (FAILED(result)) LOG("Failed to install hook for GetRawInputData: %S", RtlGetLastErrorString());

			LOG("Installing RegisterRawInputDevicesHook...");
            result = LhInstallHook(GetProcAddress(hUser32, "RegisterRawInputDevices"), GtoMnK::RawInputHooks::RegisterRawInputDevicesHook, NULL, &g_registerRawInputDevicesHook);
            if (FAILED(result)) LOG("Failed to install hook for RegisterRawInputDevices: %S", RtlGetLastErrorString());
        }
        // From ProtoInput
        if (drawProtoFakeCursor == 1) {
            LOG("Installing hooks for ProtoInput Fake Cursor...");

			LOG("Installing Hook_ShowCursor...");
            result = LhInstallHook(GetProcAddress(hUser32, "ShowCursor"), GtoMnK::CursorVisibilityHook::Hook_ShowCursor, NULL, &g_HookShowCursorHandle);
            if (FAILED(result)) LOG("Failed to install hook for ShowCursor: %S", RtlGetLastErrorString());

			LOG("Installing Hook_SetCursor...");
            result = LhInstallHook(GetProcAddress(hUser32, "SetCursor"), GtoMnK::CursorVisibilityHook::Hook_SetCursor, NULL, &g_HookSetCursorHandle);
            if (FAILED(result)) LOG("Failed to install hook for SetCursor: %S", RtlGetLastErrorString());

			LOG("Installing Hook_SetSystemCursor...");
            result = LhInstallHook(GetProcAddress(hUser32, "SetSystemCursor"), GtoMnK::CursorVisibilityHook::Hook_SetSystemCursor, NULL, &g_HookSetSystemCursorHandle);
            if (FAILED(result)) LOG("Failed to install hook for SetSystemCursor: %S", RtlGetLastErrorString());
		}
        if (g_filterRawInput || g_filterMouseMove || g_filterMouseActivate || g_filterWindowActivate || g_filterWindowActivateApp || g_filterMouseWheel || g_filterMouseButton || g_filterKeyboardButton) {
            LOG("Installing hooks for Message Filtering...");

			LOG("Installing Hook_GetMessageA");
            result = LhInstallHook(GetProcAddress(hUser32, "GetMessageA"), GtoMnK::MessageFilterHook::Hook_GetMessageA, NULL, &g_HookGetMessageAHandle);
            if (FAILED(result)) LOG("Failed to install hook for GetMessageA: %S", RtlGetLastErrorString());

			LOG("Installing Hook_GetMessageW");
            result = LhInstallHook(GetProcAddress(hUser32, "GetMessageW"), GtoMnK::MessageFilterHook::Hook_GetMessageW, NULL, &g_HookGetMessageWHandle);
            if (FAILED(result)) LOG("Failed to install hook for GetMessageW: %S", RtlGetLastErrorString());

			LOG("Installing Hook_PeekMessageA");
            result = LhInstallHook(GetProcAddress(hUser32, "PeekMessageA"), GtoMnK::MessageFilterHook::Hook_PeekMessageA, NULL, &g_HookPeekMessageAHandle);
            if (FAILED(result)) LOG("Failed to install hook for PeekMessageA: %S", RtlGetLastErrorString());

			LOG("Installing Hook_PeekMessageW");
            result = LhInstallHook(GetProcAddress(hUser32, "PeekMessageW"), GtoMnK::MessageFilterHook::Hook_PeekMessageW, NULL, &g_HookPeekMessageWHandle);
            if (FAILED(result)) LOG("Failed to install hook for PeekMessageW: %S", RtlGetLastErrorString());
		}
		// General hooks
        if (getCursorPosHook) {
			LOG("Installing GetCursorPosHook...");
            result = LhInstallHook(GetProcAddress(hUser32, "GetCursorPos"), Mouse::GetCursorPosHook, NULL, &g_getCursorPosHookHandle);
            if (FAILED(result)) LOG("Failed to install hook for GetCursorPos: %S", RtlGetLastErrorString());
        }
        if (setCursorPosHook) {
			LOG("Installing SetCursorPosHook...");
            result = LhInstallHook(GetProcAddress(hUser32, "SetCursorPos"), Mouse::SetCursorPosHook, NULL, &g_setCursorPosHookHandle);
            if (FAILED(result)) LOG("Failed to install hook for SetCursorPos: %S", RtlGetLastErrorString());
        }
        if (clipCursorHook) {
			LOG("Installing ClipCursorHook...");
            result = LhInstallHook(GetProcAddress(hUser32, "ClipCursor"), ClipCursorHook, NULL, &g_clipCursorHookHandle);
            if (FAILED(result)) LOG("Failed to install hook for ClipCursor: %S", RtlGetLastErrorString());
        }
        if (getKeyStateHook) {
			LOG("Installing GetKeyStateHook...");
            result = LhInstallHook(GetProcAddress(hUser32, "GetKeyState"), Keyboard::GetKeyStateHook, NULL, &g_getKeyStateHookHandle);
            if (FAILED(result)) LOG("Failed to install hook for GetKeyState: %S", RtlGetLastErrorString());
        }
        if (getAsyncKeyStateHook) {
			LOG("Installing GetAsyncKeyStateHook...");
            result = LhInstallHook(GetProcAddress(hUser32, "GetAsyncKeyState"), Keyboard::GetAsyncKeyStateHook, NULL, &g_getAsyncKeyStateHookHandle);
            if (FAILED(result)) LOG("Failed to install hook for GetAsyncKeyState: %S", RtlGetLastErrorString());
        }
        if (getKeyboardStateHook) {
			LOG("Installing GetKeyboardStateHook...");
            result = LhInstallHook(GetProcAddress(hUser32, "GetKeyboardState"), Keyboard::GetKeyboardStateHook, NULL, &g_getKeyboardStateHookHandle);
            if (FAILED(result)) LOG("Failed to install hook for GetKeyboardState: %S", RtlGetLastErrorString());
        }
        if (setRectHook) {
			LOG("Installing SetRectHook...");
            result = LhInstallHook(GetProcAddress(hUser32, "SetRect"), SetRectHook, NULL, &g_setRectHookHandle);
            if (FAILED(result)) LOG("Failed to install hook for SetRect: %S", RtlGetLastErrorString());

			LOG("Installing AdjustWindowRectHook...");
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
        if (setRectHook) {
            LhSetExclusiveACL(threadIdList, 1, &g_setRectHookHandle);
            LhSetExclusiveACL(threadIdList, 1, &g_adjustWindowRectHookHandle);
        }
        if (drawProtoFakeCursor == 1) {
            LhSetExclusiveACL(threadIdList, 1, &g_HookShowCursorHandle);
            LhSetExclusiveACL(threadIdList, 1, &g_HookSetCursorHandle);
            LhSetExclusiveACL(threadIdList, 1, &g_HookSetSystemCursorHandle);
        }
        if (g_filterRawInput || g_filterMouseMove || g_filterMouseActivate || g_filterWindowActivate || g_filterWindowActivateApp || g_filterMouseWheel || g_filterMouseButton || g_filterKeyboardButton) {
            LhSetExclusiveACL(threadIdList, 1, &g_HookGetMessageAHandle);
            LhSetExclusiveACL(threadIdList, 1, &g_HookGetMessageWHandle);
            LhSetExclusiveACL(threadIdList, 1, &g_HookPeekMessageAHandle);
            LhSetExclusiveACL(threadIdList, 1, &g_HookPeekMessageWHandle);
        }
        LOG("All selected hooks are now enabled.");
    }

    void Hooks::RemoveHooks() {
		LOG("Removing hooks...");
        LhUninstallAllHooks();
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