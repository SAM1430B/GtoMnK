#include "pch.h"
#include "RawInput.h"
#include "RawInputHooks.h"
#include "Logging.h"

namespace GtoMnK {
    namespace RawInput {

        RAWINPUT g_inputBuffer[RAWINPUT_BUFFER_SIZE]{};
        std::vector<HWND> g_forwardingWindows{};
        HWND g_rawInputHwnd = nullptr;

        LRESULT WINAPI RawInputWindowWndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
        DWORD WINAPI RawInputWindowThread(LPVOID lpParameter);
        void ProcessRealRawInput(HRAWINPUT rawInputHandle);

        void Initialize() {
            LOG("RawInput System: Initializing...");

            HANDLE hWindowReadyEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
            if (hWindowReadyEvent == NULL) {
                LOG("FATAL: Could not create window ready event!");
                return;
            }

            HANDLE hThread = CreateThread(nullptr, 0, RawInputWindowThread, hWindowReadyEvent, 0, 0);
            if (hThread) {
                WaitForSingleObject(hWindowReadyEvent, 2000);
                CloseHandle(hThread);
            }
            CloseHandle(hWindowReadyEvent);
        }

        void Shutdown() {
            LOG("RawInput System: Shutting down...");

            if (g_rawInputHwnd) {
                PostMessage(g_rawInputHwnd, WM_QUIT, 0, 0);
            }
        }

        void InjectFakeRawInput(const RAWINPUT& fakeInput) {
            static size_t bufferCounter = 0;
            bufferCounter = (bufferCounter + 1) % RAWINPUT_BUFFER_SIZE;
            g_inputBuffer[bufferCounter] = fakeInput;

            const LPARAM magicLParam = (bufferCounter) | 0xAB000000;

            for (const auto& hwnd : g_forwardingWindows) {
                PostMessageW(hwnd, WM_INPUT, 0, magicLParam);
            }
        }

        void ProcessRealRawInput(HRAWINPUT rawInputHandle) {
            // We could add logic to block the real mouse/keyboard here.
        }

        LRESULT WINAPI RawInputWindowWndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
            switch (msg) {
            case WM_INPUT: {
                ProcessRealRawInput((HRAWINPUT)lParam);
                break;
            }
            case WM_DESTROY: {
                PostQuitMessage(0);
                return 0;
            }
            }
            return DefWindowProc(hWnd, msg, wParam, lParam);
        }

        DWORD WINAPI RawInputWindowThread(LPVOID lpParameter) {
            HANDLE hWindowReadyEvent = (HANDLE)lpParameter;

            LOG("RawInput hidden window thread started.");
            WNDCLASSW wc = { 0 };
            wc.lpfnWndProc = RawInputWindowWndProc;
            wc.hInstance = GetModuleHandle(NULL);
            wc.lpszClassName = L"GtoMnK_RawInput_Window";

            if (RegisterClassW(&wc)) {
                g_rawInputHwnd = CreateWindowW(wc.lpszClassName, L"GtoMnK RI Sink", 0, 0, 0, 0, 0, NULL, NULL, wc.hInstance, NULL);
            }

            if (!g_rawInputHwnd) {
                LOG("FATAL: Failed to create RawInput hidden window!");
                SetEvent(hWindowReadyEvent);
                return 1;
            }

            SetEvent(hWindowReadyEvent);
            LOG("RawInput hidden window created and ready.");

            MSG msg;
            while (GetMessage(&msg, NULL, 0, 0) > 0) {
                TranslateMessage(&msg);
                DispatchMessage(&msg);
            }

            LOG("RawInput hidden window thread finished.");
            return 0;
        }

    }
}