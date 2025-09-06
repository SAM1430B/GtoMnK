#include "pch.h"
#include "Keyboard.h"
#include "Hooks.h"

// External global variables from dllmain.cpp
extern HWND hwnd;

namespace ScreenshotInput {

    // Initialize static members
    int Keyboard::keystatesend = 0;
    int Keyboard::samekey = 0;
    int Keyboard::samekeyA = 0;


    SHORT WINAPI Keyboard::HookedGetAsyncKeyState(int vKey) {
        if (samekeyA == vKey) {
            return 0x8001;
        }
        samekeyA = 0;

        if (vKey == keystatesend) {
            samekeyA = vKey;
            return 0x8000;
        }
        return Hooks::fpGetAsyncKeyState(vKey);
    }

    SHORT WINAPI Keyboard::HookedGetKeyState(int nVirtKey) {
        if (samekey == nVirtKey) {
            return 0x8001;
        }
        samekey = 0;

        if (nVirtKey == keystatesend) {
            samekey = nVirtKey;
            return 0x8000;
        }
        return Hooks::fpGetKeyState(nVirtKey);
    }

    void Keyboard::PostKeyFunction(int keytype, bool press) {
        if (!hwnd) return;

        DWORD mykey = 0;
        // Map keytype to a virtual key code
        if (keytype >= -4 && keytype <= 81) {
            // ... (insert the large if/else if block from the original PostKeyFunction here)
            // Example:
            if (keytype == -1) mykey = VK_UP;
            if (keytype == -2) mykey = VK_DOWN;
            if (keytype == -3) mykey = VK_LEFT;
            if (keytype == -4) mykey = VK_RIGHT;
            if (keytype == 3) mykey = VK_ESCAPE;
            if (keytype == 4) mykey = VK_RETURN;
            if (keytype == 5) mykey = VK_TAB;
            if (keytype == 6) mykey = VK_SHIFT;
            if (keytype == 7) mykey = VK_CONTROL;
            if (keytype == 8) mykey = VK_SPACE;
            if (keytype == 9) mykey = 0x4D; //M
            if (keytype == 10) mykey = 0x57; //W
            if (keytype == 11) mykey = 0x53; //S
            if (keytype == 12) mykey = 0x41; //A
            if (keytype == 13) mykey = 0x44; //D
            if (keytype == 14) mykey = 0x45; //E
            if (keytype == 15) mykey = 0x46; //F
            if (keytype == 16) mykey = 0x47; //G
            if (keytype == 17) mykey = 0x48; //H
            if (keytype == 18) mykey = 0x49; //I
            if (keytype == 19) mykey = 0x51; //Q
            if (keytype == 20) mykey = VK_OEM_PERIOD;
            if (keytype == 21) mykey = 0x52; //R
            if (keytype == 22) mykey = 0x54; //T
            if (keytype == 23) mykey = 0x42; //B
            if (keytype == 24) mykey = 0x43; //C
            if (keytype == 25) mykey = 0x4B; //K
            if (keytype == 26) mykey = 0x55; //U
            if (keytype == 27) mykey = 0x56; //V
            if (keytype == 28) mykey = 0x57; //W
            if (keytype == 30) mykey = 0x30; //0
            if (keytype == 31) mykey = 0x31; //1
            if (keytype == 32) mykey = 0x32; //2
            if (keytype == 33) mykey = 0x33; //3
            if (keytype == 34) mykey = 0x34; //4
            if (keytype == 35) mykey = 0x35; //5
            if (keytype == 36) mykey = 0x36; //6
            if (keytype == 37) mykey = 0x37; //7
            if (keytype == 38) mykey = 0x38; //8
            if (keytype == 39) mykey = 0x39; //9
            if (keytype == 40) mykey = VK_UP;
            if (keytype == 41) mykey = VK_DOWN;
            if (keytype == 42) mykey = VK_LEFT;
            if (keytype == 43) mykey = VK_RIGHT;
            if (keytype == 44) mykey = 0x58; //X
            if (keytype == 45) mykey = 0x5A; //Z
            if (keytype == 20) mykey = VK_OEM_PERIOD;
            if (keytype == 51) mykey = VK_F1;
            if (keytype == 52) mykey = VK_F2;
            if (keytype == 53) mykey = VK_F3;
            if (keytype == 54) mykey = VK_F4;
            if (keytype == 55) mykey = VK_F5;
            if (keytype == 56) mykey = VK_F6;
            if (keytype == 57) mykey = VK_F7;
            if (keytype == 58) mykey = VK_F8;
            if (keytype == 59) mykey = VK_F9;
            if (keytype == 60) mykey = VK_F10;
            if (keytype == 61) mykey = VK_F11;
            if (keytype == 62) mykey = VK_F12;
            if (keytype == 63) { mykey = VK_CONTROL;} //control+C
            if (keytype == 70) mykey = VK_NUMPAD0;
            if (keytype == 71) mykey = VK_NUMPAD1;
            if (keytype == 72) mykey = VK_NUMPAD2;
            if (keytype == 73) mykey = VK_NUMPAD3;
            if (keytype == 74) mykey = VK_NUMPAD4;
            if (keytype == 75) mykey = VK_NUMPAD5;
            if (keytype == 76) mykey = VK_NUMPAD6;
            if (keytype == 77) mykey = VK_NUMPAD7;
            if (keytype == 78) mykey = VK_NUMPAD8;
            if (keytype == 79) mykey = VK_NUMPAD9;
            if (keytype == 80) mykey = VK_SUBTRACT;
            if (keytype == 81) mykey = VK_ADD;
        }

        if (mykey == 0) return;

        keystatesend = mykey;
        UINT scanCode = MapVirtualKey(mykey, MAPVK_VK_TO_VSC);
        LPARAM lParam = (1 | (scanCode << 16));
        DWORD presskey = press ? WM_KEYDOWN : WM_KEYUP;

        if (!press) {
            lParam |= (1 << 30) | (1 << 31); // Add previous key state and transition state for WM_KEYUP
        }

        PostMessage(hwnd, presskey, mykey, lParam);
    }

}