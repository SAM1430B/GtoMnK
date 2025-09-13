#include "pch.h"
#include "Input.h"
#include "Mouse.h"
#include <vector>
#include <sstream>
#include <map>
#include "InputState.h"

extern HWND hwnd;
extern ScreenshotInput::SendMethod g_SendMethod;
extern int keystatesend;
extern bool g_EnableMouseDoubleClick;

namespace ScreenshotInput {
    namespace Input {

        static bool isLMB_Down = false, isRMB_Down = false, isMMB_Down = false, isX1B_Down = false, isX2B_Down = false;

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

        void SendInputMouse(int actionCode, bool press) {
            // Update internal state FIRST
            if (actionCode == 1) isLMB_Down = press;
            if (actionCode == 2) isRMB_Down = press;
            if (actionCode == 3) isMMB_Down = press;
            if (actionCode == 4) isX1B_Down = press;
            if (actionCode == 5) isX2B_Down = press;
            if (actionCode == 8) isLMB_Down = press;

            INPUT input = {};
            input.type = INPUT_MOUSE;
            switch (actionCode) {
            case 1: input.mi.dwFlags = press ? MOUSEEVENTF_LEFTDOWN : MOUSEEVENTF_LEFTUP; break;
            case 2: input.mi.dwFlags = press ? MOUSEEVENTF_RIGHTDOWN : MOUSEEVENTF_RIGHTUP; break;
            case 3: input.mi.dwFlags = press ? MOUSEEVENTF_MIDDLEDOWN : MOUSEEVENTF_MIDDLEUP; break;
            case 4: input.mi.dwFlags = press ? MOUSEEVENTF_XDOWN : MOUSEEVENTF_XUP; input.mi.mouseData = XBUTTON1; break;
            case 5: input.mi.dwFlags = press ? MOUSEEVENTF_XDOWN : MOUSEEVENTF_XUP; input.mi.mouseData = XBUTTON2; break;
            case 6: if (press) { input.mi.dwFlags = MOUSEEVENTF_WHEEL; input.mi.mouseData = WHEEL_DELTA; } break;
            case 7: if (press) { input.mi.dwFlags = MOUSEEVENTF_WHEEL; input.mi.mouseData = -WHEEL_DELTA; } break;
            case 8: SendInputMouse(1, true); SendInputMouse(1, false); SendInputMouse(1, press); return;
            }
            if (input.mi.dwFlags != 0) SendInput(1, &input, sizeof(INPUT));
        }

        void SendInputKey(int vkCode, bool press, bool isExtended) {
            if (vkCode == 0) return;
            keystatesend = press ? vkCode : 0;
            INPUT input = {};
            input.type = INPUT_KEYBOARD;
            input.ki.wVk = vkCode;

            if (isExtended) {
                input.ki.dwFlags |= KEYEVENTF_EXTENDEDKEY;
            }

            if (!press) {
                input.ki.dwFlags |= KEYEVENTF_KEYUP;
            }
            SendInput(1, &input, sizeof(INPUT));
        }

