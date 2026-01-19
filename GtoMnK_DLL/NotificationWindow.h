#pragma once
#include <vector>
#include <string>
#include <mutex>
#include <utility>
#include <gdiplus.h>
#include <condition_variable>

#pragma comment (lib,"Gdiplus.lib")

namespace GtoMnK
{
	class NotificationWindow
	{
		std::mutex mutex;
		std::condition_variable cvWindowCreated;
		bool windowReady = false;

		HWND notifyWindow = nullptr;
		bool isVisible = false;

		ULONGLONG showStartTime = 0;

		// Message System
		int currentMessageID = 0;
		ULONGLONG messageStartTime = 0;
		const int MESSAGE_DURATION = 2000;

		bool tutorialComplete = false;

		ULONG_PTR gdiplusToken;
		std::vector<std::pair<Gdiplus::Image*, IStream*>> tutorialImages;

		float imageAspectRatio = 1.0f;

		// Resources for blinking title
		Gdiplus::Image* iconBack = nullptr;
		IStream* streamBack = nullptr;
		Gdiplus::Image* iconDown = nullptr;
		IStream* streamDown = nullptr;

		const wchar_t* className = L"GtoMnK_Tutorial_Window";

		int currentW = 200;
		int currentH = 150;
		int currentX = 0;
		int currentY = 0;

		static LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

		void DrawUI();
		void CalculateLayout();
		std::wstring GetMessageText(int id);
		std::pair<Gdiplus::Image*, IStream*> LoadImageFromResource(int resourceID, const wchar_t* resourceType);

	public:
		static NotificationWindow state;

		void Initialise();
		void StartInternal();
		void UpdatePositionLoopInternal();
		void LoadTutorialResources(const std::vector<int>& resourceIDs);

		void ShowMessage(int messageID);

		void Show();
		void Hide();
		void Shutdown();
		bool IsVisible() { return isVisible; }
	};
}