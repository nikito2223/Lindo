#pragma once
#include <iostream>
#include <fstream>
#include <string>
#include <ctime>
#include <sstream>

namespace Lindo::Core {

    enum class LogLevel {
        Info,
        Warning,
        Error,
        Critical,
        Debug  // Добавляем уровень Debug
    };

    class DebugLogger {
    public:
        static void Init(const std::string& filename = "engine_log.txt");
        static void Close();

        // �������� ����� ��� �����������
        static void Log(LogLevel level, const std::string& message, const char* file = nullptr, int line = -1);
        static void CheckGLState(const std::string& context);
        static void CheckGLError(const char* file, int line);

    private:
        static std::ofstream logFile;
        static std::string GetTimestamp();
        static void SetConsoleColor(LogLevel level);
        static void ResetConsoleColor();
    };

    // ������� ������� (����� �� ������ ���� � ������ �������)
#define LOG_INFO(msg) Lindo::Core::DebugLogger::Log(Lindo::Core::LogLevel::Info, msg)
#define LOG_WARN(msg) Lindo::Core::DebugLogger::Log(Lindo::Core::LogLevel::Warning, msg, __FILE__, __LINE__)
#define LOG_ERROR(msg) Lindo::Core::DebugLogger::Log(Lindo::Core::LogLevel::Error, msg, __FILE__, __LINE__)
#define LOG_CRITICAL(msg) Lindo::Core::DebugLogger::Log(Lindo::Core::LogLevel::Critical, msg, __FILE__, __LINE__)
#define LOG_DEBUG(msg) Lindo::Core::DebugLogger::Log(Lindo::Core::LogLevel::Debug, msg, __FILE__, __LINE__)  // Добавляем макрос DEBUG
}