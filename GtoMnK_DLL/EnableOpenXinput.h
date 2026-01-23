#pragma once
#include <Windows.h>
#include <Xinput.h>

typedef DWORD(WINAPI* XInputGetStateFunc)(DWORD, XINPUT_STATE*);

extern XInputGetStateFunc pXInputGetState;
extern bool g_EnableOpenXinput;