// pch.h: This is a precompiled header file.
// Files listed below are compiled only once, improving build performance for future builds.
// This also affects IntelliSense performance, including code completion and many code browsing features.
// However, files listed here are ALL re-compiled if any one of them is updated between builds.
// Do not add files here that you will be updating frequently as this negates the performance advantage.

#ifndef PCH_H
#define PCH_H

// Windows Headers
#define NOMINMAX
#include <windows.h>
#include <hidusage.h>

// Standard C++ Headers
#include <iostream>
#include <vector>
#include <string>
#include <fstream>
#include <mutex>
#include <chrono>
#include <iomanip>
#include <cmath>
#include <algorithm>
#include <cstdio>
#include <cstdlib>

// Other Libraries
//#include <Xinput.h>
//#pragma comment(lib, "Xinput9_1_0.lib")

#include <dwmapi.h>
#include <psapi.h>
#include <tchar.h>
#include <tlhelp32.h>

// Project-specific
//#include "MinHook.h"
#include <easyhook.h>

// OpenXinput
#define OPENXINPUT_XUSER_MAX_COUNT 64
#include "OpenXinput.h"

#endif