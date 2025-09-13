#pragma once
#include "pch.h"

namespace GtoMnK {

    class Keyboard {
    public:
        static int keystatesend;

        static SHORT WINAPI HookedGetAsyncKeyState(int vKey);
        static SHORT WINAPI HookedGetKeyState(int nVirtKey);


    private:
        static int samekey;
        static int samekeyA;
    };

}