#include "pch.h"
#include "Hooks.h"
#include "Logging.h"
#include "Mouse.h"
#include "Keyboard.h"

// External global variables from dllmain.cpp
extern int leftrect, toprect, rightrect, bottomrect;
extern int clipcursorhook, getkeystatehook, getasynckeystatehook, getcursorposhook, setcursorposhook, setcursorhook, setrecthook;

namespace GtoMnK {

    // Initialize static function pointers
    GetCursorPos_t Hooks::fpGetCursorPos = nullptr;
    SetCursorPos_t Hooks::fpSetCursorPos = nullptr;
    GetAsyncKeyState_t Hooks::fpGetAsyncKeyState = nullptr;
    GetKeyState_t Hooks::fpGetKeyState = nullptr;
    ClipCursor_t Hooks::fpClipCursor = nullptr;
    SetCursor_t Hooks::fpSetCursor = nullptr;
    SetRect_t Hooks::fpSetRect = nullptr;
    AdjustWindowRect_t Hooks::fpAdjustWindowRect = nullptr;

    void Hooks::SetupHooks() {
        if (MH_Initialize() != MH_OK) {
           LOG("Failed to initialize MinHook");
            return;
        }

       LOG("--- Setting up API hooks ---");

        if (getcursorposhook) {
            if (MH_CreateHook(&GetCursorPos, &Mouse::MyGetCursorPos, reinterpret_cast<LPVOID*>(&fpGetCursorPos)) == MH_OK) {
                MH_EnableHook(&GetCursorPos);
               LOG("GetCursorPos hook enabled.");
            }
        }
        if (setcursorposhook) {
            if (MH_CreateHook(&SetCursorPos, &Mouse::MySetCursorPos, reinterpret_cast<LPVOID*>(&fpSetCursorPos)) == MH_OK) {
                MH_EnableHook(&SetCursorPos);
               LOG("SetCursorPos hook enabled.");
            }
        }
        if (getasynckeystatehook) {
            if (MH_CreateHook(&GetAsyncKeyState, &Keyboard::HookedGetAsyncKeyState, reinterpret_cast<LPVOID*>(&fpGetAsyncKeyState)) == MH_OK) {
                MH_EnableHook(&GetAsyncKeyState);
               LOG("GetAsyncKeyState hook enabled.");
            }
        }
        if (getkeystatehook) {
            if (MH_CreateHook(&GetKeyState, &Keyboard::HookedGetKeyState, reinterpret_cast<LPVOID*>(&fpGetKeyState)) == MH_OK) {
                MH_EnableHook(&GetKeyState);
               LOG("GetKeyState hook enabled.");
            }
        }
        if (clipcursorhook) {
            if (MH_CreateHook(&ClipCursor, &HookedClipCursor, reinterpret_cast<LPVOID*>(&fpClipCursor)) == MH_OK) {
                MH_EnableHook(&ClipCursor);
               LOG("ClipCursor hook enabled.");
            }
        }
        if (setrecthook) {
            if (MH_CreateHook(&SetRect, &HookedSetRect, reinterpret_cast<LPVOID*>(&fpSetRect)) == MH_OK) {
                MH_EnableHook(&SetRect);
               LOG("SetRect hook enabled.");
            }
            if (MH_CreateHook(&AdjustWindowRect, &HookedAdjustWindowRect, reinterpret_cast<LPVOID*>(&fpAdjustWindowRect)) == MH_OK) {
                MH_EnableHook(&AdjustWindowRect);
               LOG("AdjustWindowRect hook enabled.");
            }
        }
        if (setcursorhook) {
            if (MH_CreateHook(&SetCursor, &HookedSetCursor, reinterpret_cast<LPVOID*>(&fpSetCursor)) == MH_OK) {
                MH_EnableHook(&SetCursor);
               LOG("SetCursor hook enabled.");
            }
        }
        LOG("--- Hook setup finished ---");
    }

    void Hooks::RemoveHooks() {
        MH_DisableHook(MH_ALL_HOOKS);
        MH_Uninitialize();
    }

    HCURSOR WINAPI Hooks::HookedSetCursor(HCURSOR hcursor) {
        Mouse::hCursor = hcursor;
        return fpSetCursor(hcursor);
    }

    BOOL WINAPI Hooks::HookedClipCursor(const RECT* lpRect) {
        return TRUE; // Ignore ClipCursor calls
    }

    BOOL WINAPI Hooks::HookedSetRect(LPRECT lprc, int xLeft, int yTop, int xRight, int yBottom) {
        return fpSetRect(lprc, leftrect, toprect, rightrect, bottomrect);
    }

    BOOL WINAPI Hooks::HookedAdjustWindowRect(LPRECT lprc, DWORD dwStyle, BOOL bMenu) {
        lprc->top = toprect;
        lprc->bottom = bottomrect;
        lprc->left = leftrect;
        lprc->right = rightrect;
        return fpAdjustWindowRect(lprc, dwStyle, bMenu);
    }

}