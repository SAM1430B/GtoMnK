#pragma once

namespace GtoMnK {
	class  CursorVisibilityHook {
    public:
        static int WINAPI Hook_ShowCursor(BOOL bShow);
        static HCURSOR WINAPI Hook_SetCursor(HCURSOR hCursor);
        static BOOL WINAPI Hook_SetSystemCursor(HCURSOR hCursor, DWORD id);

        //static bool ShowCursorWhenImageUpdated;
    };

}