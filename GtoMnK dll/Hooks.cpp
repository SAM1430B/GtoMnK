#include "pch.h"
#include "Hooks.h"
#include "Logging.h"
#include "Mouse.h"
#include "Keyboard.h"

// External global variables from dllmain.cpp
extern int leftrect, toprect, rightrect, bottomrect;
extern int clipcursorhook, getkeystatehook, getasynckeystatehook, getcursorposhook, setcursorposhook, setcursorhook, setrecthook;

HOOK_TRACE_INFO g_getCursorPosHookHandle = { NULL };
HOOK_TRACE_INFO g_setCursorPosHookHandle = { NULL };
HOOK_TRACE_INFO g_getAsyncKeyStateHookHandle = { NULL };
HOOK_TRACE_INFO g_getKeyStateHookHandle = { NULL };
HOOK_TRACE_INFO g_clipCursorHookHandle = { NULL };
HOOK_TRACE_INFO g_setCursorHookHandle = { NULL };
HOOK_TRACE_INFO g_setRectHookHandle = { NULL };
HOOK_TRACE_INFO g_adjustWindowRectHookHandle = { NULL };

namespace GtoMnK {

    void Hooks::SetupHooks() {
        LOG("--- Setting up the hooks ---");
        HMODULE hUser32 = GetModuleHandleA("user32");
        if (!hUser32) {
            LOG("FATAL: Could not get a handle to user32.dll! Hooks will not be installed.");
            return;
        }

        NTSTATUS result;
        
        if (getcursorposhook) {
            result = LhInstallHook(
                GetProcAddress(GetModuleHandle("user32"), "GetCursorPos"),
                Mouse::MyGetCursorPos,
                NULL,
                &g_getCursorPosHookHandle
            );
            if (FAILED(result)) {
                LOG("Failed to install hook for GetCursorPos: %S", RtlGetLastErrorString());
            }
            else {
                LOG("GetCursorPos hook installed successfully.");
            }
        }

        if (setcursorposhook) {
            result = LhInstallHook(
                GetProcAddress(GetModuleHandle("user32"), "SetCursorPos"),
                Mouse::MySetCursorPos,
                NULL,
                &g_setCursorPosHookHandle
            );
            if (FAILED(result)) {
                LOG("Failed to install hook for SetCursorPos: %S", RtlGetLastErrorString());
            }
            else {
                LOG("SetCursorPos hook installed successfully.");
            }
		}

        if (getasynckeystatehook) {
            result = LhInstallHook(
                GetProcAddress(GetModuleHandle("user32"), "GetAsyncKeyState"),
                Keyboard::HookedGetAsyncKeyState,
                NULL,
                &g_getAsyncKeyStateHookHandle
            );
            if (FAILED(result)) {
                LOG("Failed to install hook for GetAsyncKeyState: %S", RtlGetLastErrorString());
            }
            else {
                LOG("GetAsyncKeyState hook installed successfully.");
            }
        }

        if (getkeystatehook) {
            result = LhInstallHook(
                GetProcAddress(GetModuleHandle("user32"), "GetKeyState"),
                Keyboard::HookedGetKeyState,
                NULL,
                &g_getKeyStateHookHandle
            );
            if (FAILED(result)) {
                LOG("Failed to install hook for GetKeyState: %S", RtlGetLastErrorString());
            }
            else {
                LOG("GetKeyState hook installed successfully.");
            }
		}

        if (clipcursorhook) {
            result = LhInstallHook(
                GetProcAddress(GetModuleHandle("user32"), "ClipCursor"),
                HookedClipCursor,
                NULL,
                &g_clipCursorHookHandle
            );
            if (FAILED(result)) {
                LOG("Failed to install hook for ClipCursor: %S", RtlGetLastErrorString());
            }
            else {
                LOG("ClipCursor hook installed successfully.");
            }
		}

        if (setrecthook) {
            result = LhInstallHook(
                GetProcAddress(GetModuleHandle("user32"), "SetRect"),
                HookedSetRect,
                NULL,
				&g_setRectHookHandle
            );
            if (FAILED(result)) {
                LOG("Failed to install hook for SetRect: %S", RtlGetLastErrorString());
            }
            else {
                LOG("SetRect hook installed successfully.");
            }

            result = LhInstallHook(
                GetProcAddress(GetModuleHandle("user32"), "AdjustWindowRect"),
                HookedAdjustWindowRect,
				NULL,
                &g_adjustWindowRectHookHandle
			);
            if (FAILED(result)) {
                LOG("Failed to install hook for AdjustWindowRect: %S", RtlGetLastErrorString());
            }
            else {
                LOG("AdjustWindowRect hook installed successfully.");
			}
        }

        if (setcursorhook) {
            result = LhInstallHook(
                GetProcAddress(GetModuleHandle("user32"), "SetCursor"),
                HookedSetCursor,
                NULL,
                &g_setCursorHookHandle
            );
            if (FAILED(result)) {
                LOG("Failed to install hook for SetCursor: %S", RtlGetLastErrorString());
            }
            else {
                LOG("SetCursor hook installed successfully.");
            }
		}
        ULONG threadIdList = 0;
        LhSetExclusiveACL(&threadIdList, 0, &g_getCursorPosHookHandle);
        LhSetExclusiveACL(&threadIdList, 0, &g_setCursorPosHookHandle);
        LhSetExclusiveACL(&threadIdList, 0, &g_getAsyncKeyStateHookHandle);
        LhSetExclusiveACL(&threadIdList, 0, &g_getKeyStateHookHandle);
		LhSetExclusiveACL(&threadIdList, 0, &g_clipCursorHookHandle);
		LhSetExclusiveACL(&threadIdList, 0, &g_setCursorHookHandle);
		LhSetExclusiveACL(&threadIdList, 0, &g_setRectHookHandle);
		LhSetExclusiveACL(&threadIdList, 0, &g_adjustWindowRectHookHandle);
        LOG("--- All hooks enabled for current process ---");
    }

    void Hooks::RemoveHooks() {
        LhUninstallAllHooks();
        LOG("All hooks have been uninstalled.");
    }

    HCURSOR WINAPI Hooks::HookedSetCursor(HCURSOR hcursor) {
        Mouse::hCursor = hcursor;
        return SetCursor(hcursor);
    }

    BOOL WINAPI Hooks::HookedClipCursor(const RECT* lpRect) {
        return TRUE;
    }

    BOOL WINAPI Hooks::HookedSetRect(LPRECT lprc, int xLeft, int yTop, int xRight, int yBottom) {
        *lprc = { leftrect, toprect, rightrect, bottomrect };
        return TRUE;
    }

    BOOL WINAPI Hooks::HookedAdjustWindowRect(LPRECT lprc, DWORD dwStyle, BOOL bMenu) {
        *lprc = { leftrect, toprect, rightrect, bottomrect };
        return TRUE;
    }

}