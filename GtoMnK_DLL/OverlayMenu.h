#pragma once
#include <windows.h>
#include <vector>
#include <mutex>

namespace GtoMnK {

    struct OverlayOption {
        const char* name;
        float* value;        // Pointer to the real variable in MainThread
        float step;          // Increment/Decrement amount
        float minVal;
        float maxVal;
        int id;              // Unique ID for this item (e.g., 1, 2, 3...)
        int parentId;        // 0 if it's a top-level item. If > 0, it belongs to that ID.
        bool isExpanded;     // (For Parents) Are the children shown?
    };

    class OverlayMenu {
    public:
        static OverlayMenu state;

        void Initialise();
        void EnableDisableMenu(bool enable);
        void ProcessInput(int gamepadButtons); // Handles the overlay buttons
        void GetWindowDimensions(HWND mWnd);
        void GetWindowZorder(HWND mWnd);

        void SetupOptions();

        bool isMenuOpen = false;

    private:
        void StartInternal();
        void StartDrawLoopInternal();
        void UpdatePositionLoopInternal();
        void DrawUI();
        bool IsOptionVisible(int index);

        HWND menuWindow = nullptr;
        HDC hdc = nullptr;
        HBRUSH transparencyBrush = nullptr;
        HBRUSH backgroundBrush = nullptr;

        // Visual settings
        const COLORREF transparencyKey = RGB(1, 1, 1);
        const COLORREF backgroundColor = RGB(20, 20, 20); // Menu background color

        // Threading
        std::mutex mutex;
        std::condition_variable conditionvar;

        // Navigation state
        int menuSelection = 0;
        ULONGLONG lastInputTime = 0;
        std::vector<OverlayOption> options;
    };
}