#pragma once
#include <windows.h>

namespace GtoMnK
{
    class OverlayNotification
    {
    private:
        HWND overlayNotificationWindow = nullptr;
        ULONG_PTR gdiplusToken = 0;

        void UpdatePositionLoopInternal();

    public:
        static OverlayNotification state;

        void Initialise();

        void StartInternal();
        void GetWindowDimensions(HWND hWnd);

        void DrawOverlay(int showmessage);

        void CleanupGDIOverlay();

        static HWND GetOverlayNotificationWindow()
        {
            return state.overlayNotificationWindow;
        }
    };
}