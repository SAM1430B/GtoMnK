#include "pch.h"
#include "Input.h"
#include "Mouse.h"
#include "InputState.h"
#include "KeyboardState.h"
#include "RawInput.h"
#include <vector>
#include <sstream>
#include <map>
#include <algorithm>
#include "MainThread.h"

//extern HWND hwnd;
//extern GtoMnK::InputMethod g_InputMethod;
//extern int keystatesend;
//extern bool g_EnableMouseDoubleClick;

namespace GtoMnK {
    namespace Input {

        static bool isLMB_Down = false, isRMB_Down = false, isMMB_Down = false, isX1B_Down = false, isX2B_Down = false;

		// For PostMessage method
        WPARAM BuildWParam() {
            WPARAM wParam = 0;
            if (isLMB_Down) wParam |= MK_LBUTTON;
            if (isRMB_Down) wParam |= MK_RBUTTON;
            if (isMMB_Down) wParam |= MK_MBUTTON;
            if (isX1B_Down) wParam |= MK_XBUTTON1;
            if (isX2B_Down) wParam |= MK_XBUTTON2;
            return wParam;
        }

        void PostMessageMouse(int actionCode, bool press) {
            if (!hwnd) return;
            if (actionCode == -1) isLMB_Down = press; if (actionCode == -2) isRMB_Down = press;
            if (actionCode == -3) isMMB_Down = press; if (actionCode == -4) isX1B_Down = press;
            if (actionCode == -5) isX2B_Down = press; if (actionCode == -8) isLMB_Down = press;

            POINT currentPos = { Mouse::Xf, Mouse::Yf };
            LPARAM clickPos = MAKELPARAM(currentPos.x, currentPos.y);
            UINT msg = 0;
            WPARAM wParam = BuildWParam();

            switch (actionCode) {
            case -1: msg = press ? WM_LBUTTONDOWN : WM_LBUTTONUP; break;
            case -2: msg = press ? WM_RBUTTONDOWN : WM_RBUTTONUP; break;
            case -3: msg = press ? WM_MBUTTONDOWN : WM_MBUTTONUP; break;
            case -4: msg = press ? WM_XBUTTONDOWN : WM_XBUTTONUP; wParam |= MAKEWPARAM(0, XBUTTON1); break;
            case -5: msg = press ? WM_XBUTTONDOWN : WM_XBUTTONUP; wParam |= MAKEWPARAM(0, XBUTTON2); break;
            case -6: if (press) { msg = WM_MOUSEWHEEL; wParam |= MAKEWPARAM(0, 120); } break;
            case -7: if (press) { msg = WM_MOUSEWHEEL; wParam |= MAKEWPARAM(0, -120); } break;
            case -8: msg = press ? WM_LBUTTONDBLCLK : WM_LBUTTONUP; break;
            }
            if (msg != 0) PostMessage(hwnd, msg, wParam, clickPos);
        }

        void PostMessageKey(int vkCode, bool press, bool isExtended) {
            if (vkCode == 0 || !hwnd) return;
            keystatesend = press ? vkCode : 0;
            UINT scanCode = MapVirtualKey(vkCode, MAPVK_VK_TO_VSC);
            LPARAM lParam = (1 | (scanCode << 16));

            if (isExtended) {
                lParam |= (1 << 24);
            }

            UINT msg = press ? WM_KEYDOWN : WM_KEYUP;
            if (!press) lParam |= (1 << 30) | (1 << 31);

            PostMessage(hwnd, msg, vkCode, lParam);
        }

		// For RawInput method
        void GenerateRawKey(int vkCode, bool press, bool isExtended) {
            if (vkCode == 0) return;

            RAWINPUT ri = {};
            ri.header.dwType = RIM_TYPEKEYBOARD;
            ri.header.hDevice = NULL;

            UINT scanCode = MapVirtualKey(vkCode, MAPVK_VK_TO_VSC);
            ri.data.keyboard.MakeCode = scanCode;
            ri.data.keyboard.Message = press ? WM_KEYDOWN : WM_KEYUP;
            ri.data.keyboard.VKey = vkCode;
            ri.data.keyboard.Flags = press ? RI_KEY_MAKE : RI_KEY_BREAK;

            if (isExtended) {
                ri.data.keyboard.Flags |= RI_KEY_E0;
            }

            RawInput::InjectFakeRawInput(ri);
        }

