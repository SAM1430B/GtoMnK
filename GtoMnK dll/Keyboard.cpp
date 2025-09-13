#include "pch.h"
#include "Keyboard.h"
#include "Hooks.h"

namespace GtoMnK {

    // Initialize static members
    int Keyboard::keystatesend = 0;
    int Keyboard::samekey = 0;
    int Keyboard::samekeyA = 0;

    SHORT WINAPI Keyboard::HookedGetAsyncKeyState(int vKey) {
        if (samekeyA == vKey) {
            return (short)0x8001;
        }
        samekeyA = 0;

        if (vKey == keystatesend) {
            samekeyA = vKey;
            return (short)0x8000;
        }
        return Hooks::fpGetAsyncKeyState(vKey);
    }

    SHORT WINAPI Keyboard::HookedGetKeyState(int nVirtKey) {
        if (samekey == nVirtKey) {
            return (short)0x8001;
        }
        samekey = 0;
        
        if (nVirtKey == keystatesend) {
            samekey = nVirtKey;
            return (short)0x8000;
        }
        return Hooks::fpGetKeyState(nVirtKey);
    }
}