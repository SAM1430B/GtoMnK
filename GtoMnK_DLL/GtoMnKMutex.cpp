#include "pch.h"
#include "GtoMnKMutex.h"
#include "Logging.h" 
#include <stdio.h>

namespace GtoMnK {

    HANDLE g_hMutex = nullptr;

    bool CreateSingleInstanceMutex() {
        char mutexName[256];
        snprintf(mutexName, sizeof(mutexName), "GtoMnK_Mutex_PID_%lu", GetCurrentProcessId());

        g_hMutex = CreateMutexA(NULL, FALSE, mutexName);

        if (GetLastError() == ERROR_ALREADY_EXISTS) {
            if (g_hMutex) {
                CloseHandle(g_hMutex);
                g_hMutex = nullptr;
            }
            LOG("CRITICAL: Another instance of GtoMnK is already injected into this process.");
            return false;
        }

        LOG("Mutex created successfully: %s", mutexName);
        return true;
    }

    void ReleaseSingleInstanceMutex() {
        if (g_hMutex) {
            CloseHandle(g_hMutex);
            g_hMutex = nullptr;
            LOG("Mutex released cleanly.");
        }
    }
}