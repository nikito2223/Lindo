#include "DebugLogger.h"
#ifdef _WIN32
#include <windows.h>
#endif
#include <functional>

namespace Lindo::Core {

    std::ofstream DebugLogger::logFile;
    std::function<void(LogLevel, const std::string&)> DebugLogger::s_ConsoleCallback = nullptr;
    std::vector<std::pair<LogLevel, std::string>> DebugLogger::s_EarlyLogBuffer;

    void DebugLogger::SetConsoleCallback(std::function<void(LogLevel, const std::string&)> callback) {
        s_ConsoleCallback = callback;

        // Сразу выводим все логи, собранные до инициализации UI
        if (s_ConsoleCallback) {
            for (const auto& [level, msg] : s_EarlyLogBuffer) {
                s_ConsoleCallback(level, msg);
            }
            s_EarlyLogBuffer.clear();
            s_EarlyLogBuffer.shrink_to_fit();
        }
    }
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
        case LogLevel::Debug:    levelStr = "[DEBUG]"; break;
        }

        std::stringstream ss;
        ss << GetTimestamp() << " " << levelStr << " " << message;

        if (file) {
            ss << " | File: " << file << " Line: " << line;
        }

        // Вывод в системную консоль Windows/Linux
        std::cout << ss.str() << std::endl;
        if (logFile.is_open()) {
            logFile << ss.str() << std::endl;
            logFile.flush();
        }

        // Отправка строго в вашу внутреннюю консоль движка (Console.h/cpp)
        if (s_ConsoleCallback) {
            s_ConsoleCallback(level, ss.str());
        }
        else {
            // Если UI еще не загрузился — сохраняем во временный буфер (до 200 строк)
            if (s_EarlyLogBuffer.size() < 200) {
                s_EarlyLogBuffer.push_back({ level, ss.str() });
            }
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
        case LogLevel::Info:     SetConsoleTextAttribute(hConsole, 10); break;
        case LogLevel::Warning:  SetConsoleTextAttribute(hConsole, 14); break;
        case LogLevel::Error:    SetConsoleTextAttribute(hConsole, 12); break;
        case LogLevel::Critical: SetConsoleTextAttribute(hConsole, 79); break;
        case LogLevel::Debug:    SetConsoleTextAttribute(hConsole, 8); break;
        }
#endif
    }

    void DebugLogger::ResetConsoleColor() {
#ifdef _WIN32
        HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
        SetConsoleTextAttribute(hConsole, 7);
#endif
    }
}