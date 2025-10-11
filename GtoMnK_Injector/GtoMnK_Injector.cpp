#include <windows.h>
#include <string>
#include <iostream>

#define EASYHOOK_STATIC
#include <easyhook.h>

int main(int argc, char* argv[]) {
    if (argc == 2) {
        unsigned int pid = 0;
        try { pid = std::stoi(argv[1]); }
        catch (...) { return 1; }

        wchar_t pathChars[MAX_PATH] = { 0 };
        GetModuleFileNameW(NULL, pathChars, MAX_PATH);
        std::wstring folderPath = pathChars;
        size_t pos = folderPath.find_last_of(L"\\");
        if (pos == std::string::npos) return 2;
        folderPath = folderPath.substr(0, pos + 1);

#ifdef _WIN64
        const std::wstring dllToInject = folderPath + L"GtoMnK64.dll";
#else
        const std::wstring dllToInject = folderPath + L"GtoMnK32.dll";
#endif

        return RhInjectLibrary(pid, 0, EASYHOOK_INJECT_DEFAULT, (WCHAR*)dllToInject.c_str(), NULL, NULL, 0);
    }
    // Startup Injection: InjectorBridge.exe <ExePath> <DllPath32> <DllPath64>
    else if (argc == 4) {
        std::string exePath_s = argv[1];
        std::string dllPath32_s = argv[2];
        std::string dllPath64_s = argv[3];

        std::wstring exePath(exePath_s.begin(), exePath_s.end());
        std::wstring dllPath32(dllPath32_s.begin(), dllPath32_s.end());
        std::wstring dllPath64(dllPath64_s.begin(), dllPath64_s.end());

        ULONG processId = 0;
        return RhCreateAndInject((WCHAR*)exePath.c_str(), NULL, 0, EASYHOOK_INJECT_DEFAULT,
            (WCHAR*)dllPath32.c_str(), (WCHAR*)dllPath64.c_str(),
            NULL, 0, &processId);
    }

    return 99;
}