        void GenerateRawMouseButton(int actionCode, bool press) {
            RAWINPUT ri = {};
            ri.header.dwType = RIM_TYPEMOUSE;
            ri.header.hDevice = NULL;
            if (actionCode == -8) {
                GenerateRawMouseButton(-1, true); GenerateRawMouseButton(-1, false);
                GenerateRawMouseButton(-1, press); return;
            }
            switch (actionCode) {
            case -1: ri.data.mouse.usButtonFlags = press ? RI_MOUSE_LEFT_BUTTON_DOWN : RI_MOUSE_LEFT_BUTTON_UP; break;
            case -2: ri.data.mouse.usButtonFlags = press ? RI_MOUSE_RIGHT_BUTTON_DOWN : RI_MOUSE_RIGHT_BUTTON_UP; break;
            case -3: ri.data.mouse.usButtonFlags = press ? RI_MOUSE_MIDDLE_BUTTON_DOWN : RI_MOUSE_MIDDLE_BUTTON_UP; break;
            case -4: ri.data.mouse.usButtonFlags = press ? RI_MOUSE_BUTTON_4_DOWN : RI_MOUSE_BUTTON_4_UP; break;
            case -5: ri.data.mouse.usButtonFlags = press ? RI_MOUSE_BUTTON_5_DOWN : RI_MOUSE_BUTTON_5_UP; break;
            case -6: if (press) ri.data.mouse.usButtonFlags = RI_MOUSE_WHEEL; ri.data.mouse.usButtonData = WHEEL_DELTA; break;
            case -7: if (press) ri.data.mouse.usButtonFlags = RI_MOUSE_WHEEL; ri.data.mouse.usButtonData = -WHEEL_DELTA; break;
            }
            RawInput::InjectFakeRawInput(ri);
        }

