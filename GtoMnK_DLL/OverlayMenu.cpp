// The overlay options file.

#include "pch.h"
#include "OverlayMenu.h"
#include "Logging.h"
#include "MainThread.h"
#include <time.h>
#include "EnableOpenXinput.h"

extern bool disableOverlayOptions;
extern int controllerID;

extern float sensitivity;
extern float sensitivity_multiplier;
extern float horizontal_sensitivity;
extern float vertical_sensitivity;
extern float max_threshold;

extern float radial_deadzone;
extern float axial_deadzone;
extern float stick_as_button_deadzone;
extern float g_TriggerThreshold;

extern float look_accel_multiplier;
extern float curve_slope;
extern float curve_exponent;

namespace GtoMnK {

    OverlayMenu OverlayMenu::state{};
#define WM_MOVE_menuWindow (WM_APP + 2)

    LRESULT WINAPI OverlayMenuWndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
        switch (msg) {
        case WM_MOVE_menuWindow:
            OverlayMenu::state.GetWindowDimensions(hWnd);
            break;
        case WM_DESTROY:
            PostQuitMessage(0);
            return 0;
        }
        return DefWindowProc(hWnd, msg, wParam, lParam);
    }

    void OverlayMenu::SetupOptions() {

        options = {
            // { Name, Value, Step, Min, Max, ID, ParentID, IsExpanded }
            // It has a value, but also holds children. Starts Collapsed (false).

            { "Sensitivity", &sensitivity,                              0.05f,   0.1f,  30.0f,  1,  0, false },
#if defined(_DEBUG) || defined(ENABLE_LOGGING)
            { "Sensitivity Multiplier", &sensitivity_multiplier,        0.05f,   0.1f,  30.0f,  2,  1, false },
#endif
            { "Horiz Sens", &horizontal_sensitivity,                   0.005f,  -1.0f,   1.0f,  3,  1, false },
            { "Vert Sens", &vertical_sensitivity,                      0.005f,  -1.0f,   1.0f,  4,  1, false },
#if defined(_DEBUG) || defined(ENABLE_LOGGING)
            { "Max Threshold", &max_threshold,                         0.005f,   0.0f,  0.15f,  5,  1, false },
#endif
            { "Radial Deadzone", &radial_deadzone,                      0.01f,   0.0f,   1.0f,  6,  0, false },
            { "Axial Deadzone", &axial_deadzone,                        0.01f,   0.0f,   1.0f,  7,  6, false },
            { "Stick As Btn Deadzone", &stick_as_button_deadzone,       0.01f,   0.0f,   1.0f,  8,  6, false },
            { "Trigger Threshold", &g_TriggerThreshold,                 1.00f,   0.0f, 255.0f,  9,  6, false },

#if defined(_DEBUG) || defined(ENABLE_LOGGING)
            { "LookAccel Multiplier", &look_accel_multiplier,           0.005f,   0.1f,  10.0f, 10,   0, false },
            { "Curve Slope", &curve_slope,                              0.005f,   0.0f,   5.0f, 11,  10, false },
            { "Curve Exponent", &curve_exponent,                        0.005f,   0.0f,  15.0f, 12,  10, false }
#endif
        };
    }

    void OverlayMenu::EnableDisableMenu(bool enable) {
        isMenuOpen = enable;

        if (enable) {
            lastInputTime = GetTickCount64();

            int x = 0, y = 0, w = 800, h = 600;
            RECT cRect;
            if (IsWindow(hwnd) && GetClientRect(hwnd, &cRect)) {
                POINT topLeft = { cRect.left, cRect.top };
                ClientToScreen(hwnd, &topLeft);
                x = topLeft.x;
                y = topLeft.y;
                w = cRect.right - cRect.left;
                h = cRect.bottom - cRect.top;
            }

            // TOPMOST should be added to be in front of the game window
            SetWindowPos(menuWindow, HWND_TOP, x, y, w, h,
                SWP_SHOWWINDOW | SWP_NOACTIVATE);

            DrawUI();
        }
        else {
            ShowWindow(menuWindow, SW_HIDE);
        }
    }

    // Overlay menu input process
    void OverlayMenu::ProcessInput(int gamepadButtons) {
        if (!isMenuOpen) return;

        // Delay between presses
        ULONGLONG now = GetTickCount64();
        if (now - lastInputTime < 200) return;

        bool input = false;

        static ULONGLONG dirHoldTimer = 0;
        static int lastDir = 0;

        XINPUT_STATE state;
        ZeroMemory(&state, sizeof(XINPUT_STATE));
        if (pXInputGetState(controllerID, &state) != ERROR_SUCCESS) {
        return;
    }
        float stickX = state.Gamepad.sThumbLX;
        float stickY = state.Gamepad.sThumbLY;
        const int StickMenuDeadzone = 12000;
        if (std::abs(stickX) < StickMenuDeadzone) stickX = 0;
        if (std::abs(stickY) < StickMenuDeadzone) stickY = 0;

        // Close the overlay
        if ((gamepadButtons & XINPUT_GAMEPAD_START) || (gamepadButtons & XINPUT_GAMEPAD_B)) {
            EnableDisableMenu(false);
            input = true;

            // Hide the Overlay Menu
            XINPUT_STATE state;
            ZeroMemory(&state, sizeof(XINPUT_STATE));
            do {
                Sleep(100); // Important
                if (pXInputGetState(controllerID, &state) != ERROR_SUCCESS) {
                    break;
                }
            } while ((state.Gamepad.wButtons & XINPUT_GAMEPAD_START) ||
                (state.Gamepad.wButtons & XINPUT_GAMEPAD_B));
            lastInputTime = GetTickCount64();
            return;
        }
        // Toggle expand children option
        if (gamepadButtons & XINPUT_GAMEPAD_X) {
            int currentPId = options[menuSelection].parentId;
            int targetId = (currentPId == 0) ? options[menuSelection].id : currentPId;
            for (int i = 0; i < (int)options.size(); i++) {
                if (options[i].id == targetId) {
                    // Toggle the state
                    options[i].isExpanded = !options[i].isExpanded;
                    if (!options[i].isExpanded && currentPId != 0) {
                        menuSelection = i;
                    }
                    input = true;
                    break;
                }
            }
        }
        // Navigate up
        if (gamepadButtons & XINPUT_GAMEPAD_DPAD_UP || (stickY > 0)) {
            do {
                menuSelection--;
                if (menuSelection < 0) menuSelection = (int)options.size() - 1;
            } while (!IsOptionVisible(menuSelection));
            input = true;
        }
        // Navigate down
        else if (gamepadButtons & XINPUT_GAMEPAD_DPAD_DOWN || (stickY < 0)) {
            do {
                menuSelection++;
                if (menuSelection >= options.size()) menuSelection = 0;
            } while (!IsOptionVisible(menuSelection));
            input = true;
        }
        // Value decrement
        else if (gamepadButtons & XINPUT_GAMEPAD_DPAD_LEFT || (stickX < 0)) {
            if (lastDir != 1 || (now - lastInputTime > 250)) {
                dirHoldTimer = now;
                lastDir = 1;
            }
            // If held more than 500ms, decrease triple of the value else just increase it normally
            float multiplier = (now - dirHoldTimer > 500) ? 3.0f : 1.0f;

            OverlayOption& opt = options[menuSelection];
            *opt.value -= (opt.step * multiplier);
            if (*opt.value < opt.minVal) *opt.value = opt.minVal;
            input = true;
        }
        // Value increment
        else if (gamepadButtons & XINPUT_GAMEPAD_DPAD_RIGHT || (stickX > 0)) {
            if (lastDir != 2 || (now - lastInputTime > 250)) {
                dirHoldTimer = now;
                lastDir = 2;
            }
            // If held more than 500ms, increase triple of the value else just increase it normally
            float multiplier = (now - dirHoldTimer > 500) ? 3.0f : 1.0f;

            OverlayOption& opt = options[menuSelection];
            *opt.value += (opt.step * multiplier);
            if (*opt.value > opt.maxVal) *opt.value = opt.maxVal;
            input = true;
        }

        if (input) lastInputTime = now;
    }

    bool OverlayMenu::IsOptionVisible(int index) {
        int pId = options[index].parentId;

        // If no parent, it's always visible
        if (pId == 0) return true;

        // Find the parent to check its 'isExpanded' status
        for (const auto& opt : options) {
            if (opt.id == pId) {
                return opt.isExpanded;
            }
        }
        return true;
    }

    void OverlayMenu::DrawUI() {
        if (!isMenuOpen || !hdc) return;

        RECT clientRect;
        GetClientRect(menuWindow, &clientRect);

        FillRect(hdc, &clientRect, transparencyBrush);

        int winW = clientRect.right;
        int winH = clientRect.bottom;

        const float REFERENCE_HEIGHT = 1080.0f;

        float scale = (float)winH / REFERENCE_HEIGHT;

        if (scale < 0.3f) scale = 0.3f;

        int baseBoxW = 400;
        int baseBoxX = 50;
        int baseBoxY = 50;
        int baseRowH = 30;
        int baseFontH = 24;
        int baseIndent = 20;

        int scaledBoxW = (int)(baseBoxW * scale);
        int scaledBoxX = (int)(baseBoxX * scale);
        int scaledBoxY = (int)(baseBoxY * scale);
        int scaledRowH = (int)(baseRowH * scale);
        int scaledIndent = (int)(baseIndent * scale);

        int visibleCount = 0;
        for (const auto& opt : options) { if (IsOptionVisible(opt.id)) visibleCount++; }

        int visibleLines = 0;
        for (size_t i = 0; i < options.size(); i++) {
            if (IsOptionVisible((int)i)) visibleLines++;
        }

        int headerHeight = (int)(50 * scale);
        int footerHeight = (int)(60 * scale);
        int scaledBoxH = headerHeight + (visibleLines * scaledRowH) + footerHeight;

        RECT bgRect = { scaledBoxX, scaledBoxY, scaledBoxX + scaledBoxW, scaledBoxY + scaledBoxH };

        // Draw background
        FillRect(hdc, &bgRect, backgroundBrush);
        FrameRect(hdc, &bgRect, (HBRUSH)GetStockObject(WHITE_BRUSH));

        // For the text highlight
        HBRUSH highlightBrush = CreateSolidBrush(RGB(60, 60, 60));
        int activeFamilyId = -1;
        if (menuSelection >= 0 && menuSelection < (int)options.size()) {
            // If selected is Parent, use its ID. If Child, use its ParentID.
            activeFamilyId = (options[menuSelection].parentId == 0) ?
                options[menuSelection].id : options[menuSelection].parentId;
        }

        // Create a font that matches the scale
        HFONT hNewFont = CreateFontA(
            -(int)(baseFontH * scale), // Height (negative for character height, not cell)
            0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE, DEFAULT_CHARSET,
            OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY,
            DEFAULT_PITCH | FF_DONTCARE, "Arial");

        HFONT hOldFont = (HFONT)SelectObject(hdc, hNewFont);

        SetBkMode(hdc, TRANSPARENT);
        SetTextColor(hdc, RGB(255, 255, 0));

        // Title
        int textX = scaledBoxX + scaledIndent;
        int textY = scaledBoxY + (int)(10 * scale);
#if defined(_DEBUG) || defined(ENABLE_LOGGING)
        const char* title = "Controller Options (Advanced)";
#else
        const char* title = "Controller Options";
#endif
        TextOutA(hdc, textX, textY, title, (int)strlen(title));

        int currentY = scaledBoxY + headerHeight;

        // Overlay drawing loop content
        for (size_t i = 0; i < options.size(); i++) {
            if (!IsOptionVisible((int)i)) continue;

            // Highlight the text
            if (options[i].id == activeFamilyId || options[i].parentId == activeFamilyId) {
                RECT rowRect = { scaledBoxX + 1, currentY, scaledBoxX + scaledBoxW - 1, currentY + scaledRowH };
                FillRect(hdc, &rowRect, highlightBrush);
            }

            char buffer[128];
            const char* selectionMarker = (i == menuSelection ? ">" : " ");

            // Parent check
            bool isParent = false;
            for (const auto& opt : options) { if (opt.parentId == options[i].id) { isParent = true; break; } }

            const char* toggleSymbol = "";
            if (isParent) {
                toggleSymbol = options[i].isExpanded ? "[ - ]" : "[ + ]";
            }

            // Indentation
            int xOffset = 0;
            if (options[i].parentId != 0) xOffset = scaledIndent;

            char nameBuffer[128];
            char valBuffer[64] = "";

            // "> [+] Sensitivity: "
            sprintf_s(nameBuffer, "%s %s %s: ", selectionMarker, toggleSymbol, options[i].name);

            // Value format
            if (options[i].value != nullptr) {
                sprintf_s(valBuffer, "%.3f", *options[i].value);
            }
            else {
                sprintf_s(nameBuffer, "%s %s %s", selectionMarker, toggleSymbol, options[i].name);
            }

            COLORREF nameColor;
            COLORREF valColor;

            if (i == menuSelection) {
                nameColor = RGB(0, 255, 0);   // Selected Name = Green
                valColor = RGB(255, 255, 0); // Selected Value = Yellow
            }
            else {
                nameColor = RGB(255, 255, 255); // Unselected = White
                valColor = RGB(255, 255, 255); // Unselected = White
            }

            // Draw the name
            SetTextColor(hdc, nameColor);
            TextOutA(hdc, textX + xOffset, currentY, nameBuffer, (int)strlen(nameBuffer));

            // Draw the value
            if (valBuffer[0] != '\0') {
                SIZE size;
                GetTextExtentPoint32A(hdc, nameBuffer, (int)strlen(nameBuffer), &size);

                SetTextColor(hdc, valColor);
                TextOutA(hdc, textX + xOffset + size.cx, currentY, valBuffer, (int)strlen(valBuffer));
            }

            currentY += scaledRowH;
        }

        // Footer
        SetTextColor(hdc, RGB(150, 150, 150));
        const char* footerText = "(X): Expand | (D-PAD): Adjust";
        TextOutA(hdc, textX, bgRect.bottom - (int)(40 * scale), footerText, (int)strlen(footerText));

        SelectObject(hdc, hOldFont);
        DeleteObject(hNewFont);
        DeleteObject(highlightBrush);
    }

    void OverlayMenu::StartInternal() {
        const auto hInstance = GetModuleHandle(NULL);

		Sleep(5000);

       /* hwnd = GetMainWindowHandle(GetCurrentProcessId(), iniWindowName, iniClassName, 60000);

        if (!hwnd || !IsWindow(hwnd)) {
            LOG("Timeout: Game Window never appeared after 60 seconds. Overlay Menu aborting.");
            return;
        }*/

		LOG("Overlay Menu Window is being created...");

        transparencyBrush = CreateSolidBrush(transparencyKey);
        backgroundBrush = CreateSolidBrush(backgroundColor);

        WNDCLASSW wc = { 0 };
        wc.lpfnWndProc = OverlayMenuWndProc;
        wc.hInstance = hInstance;
        wc.hbrBackground = transparencyBrush;
        wc.lpszClassName = L"GtoMnK_Overlay_Window";
        wc.style = CS_OWNDC | CS_NOCLOSE;

        if (!RegisterClassW(&wc)) {
            LOG("Failed to register Overlay Menu window class!");
            return;
        }
        else
        {

            menuWindow = CreateWindowExW(WS_EX_NOACTIVATE | WS_EX_NOINHERITLAYOUT | WS_EX_NOPARENTNOTIFY |
                /*WS_EX_TOPMOST |*/ WS_EX_TRANSPARENT | WS_EX_TOOLWINDOW | WS_EX_LAYERED,
                wc.lpszClassName,
                L"OverlayMenu",
                WS_POPUP,
                0, 0, 800, 600,
                hwnd, // The game window as the owner `GW_OWNER`
                nullptr, hInstance, nullptr);

            SetLayeredWindowAttributes(menuWindow, transparencyKey, 0, LWA_COLORKEY);

            hdc = GetDC(menuWindow);
            SetupOptions();

            CreateThread(nullptr, 0, (LPTHREAD_START_ROUTINE)([](LPVOID) -> DWORD {
                OverlayMenu::state.StartDrawLoopInternal(); return 0;
                }), 0, 0, 0);

            CreateThread(nullptr, 0, (LPTHREAD_START_ROUTINE)([](LPVOID) -> DWORD {
                OverlayMenu::state.UpdatePositionLoopInternal(); return 0;
                }), 0, 0, 0);

            MSG msg;
            ZeroMemory(&msg, sizeof(msg));
            while (msg.message != WM_QUIT) {
                if (GetMessageW(&msg, menuWindow, 0U, 0U)) {
                    TranslateMessage(&msg);
                    DispatchMessage(&msg);
                }
            }
        }
    }

    void OverlayMenu::StartDrawLoopInternal() {
        while (true) {
            // Only draw if menu is actually open to save CPU
            if (isMenuOpen) {
                DrawUI();
            }
            Sleep(100); // ~10 FPS is enough i think
        }
    }

    void OverlayMenu::UpdatePositionLoopInternal() {
        while (true) {
            if (isMenuOpen && IsWindow(hwnd)) {
                PostMessage(menuWindow, WM_MOVE_menuWindow, 0, 0);
            }
            Sleep(500);
        }
    }

    void OverlayMenu::GetWindowDimensions(HWND mWnd) {
        if (!IsWindow(hwnd)) return;

        RECT cRect;
        GetClientRect(hwnd, &cRect);
        POINT topLeft = { cRect.left, cRect.top };
        ClientToScreen(hwnd, &topLeft);

        SetWindowPos(mWnd, HWND_TOP,
            topLeft.x, topLeft.y,
            cRect.right - cRect.left, cRect.bottom - cRect.top,
            SWP_NOZORDER | SWP_NOACTIVATE | SWP_NOREDRAW);
    }

    void OverlayMenu::Initialise() {
        LOG("Initializing Overlay Menu...");
        CreateThread(nullptr, 0, (LPTHREAD_START_ROUTINE)([](LPVOID) -> DWORD {
            OverlayMenu::state.StartInternal(); return 0;
            }), 0, 0, 0);
    }
}