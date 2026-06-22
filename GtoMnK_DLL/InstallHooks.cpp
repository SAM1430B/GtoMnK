#include "pch.h"
#include "InstallHooks.h"
#include "Logging.h"
#include "FakeInput.h"
#include "INISettings.h"
#include "FakeMouse.h"
#include "FakeKeyboard.h"
#include "RawInput.h"
#include "RawInputHooks.h"
#include "CursorVisibilityHooks.h"
#include "MessageFilterHooks.h"
#include "XInput_GamepadHooks.h"
#include "SDL2_GamepadHooks.h"

// External global variables from dllmain.cpp
extern int leftrect, toprect, rightrect, bottomrect;
extern int getCursorPosHook, setCursorPosHook, clipCursorHook, getKeyStateHook, getAsyncKeyStateHook, getKeyboardStateHook, setRectHook;

extern GtoMnK::FakeInputMethod g_FakeInputMethod;
extern GamepadMethod g_GamepadMethod;
extern int drawProtoFakeCursor;
extern bool g_filterRawInput, g_filterMouseMove, g_filterMouseActivate, g_filterWindowActivate, g_filterWindowActivateApp, g_filterMouseWheel, g_filterMouseButton, g_filterKeyboardButton;

extern int gamepadMaskHook;

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

// Gamepad Mask Hooks
HOOK_TRACE_INFO g_xinputGetStateMaskHook = { NULL };
HOOK_TRACE_INFO g_xinputGetStateExMaskHook = { NULL };

HOOK_TRACE_INFO g_sdlPollEventMaskHook = { NULL };

HOOK_TRACE_INFO g_sdlGetButtonMaskHook = { NULL };
HOOK_TRACE_INFO g_sdlGetAxisMaskHook = { NULL };
HOOK_TRACE_INFO g_sdlGetNumTouchpadsMaskHook = { NULL };
HOOK_TRACE_INFO g_sdlGetTouchpadFingerMaskHook = { NULL };

namespace GtoMnK {

