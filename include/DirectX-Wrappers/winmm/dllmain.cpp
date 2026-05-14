/*
* Copyright (c) 2012 Toni Spets <toni.spets@iki.fi>
*
* Permission to use, copy, modify, and distribute this software for any
* purpose with or without fee is hereby granted, provided that the above
* copyright notice and this permission notice appear in all copies.
*
* THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
* WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
* MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
* ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
* WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
* ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
* OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
*/

#include <windows.h>
#include <ctype.h>
#include <stdlib.h>

#include "..\Common\Logging.h"
#include "ChainLoad.h"

std::ofstream Log::LOG("winmm.log");

bool WINAPI DllMain(HMODULE hModule, DWORD dwReason, LPVOID lpReserved)
{
    static HMODULE winmmdll = nullptr;

    switch (dwReason)
    {
    case DLL_PROCESS_ATTACH:
        DisableThreadLibraryCalls(hModule);

        // Load dll
        char path[MAX_PATH];
        if (GetFileAttributesA("winmm.Chained.dll") != INVALID_FILE_ATTRIBUTES)
        {
            // If winmm.Chained.dll exists, load it
            strcpy_s(path, "winmm.Chained.dll");
        }
        else
        {
            // Load system winmm.dll
            GetSystemDirectoryA(path, MAX_PATH);
            strcat_s(path, "\\winmm.dll");
        }
        Log() << "Loading " << path;
        winmmdll = LoadLibraryA(path);

        // Load GtoMnK dll
#ifdef _WIN64
        if (LoadLibraryA("GtoMnK64.dll"))
        {
            Log() << "Successfully loaded GtoMnK64.dll";
        }
        else
        {
            Log() << "Failed to load GtoMnK64.dll. Error code: " << GetLastError();
        }
#else
        if (LoadLibraryA("GtoMnK32.dll"))
        {
            Log() << "Successfully loaded GtoMnK32.dll";
        }
        else
        {
            Log() << "Failed to load GtoMnK32.dll. Error code: " << GetLastError();
        }
#endif
        // Load any .ChainLoad$.dll files
        ChainLoader::LoadDlls();
        break;

    case DLL_PROCESS_DETACH:
        if (winmmdll)
        {
            FreeLibrary(winmmdll);
        }
        break;
    }

    return true;
}