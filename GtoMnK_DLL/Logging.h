#pragma once

#include <mutex>
#include <fstream>
#include <string>

namespace GtoMnK {

    class Logging {
    public:
        static void InitializeLogger();
        static void ShutdownLogger();
        static void Log(const char* file, int line, const char* format, ...);

    private:
        static std::ofstream g_logFile;
        static std::mutex g_logMutex;
        static std::string UGetExecutableFolder();
    };

}

extern bool enableDev;

#define INIT_LOGGER()     do { if (enableDev) GtoMnK::Logging::InitializeLogger(); } while(0)
#define SHUTDOWN_LOGGER() do { if (enableDev) GtoMnK::Logging::ShutdownLogger(); } while(0)
#define LOG(...)          do { if (enableDev) GtoMnK::Logging::Log(__FILE__, __LINE__, __VA_ARGS__); } while(0)