    void InstallHooks::SetupHooks() {
        LOG("Setting up hooks...");
        HMODULE hUser32 = GetModuleHandleA("user32");
        if (!hUser32) {
            LOG("FATAL: Could not get a handle to user32.dll! Hooks will not be installed.");
            return;
        }

        NTSTATUS result;

		// RawInput hooks
        if (g_FakeInputMethod == FakeInputMethod::RawInput || g_FakeInputMethod == FakeInputMethod::Hybrid) {
            LOG("Installing hooks for RawInput mode...");

            RawInputHooks::TrueGetRawInputData = (UINT(WINAPI*)(HRAWINPUT, UINT, LPVOID, PUINT, UINT))GetProcAddress(hUser32, "GetRawInputData");
            RawInputHooks::TrueRegisterRawInputDevices = (BOOL(WINAPI*)(PCRAWINPUTDEVICE, UINT, UINT))GetProcAddress(hUser32, "RegisterRawInputDevices");

			LOG("Installing GetRawInputDataHook...");
            result = LhInstallHook(GetProcAddress(hUser32, "GetRawInputData"), GtoMnK::RawInputHooks::GetRawInputDataHook, NULL, &g_getRawInputDataHook);
            if (FAILED(result)) LOG("Failed to install hook for GetRawInputData: %S", RtlGetLastErrorString());

			LOG("Installing RegisterRawInputDevicesHook...");
            result = LhInstallHook(GetProcAddress(hUser32, "RegisterRawInputDevices"), GtoMnK::RawInputHooks::RegisterRawInputDevicesHook, NULL, &g_registerRawInputDevicesHook);
            if (FAILED(result)) LOG("Failed to install hook for RegisterRawInputDevices: %S", RtlGetLastErrorString());
        }
        // From ProtoInput
        if (drawProtoFakeCursor) {
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
            result = LhInstallHook(GetProcAddress(hUser32, "GetCursorPos"), FakeMouse::GetCursorPosHook, NULL, &g_getCursorPosHookHandle);
            if (FAILED(result)) LOG("Failed to install hook for GetCursorPos: %S", RtlGetLastErrorString());
        }
        if (setCursorPosHook) {
			LOG("Installing SetCursorPosHook...");
            result = LhInstallHook(GetProcAddress(hUser32, "SetCursorPos"), FakeMouse::SetCursorPosHook, NULL, &g_setCursorPosHookHandle);
            if (FAILED(result)) LOG("Failed to install hook for SetCursorPos: %S", RtlGetLastErrorString());
        }
        if (clipCursorHook) {
			LOG("Installing ClipCursorHook...");
            result = LhInstallHook(GetProcAddress(hUser32, "ClipCursor"), FakeMouse::ClipCursorHook, NULL, &g_clipCursorHookHandle);
            if (FAILED(result)) LOG("Failed to install hook for ClipCursor: %S", RtlGetLastErrorString());
        }
        if (getKeyStateHook) {
			LOG("Installing GetKeyStateHook...");
            result = LhInstallHook(GetProcAddress(hUser32, "GetKeyState"), FakeKeyboard::GetKeyStateHook, NULL, &g_getKeyStateHookHandle);
            if (FAILED(result)) LOG("Failed to install hook for GetKeyState: %S", RtlGetLastErrorString());
        }
        if (getAsyncKeyStateHook) {
			LOG("Installing GetAsyncKeyStateHook...");
            result = LhInstallHook(GetProcAddress(hUser32, "GetAsyncKeyState"), FakeKeyboard::GetAsyncKeyStateHook, NULL, &g_getAsyncKeyStateHookHandle);
            if (FAILED(result)) LOG("Failed to install hook for GetAsyncKeyState: %S", RtlGetLastErrorString());
        }
        if (getKeyboardStateHook) {
			LOG("Installing GetKeyboardStateHook...");
            result = LhInstallHook(GetProcAddress(hUser32, "GetKeyboardState"), FakeKeyboard::GetKeyboardStateHook, NULL, &g_getKeyboardStateHookHandle);
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
		// Gamepad Mask Hooks
        if (gamepadMaskHook) {
            LOG("Gamepad Mask Hook is enabled");

            if (g_GamepadMethod == GamepadMethod::XInput) {
                LOG("Installing XInput Mask Hooks...");
                HMODULE hXInput = GetModuleHandleA("xinput1_4.dll");
                if (!hXInput) hXInput = GetModuleHandleA("xinput1_3.dll");
                if (!hXInput) hXInput = GetModuleHandleA("xinput9_1_0.dll");

                if (hXInput) {
                    GtoMnK::XInputHooks::TrueXInputGetState = (GtoMnK::XInputHooks::XInputGetState_t)GetProcAddress(hXInput, "XInputGetState");
                    if (GtoMnK::XInputHooks::TrueXInputGetState) {
                        result = LhInstallHook(GetProcAddress(hXInput, "XInputGetState"), GtoMnK::XInputHooks::MaskHook_XInputGetState, NULL, &g_xinputGetStateMaskHook);
                        if (FAILED(result)) LOG("Failed to hook XInputGetState: %S", RtlGetLastErrorString());
                    }

                    GtoMnK::XInputHooks::TrueXInputGetStateEx = (GtoMnK::XInputHooks::XInputGetState_t)GetProcAddress(hXInput, (LPCSTR)100);
                    if (GtoMnK::XInputHooks::TrueXInputGetStateEx) {
                        result = LhInstallHook(GetProcAddress(hXInput, (LPCSTR)100), GtoMnK::XInputHooks::MaskHook_XInputGetStateEx, NULL, &g_xinputGetStateExMaskHook);
                        if (FAILED(result)) LOG("Failed to hook XInputGetStateEx (Ordinal 100): %S", RtlGetLastErrorString());
                    }
                }
                else {
                    LOG("XInput module not found; skipped hooking.");
                }
            }
            else if (g_GamepadMethod == GamepadMethod::SDL2) {
                LOG("Installing SDL2 Mask Hooks...");

                HMODULE hSDL2 = GetModuleHandleA("SDL2.dll");
                if (!hSDL2) {
                    LOG("SDL2.dll not in memory yet. Forcing load...");
                    hSDL2 = LoadLibraryA("SDL2.dll");
                }

                if (hSDL2) {
                    GtoMnK::SDL2Hooks::TrueSDL_PollEvent = (GtoMnK::SDL2Hooks::SDL_PollEvent_t)GetProcAddress(hSDL2, "SDL_PollEvent");
                    if (GtoMnK::SDL2Hooks::TrueSDL_PollEvent) {
                        result = LhInstallHook(GetProcAddress(hSDL2, "SDL_PollEvent"), GtoMnK::SDL2Hooks::Hook_SDL_PollEvent, NULL, &g_sdlPollEventMaskHook);
                        if (FAILED(result)) LOG("Failed to hook SDL_PollEvent: %S", RtlGetLastErrorString());
                    }

                    GtoMnK::SDL2Hooks::TrueSDL_GameControllerGetButton = (GtoMnK::SDL2Hooks::SDL_GameControllerGetButton_t)GetProcAddress(hSDL2, "SDL_GameControllerGetButton");
                    if (GtoMnK::SDL2Hooks::TrueSDL_GameControllerGetButton) {
                        result = LhInstallHook(GetProcAddress(hSDL2, "SDL_GameControllerGetButton"), GtoMnK::SDL2Hooks::Hook_SDL_GameControllerGetButton, NULL, &g_sdlGetButtonMaskHook);
                        if (FAILED(result)) LOG("Failed to hook SDL_GameControllerGetButton: %S", RtlGetLastErrorString());
                    }

                    GtoMnK::SDL2Hooks::TrueSDL_GameControllerGetAxis = (GtoMnK::SDL2Hooks::SDL_GameControllerGetAxis_t)GetProcAddress(hSDL2, "SDL_GameControllerGetAxis");
                    if (GtoMnK::SDL2Hooks::TrueSDL_GameControllerGetAxis) {
                        result = LhInstallHook(GetProcAddress(hSDL2, "SDL_GameControllerGetAxis"), GtoMnK::SDL2Hooks::Hook_SDL_GameControllerGetAxis, NULL, &g_sdlGetAxisMaskHook);
                        if (FAILED(result)) LOG("Failed to hook SDL_GameControllerGetAxis: %S", RtlGetLastErrorString());
                    }

                    GtoMnK::SDL2Hooks::TrueSDL_GameControllerGetNumTouchpads = (GtoMnK::SDL2Hooks::SDL_GameControllerGetNumTouchpads_t)GetProcAddress(hSDL2, "SDL_GameControllerGetNumTouchpads");
                    if (GtoMnK::SDL2Hooks::TrueSDL_GameControllerGetNumTouchpads) {
                        result = LhInstallHook(GetProcAddress(hSDL2, "SDL_GameControllerGetNumTouchpads"), GtoMnK::SDL2Hooks::Hook_SDL_GameControllerGetNumTouchpads, NULL, &g_sdlGetNumTouchpadsMaskHook);
                        if (FAILED(result)) LOG("Failed to hook SDL_GameControllerGetNumTouchpads: %S", RtlGetLastErrorString());
                    }

                    GtoMnK::SDL2Hooks::TrueSDL_GameControllerGetTouchpadFinger = (GtoMnK::SDL2Hooks::SDL_GameControllerGetTouchpadFinger_t)GetProcAddress(hSDL2, "SDL_GameControllerGetTouchpadFinger");
                    if (GtoMnK::SDL2Hooks::TrueSDL_GameControllerGetTouchpadFinger) {
                        result = LhInstallHook(GetProcAddress(hSDL2, "SDL_GameControllerGetTouchpadFinger"), GtoMnK::SDL2Hooks::Hook_SDL_GameControllerGetTouchpadFinger, NULL, &g_sdlGetTouchpadFingerMaskHook);
                        if (FAILED(result)) LOG("Failed to hook SDL_GameControllerGetTouchpadFinger: %S", RtlGetLastErrorString());
                    }
                }
                else {
                    LOG("SDL2 module could not be found or loaded; skipped hooking.");
                }
            }
        }

        LOG("Activating installed hooks...");
        ULONG threadIdList[] = { 0 };

        if (g_FakeInputMethod == FakeInputMethod::RawInput || g_FakeInputMethod == FakeInputMethod::Hybrid) {
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
        if (drawProtoFakeCursor) {
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
        if (gamepadMaskHook) {
            if (g_GamepadMethod == GamepadMethod::XInput) {
                 if (g_xinputGetStateMaskHook.Link) LhSetExclusiveACL(threadIdList, 1, &g_xinputGetStateMaskHook);
                if (g_xinputGetStateExMaskHook.Link) LhSetExclusiveACL(threadIdList, 1, &g_xinputGetStateExMaskHook);
            }
            else if (g_GamepadMethod == GamepadMethod::SDL2) {
                if (g_sdlPollEventMaskHook.Link) LhSetExclusiveACL(threadIdList, 1, &g_sdlPollEventMaskHook);

                if (g_sdlGetButtonMaskHook.Link) LhSetExclusiveACL(threadIdList, 1, &g_sdlGetButtonMaskHook);
                if (g_sdlGetAxisMaskHook.Link) LhSetExclusiveACL(threadIdList, 1, &g_sdlGetAxisMaskHook);
                if (g_sdlGetNumTouchpadsMaskHook.Link) LhSetExclusiveACL(threadIdList, 1, &g_sdlGetNumTouchpadsMaskHook);
                if (g_sdlGetTouchpadFingerMaskHook.Link) LhSetExclusiveACL(threadIdList, 1, &g_sdlGetTouchpadFingerMaskHook);
            }
		}
        LOG("All selected hooks are now enabled.");
    }

    void InstallHooks::RemoveHooks() {
		LOG("Removing hooks...");
        LhUninstallAllHooks();
    }

    BOOL WINAPI InstallHooks::SetRectHook(LPRECT lprc, int xLeft, int yTop, int xRight, int yBottom) {
        if (lprc) *lprc = { leftrect, toprect, rightrect, bottomrect };
        return TRUE;
    }

    BOOL WINAPI InstallHooks::AdjustWindowRectHook(LPRECT lprc, DWORD dwStyle, BOOL bMenu) {
        if (lprc) *lprc = { leftrect, toprect, rightrect, bottomrect };
        return TRUE;
    }

}