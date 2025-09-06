#pragma once
#include "pch.h"

namespace ScreenshotInput {

    class Keyboard {
    public:
        static int keystatesend;

        // Hooked functions
        static SHORT WINAPI HookedGetAsyncKeyState(int vKey);
        static SHORT WINAPI HookedGetKeyState(int nVirtKey);

        // Input simulation
        static void PostKeyFunction(int keytype, bool press);

    private:
        static int samekey;
        static int samekeyA;
    };

}