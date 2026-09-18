#pragma once
#include <memory>
#include <string>
#include "system/EngineContext.h"
#include "../Window/Window.h"
#include "Debug/DebugLogger.h"

namespace Lindo {

    /**
     * @brief ���������� � ���������� � ������ ������.
     */
    struct AppInfo {
        static constexpr const char* Name = "Lindo";
        static constexpr int VersionMajor = 26;
        static constexpr int VersionMinor = 2;
        static constexpr int VersionPatch = 4;
        static constexpr const char* Stage = "dev";

        /**
         * @brief ��������� ��������� ������������� ������ ������.
         * @return ������ ������� "Major.Minor.Patch-Stage".
         */
        static std::string GetVersionString() {
            return std::to_string(VersionMajor) + "." +
                std::to_string(VersionMinor) + "." +
                std::to_string(VersionPatch) + "-" + Stage;
        }

        /**
         * @brief ��������� ������ ��������� ���� � ��������� ������������ ������.
         * @return ��������� ���� � ����������� �� ������ _DEBUG / Release.
         */
        static std::string GetFormattedTitle() {
            std::string title = std::string(Name);
#ifdef _DEBUG
            title += " [Debug]";
#else
            title += " [Release]";
#endif
            return title;
        }
    };

    /**
     * @brief ������� ����� ����������, ����������� ��������� ������ � �������� ������ ������.
     */
    class Application {
    public:
        /**
         * @brief �����������. �������������� �����������, ������� ������� ���� � �������� ������.
         */
        Application();

        /**
         * @brief ����������. ��������� ������ ������ � ��������� ������.
         */
        ~Application();

        /**
         * @brief ��������� ������� ������� ���� ����������.
         */
        void run();

    private:
        /**
         * @brief ������� ��������� ����� ������ � ���.
         */
        void logAppHeader();

        /**
         * @brief �������� � ������� � ��� ���������� �� ��������� GPU � ������ OpenGL.
         */
        void logGPUInfo();

        /**
         * @brief ������������ ��������� ���������� �������.
         */
        void registerConsoleCommands();

    private:
        std::unique_ptr<Lindo::Window> m_window;      ///< ��������� �� ���� ����������.
        std::unique_ptr<Lindo::EngineContext> m_context; ///< ��������� �� ����������� �������� ������ ������.
    };
}