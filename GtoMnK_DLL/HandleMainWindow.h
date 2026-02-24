#pragma once
#include "pch.h"

HWND GetMainWindowHandle(DWORD targetPID, const char* requiredName, const char* requiredClass, DWORD timeoutMS);