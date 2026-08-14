#include "DebugLogger.h"
#ifdef _WIN32
#include <windows.h>
#endif

namespace Lindo::Core {

    std::ofstream DebugLogger::logFile;

    void DebugLogger::Init(const std::string& filename) {
        logFile.open(filename, std::ios::out | std::ios::trunc);
        LOG_INFO("--- Logger Initialized ---");
    }

    void DebugLogger::Close() {
        if (logFile.is_open()) {
            LOG_INFO("--- Logger Closed ---");
            logFile.close();
        }
    }

    void DebugLogger::Log(LogLevel level, const std::string& message, const char* file, int line) {
        std::string levelStr;
        SetConsoleColor(level);

        switch (level) {
        case LogLevel::Info:     levelStr = "[INFO]"; break;
        case LogLevel::Warning:  levelStr = "[WARN]"; break;
        case LogLevel::Error:    levelStr = "[ERROR]"; break;
        case LogLevel::Critical: levelStr = "[CRIT]"; break;
        case LogLevel::Debug:    levelStr = "[DEBUG]"; break;  // Добавляем обработку Debug
        }

        std::stringstream ss;
        ss << GetTimestamp() << " " << levelStr << " " << message;

        if (file) {
            ss << " | File: " << file << " Line: " << line;
        }

        // ����� � �������
        std::cout << ss.str() << std::endl;

        // ������ � ����
        if (logFile.is_open()) {
            logFile << ss.str() << std::endl;
            logFile.flush(); // ���������� ����� �����, ����� ��� ������ ��� ����������
        }

        ResetConsoleColor();
    }

    std::string DebugLogger::GetTimestamp() {
        std::time_t now = std::time(nullptr);
        char buf[20];
        std::strftime(buf, sizeof(buf), "%H:%M:%S", std::localtime(&now));
        return std::string(buf);
    }

    void DebugLogger::SetConsoleColor(LogLevel level) {
#ifdef _WIN32
        HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
        switch (level) {
        case LogLevel::Info:     SetConsoleTextAttribute(hConsole, 10); break; // зеленый
        case LogLevel::Warning:  SetConsoleTextAttribute(hConsole, 14); break; // желтый
        case LogLevel::Error:    SetConsoleTextAttribute(hConsole, 12); break; // красный
        case LogLevel::Critical: SetConsoleTextAttribute(hConsole, 79); break; // белый на красном
        case LogLevel::Debug:    SetConsoleTextAttribute(hConsole, 8); break;  // серый (для отладочных сообщений)
        }
#endif
    }

    void DebugLogger::ResetConsoleColor() {
#ifdef _WIN32
        HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
        SetConsoleTextAttribute(hConsole, 7); // ����������� ����� (белый)
#endif
    }
}