#include "LindoCrashHandle.h"

#include <atomic>
#include <cstdlib>
#include <cstdint>
#include <exception>
#include <fstream>
#include <mutex>
#include <sstream>
#include <thread>
#include <utility>

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <dbghelp.h>
#pragma comment(lib, "dbghelp.lib")
#endif

namespace {
    std::mutex g_crashMutex;
    std::string g_reportPath = "lindo_crash.log";
    std::string g_applicationName = "Lindo";
    std::atomic<bool> g_installed = false;
    std::atomic<bool> g_reportWritten = false;

    void WriteReport(const std::string& source, const std::string& message) noexcept {
        if (g_reportWritten.exchange(true)) return;

        try {
            std::lock_guard lock(g_crashMutex);
            std::ofstream report(g_reportPath, std::ios::out | std::ios::app);
            if (!report.is_open()) return;

            report << "\n========== " << g_applicationName << " crash ==========" << '\n';
            report << "Source: " << source << '\n';
            report << "Thread: " << std::this_thread::get_id() << '\n';
            report << "Message: " << message << '\n';
            report << "==========================================\n";
        }
        catch (...) {
            // Crash reporting must never throw while handling another failure.
        }
    }

#ifdef _WIN32
    LONG WINAPI HandleWindowsException(EXCEPTION_POINTERS* exceptionInfo) noexcept {
        std::ostringstream message;
        if (exceptionInfo && exceptionInfo->ExceptionRecord) {
            message << "Unhandled SEH exception at 0x"
                << std::hex
                << reinterpret_cast<std::uintptr_t>(exceptionInfo->ExceptionRecord->ExceptionAddress)
                << ", code 0x"
                << exceptionInfo->ExceptionRecord->ExceptionCode;
        }
        else {
            message << "Unhandled Windows exception";
        }

        WriteReport("Windows unhandled exception", message.str());
        return EXCEPTION_EXECUTE_HANDLER;
    }

    void WriteMiniDump(EXCEPTION_POINTERS* exceptionInfo) noexcept {
        HANDLE file = CreateFileA("lindo_crash.dmp", GENERIC_WRITE, 0, nullptr,
            CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
        if (file == INVALID_HANDLE_VALUE) return;

        MINIDUMP_EXCEPTION_INFORMATION exceptionData{};
        exceptionData.ThreadId = GetCurrentThreadId();
        exceptionData.ExceptionPointers = exceptionInfo;
        exceptionData.ClientPointers = FALSE;
        MiniDumpWriteDump(GetCurrentProcess(), GetCurrentProcessId(), file,
            MiniDumpWithIndirectlyReferencedMemory, exceptionInfo ? &exceptionData : nullptr,
            nullptr, nullptr);
        CloseHandle(file);
    }

    LONG WINAPI HandleWindowsExceptionWithDump(EXCEPTION_POINTERS* exceptionInfo) noexcept {
        HandleWindowsException(exceptionInfo);
        WriteMiniDump(exceptionInfo);
        return EXCEPTION_EXECUTE_HANDLER;
    }
#endif

    void HandleTerminate() noexcept {
        const char* message = "std::terminate called";
        try {
            if (auto exception = std::current_exception()) {
                try {
                    std::rethrow_exception(exception);
                }
                catch (const std::exception& error) {
                    message = error.what();
                }
                catch (...) {
                    message = "unknown unhandled C++ exception";
                }
            }
            WriteReport("std::terminate", message);
        }
        catch (...) {
            WriteReport("std::terminate", "failed to inspect active exception");
        }
        std::_Exit(EXIT_FAILURE);
    }
}

namespace Lindo::Core {

    void LindoCrashHandle::Install(const std::string& reportPath) {
        if (g_installed.exchange(true)) return;
        g_reportPath = reportPath;
        std::set_terminate(HandleTerminate);
#ifdef _WIN32
        SetUnhandledExceptionFilter(HandleWindowsExceptionWithDump);
        _set_purecall_handler([]() { WriteReport("pure virtual call", "pure virtual function call"); });
        _set_invalid_parameter_handler([](const wchar_t*, const wchar_t*, const wchar_t*, unsigned int, uintptr_t) {
            WriteReport("invalid parameter", "CRT invalid parameter");
        });
#endif
    }

    void LindoCrashHandle::SetApplicationName(const std::string& name) {
        g_applicationName = name;
    }

    void LindoCrashHandle::ReportException(const char* source, const std::string& message) {
        WriteReport(source ? source : "application", message);
    }

    void LindoCrashHandle::Shutdown() {
        g_installed = false;
    }
}
