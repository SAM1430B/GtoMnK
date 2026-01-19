#include "pch.h"
#include "NotificationWindow.h"
#include "MainThread.h"
#include "Logging.h"
#include <shlwapi.h>
#pragma comment(lib, "shlwapi.lib")

#define WM_MOVE_notifyWindow (WM_APP + 3)

namespace GtoMnK
{
	NotificationWindow NotificationWindow::state;

	LRESULT WINAPI NotificationWindow::WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
	{
		switch (msg)
		{
		case WM_MOVE_notifyWindow:
			if (state.isVisible) state.DrawUI();
			break;
		case WM_NCHITTEST:
		{
			LRESULT hit = DefWindowProc(hWnd, msg, wParam, lParam);
			if (hit == HTCLIENT) return HTCAPTION;
			return hit;
		}
		case WM_KEYDOWN:
			if (wParam == VK_ESCAPE) state.Hide();
			break;
		case WM_CLOSE:
			state.Hide();
			return 0;
		}
		return DefWindowProc(hWnd, msg, wParam, lParam);
	}

	void NotificationWindow::Initialise()
	{
		Sleep(5000); // Wait for game to stabilize
		Gdiplus::GdiplusStartupInput gdiplusStartupInput;
		Gdiplus::GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, NULL);

		LOG("Initializing Notification Window...");

		CreateThread(nullptr, 0, [](LPVOID) -> DWORD {
			NotificationWindow::state.StartInternal();
			return 0;
			}, nullptr, 0, 0);

