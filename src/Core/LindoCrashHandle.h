#pragma once

#include <string>

namespace Lindo::Core {

    class LindoCrashHandle {
    public:
        static void Install(const std::string& reportPath = "lindo_crash.log");
        static void SetApplicationName(const std::string& name);
        static void ReportException(const char* source, const std::string& message);
        static void Shutdown();

    private:
        LindoCrashHandle() = delete;
    };
}
