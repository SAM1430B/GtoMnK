#include "pch.h"
#include "HandleMainWindow.h"
#include "FakeCursor.h" 
#include "RawInput.h"
#include "OverlayMenu.h"
#include "Logging.h"

HWND GetMainWindowHandle(DWORD targetPID, const char* requiredName, const char* requiredClass, DWORD timeoutMS) {
    struct HandleData {
        DWORD pid;
        HWND bestHandle;
        const char* reqName;
        const char* reqClass;
    };

    ULONGLONG startTime = GetTickCount64();

    while (true) {
        HandleData data = { targetPID, nullptr, requiredName, requiredClass };

        auto EnumWindowsCallback = [](HWND hWnd, LPARAM lParam) -> BOOL {
            HandleData* pData = reinterpret_cast<HandleData*>(lParam);
            DWORD windowPID = 0;
            GetWindowThreadProcessId(hWnd, &windowPID);

            if (windowPID != pData->pid) return TRUE;

            if (!IsWindowVisible(hWnd)) return TRUE;
            if (hWnd == GtoMnK::FakeCursor::GetPointerWindow()) return TRUE;
            if (hWnd == GtoMnK::RawInput::g_rawInputHwnd) return TRUE;
			if (hWnd == GtoMnK::OverlayMenu::state.menuWindow) return TRUE;

            bool usingStrictFilters = (pData->reqName && pData->reqName[0]) ||
                (pData->reqClass && pData->reqClass[0]);

            if (usingStrictFilters) {

                if (pData->reqClass && pData->reqClass[0] != '\0') {
                    char className[256];
                    GetClassNameA(hWnd, className, sizeof(className));
                    if (strcmp(className, pData->reqClass) != 0) return TRUE;
                }

                if (pData->reqName && pData->reqName[0] != '\0') {
                    char windowName[256];
                    GetWindowTextA(hWnd, windowName, sizeof(windowName));
                    if (strcmp(windowName, pData->reqName) != 0) return TRUE;
                }
            }
            else {
                if (GetWindowLongPtr(hWnd, GWL_EXSTYLE) & WS_EX_TOOLWINDOW) return TRUE;
                if (GetWindow(hWnd, GW_OWNER) != (HWND)0) return TRUE;
            }

            pData->bestHandle = hWnd;
            return FALSE;
            };

        EnumWindows(EnumWindowsCallback, reinterpret_cast<LPARAM>(&data));

        if (data.bestHandle != nullptr) {
            return data.bestHandle;
        }

        if ((GetTickCount64() - startTime) >= timeoutMS) {
            break;
        }
        Sleep(500);
    }

    return nullptr;
}

bool RecoverMainWindow(HWND& hwnd, bool recheckHWND, const char* windowName, const char* className) {
    if (recheckHWND) {
        if (hwnd && !IsWindow(hwnd)) {
            LOG("Window handle became invalid. Resetting...");
            hwnd = nullptr;
        }
    }

    if (!hwnd) {
        hwnd = GetMainWindowHandle(GetCurrentProcessId(), windowName, className, 300000);
        if (!hwnd) {
            LOG("CRITICAL: Game window missing for 5 minute.");
            return false;
        }
        LOG("Acquired new window handle: 0x%p", hwnd);

        /*if (g_InputMethod == InputMethod::RawInput || g_InputMethod == InputMethod::Hybrid) {
            LOG("Window changed! Re-running RawInput recovery...");
            RawInput::RecoverMissedRegistration();
        }*/
    }

    return true;
}