#include "pch.h"
#include "Logging.h"

#include <cstdarg>
#include <chrono>
#include <iomanip>

namespace GtoMnK {

    std::ofstream Logging::g_logFile;
    std::mutex Logging::g_logMutex;

    std::string Logging::UGetExecutableFolder() {
        char path[MAX_PATH];
        GetModuleFileNameA(NULL, path, MAX_PATH);
        std::string exePath(path);
        size_t lastSlash = exePath.find_last_of("\\/");
        return exePath.substr(0, lastSlash);
    }

    void Logging::InitializeLogger() {
        std::string logFilePath = UGetExecutableFolder() + "\\GtoMnK_Log.txt";
        g_logFile.open(logFilePath, std::ios_base::out | std::ios_base::app);
        if (!g_logFile.is_open()) {
            MessageBoxA(NULL, "Failed to open log file!", "Logging Error", MB_OK | MB_ICONERROR);
        }
    }

    void Logging::ShutdownLogger() {
        if (g_logFile.is_open()) {
            g_logFile.close();
        }
    }

    void Logging::Log(const char* file, int line, const char* format, ...) {
        std::lock_guard<std::mutex> lock(g_logMutex);
        if (!g_logFile.is_open()) return;

        auto now = std::chrono::system_clock::now();
        auto in_time_t = std::chrono::system_clock::to_time_t(now);
        struct tm time_info;
        localtime_s(&time_info, &in_time_t);

        const char* fileName = strrchr(file, '\\');
        if (!fileName) fileName = strrchr(file, '/');
        fileName = fileName ? fileName + 1 : file;

        char timeStr[16];
        strftime(timeStr, sizeof(timeStr), "%T", &time_info);

        char prefixBuffer[128];
        snprintf(prefixBuffer, sizeof(prefixBuffer), "[%s] [%s:%d]", timeStr, fileName, line);

        char messageBuffer[1024];
        va_list args;
        va_start(args, format);
        vsnprintf(messageBuffer, sizeof(messageBuffer), format, args);
        va_end(args);

        g_logFile << std::left << std::setw(40) << prefixBuffer << " " << messageBuffer << std::endl;
    }

}