		// For keyboard actions for both methods
        void TranslateKeyboardAction(int actionCode, int& outVkCode, bool& outIsExtended) {
            outVkCode = 0;
            outIsExtended = false;

            // Special & Modifier Keys
            if (actionCode == 1) outVkCode = VK_ESCAPE;
			if (actionCode == 2) outVkCode = VK_RETURN;      // Main Enter Key
			if (actionCode == 3) outVkCode = VK_TAB;
			if (actionCode == 4) outVkCode = VK_SHIFT;       // Generic Shift
			if (actionCode == 5) outVkCode = VK_LSHIFT;      // Left Shift
			if (actionCode == 6) outVkCode = VK_RSHIFT;      // Right Shift
			if (actionCode == 7) outVkCode = VK_CONTROL;     // Generic Control
			if (actionCode == 8) outVkCode = VK_LCONTROL;    // Left Control
			if (actionCode == 9) outVkCode = VK_RCONTROL;    // Right Control
			if (actionCode == 10) outVkCode = VK_MENU;       // Generic Alt
			if (actionCode == 11) outVkCode = VK_LMENU;      // Left Alt
			if (actionCode == 12) outVkCode = VK_RMENU;      // Right Alt
			if (actionCode == 13) outVkCode = VK_SPACE;
			if (actionCode == 14) outVkCode = VK_UP;         // Arrow Up
			if (actionCode == 15) outVkCode = VK_DOWN;       // Arrow Down
			if (actionCode == 16) outVkCode = VK_LEFT;       // Arrow Left
			if (actionCode == 17) outVkCode = VK_RIGHT;      // Arrow Right
			if (actionCode == 18) outVkCode = VK_BACK;       // Backspace
			if (actionCode == 19) outVkCode = VK_DELETE;
			if (actionCode == 20) outVkCode = VK_INSERT;
			if (actionCode == 21) outVkCode = VK_END;
			if (actionCode == 22) outVkCode = VK_HOME;
			if (actionCode == 23) outVkCode = VK_PRIOR;      // Page Up
			if (actionCode == 24) outVkCode = VK_NEXT;       // Page Down
			// Alphabet
			if (actionCode >= 25 && actionCode <= 50) outVkCode = 'A' + (actionCode - 25);
			// Top Row Numbers
			if (actionCode >= 51 && actionCode <= 60) outVkCode = '0' + (actionCode - 51);
			// F-Keys
			if (actionCode >= 61 && actionCode <= 72) outVkCode = VK_F1 + (actionCode - 61);
			// Numpad Numbers & Operators
			if (actionCode >= 73 && actionCode <= 82) outVkCode = VK_NUMPAD0 + (actionCode - 73);
			if (actionCode == 83) outVkCode = VK_ADD;
			if (actionCode == 84) outVkCode = VK_SUBTRACT;
			if (actionCode == 85) outVkCode = VK_MULTIPLY;
			if (actionCode == 86) outVkCode = VK_DIVIDE;
			if (actionCode == 87) outVkCode = VK_DECIMAL;
			if (actionCode == 88) outVkCode = VK_RETURN;     // Numpad Enter
			// Numpad Navigation (when NumLock is OFF)
			if (actionCode == 91) outVkCode = VK_INSERT;     // Numpad 0
			if (actionCode == 92) outVkCode = VK_END;        // Numpad 1
			if (actionCode == 93) outVkCode = VK_DOWN;       // Numpad 2
			if (actionCode == 94) outVkCode = VK_NEXT;       // Numpad 3
			if (actionCode == 95) outVkCode = VK_LEFT;       // Numpad 4
			if (actionCode == 96) outVkCode = VK_RIGHT;      // Numpad 6
			if (actionCode == 97) outVkCode = VK_HOME;       // Numpad 7
			if (actionCode == 98) outVkCode = VK_UP;         // Numpad 8
			if (actionCode == 99) outVkCode = VK_PRIOR;      // Numpad 9
			if (actionCode == 100) outVkCode = VK_DELETE;    // Numpad .
			// Extended Keys
			if ((actionCode >= 14 && actionCode <= 17) || // Dedicated Arrow Keys
				(actionCode >= 19 && actionCode <= 24) || // Insert, Del, Home, End, PgUp, PgDn
				actionCode == 9 ||  // Right Control
				actionCode == 12 || // Right Alt
				actionCode == 88)   // Numpad Enter
			{
				outIsExtended = true;
			}
		}

        void DispatchAction_PostMessage(int actionCode, bool press) {
            bool isMouseAction = (actionCode < 0);
            if (isMouseAction) {
                int vkCode = 0;
                switch (actionCode) {
                case -1: case -8: vkCode = VK_LBUTTON; break;
                case -2: vkCode = VK_RBUTTON; break;
                case -3: vkCode = VK_MBUTTON; break;
                case -4: vkCode = VK_XBUTTON1; break;
                case -5: vkCode = VK_XBUTTON2; break;
                }
                if (vkCode != 0) {
                    if (press) g_heldVirtualKeys.insert(vkCode);
                    else g_heldVirtualKeys.erase(vkCode);
                }
                PostMessageMouse(actionCode, press);
            }
            // Keyboard actions
            else {
                int vkCode; bool isExtended;
                TranslateKeyboardAction(actionCode, vkCode, isExtended);
                if (vkCode != 0) {
                    if (press) g_heldVirtualKeys.insert(vkCode);
                    else g_heldVirtualKeys.erase(vkCode);
                }
                PostMessageKey(vkCode, press, isExtended);
            }
        }

		// For RawInput method
        void DispatchAction_RawInput(int actionCode, bool press) {
            bool isMouseAction = (actionCode < 0);
            if (isMouseAction) {
                int vkCode = 0;
                switch (actionCode) {
                case -1: case -8: vkCode = VK_LBUTTON; break;
                case -2: vkCode = VK_RBUTTON; break;
                case -3: vkCode = VK_MBUTTON; break;
                case -4: vkCode = VK_XBUTTON1; break;
                case -5: vkCode = VK_XBUTTON2; break;
                }
                if (vkCode != 0) {
                    if (press) g_heldVirtualKeys.insert(vkCode);
                    else g_heldVirtualKeys.erase(vkCode);
                }
                GenerateRawMouseButton(actionCode, press);
            }
            // Keyboard actions
            else {
                int vkCode; bool isExtended;
                TranslateKeyboardAction(actionCode, vkCode, isExtended);
                if (vkCode != 0) {
                    if (press) g_heldVirtualKeys.insert(vkCode);
                    else g_heldVirtualKeys.erase(vkCode);
                }
                GenerateRawKey(vkCode, press, isExtended);
            }
        }

