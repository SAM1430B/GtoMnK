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

volatile bool g_isInitialized = false;
bool g_isLoadedByDll = false;

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
				g_isLoadedByDll = true;
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
        loop = false;

        if (lpReserved == nullptr) {
            LOG("DLL Detaching gracefully (FreeLibrary called).");
            Sleep(50);

            if (g_isLoadedByDll && hooksinited) {
                LOG("Running as master. Performing full cleanup...");
                Hooks::RemoveHooks();
                LOG("Hooks removed.");
                if (g_InputMethod == InputMethod::RawInput) {
                    RawInput::Shutdown();
                    LOG("RawInput system shut down.");
                }
            }
        }
        else {
            LOG("DLL detaching due to process termination. Skipping explicit cleanup.");
        }

        SHUTDOWN_LOGGER();
        break;
    }
    }
    return TRUE;
}