        void DispatchAction(int actionCode, bool press) {
            static ULONGLONG lastLeftClickTime = 0;
            // Mouse left click double click
			// TODO: Support for right/middle and (X1/X2 ?) double click
            if (actionCode == 1 && g_EnableMouseDoubleClick && press) {
                ULONGLONG currentTime = GetTickCount64();
                if (currentTime - lastLeftClickTime < GetDoubleClickTime()) {
                    actionCode = 8;
                    lastLeftClickTime = 0;
                }
                else {
                    lastLeftClickTime = currentTime;
                }
            }

            bool isMouseAction = (actionCode < 0);
            int vkCode = 0;
            bool isExtended = false;

            if (!isMouseAction) {
                // Special & Modifier Keys
                if (actionCode == 1) vkCode = VK_ESCAPE;
                if (actionCode == 2) vkCode = VK_RETURN;      // Main Enter Key
                if (actionCode == 3) vkCode = VK_TAB;
                if (actionCode == 4) vkCode = VK_SHIFT;       // Generic Shift
                if (actionCode == 5) vkCode = VK_LSHIFT;      // Left Shift
                if (actionCode == 6) vkCode = VK_RSHIFT;      // Right Shift
                if (actionCode == 7) vkCode = VK_CONTROL;     // Generic Control
                if (actionCode == 8) vkCode = VK_LCONTROL;    // Left Control
                if (actionCode == 9) vkCode = VK_RCONTROL;    // Right Control
                if (actionCode == 10) vkCode = VK_MENU;       // Generic Alt
                if (actionCode == 11) vkCode = VK_LMENU;      // Left Alt
                if (actionCode == 12) vkCode = VK_RMENU;      // Right Alt
                if (actionCode == 13) vkCode = VK_SPACE;
                if (actionCode == 14) vkCode = VK_UP;         // Arrow Up
                if (actionCode == 15) vkCode = VK_DOWN;       // Arrow Down
                if (actionCode == 16) vkCode = VK_LEFT;       // Arrow Left
                if (actionCode == 17) vkCode = VK_RIGHT;      // Arrow Right
                if (actionCode == 18) vkCode = VK_BACK;       // Backspace
                if (actionCode == 19) vkCode = VK_DELETE;
                if (actionCode == 20) vkCode = VK_INSERT;
                if (actionCode == 21) vkCode = VK_END;
                if (actionCode == 22) vkCode = VK_HOME;
                if (actionCode == 23) vkCode = VK_PRIOR;      // Page Up
                if (actionCode == 24) vkCode = VK_NEXT;       // Page Down

                // Alphabet
                if (actionCode >= 25 && actionCode <= 50) vkCode = 'A' + (actionCode - 25);

                // Top Row Numbers
                if (actionCode >= 51 && actionCode <= 60) vkCode = '0' + (actionCode - 51);

                // F-Keys
                if (actionCode >= 61 && actionCode <= 72) vkCode = VK_F1 + (actionCode - 61);

                // Numpad Numbers & Operators
                if (actionCode >= 73 && actionCode <= 82) vkCode = VK_NUMPAD0 + (actionCode - 73);
                if (actionCode == 83) vkCode = VK_ADD;
                if (actionCode == 84) vkCode = VK_SUBTRACT;
                if (actionCode == 85) vkCode = VK_MULTIPLY;
                if (actionCode == 86) vkCode = VK_DIVIDE;
                if (actionCode == 87) vkCode = VK_DECIMAL;
                if (actionCode == 88) vkCode = VK_RETURN;     // Numpad Enter

                // Numpad Navigation (when NumLock is OFF)
                if (actionCode == 91) vkCode = VK_INSERT;     // Numpad 0
                if (actionCode == 92) vkCode = VK_END;        // Numpad 1
                if (actionCode == 93) vkCode = VK_DOWN;       // Numpad 2
                if (actionCode == 94) vkCode = VK_NEXT;       // Numpad 3
                if (actionCode == 95) vkCode = VK_LEFT;       // Numpad 4
                if (actionCode == 96) vkCode = VK_RIGHT;      // Numpad 6
                if (actionCode == 97) vkCode = VK_HOME;       // Numpad 7
                if (actionCode == 98) vkCode = VK_UP;         // Numpad 8
                if (actionCode == 99) vkCode = VK_PRIOR;      // Numpad 9
                if (actionCode == 100) vkCode = VK_DELETE;    // Numpad .

                if ((actionCode >= 14 && actionCode <= 17) || // Dedicated Arrow Keys
                    (actionCode >= 19 && actionCode <= 24) || // Insert, Del, Home, End, PgUp, PgDn
                    actionCode == 9 ||  // Right Control
                    actionCode == 12 || // Right Alt
                    actionCode == 88)   // Numpad Enter
                {
                    isExtended = true;
                }
            }

            if (g_SendMethod == SendMethod::PostMessage) {
                if (isMouseAction) PostMessageMouse(actionCode, press); else PostMessageKey(vkCode, press, isExtended);
            }
            else {
                if (isMouseAction) SendInputMouse(actionCode, press); else SendInputKey(vkCode, press, isExtended);
            }
        }

        void SendAction(int screenX, int screenY) {
            if (g_SendMethod == SendMethod::PostMessage) {
                if (!hwnd) return;

                WPARAM wParam = BuildWParam();

                POINT clientPos = { screenX, screenY };
                ScreenToClient(hwnd, &clientPos);
                PostMessage(hwnd, WM_MOUSEMOVE, wParam, MAKELPARAM(clientPos.x, clientPos.y));

            }
            else {
                INPUT input = {};
                input.type = INPUT_MOUSE;
                input.mi.dwFlags = MOUSEEVENTF_MOVE | MOUSEEVENTF_ABSOLUTE;
                double screenWidth = GetSystemMetrics(SM_CXSCREEN) - 1;
                double screenHeight = GetSystemMetrics(SM_CYSCREEN) - 1;
                input.mi.dx = static_cast<LONG>((screenX / screenWidth) * 65535.0f);
                input.mi.dy = static_cast<LONG>((screenY / screenHeight) * 65535.0f);
                SendInput(1, &input, sizeof(INPUT));
            }
        }

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
            try { DispatchAction(std::stoi(actionString), press); }
            catch (...) {}
        }

    }
}