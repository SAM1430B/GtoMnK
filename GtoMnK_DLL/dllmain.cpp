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

HMODULE g_hModule = nullptr;

// Entry point for EasyHook DLL injection
extern "C" __declspec(dllexport) void __stdcall NativeInjectionEntryPoint(REMOTE_ENTRY_INFO* inRemoteInfo)
{
    LOG("EasyHook NativeInjectionEntryPoint called");

    RhWakeUpProcess();
}

EASYHOOK_BOOL_EXPORT EasyHookDllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved);

BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved) {

    EasyHookDllMain(hModule, ul_reason_for_call, lpReserved);

    switch (ul_reason_for_call) {
    case DLL_PROCESS_ATTACH: {
        g_hModule = hModule;
        DisableThreadLibraryCalls(hModule);

        INIT_LOGGER();
        LOG("================== DLL Attached (PID: %lu) ==================", GetCurrentProcessId());

        HANDLE hThread = CreateThread(nullptr, 0, ThreadFunction, NULL, 0, NULL);
        if (hThread) {
            CloseHandle(hThread);
        }
        break;
    }

    case DLL_PROCESS_DETACH: {
        loop = false;

        if (lpReserved == nullptr) {
            LOG("DLL Detaching gracefully (FreeLibrary called).");
            Sleep(50);

            if (hooksinited) {
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