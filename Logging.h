#pragma once

#include <fstream>
#include <mutex>
#include <string>

namespace ScreenshotInput {

    class Logging {
    public:
        static void InitializeLogger();
        static void ShutdownLogger();
        static void Log(const char* format, ...);

    private:
        static std::ofstream g_logFile;
        static std::mutex g_logMutex;
        static std::string UGetExecutableFolder();
    };

}

#ifdef _DEBUG
#define INIT_LOGGER()     ScreenshotInput::Logging::InitializeLogger()
#define SHUTDOWN_LOGGER() ScreenshotInput::Logging::ShutdownLogger()
#define LOG(...)          ScreenshotInput::Logging::Log(__VA_ARGS__)

#else
#define INIT_LOGGER()
#define SHUTDOWN_LOGGER()
#define LOG(...)

#endif