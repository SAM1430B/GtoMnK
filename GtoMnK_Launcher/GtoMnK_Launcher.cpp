#include <iostream>
#include <string>
#include <windows.h>
#include <vector>

std::wstring GetCurrentExeFolder() {
    wchar_t pathChars[MAX_PATH] = { 0 };
    GetModuleFileNameW(NULL, pathChars, MAX_PATH);
    std::wstring exePath = pathChars;
    size_t pos = exePath.find_last_of(L"\\");
    if (pos != std::wstring::npos) {
        return exePath.substr(0, pos + 1);
    }
    return L"";
}

bool IsProcess64Bit(unsigned int pid) {
    HANDLE hProcess = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, pid);
    if (!hProcess) return false;
    BOOL isWow64;
    IsWow64Process(hProcess, &isWow64);
    CloseHandle(hProcess);
#ifdef _WIN64
    return !isWow64;
#else
    return false;
#endif
}

// Helper to check if an EXE file is 64-bit
bool IsExe64Bit(const std::wstring& exePath) {
    DWORD binaryType;
    if (GetBinaryTypeW(exePath.c_str(), &binaryType)) {
        return (binaryType == SCS_64BIT_BINARY);
    }
    return false; // Assume 32-bit on failure
}

// Helper to get a human-readable string for EasyHook's NTSTATUS codes
std::string GetNtStatusString(DWORD code) {
    switch (code) {
    case 0: return "Success!";
    case 1:
    case 2:
    case 99: return "Launcher Error: Invalid arguments passed to bridge.";
    case 0xC0000022: return "Access Denied. Try running the launcher as an Administrator.";
    case 0xC0000034: return "Object Name Not Found. (Is the PID correct?)";
    default: {
        char buffer[256];
        sprintf_s(buffer, "Unknown EasyHook NTSTATUS code: 0x%08X", code);
        return buffer;
    }
    }
}

std::string WStringToString(const std::wstring& wstr) {
    int size_needed = WideCharToMultiByte(CP_UTF8, 0, &wstr[0], (int)wstr.size(), NULL, 0, NULL, NULL);
    std::string strTo(size_needed, 0);
    WideCharToMultiByte(CP_UTF8, 0, &wstr[0], (int)wstr.size(), &strTo[0], size_needed, NULL, NULL);
    return strTo;
}

void InjectOnStartup() {
    std::wcout << L"\n--- Startup Injection ---\n";
    std::wcout << L"Enter the full path to the game's executable (.exe): ";
    std::wstring exePath;
    std::getline(std::wcin, exePath);
    if (!exePath.empty() && exePath.front() == L'"') { exePath.erase(0, 1); exePath.pop_back(); }

    std::wstring launcherFolder = GetCurrentExeFolder();
    if (launcherFolder.empty()) {
        std::cerr << "Error: Could not determine the launcher's own directory.\n";
        return;
    }

    std::wstring dllPath32 = launcherFolder + L"GtoMnK32.dll";
    std::wstring dllPath64 = launcherFolder + L"GtoMnK64.dll";

    std::wcout << L"Automatically using payload DLLs from launcher directory.\n";

    bool targetIs64Bit = IsExe64Bit(exePath);
    std::string bridgeExeName = targetIs64Bit ? "GtoMnK_IJ64.exe" : "GtoMnK_IJ32.exe";

    std::string commandLineStr = "\"" + bridgeExeName + "\" \"" + WStringToString(exePath) + "\" \"" + WStringToString(dllPath32) + "\" \"" + WStringToString(dllPath64) + "\"";

    std::vector<char> commandLineVec(commandLineStr.begin(), commandLineStr.end());
    commandLineVec.push_back('\0');

    STARTUPINFOA si = { sizeof(si) };
    PROCESS_INFORMATION pi = {};

    if (CreateProcessA(NULL, commandLineVec.data(), NULL, NULL, FALSE, CREATE_NO_WINDOW, NULL, NULL, &si, &pi)) {
        WaitForSingleObject(pi.hProcess, 5000);
        DWORD exitCode;
        GetExitCodeProcess(pi.hProcess, &exitCode);
        std::cout << "Result: " << GetNtStatusString(exitCode) << "\n";
        CloseHandle(pi.hProcess); CloseHandle(pi.hThread);
    }
    else {
        std::cerr << "Failed to launch the injector bridge: " << bridgeExeName << ". Error: " << GetLastError() << "\n";
    }
}

void InjectAtRuntime() {
    std::cout << "\n--- Runtime Injection ---\n";
    std::cout << "Enter the Process ID (PID) of the running game: ";
    unsigned int pid;
    std::cin >> pid;
    if (!std::cin) { std::cin.clear(); std::cin.ignore(10000, '\n'); std::cerr << "Invalid input.\n"; return; }
    std::cin.ignore(10000, '\n');
    if (pid == 0) return;

    bool targetIs64Bit = IsProcess64Bit(pid);
    std::string bridgeExeName = targetIs64Bit ? "GtoMnK_IJ64.exe" : "GtoMnK_IJ32.exe";

    std::string commandLineStr = bridgeExeName + " " + std::to_string(pid);

    std::vector<char> commandLineVec(commandLineStr.begin(), commandLineStr.end());
    commandLineVec.push_back('\0');

    STARTUPINFOA si = { sizeof(si) };
    PROCESS_INFORMATION pi = {};

    if (CreateProcessA(NULL, commandLineVec.data(), NULL, NULL, FALSE, CREATE_NO_WINDOW, NULL, NULL, &si, &pi)) {
        WaitForSingleObject(pi.hProcess, 5000);
        DWORD exitCode;
        GetExitCodeProcess(pi.hProcess, &exitCode);
        if (exitCode == 0) {
            std::cout << "Result: Success! Injection reported a clean exit.\n";
        }
        else if (exitCode == 0xC00000F2) {
            std::cout << "Result: Injection likely SUCCEEDED, but reported a finalization error (0xC00000F2).\n";
            std::cout << "This is common. If the mod is working in-game, you can safely ignore this message.\n";
            std::cout << "Ensure EasyHook32.dll (or 64) is in the same folder as the game's .exe.\n";
        }
        else {
            std::cerr << "Result: Injection FAILED. " << GetNtStatusString(exitCode) << "\n";
        }

        CloseHandle(pi.hProcess); CloseHandle(pi.hThread);
    }
    else {
        std::cerr << "Failed to launch the injector bridge: " << bridgeExeName << ". Error: " << GetLastError() << "\n";
    }
}

int main() {
    SetConsoleTitleA("GtoMnK Launcher");
    while (true) {
        system("cls");
        std::cout << "\n================================\n";
        std::cout << "          GtoMnK Launcher\n";
        std::cout << "================================\n\n";
        std::cout << "  1. Startup   (Launch and Inject)\n";
        std::cout << "  2. Runtime   (Inject into Running Process)\n";
        std::cout << "  3. Exit\n\n";
        std::cout << "Selection: ";
        char choice;
        std::cin >> choice;
        if (!std::cin) { std::cin.clear(); std::cin.ignore(10000, '\n'); continue; }
        std::cin.ignore(10000, '\n');

        switch (choice) {
        case '1': InjectOnStartup(); break;
        case '2': InjectAtRuntime(); break;
        case '3': return 0;
        default: std::cout << "Invalid selection.\n"; break;
        }
		Sleep(3500);
		return 0;
    }
    return 0;
}