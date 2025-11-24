/*
This file is imported from ProtoInput project, created by Ilyaki https://github.com/Ilyaki/ProtoInput
And is modified for GtoMnK usage. All credits to the original author.
*/

#include "pch.h"
#include "MessageFilterHooks.h"
#include "MessageFilters.h"
#include "Logging.h"
#include "Mouse.h"

extern bool g_filterRawInput;
extern bool g_filterMouseMove;
extern bool g_filterMouseActivate;
extern bool g_filterWindowActivate;
extern bool g_filterWindowActivateApp;
extern bool g_filterMouseWheel;
extern bool g_filterMouseButton;
extern bool g_filterKeyboardButton;

namespace GtoMnK
{

	template<typename T>
	bool* IsFilterEnabled()
	{
		// For the INI file.
		if constexpr (std::is_same_v<T, RawInputFilter>) return &g_filterRawInput;
		if constexpr (std::is_same_v<T, MouseMoveFilter>) return &g_filterMouseMove;
		if constexpr (std::is_same_v<T, MouseActivateFilter>) return &g_filterMouseActivate;
		if constexpr (std::is_same_v<T, WindowActivateFilter>) return &g_filterWindowActivate;
		if constexpr (std::is_same_v<T, WindowActivateAppFilter>) return &g_filterWindowActivateApp;
		if constexpr (std::is_same_v<T, MouseWheelFilter>) return &g_filterMouseWheel;
		if constexpr (std::is_same_v<T, MouseButtonFilter>) return &g_filterMouseButton;
		if constexpr (std::is_same_v<T, KeyboardButtonFilter>) return &g_filterKeyboardButton;

		static bool dummy = false; return &dummy;
	}

	template<typename X, typename... T>
	bool MessageFilterAllow(unsigned int message, unsigned int* lparam, unsigned int* wparam, intptr_t hwnd)
	{
		bool allow = true;

		// Check if the specific filter is enabled in the INI file.
		if (*IsFilterEnabled<X>()) {
			if (message >= X::MessageMin() && message <= X::MessageMax()) {
				allow = X::Filter(message, lparam, wparam, hwnd);
			}
		}

		if constexpr (sizeof...(T) != 0) {
			return allow && MessageFilterAllow<T...>(message, lparam, wparam, hwnd);
		}
		else {
			return allow;
		}
	}

	inline BOOL FilterMessage(MSG* lpMsg)
	{
		if (!MessageFilterAllow<GtoMnK_MESSAGE_FILTERS>(lpMsg->message, (unsigned int*)&lpMsg->lParam, (unsigned int*)&lpMsg->wParam, (intptr_t)lpMsg->hwnd))
		{
			memset(lpMsg, 0, sizeof(MSG));
			lpMsg->message = WM_NULL; // For safety
		}

		return TRUE;
	}

	BOOL WINAPI MessageFilterHook::Hook_GetMessageA(LPMSG lpMsg, HWND hWnd, UINT wMsgFilterMin, UINT wMsgFilterMax)
	{
		const auto ret = GetMessageA(lpMsg, hWnd, wMsgFilterMin, wMsgFilterMax);

		return ret == -1 ? -1 : FilterMessage(lpMsg);
	}

	BOOL WINAPI MessageFilterHook::Hook_GetMessageW(LPMSG lpMsg, HWND hWnd, UINT wMsgFilterMin, UINT wMsgFilterMax)
	{
		const auto ret = GetMessageW(lpMsg, hWnd, wMsgFilterMin, wMsgFilterMax);

		return ret == -1 ? -1 : FilterMessage(lpMsg);
	}

	BOOL WINAPI MessageFilterHook::Hook_PeekMessageA(LPMSG lpMsg, HWND hWnd, UINT wMsgFilterMin, UINT wMsgFilterMax, UINT wRemoveMsg)
	{
		const auto ret = PeekMessageA(lpMsg, hWnd, wMsgFilterMin, wMsgFilterMax, wRemoveMsg);

		return ret == FALSE ? FALSE : ((1 + FilterMessage(lpMsg)) / 2);
	}

	BOOL WINAPI MessageFilterHook::Hook_PeekMessageW(LPMSG lpMsg, HWND hWnd, UINT wMsgFilterMin, UINT wMsgFilterMax, UINT wRemoveMsg)
	{
		const auto ret = PeekMessageW(lpMsg, hWnd, wMsgFilterMin, wMsgFilterMax, wRemoveMsg);

		return ret == FALSE ? FALSE : ((1 + FilterMessage(lpMsg)) / 2);
	}

	bool MessageFilterHook::IsKeyboardButtonFilterEnabled()
	{
		return *IsFilterEnabled<KeyboardButtonFilter>();
	}

}
