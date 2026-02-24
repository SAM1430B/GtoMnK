#pragma once
#include <WinUser.h>
#include "MessageFilterBase.h"
#include "ProtoMessageFilterIDs.h"
#include "KeyboardState.h"
#include "Mouse.h"
#include "Input.h"

namespace GtoMnK
{
#define GtoMnK_MESSAGE_FILTERS \
RawInputFilter, \
MouseMoveFilter, \
MouseActivateFilter, \
WindowActivateFilter, \
WindowActivateAppFilter, \
MouseWheelFilter, \
MouseButtonFilter, \
KeyboardButtonFilter

class RawInputFilter : public GtoMnK::MessageFilterBase<RawInputFilterID, WM_INPUT> {
public:
    static bool Filter(unsigned int message, unsigned int* lparam, unsigned int* wparam, intptr_t hwnd) {
        // GtoMnK RawInput Signature
        if ((*lparam & 0xFF000000) == 0xAB000000) {
            return true;
        }

        // Allow SDL/DI Controller Input
        char className[256];
        if (GetClassNameA((HWND)hwnd, className, sizeof(className))) {
		    // SDL2 window classes and DirectInput helper classes. We want to allow RawInput messages to these windows so that SDL2 and DirectInput can function properly alongside GtoMnK's RawInput.
		    if (strcmp(className, "Message") == 0 || // SDL2's RawInput window class.
                strcmp(className, "SDL_HIDAPI_DEVICE_DETECTION") == 0 ||
                strcmp(className, "SDL_app") == 0 ||
                strcmp(className, "SDLHelperWindowInputCatcher") == 0) {

                return true;
            }
        }
        return false;
    }
};

class MouseMoveFilter : public GtoMnK::MessageFilterBase<MouseMoveFilterID, WM_MOUSEMOVE> {
public:
    static bool Filter(unsigned int message, unsigned int* lparam, unsigned int* wparam, intptr_t hwnd) {
		*lparam = MAKELPARAM(Mouse::Xf, Mouse::Yf); // Use GtoMnK's mouse position.
        return true;
    }
};

class MouseActivateFilter : public GtoMnK::MessageFilterBase<MouseActivateFilterID, WM_MOUSEACTIVATE> {
public:
    static bool Filter(unsigned int message, unsigned int* lparam, unsigned int* wparam, intptr_t hwnd) {
        return MA_ACTIVATE;
    }
};

class WindowActivateFilter : public GtoMnK::MessageFilterBase<WindowActivateFilterID, WM_ACTIVATE> {
public:
    static bool Filter(unsigned int message, unsigned int* lparam, unsigned int* wparam, intptr_t hwnd) {
        *wparam = WA_ACTIVE;
        *lparam = 0;
        return true;
    }
};

class WindowActivateAppFilter : public GtoMnK::MessageFilterBase<WindowActivateAppFilterID, WM_ACTIVATEAPP> {
public:
    static bool Filter(unsigned int message, unsigned int* lparam, unsigned int* wparam, intptr_t hwnd) {
        *wparam = TRUE;
        *lparam = 0;
        return true;
    }
};

class MouseWheelFilter : public GtoMnK::MessageFilterBase<MouseWheelFilterID, WM_MOUSEWHEEL, WM_MOUSEHWHEEL> {
public:
    static bool Filter(unsigned int message, unsigned int* lparam, unsigned int* wparam, intptr_t hwnd) {
        if ((*wparam & Input::GtoMnK_MOUSE_SIGNATURE) != 0) {
            *wparam &= ~Input::GtoMnK_MOUSE_SIGNATURE;
            return true;
		}
        return false;
    }
};

class MouseButtonFilter : public GtoMnK::MessageFilterBase<MouseButtonFilterID, WM_LBUTTONDOWN, WM_XBUTTONUP> {
public:
    static bool Filter(unsigned int message, unsigned int* lparam, unsigned int* wparam, intptr_t hwnd) {
        if ((*wparam & Input::GtoMnK_MOUSE_SIGNATURE) != 0) { // Is set in Input.cpp
            *wparam &= ~Input::GtoMnK_MOUSE_SIGNATURE;
            return true;
        }
        return false;
    }
};

class KeyboardButtonFilter : public GtoMnK::MessageFilterBase<KeyboardButtonFilterID, WM_KEYDOWN, WM_SYSKEYUP> {
public:
    static bool Filter(unsigned int message, unsigned int* lparam, unsigned int* wparam, intptr_t hwnd) {
        if ((*lparam & Input::GtoMnK_KEYBOARD_SIGNATURE) != 0) { // Is set in Input.cpp
            *lparam &= ~Input::GtoMnK_KEYBOARD_SIGNATURE;
            return true;
        }
        return false;
    }
};

}