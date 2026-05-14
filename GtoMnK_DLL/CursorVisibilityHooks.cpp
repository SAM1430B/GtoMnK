/*
This file is imported from ProtoInput project, created by Ilyaki https://github.com/Ilyaki/ProtoInput
And is modified for GtoMnK usage. All credits to the original author.
*/

#include "pch.h"
#include "CursorVisibilityHooks.h"
#include "FakeCursor.h"

extern int ShowProtoCursorWhenImageUpdated;
extern int DontShowCursorWhenImageUpdated;

namespace GtoMnK
{
	int WINAPI CursorVisibilityHook::Hook_ShowCursor(BOOL bShow)
	{
		FakeCursor::SetCursorVisibility(bShow == TRUE);

		if (bShow == FALSE) ShowCursor(FALSE);
		
		return bShow == TRUE ? 0 : -1;
	}

	HCURSOR WINAPI CursorVisibilityHook::Hook_SetCursor(HCURSOR hCursor)
	{
		// Nucleus app uses `DontShowCursorWhenImageUpdated` as a way to enable/disable it.
		if (ShowProtoCursorWhenImageUpdated == 1 && DontShowCursorWhenImageUpdated == 0)
		{
			// This is the original hook implementation. Required for cursors to show at all on Minecraft, for example

			FakeCursor::SetCursorVisibility(hCursor != nullptr);
		}
		else
		{
			// This is what the API documentation would suggest is the correct thing to do
			// Required for Unity engine games

			if (hCursor == nullptr)
				FakeCursor::SetCursorVisibility(false);
		}

		if (hCursor != nullptr)
			FakeCursor::SetCursorHandle(hCursor); // Custom cursor image

		return hCursor;
	}

	BOOL WINAPI CursorVisibilityHook::Hook_SetSystemCursor(HCURSOR hCursor, DWORD id)
	{
		if (id == 32512) // OCR_NORMAL: Standard arrow
		{
			FakeCursor::SetCursorVisibility(hCursor != nullptr);

			if (hCursor != nullptr)
				FakeCursor::SetCursorHandle(hCursor); // Custom cursor image
		}

		return TRUE;
	}

}
