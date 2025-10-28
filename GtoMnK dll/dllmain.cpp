#include "pch.h"
#include "Logging.h"
#include "Hooks.h"
#include "Mouse.h"
#include "Keyboard.h"
#include "Input.h"
#include "InputState.h"
#include <map>
#include "RawInput.h"
#include <easyhook.h>
#include "MainThread.h"


using namespace GtoMnK;

// Entry point for EasyHook DLL injection
extern "C" void __declspec(dllexport) __stdcall NativeInjectionEntryPoint(REMOTE_ENTRY_INFO* inRemoteInfo) {
    if (g_isInitialized) {
        RhWakeUpProcess();
        return;
    }

    g_isInitialized = true;

    LOG("Initialization started via NativeInjectionEntryPoint (EasyHook).");

    HANDLE hThread = CreateThread(nullptr, 0, ThreadFunction, NULL, 0, NULL);
    if (hThread) {
        CloseHandle(hThread);
    }

    RhWakeUpProcess();
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved) {
    switch (ul_reason_for_call) {
    case DLL_PROCESS_ATTACH: {
        g_hModule = hModule;
        DisableThreadLibraryCalls(hModule);

        INIT_LOGGER();
        LOG("================== DLL Attached (PID: %lu) ==================", GetCurrentProcessId());

		// Create a thread if not using EasyHook injection
        CreateThread(nullptr, 0, [](LPVOID) -> DWORD {
            Sleep(100);
            if (!g_isInitialized) {
                g_isInitialized = true;
                LOG("Initialization started via DllMain.");

                HANDLE hThread = CreateThread(nullptr, 0, ThreadFunction, NULL, 0, NULL);
                if (hThread) {
                    CloseHandle(hThread);
                }
            }
            return 0;
            }, NULL, 0, NULL);
        break;
    }

    case DLL_PROCESS_DETACH: {
        if (lpReserved == nullptr) {
            LOG("DLL Detaching gracefully.");
            loop = false;
            Sleep(50);
        }
        else {
            LOG("DLL detaching due to process termination.");
        }

        if (hooksinited) {
            Hooks::RemoveHooks();
            LOG("Hooks removed.");
            if (g_InputMethod == InputMethod::RawInput) {
                RawInput::Shutdown();
                LOG("RawInput system shut down.");
            }
        }
        SHUTDOWN_LOGGER();
        break;
    }
    }
    return TRUE;
}