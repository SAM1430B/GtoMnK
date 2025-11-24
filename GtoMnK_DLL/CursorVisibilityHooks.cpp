/*
This file is imported from ProtoInput project, created by Ilyaki https://github.com/Ilyaki/ProtoInput
And is modified for GtoMnK usage. All credits to the original author.
*/

#include "pch.h"
#include "CursorVisibilityHooks.h"
#include "FakeCursor.h"

extern int ShowProtoCursorWhenImageUpdated;

namespace GtoMnK
{
	int WINAPI CursorVisibilityHook::Hook_ShowCursor(BOOL bShow)
	{
		FakeCursor::SetCursorVisibility(bShow == TRUE);
		
		//Recursion safety
		ULONG threadIdList[] = { 0 };
		LhSetGlobalExclusiveACL(threadIdList, 1);
		int result = ShowCursor(bShow);
		LhSetGlobalInclusiveACL(threadIdList, 1);

		return bShow == TRUE ? 0 : -1;
	}

	HCURSOR WINAPI CursorVisibilityHook::Hook_SetCursor(HCURSOR hCursor)
	{
		if (ShowProtoCursorWhenImageUpdated == 1)
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

		//Recursion safety
		ULONG threadIdList[] = { 0 };
		LhSetGlobalExclusiveACL(threadIdList, 1);
		HCURSOR result = SetCursor(hCursor);
		LhSetGlobalInclusiveACL(threadIdList, 1);
		return result;
	}

	BOOL WINAPI CursorVisibilityHook::Hook_SetSystemCursor(HCURSOR hCursor, DWORD id)
	{
		if (id == 32512) // OCR_NORMAL: Standard arrow
		{
			FakeCursor::SetCursorVisibility(hCursor != nullptr);

			if (hCursor != nullptr)
				FakeCursor::SetCursorHandle(hCursor); // Custom cursor image
		}

		//Recursion safety
		ULONG threadIdList[] = { 0 };
		LhSetGlobalExclusiveACL(threadIdList, 1);
		BOOL originalResult = SetSystemCursor(hCursor, id);
		LhSetGlobalInclusiveACL(threadIdList, 1);
		return originalResult;
	}

}
