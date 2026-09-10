#pragma once
#include <string>
#include <vector>
#include <deque>
#include <functional>
#include <unordered_map>
#include "Core/OGL.h"
#include "../Graphics/ui/UIRenderer.h"
#include "../Graphics/ui/UIFont.h"
#include "DebugLogger.h"

namespace Lindo {
    namespace Debug {

        // ���������� ������� �������: �������� ������ ���������� (��� ����� ����� �������)
        using ConsoleCommandFn = std::function<void(const std::vector<std::string>& args)>;

        struct ConsoleCommand {
            std::string name;
            std::string description;
            ConsoleCommandFn fn;
        };

        struct ConsoleLine {
            std::string text;
            Lindo::Graphics::UI::Color color;
        };

        // ������������� ������� � ����� Unity Console / X-Ray Engine (S.T.A.L.K.E.R.).
        // �������� ������ ������ �� ������� (������ ~ / `), ������� ��� ���� ������
        // (����� DebugLogger) � ��������� ������� ������� � �������� � ���������������.
        //
        // ������� ������ �� ����� ��� ���������� ������� ������� (GLFW/WinAPI/SDL) -
        // ������ onChar/onEnter/onTab/... ����� ������ �� �������� ���� �������,
        // ��. ������ ���������� � ����� ������.
        class Console {
        public:
            Console();
            ~Console() = default;

            void init(Lindo::Graphics::UI::UIFont* font, int screenWidth, int screenHeight);
            void onResize(int screenWidth, int screenHeight);

            void update();
            void render(Lindo::Graphics::UI::UIRenderer& renderer);

            // --- ��������� ---
            void toggle();
            void show();
            void hide();
            bool isVisible() const { return m_targetOpen; }
            bool isFullyClosed() const { return !m_targetOpen && m_openAmount <= 0.001f; }

            // --- ���� ������ (������ �� �������� ����, ����� ������� �������) ---
            void onChar(unsigned int codepoint);
            void onBackspace();
            void onDeleteForward();
            void onEnter();
            void onTab();
            void onEscape();
            void onHome();
            void onEnd();
            void onMoveCursorLeft();
            void onMoveCursorRight();
            void onHistoryUp();
            void onHistoryDown();
            void onScroll(float delta);

            // --- ������� ---
            void registerCommand(const std::string& name, const std::string& description, ConsoleCommandFn fn);
            void executeLine(const std::string& line);

            // --- �����/���� ---
            void addLog(Lindo::Core::LogLevel level, const std::string& text);
            void print(const std::string& text, const Lindo::Graphics::UI::Color& color = Lindo::Graphics::UI::Color(1, 1, 1, 1));
            void clear();

            // ���������� ������� � DebugLogger: ��� LOG_INFO/LOG_WARN/LOG_ERROR/... ����� �������� ���� ��
            static void hookLogger(Console* console);

            void setGLFWWindow(GLFWwindow* window) { m_glfwWindow = window; }

            // ������ �������������� � ������ ������
            void onSelectAll();
            void onCopy();
            void onPaste();
            void onCut();

        private:
            void resetTabCompletion();
            void registerBuiltinCommands();
            std::vector<std::string> tokenize(const std::string& line) const;
            Lindo::Graphics::UI::Color colorForLevel(Lindo::Core::LogLevel level) const;

            std::vector<std::string> m_tabMatchesCache;
            size_t m_tabIndex = 0;

            size_t m_selectionAnchor = 0; // ����� ������ ��������� ������
            GLFWwindow* m_glfwWindow = nullptr; // ������ �� ���� ��� ������ ������

        private:
            Lindo::Graphics::UI::UIFont* m_font = nullptr;

            bool hasSelection() const;
            size_t getSelectionMin() const;
            size_t getSelectionMax() const;
            void clearSelection();
            void deleteSelection();

            int m_screenWidth = 0;
            int m_screenHeight = 0;

            // �������� ��������� �������
            bool m_targetOpen = false;
            float m_openAmount = 0.0f;   // 0 = �������, 1 = ��������� �������
            float m_animSpeed = 10.0f;
            float m_heightRatio = 0.5f;  // ����� ���� ������ �������� �������� �������

            // ������ ���������
            std::deque<ConsoleLine> m_lines;
            size_t m_maxLines = 1000;
            int m_scrollOffset = 0;      // �� ������� ����� ������������ ����� �� ����

            // ������� ������ �����
            std::string m_inputBuffer;
            size_t m_cursorPos = 0;
            float m_cursorBlinkTimer = 0.0f;
            bool m_cursorVisible = true;

            // ������� �������� ������ (������� �����/����)
            std::vector<std::string> m_history;
            int m_historyIndex = -1;
            size_t m_maxHistory = 100;

            // ������������������ ������� (���� - ��� ������� � ������ ��������)
            std::unordered_map<std::string, ConsoleCommand> m_commands;

            const float LINE_HEIGHT = 18.0f;
            const float TEXT_SIZE = 15.0f;
            const float PADDING = 8.0f;
            const float INPUT_HEIGHT = 26.0f;
        };

    }
}