		std::unique_lock<std::mutex> lock(mutex);
		cvWindowCreated.wait(lock, [] { return state.windowReady; });
	}

	void NotificationWindow::StartInternal()
	{
		WNDCLASSW wc = { 0 };
		wc.lpfnWndProc = WndProc;
		wc.hInstance = g_hModule;
		wc.hCursor = LoadCursor(NULL, IDC_ARROW);
		wc.lpszClassName = className;

		RegisterClassW(&wc);

		notifyWindow = CreateWindowExW(
			WS_EX_TOPMOST | WS_EX_TOOLWINDOW | WS_EX_LAYERED | WS_EX_NOACTIVATE,
			className, L"", WS_POPUP,
			0, 0, 100, 100,
			hwnd,
			nullptr, g_hModule, nullptr);

		{
			std::lock_guard<std::mutex> lock(mutex);
			windowReady = true;
		}
		cvWindowCreated.notify_one();

		CreateThread(nullptr, 0, [](LPVOID) -> DWORD {
			NotificationWindow::state.UpdatePositionLoopInternal();
			return 0;
			}, nullptr, 0, 0);

		MSG msg;
		while (GetMessage(&msg, nullptr, 0, 0))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);

			if (isVisible && !IsWindowVisible(notifyWindow)) {
				DrawUI();
				ShowWindow(notifyWindow, SW_SHOWNOACTIVATE);
			}
			else if (!isVisible && IsWindowVisible(notifyWindow)) {
				ShowWindow(notifyWindow, SW_HIDE);
			}
		}

		Gdiplus::GdiplusShutdown(gdiplusToken);
	}

	void NotificationWindow::UpdatePositionLoopInternal()
	{
		while (true)
		{
			if (isVisible)
			{
				// Message Timer Logic
				if (currentMessageID != 0)
				{
					if (GetTickCount64() - messageStartTime > MESSAGE_DURATION)
					{
						currentMessageID = 0;
						if (tutorialComplete) {
							Hide(); // Only hide if tutorial is done.
						}
						else {
							PostMessage(notifyWindow, WM_MOVE_notifyWindow, 0, 0);
						}
					}
				}
				else
				{
					if (GetTickCount64() - showStartTime > 5000)
					{
						tutorialComplete = true;
						Hide();
					}
				}

				if (notifyWindow && IsWindow(notifyWindow))
				{
					PostMessage(notifyWindow, WM_MOVE_notifyWindow, 0, 0);
				}
			}
			Sleep(500);
		}
	}

	void NotificationWindow::ShowMessage(int messageID)
	{
		if (!tutorialComplete) return;

		currentMessageID = messageID;
		messageStartTime = GetTickCount64();
		if (!isVisible) Show();
		if (notifyWindow) PostMessage(notifyWindow, WM_MOVE_notifyWindow, 0, 0);
	}

	std::wstring NotificationWindow::GetMessageText(int id) {
		switch (id) {
		case 1: return L"KEYBOARD MODE";
		case 2: return L"CURSOR MODE";
		case 69: return L"DISABLED";
		case 70: return L"ENABLED";
		case 12: return L"CONTROLLER DISCONNECTED";
		default: return L"";
		}
	}

	struct HighlightSpot {
		float relX; // 0.0 = Left edge, 1.0 = Right edge
		float relY; // 0.0 = Top edge, 1.0 = Bottom edge
		float relW; // Width relative to image width
		float relH; // Height relative to image height
	};

	void NotificationWindow::DrawUI()
	{
		if (!notifyWindow) return;

		CalculateLayout();

		int w = currentW;
		int h = currentH;

		// GDI+ Buffer
		BITMAPINFO bmi = { 0 };
		bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
		bmi.bmiHeader.biWidth = w;
		bmi.bmiHeader.biHeight = -h;
		bmi.bmiHeader.biPlanes = 1;
		bmi.bmiHeader.biBitCount = 32;
		bmi.bmiHeader.biCompression = BI_RGB;

		void* pBits = nullptr;
		HBITMAP hBitmap = CreateDIBSection(NULL, &bmi, DIB_RGB_COLORS, &pBits, NULL, 0);
		if (!hBitmap || !pBits) return;

		Gdiplus::Bitmap buffer(w, h, w * 4, PixelFormat32bppPARGB, (BYTE*)pBits);
		Gdiplus::Graphics graphics(&buffer);

		graphics.SetSmoothingMode(Gdiplus::SmoothingModeAntiAlias);
		graphics.SetInterpolationMode(Gdiplus::InterpolationModeHighQualityBicubic);
		graphics.SetTextRenderingHint(Gdiplus::TextRenderingHintAntiAliasGridFit);

		// Background
		graphics.Clear(Gdiplus::Color(200, 30, 30, 30));

		if (currentMessageID != 0) {
			std::wstring msg = GetMessageText(currentMessageID);

			// Simple centered text for status messages
			Gdiplus::FontFamily fontFamily(L"Segoe UI");
			Gdiplus::Font font(&fontFamily, 32, Gdiplus::FontStyleBold, Gdiplus::UnitPixel);
			Gdiplus::SolidBrush textBrush(Gdiplus::Color(255, 255, 255, 255));

			Gdiplus::StringFormat format;
			format.SetAlignment(Gdiplus::StringAlignmentCenter);
			format.SetLineAlignment(Gdiplus::StringAlignmentCenter);

			Gdiplus::RectF layoutRect(0, 0, (float)w, (float)h);
			graphics.DrawString(msg.c_str(), -1, &font, layoutRect, &format, &textBrush);
		}
		else {
			// Header
			int headerH = (int)(h * 0.15f);
			if (headerH < 30) headerH = 30;

			Gdiplus::SolidBrush headerBrush(Gdiplus::Color(240, 45, 45, 48));
			graphics.FillRectangle(&headerBrush, 0, 0, w, headerH);

			// Title Text
			float fontSize = (float)headerH * 0.40f;
			Gdiplus::FontFamily fontFamily(L"Segoe UI");
			Gdiplus::Font font(&fontFamily, fontSize, Gdiplus::FontStyleBold, Gdiplus::UnitPixel);
			Gdiplus::SolidBrush textBrush(Gdiplus::Color(255, 255, 255, 255));

			graphics.DrawString(
				L"Press (Back) + (D-Pad Down) to open the overlay",
				-1, &font, Gdiplus::PointF(10.0f, (float)headerH * 0.25f), &textBrush);

			// Draw Image AND Blinking Shapes
			{
				std::lock_guard<std::mutex> lock(mutex);

				std::vector<HighlightSpot> spots;
				//	spots.push_back({X, Y, W, H});
				spots.push_back({ 0.372f, 0.469f, 0.055f, 0.055f }); // Back
				spots.push_back({ 0.3425f, 0.73f, 0.04f, 0.07f });  // D-Pad Down

				// Blink Timer: 600ms ON, 400ms OFF
				bool blinkOn = (GetTickCount64() % 1000) < 600;

				// Brush for the glow (Semi-transparent Yellow/Orange)
				Gdiplus::SolidBrush blinkBrush(Gdiplus::Color(180, 255, 200, 0));

				float currentY = (float)headerH + 10.0f;
				float availableH = (float)h - currentY - 10.0f;

				for (auto& pair : tutorialImages)
				{
					Gdiplus::Image* img = pair.first;
					if (img)
					{
						// Calculate Draw Dimensions
						float drawW = (float)w - 20.0f;
						float drawH = availableH;
						float scale = drawW / (float)img->GetWidth();
						drawH = (float)img->GetHeight() * scale;

						if (drawH > availableH) {
							drawH = availableH;
							scale = drawH / (float)img->GetHeight();
							drawW = (float)img->GetWidth() * scale;
						}

						float drawX = ((float)w - drawW) / 2.0f;
						Gdiplus::RectF destRect(drawX, currentY, drawW, drawH);

						// Draw the Controller Image
						graphics.DrawImage(img, destRect);

						// Draw Blinking Spots ON TOP
						if (blinkOn) {
							for (const auto& spot : spots) {
								float spotX = destRect.X + (destRect.Width * spot.relX);
								float spotY = destRect.Y + (destRect.Height * spot.relY);
								float spotW = destRect.Width * spot.relW;
								float spotH = destRect.Height * spot.relH;
								graphics.FillEllipse(&blinkBrush, spotX, spotY, spotW, spotH);
							}
						}
					}
				}
			}
		}

		// Update Window
		HDC screenDC = GetDC(NULL);
		HDC memDC = CreateCompatibleDC(screenDC);
		HBITMAP oldBitmap = (HBITMAP)SelectObject(memDC, hBitmap);
		SIZE sizeWnd = { w, h };
		POINT ptSrc = { 0, 0 };
		POINT ptWinPos = { currentX, currentY };
		BLENDFUNCTION blend = { 0 };
		blend.BlendOp = AC_SRC_OVER;
		blend.SourceConstantAlpha = 255;
		blend.AlphaFormat = AC_SRC_ALPHA;

		UpdateLayeredWindow(notifyWindow, screenDC, &ptWinPos, &sizeWnd, memDC, &ptSrc, 0, &blend, ULW_ALPHA);

		SelectObject(memDC, oldBitmap);
		DeleteObject(hBitmap);
		DeleteDC(memDC);
		ReleaseDC(NULL, screenDC);
	}

	std::pair<Gdiplus::Image*, IStream*> NotificationWindow::LoadImageFromResource(int resourceID, const wchar_t* resourceType)
	{
		HRSRC hResource = FindResourceW(g_hModule, MAKEINTRESOURCEW(resourceID), resourceType);
		if (!hResource) return { nullptr, nullptr };
		DWORD imageSize = SizeofResource(g_hModule, hResource);
		HGLOBAL hGlobal = LoadResource(g_hModule, hResource);
		if (!hGlobal) return { nullptr, nullptr };
		void* pData = LockResource(hGlobal);
		IStream* pStream = nullptr;
		HGLOBAL hBuffer = GlobalAlloc(GMEM_MOVEABLE, imageSize);
		if (hBuffer) {
			void* pBuffer = GlobalLock(hBuffer);
			if (pBuffer) {
				CopyMemory(pBuffer, pData, imageSize);
				GlobalUnlock(hBuffer);
				CreateStreamOnHGlobal(hBuffer, TRUE, &pStream);
			}
		}
		if (!pStream) return { nullptr, nullptr };
		Gdiplus::Image* pImage = new Gdiplus::Image(pStream);
		if (pImage->GetLastStatus() != Gdiplus::Ok) {
			delete pImage;
			pStream->Release();
			return { nullptr, nullptr };
		}
		return { pImage, pStream };
	}

	void NotificationWindow::LoadTutorialResources(const std::vector<int>& resourceIDs)
	{
		std::lock_guard<std::mutex> lock(mutex);
		for (int id : resourceIDs)
		{
			auto result = LoadImageFromResource(id, L"PNG");
			if (result.first) {
				tutorialImages.push_back(result);
				if (tutorialImages.size() == 1) {
					float w = (float)result.first->GetWidth();
					float h = (float)result.first->GetHeight();
					if (h > 0) imageAspectRatio = w / h;
				}
			}
		}
		if (isVisible) DrawUI();
	}

	void NotificationWindow::Show() {
		showStartTime = GetTickCount64();
		isVisible = true;
		if (notifyWindow) PostMessage(notifyWindow, WM_NULL, 0, 0);
	}

	void NotificationWindow::Hide() {
		isVisible = false;
		if (notifyWindow) PostMessage(notifyWindow, WM_NULL, 0, 0);
	}

	void NotificationWindow::CalculateLayout()
	{
		if (!hwnd || !IsWindow(hwnd)) return;
		RECT gameRect;
		if (!GetClientRect(hwnd, &gameRect)) return;
		int gameW = gameRect.right - gameRect.left;
		int gameH = gameRect.bottom - gameRect.top;
		int targetHeight = (int)(gameH * 0.40f);
		if (targetHeight < 200) targetHeight = 200;
		if (targetHeight > 1200) targetHeight = 1200;
		int targetWidth = (int)((float)targetHeight * imageAspectRatio);
		if (targetWidth < 300) targetWidth = 300;
		currentW = targetWidth;
		currentH = targetHeight;
		POINT ptBottomRight = { gameRect.right, gameRect.bottom };
		ClientToScreen(hwnd, &ptBottomRight);
		int margin = 30;
		currentX = ptBottomRight.x - targetWidth - margin;
		currentY = ptBottomRight.y - targetHeight - margin;
	}

	void NotificationWindow::Shutdown() {
		std::lock_guard<std::mutex> lock(mutex);
		for (auto& pair : tutorialImages) {
			if (pair.first) delete pair.first;
			if (pair.second) pair.second->Release();
		}
		tutorialImages.clear();
		if (iconBack) delete iconBack;
		if (streamBack) streamBack->Release();
		if (iconDown) delete iconDown;
		if (streamDown) streamDown->Release();
		if (notifyWindow) DestroyWindow(notifyWindow);
	}
}