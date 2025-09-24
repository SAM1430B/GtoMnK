#pragma once
#include <windows.h>
#include <vector>

namespace GtoMnK {
    namespace RawInput {

		// This is the main initialization function to start the RawInput method.
        void Initialize();

        void Shutdown();

        void InjectFakeRawInput(const RAWINPUT& fakeInput);

        const int RAWINPUT_BUFFER_SIZE = 20;
        extern RAWINPUT g_inputBuffer[RAWINPUT_BUFFER_SIZE];

        extern std::vector<HWND> g_forwardingWindows;
        extern HWND g_rawInputHwnd;

    }
}