		// For Hold/Release logic in the INI.
        std::vector<Action> ParseActionString(const std::string& fullString) {
            std::vector<Action> parsedActions;
            if (fullString.empty() || fullString == "0") {
                return parsedActions;
            }

            std::stringstream ss(fullString);
            std::string part;
            while (std::getline(ss, part, '+')) {
                if (part.empty()) continue;

                Action newAction;

                size_t firstDigitPos = 0;
                bool foundDigit = false;
                for (size_t i = 0; i < part.length(); ++i) {
                    if (part[i] == '*') {
                        newAction.holdDurationMs += 500;
                    }
                    else if (part[i] == '^') {
                        newAction.onRelease = true;
                    }
                    else if (isdigit(part[i]) || part[i] == '-') {
                        firstDigitPos = i;
                        foundDigit = true;
                        break;
                    }
                }

                if (foundDigit) {
                    newAction.actionString = part.substr(firstDigitPos);
                    parsedActions.push_back(newAction);
                }
            }

            std::sort(parsedActions.begin(), parsedActions.end(), [](const Action& a, const Action& b) {
                return a.holdDurationMs < b.holdDurationMs;
                });

            return parsedActions;
        }

        void SendAction(const std::string& actionString, bool press) {
            if (actionString.empty() || actionString == "0") return;

            void (*dispatcher)(int, bool) = (g_InputMethod == InputMethod::RawInput)
                ? DispatchAction_RawInput
                : DispatchAction_PostMessage;

            static ULONGLONG lastLeftClickTime = 0;
            if (actionString.find("-1") != std::string::npos && g_EnableMouseDoubleClick && press) {
                ULONGLONG currentTime = GetTickCount64();
                if (currentTime - lastLeftClickTime < GetDoubleClickTime()) {
                    dispatcher(-8, true);
                    lastLeftClickTime = 0;
                    return;
                }
                lastLeftClickTime = currentTime;
            }

            if (actionString.find('+') == std::string::npos) {
                // Handle single actions
                try {
                    dispatcher(std::stoi(actionString), press);
                }
                catch (...) {}
            }
            else {
                // Handle combo actions
                std::vector<int> keycodes;
                std::stringstream ss(actionString);
                std::string part;
                while (std::getline(ss, part, '+')) {
                    if (part.empty()) continue;
                    size_t firstDigitPos = part.find_first_of("-0123456789");
                    if (firstDigitPos != std::string::npos) {
                        try {
                            keycodes.push_back(std::stoi(part.substr(firstDigitPos)));
                        }
                        catch (...) {}
                    }
                }

                if (press) {
                    for (int code : keycodes) {
                        dispatcher(code, true);
                    }
                }
                else {
                    for (auto it = keycodes.rbegin(); it != keycodes.rend(); ++it) {
                        dispatcher(*it, false);
                    }
                }
            }
        }

		// For PostMessage method
        void SendAction(int screenX, int screenY) {
            if (g_InputMethod != InputMethod::PostMessage) return;
            if (!hwnd) return;
            WPARAM wParam = BuildWParam();
            POINT clientPos = { screenX, screenY };
            ScreenToClient(hwnd, &clientPos);
            PostMessage(hwnd, WM_MOUSEMOVE, wParam, MAKELPARAM(clientPos.x, clientPos.y));
        }

		// For RawInput method
        void SendActionDelta(int deltaX, int deltaY) {
            if (g_InputMethod != InputMethod::RawInput) return;

            RAWINPUT ri = {};
            ri.header.dwType = RIM_TYPEMOUSE;
            ri.header.hDevice = NULL;
            ri.data.mouse.usFlags = MOUSE_MOVE_RELATIVE;
            ri.data.mouse.lLastX = deltaX;
            ri.data.mouse.lLastY = deltaY;
            RawInput::InjectFakeRawInput(ri);
        }

        
    }
}