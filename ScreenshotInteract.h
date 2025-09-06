#pragma once
#include "pch.h"

namespace ScreenshotInput {

    class ScreenshotInteract {
    public:
        static bool FindSubImage24(const BYTE* largeData, int largeW, int largeH, int strideLarge,
            const BYTE* smallData, int smallW, int smallH, int strideSmall,
            POINT& foundAt, int Xstart, int Ystart);

        static bool LoadBMP24Bit(const wchar_t* filename, std::vector<BYTE>& pixels, int& width, int& height, int& stride);
        static bool SaveWindow10x10BMP(const wchar_t* filename, int x, int y);
        static HBITMAP CaptureWindow(bool draw);
        static bool Buttonaction(const char key[3], int mode, int serchnum, int& startsearch);

    private:
        static int CalculateStride(int width);
        static bool Save24BitBMP(const wchar_t* filename, const BYTE* pixels, int width, int height);
    };

}