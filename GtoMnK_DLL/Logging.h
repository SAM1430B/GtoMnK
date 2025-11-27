#pragma once

#include <fstream>
#include <mutex>
#include <string>

namespace GtoMnK {

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

#if defined(_DEBUG) || defined(ENABLE_LOGGING)
#define INIT_LOGGER()     GtoMnK::Logging::InitializeLogger()
#define SHUTDOWN_LOGGER() GtoMnK::Logging::ShutdownLogger()
#define LOG(...)          GtoMnK::Logging::Log(__VA_ARGS__)

#else
#define INIT_LOGGER()
#define SHUTDOWN_LOGGER()
#define LOG(...)

#endif