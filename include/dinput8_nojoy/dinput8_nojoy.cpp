#include "dinput8_nojoy.hpp"
#include <string>
#include <intrin.h>

#pragma intrinsic(_ReturnAddress)

typedef HRESULT(WINAPI* DirectInput8Create_t)(HINSTANCE, DWORD, REFIID, LPVOID*, LPUNKNOWN);
typedef HRESULT(WINAPI* EnumDevices_t)(LPDIRECTINPUT8W, DWORD, LPDIENUMDEVICESCALLBACKW, LPVOID, DWORD);

HMODULE dinput8_sys = nullptr;
DirectInput8Create_t DirectInput8Create_orig = nullptr;
EnumDevices_t EnumDevices_orig = nullptr;
LPDIENUMDEVICESCALLBACKW EnumDevices_callback_orig = nullptr;

BOOL WINAPI DllMain(HINSTANCE inst, unsigned long reason, void* info) {
    switch (reason) {
    case DLL_PROCESS_ATTACH: {
        const wchar_t* loadDllName = L"dinput8Load.dll";

        dinput8_sys = LoadLibraryW(loadDllName);
        if (!dinput8_sys) {
            wchar_t systemPath[MAX_PATH];
            if (GetSystemDirectoryW(systemPath, MAX_PATH)) {
                std::wstring dinputPath = std::wstring(systemPath) + L"\\dinput8.dll";
                dinput8_sys = LoadLibraryW(dinputPath.c_str());
            }
        }
        if (dinput8_sys) {
            DirectInput8Create_orig = (DirectInput8Create_t)GetProcAddress(dinput8_sys, "DirectInput8Create");
        }

        if (!DirectInput8Create_orig) {
            MessageBoxW(NULL, L"Failed to load any dinput8.dll (System or The Loaded)", L"Proxy Error", MB_ICONERROR);
            return FALSE;
        }

    } break;

    case DLL_PROCESS_DETACH: {
        if (dinput8_sys) {
            FreeLibrary(dinput8_sys);
            dinput8_sys = nullptr;
        }
    } break;
    }
    return TRUE;
}

// The Hooked Callback: Hides controllers from the game but NOT from GtoMnK
BOOL WINAPI EnumDevices_callback_hook(LPCDIDEVICEINSTANCEW device, LPVOID data) {
    void* retAddr = _ReturnAddress();
    HMODULE hCaller = NULL;
    GetModuleHandleExA(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS, (LPCSTR)retAddr, &hCaller);

    // Try to find GtoMnK.dll in memory
#ifdef _W64
    HMODULE hGtoMnK = GetModuleHandleA("GtoMnK64.dll");
#else
    HMODULE hGtoMnK = GetModuleHandleA("GtoMnK32.dll");
#endif

    if (hCaller != NULL && hCaller == hGtoMnK) {
        return EnumDevices_callback_orig(device, data);
    }

    // If the Game (or anything else) is calling, hide the joysticks
    auto type = device->dwDevType & 0x1F;
    if (type == DI8DEVTYPE_MOUSE || type == DI8DEVTYPE_KEYBOARD) {
        // Only let Mouse and Keyboard pass through to the game
        return EnumDevices_callback_orig(device, data);
    }

    return DIENUM_CONTINUE;
}

HRESULT WINAPI EnumDevices_hook(
    LPDIRECTINPUT8W iface,
    DWORD type,
    LPDIENUMDEVICESCALLBACKW callback,
    LPVOID data,
    DWORD flags) {

    EnumDevices_callback_orig = callback;

    return EnumDevices_orig(iface, type, EnumDevices_callback_hook, data, flags);
}

extern "C" long WINAPI DirectInput8Create_hook(
    HINSTANCE inst,
    DWORD ver,
    REFIID id,
    LPVOID* iface,
    LPUNKNOWN aggreg) {

    if (!DirectInput8Create_orig) return E_FAIL;

    auto retval = DirectInput8Create_orig(inst, ver, id, iface, aggreg);
    if (FAILED(retval) || !iface || !*iface) return retval;

    IDirectInput8W* idinput8 = (IDirectInput8W*)*iface;
    auto& EnumDevices_vtable = idinput8->lpVtbl->EnumDevices;

    if (EnumDevices_vtable != (PVOID)EnumDevices_hook) {
        DWORD oldProtect;
        if (VirtualProtect(&EnumDevices_vtable, sizeof(LPVOID), PAGE_READWRITE, &oldProtect)) {

            EnumDevices_orig = (EnumDevices_t)EnumDevices_vtable;

            // Cast to the exact signature required by the VTable to fix C2440
            EnumDevices_vtable = (HRESULT(WINAPI*)(IDirectInput8W*, DWORD, LPDIENUMDEVICESCALLBACKW, LPVOID, DWORD))EnumDevices_hook;

            VirtualProtect(&EnumDevices_vtable, sizeof(LPVOID), oldProtect, &oldProtect);
        }
    }

    return retval;
}