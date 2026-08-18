#pragma once
#include <iostream>
#include <fstream>
#include <string>
#include <ctime>
#include <sstream>
#include <functional>

namespace Lindo::Core {

    enum class LogLevel {
        Info,
        Warning,
        Error,
        Critical,
        Debug  // Добавляем уровень Debug
    };

    class ConsoleStreamBuffer : public std::streambuf {
    public:
        ConsoleStreamBuffer();
        ~ConsoleStreamBuffer();

    protected:
        virtual int_type overflow(int_type ch) override;
        virtual int sync() override;

    private:
        std::string m_buffer;
        std::streambuf* m_oldCoutBuf = nullptr;
    };

    class DebugLogger {
    public:
        static void Init(const std::string& filename = "engine_log.txt");
        static void Close();

        static void SetConsoleWidget(std::function<void(LogLevel, const std::string&)> callback) {
            s_ConsoleCallback = callback;
        }

        static void SetConsoleCallback(std::function<void(LogLevel, const std::string&)> callback);

        static void Log(LogLevel level, const std::string& message, const char* file = nullptr, int line = -1);
        static void CheckGLState(const std::string& context);
        static void CheckGLError(const char* file, int line);

    private:
        static std::ofstream logFile;
        static std::string GetTimestamp();
        static void SetConsoleColor(LogLevel level);
        static void ResetConsoleColor();
        static std::function<void(LogLevel, const std::string&)> s_ConsoleCallback;
        static std::vector<std::pair<LogLevel, std::string>> s_EarlyLogBuffer;
    };

    // ������� ������� (����� �� ������ ���� � ������ �������)
#define LOG_INFO(msg) Lindo::Core::DebugLogger::Log(Lindo::Core::LogLevel::Info, msg)
#define LOG_WARN(msg) Lindo::Core::DebugLogger::Log(Lindo::Core::LogLevel::Warning, msg, __FILE__, __LINE__)
#define LOG_ERROR(msg) Lindo::Core::DebugLogger::Log(Lindo::Core::LogLevel::Error, msg, __FILE__, __LINE__)
#define LOG_CRITICAL(msg) Lindo::Core::DebugLogger::Log(Lindo::Core::LogLevel::Critical, msg, __FILE__, __LINE__)
#define LOG_DEBUG(msg) Lindo::Core::DebugLogger::Log(Lindo::Core::LogLevel::Debug, msg, __FILE__, __LINE__)  // Добавляем макрос DEBUG
}