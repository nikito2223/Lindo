#include "Console.h"
#include <algorithm>
#include <cctype>
#include <cmath>
#include <Core/Time/Time.h>

namespace Lindo {
    namespace Debug {

        using Lindo::Graphics::UI::Color;
        using Lindo::Graphics::UI::Rect;
        using Lindo::Graphics::UI::UIRenderer;
        using Lindo::Graphics::UI::UIFont;

        Console::Console() = default;

        void Console::init(UIFont* font, int screenWidth, int screenHeight) {
            m_font = font;
            m_screenWidth = screenWidth;
            m_screenHeight = screenHeight;
        }

        void Console::onResize(int screenWidth, int screenHeight) {
            m_screenWidth = screenWidth;
            m_screenHeight = screenHeight;
        }

        void Console::toggle() { m_targetOpen = !m_targetOpen; }
        void Console::show() { m_targetOpen = true; }
        void Console::hide() { m_targetOpen = false; }

        void Console::update() {
            float dt = Lindo::Time::GetUnscaledDeltaTime();
            float target = m_targetOpen ? 1.0f : 0.0f;
            float speed = std::min(1.0f, dt * m_animSpeed);
            m_openAmount += (target - m_openAmount) * speed;
            if (std::fabs(target - m_openAmount) < 0.001f) m_openAmount = target;

            m_cursorBlinkTimer += dt;
            if (m_cursorBlinkTimer >= 0.5f) {
                m_cursorBlinkTimer = 0.0f;
                m_cursorVisible = !m_cursorVisible;
            }
        }

        void Console::render(UIRenderer& renderer) {
            if (!m_font || m_openAmount <= 0.001f) return;

            float panelHeight = m_screenHeight * m_heightRatio * m_openAmount;

            // 1. ����������� ������� ���� ������� ������ ������
            renderer.drawRect(Rect(0, 0, (float)m_screenWidth, panelHeight), Color(0.02f, 0.02f, 0.03f, 0.95f));

            // 2. ����� ��������� ������� �� ������� ����
            renderer.drawRect(Rect(0, panelHeight - 2.0f, (float)m_screenWidth, 2.0f), Color(0.25f, 0.55f, 1.0f, 1.0f));

            float inputY = panelHeight - INPUT_HEIGHT;

            // 3. ��� ��� ������ ���� �����
            renderer.drawRect(Rect(0, inputY, (float)m_screenWidth, INPUT_HEIGHT), Color(0.08f, 0.08f, 0.10f, 0.98f));

            float scale = TEXT_SIZE / m_font->getLineHeight();
            float inputBaseline = inputY + INPUT_HEIGHT * 0.5f + m_font->getAscent() * scale * 0.35f;

            // ��������� ��������� ������ (���� ��� ����)
            if (hasSelection()) {
                size_t mn = getSelectionMin();
                size_t mx = getSelectionMax();
                std::string beforeMin = "> " + m_inputBuffer.substr(0, mn);
                std::string selectedStr = m_inputBuffer.substr(mn, mx - mn);

                float selStartX = PADDING + m_font->getStringWidthWithSize(beforeMin, TEXT_SIZE);
                float selWidth = m_font->getStringWidthWithSize(selectedStr, TEXT_SIZE);

                // �������������� ����� ������������� ��� ���������� �������
                renderer.drawRect(Rect(selStartX, inputY + 4.0f, selWidth, INPUT_HEIGHT - 8.0f), Color(0.25f, 0.5f, 0.8f, 0.4f));
            }

            // ��������� �������� ������
            std::string prompt = "> " + m_inputBuffer;
            std::vector<UIRenderer::Vertex> inputVerts;
            m_font->getTextVerticesWithSize(prompt, PADDING, inputBaseline, TEXT_SIZE, Color(1.0f, 1.0f, 1.0f, 1.0f), inputVerts);
            if (!inputVerts.empty()) renderer.drawRaw(inputVerts, m_font->getTextureID());

            // ������
            if (m_cursorVisible) {
                std::string beforeCursor = "> " + m_inputBuffer.substr(0, m_cursorPos);
                float cursorX = PADDING + m_font->getStringWidthWithSize(beforeCursor, TEXT_SIZE);
                renderer.drawRect(Rect(cursorX, inputY + 4.0f, 2.0f, INPUT_HEIGHT - 8.0f), Color(0.45f, 1.0f, 0.45f, 1.0f));
            }

            // ��������� ������� �����
            if (!m_lines.empty()) {
                int total = (int)m_lines.size();
                int startIdx = total - 1 - m_scrollOffset;
                if (startIdx >= total) startIdx = total - 1;
                if (startIdx < 0) startIdx = 0;

                float y = inputY - PADDING;

                for (int i = startIdx; i >= 0; --i) {
                    if (y - LINE_HEIGHT < PADDING) break;

                    const ConsoleLine& line = m_lines[i];
                    float lineTop = y - LINE_HEIGHT;
                    float lineBaseline = lineTop + m_font->getAscent() * scale;

                    std::vector<UIRenderer::Vertex> lv;
                    m_font->getTextVerticesWithSize(line.text, PADDING, lineBaseline, TEXT_SIZE, line.color, lv);
                    if (!lv.empty()) renderer.drawRaw(lv, m_font->getTextureID());

                    y -= LINE_HEIGHT;
                }
            }
        }

        // ---------------- ���� ----------------

        void Console::onChar(unsigned int codepoint) {
            if (codepoint == '`' || codepoint == '~') return; // ������� �������� ������� �� ������ ����������

            resetTabCompletion();

            if (hasSelection()) {
                deleteSelection();
            }

            std::string encoded;
            if (codepoint <= 0x7F) {
                encoded += static_cast<char>(codepoint);
            }
            else if (codepoint <= 0x7FF) {
                encoded += static_cast<char>(0xC0 | (codepoint >> 6));
                encoded += static_cast<char>(0x80 | (codepoint & 0x3F));
            }
            else if (codepoint <= 0xFFFF) {
                encoded += static_cast<char>(0xE0 | (codepoint >> 12));
                encoded += static_cast<char>(0x80 | ((codepoint >> 6) & 0x3F));
                encoded += static_cast<char>(0x80 | (codepoint & 0x3F));
            }
            else {
                encoded += static_cast<char>(0xF0 | (codepoint >> 18));
                encoded += static_cast<char>(0x80 | ((codepoint >> 12) & 0x3F));
                encoded += static_cast<char>(0x80 | ((codepoint >> 6) & 0x3F));
                encoded += static_cast<char>(0x80 | (codepoint & 0x3F));
            }

            m_inputBuffer.insert(m_cursorPos, encoded);
            m_cursorPos += encoded.size();
            m_selectionAnchor = m_cursorPos; // ���������� ��������� � ����� ������� �������
            m_cursorVisible = true;
            m_cursorBlinkTimer = 0.0f;
        }

        void Console::onBackspace() {
            resetTabCompletion();
            if (hasSelection()) {
                deleteSelection();
                return;
            }

            if (m_cursorPos == 0 || m_inputBuffer.empty()) return;
            size_t pos = m_cursorPos - 1;
            while (pos > 0 && (static_cast<unsigned char>(m_inputBuffer[pos]) & 0xC0) == 0x80) --pos;
            m_inputBuffer.erase(pos, m_cursorPos - pos);
            m_cursorPos = pos;
            m_selectionAnchor = m_cursorPos;
            m_cursorVisible = true;
            m_cursorBlinkTimer = 0.0f;
        }

        void Console::onDeleteForward() {
            resetTabCompletion();
            if (hasSelection()) {
                deleteSelection();
                return;
            }

            if (m_cursorPos >= m_inputBuffer.size()) return;
            size_t pos = m_cursorPos + 1;
            while (pos < m_inputBuffer.size() && (static_cast<unsigned char>(m_inputBuffer[pos]) & 0xC0) == 0x80) ++pos;
            m_inputBuffer.erase(m_cursorPos, pos - m_cursorPos);
            m_selectionAnchor = m_cursorPos;
        }

        void Console::onEnter() {
            resetTabCompletion();
            if (m_inputBuffer.empty()) return;
            executeLine(m_inputBuffer);
            m_inputBuffer.clear();
            m_cursorPos = 0;
            m_selectionAnchor = 0;
            m_scrollOffset = 0;
        }

        bool Console::hasSelection() const {
            return m_cursorPos != m_selectionAnchor;
        }

        size_t Console::getSelectionMin() const {
            return std::min(m_cursorPos, m_selectionAnchor);
        }

        size_t Console::getSelectionMax() const {
            return std::max(m_cursorPos, m_selectionAnchor);
        }

        void Console::clearSelection() {
            m_selectionAnchor = m_cursorPos;
        }

        void Console::deleteSelection() {
            if (!hasSelection()) return;
            size_t mn = getSelectionMin();
            size_t mx = getSelectionMax();
            m_inputBuffer.erase(mn, mx - mn);
            m_cursorPos = mn;
            m_selectionAnchor = mn;
        }

        void Console::onSelectAll() {
            m_selectionAnchor = 0;
            m_cursorPos = m_inputBuffer.size();
        }

        void Console::onCopy() {
            if (!m_glfwWindow || !hasSelection()) return;
            size_t mn = getSelectionMin();
            size_t mx = getSelectionMax();
            std::string selected = m_inputBuffer.substr(mn, mx - mn);
            glfwSetClipboardString(m_glfwWindow, selected.c_str());
        }

        void Console::onPaste() {
            if (!m_glfwWindow) return;
            resetTabCompletion();
            const char* clip = glfwGetClipboardString(m_glfwWindow);
            if (!clip) return;
            std::string text(clip);
            if (text.empty()) return;

            if (hasSelection()) {
                deleteSelection();
            }

            m_inputBuffer.insert(m_cursorPos, text);
            m_cursorPos += text.size();
            m_selectionAnchor = m_cursorPos;
            m_cursorVisible = true;
            m_cursorBlinkTimer = 0.0f;
        }

        void Console::onCut() {
            if (!m_glfwWindow || !hasSelection()) return;
            resetTabCompletion();
            onCopy();
            deleteSelection();
        }

        void Console::onTab() {
            // ���� ��� ������ (������ Tab ������� ��� �������� ��������)
            const size_t commandEnd = m_inputBuffer.find_first_of(" \t");
            if (commandEnd != std::string::npos && m_cursorPos > commandEnd) return;

            if (m_tabMatchesCache.empty()) {
                // ���� ���� ������, ������, �� ��� ������ ��������� � �������������� ������� �� �����
                const size_t prefixEnd = commandEnd == std::string::npos ? m_cursorPos : commandEnd;
                std::string prefix = m_inputBuffer.substr(0, prefixEnd);
                std::string lowerPrefix = prefix;
                for (auto& c : lowerPrefix) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));

                // �������� ��� ���������� �������
                for (auto& kv : m_commands) {
                    // ���� ������� ������, �������� ������ ��� �������
                    if (lowerPrefix.empty() || kv.first.compare(0, lowerPrefix.size(), lowerPrefix) == 0) {
                        m_tabMatchesCache.push_back(kv.second.name);
                    }
                }

                if (m_tabMatchesCache.empty()) return;
                std::sort(m_tabMatchesCache.begin(), m_tabMatchesCache.end());
                m_tabIndex = 0;
            }

            // ���� ������� ����� 1 ������� (��� ����� ���������� �������� ����)
            if (m_tabMatchesCache.size() == 1) {
                m_inputBuffer = m_tabMatchesCache[0] + " ";
                m_cursorPos = m_inputBuffer.size();
                m_selectionAnchor = m_cursorPos;
            }
            else {
                // ���� ��������� ��������� � ���������� ���������� �� �� ������ ��� ������ ������� Tab
                m_inputBuffer = m_tabMatchesCache[m_tabIndex] + " ";
                m_cursorPos = m_inputBuffer.size();
                m_selectionAnchor = m_cursorPos;

                // �������� ������ ��������� � ��� ������ ��� ������ �������, ����� �� �������� ��� ��� ��������
                if (m_tabIndex == 0) {
                    std::string list;
                    for (size_t i = 0; i < m_tabMatchesCache.size(); ++i) {
                        list += m_tabMatchesCache[i] + (i + 1 < m_tabMatchesCache.size() ? "    " : "");
                    }
                    print(list, Color(0.6f, 0.6f, 0.6f, 1.0f));
                }

                // ��������� � ��������� ������� ��� ���������� ������� Tab
                m_tabIndex = (m_tabIndex + 1) % m_tabMatchesCache.size();
            }
        }

        void Console::onEscape() { hide(); }

        void Console::onHome() {
            resetTabCompletion();
            m_cursorPos = 0;
            m_selectionAnchor = m_cursorPos;
        }

        void Console::onEnd() {
            resetTabCompletion();
            m_cursorPos = m_inputBuffer.size();
            m_selectionAnchor = m_cursorPos;
        }

        void Console::onMoveCursorLeft() {
            resetTabCompletion();
            if (m_cursorPos == 0) return;
            size_t pos = m_cursorPos - 1;
            while (pos > 0 && (static_cast<unsigned char>(m_inputBuffer[pos]) & 0xC0) == 0x80) --pos;
            m_cursorPos = pos;
            m_selectionAnchor = m_cursorPos;
        }

        void Console::onMoveCursorRight() {
            resetTabCompletion();
            if (m_cursorPos >= m_inputBuffer.size()) return;
            size_t pos = m_cursorPos + 1;
            while (pos < m_inputBuffer.size() && (static_cast<unsigned char>(m_inputBuffer[pos]) & 0xC0) == 0x80) ++pos;
            m_cursorPos = pos;
            m_selectionAnchor = m_cursorPos;
        }

        void Console::onHistoryUp() {
            resetTabCompletion();
            if (m_history.empty()) return;
            if (m_historyIndex == -1) m_historyIndex = (int)m_history.size();
            if (m_historyIndex > 0) --m_historyIndex;
            m_inputBuffer = m_history[m_historyIndex];
            m_cursorPos = m_inputBuffer.size();
            m_selectionAnchor = m_cursorPos;
        }

        void Console::onHistoryDown() {
            resetTabCompletion();
            if (m_historyIndex == -1) return;
            ++m_historyIndex;
            if (m_historyIndex >= (int)m_history.size()) {
                m_historyIndex = -1;
                m_inputBuffer.clear();
            }
            else {
                m_inputBuffer = m_history[m_historyIndex];
            }
            m_cursorPos = m_inputBuffer.size();
            m_selectionAnchor = m_cursorPos;
        }

        void Console::onScroll(float delta) {
            int step = delta > 0 ? 3 : -3;
            m_scrollOffset += step;
            int maxOffset = m_lines.empty() ? 0 : (int)m_lines.size() - 1;
            if (m_scrollOffset < 0) m_scrollOffset = 0;
            if (m_scrollOffset > maxOffset) m_scrollOffset = maxOffset;
        }

        void Console::resetTabCompletion() {
            m_tabMatchesCache.clear();
            m_tabIndex = 0;
        }

        // ---------------- ������� ----------------

        std::vector<std::string> Console::tokenize(const std::string& line) const {
            std::vector<std::string> tokens;
            std::string current;
            bool inQuotes = false;

            for (char c : line) {
                if (c == '"') {
                    inQuotes = !inQuotes;
                    continue;
                }
                if (std::isspace(static_cast<unsigned char>(c)) && !inQuotes) {
                    if (!current.empty()) {
                        tokens.push_back(current);
                        current.clear();
                    }
                }
                else {
                    current += c;
                }
            }
            if (!current.empty()) tokens.push_back(current);
            return tokens;
        }

        void Console::registerCommand(const std::string& name, const std::string& description, ConsoleCommandFn fn) {
            std::string key = name;
            for (auto& c : key) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
            m_commands[key] = ConsoleCommand{ name, description, fn };
        }

        void Console::executeLine(const std::string& line) {
            size_t start = line.find_first_not_of(" \t");
            if (start == std::string::npos) return;
            size_t end = line.find_last_not_of(" \t");
            std::string trimmed = line.substr(start, end - start + 1);
            if (trimmed.empty()) return;

            print("> " + trimmed, Color(0.5f, 0.8f, 1.0f, 1.0f));

            m_history.push_back(trimmed);
            if (m_history.size() > m_maxHistory) m_history.erase(m_history.begin());
            m_historyIndex = -1;

            std::vector<std::string> tokens = tokenize(trimmed);
            if (tokens.empty()) return;

            std::string cmdKey = tokens[0];
            for (auto& c : cmdKey) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));

            auto it = m_commands.find(cmdKey);
            if (it == m_commands.end()) {
                print("Unknown command: '" + tokens[0] + "'. Type 'help' for command list.", Color(0.5f, 0.0f, 0.0f, 1.0f));
                return;
            }

            std::vector<std::string> args(tokens.begin() + 1, tokens.end());
            it->second.fn(args);
        }

        // ---------------- ���� ----------------

        void Console::addLog(Lindo::Core::LogLevel level, const std::string& text) {
            print(text, colorForLevel(level));
        }

        void Console::print(const std::string& text, const Color& color) {
            m_lines.push_back({ text, color });
            while (m_lines.size() > m_maxLines) m_lines.pop_front();
        }

        void Console::clear() {
            m_lines.clear();
            m_scrollOffset = 0;
        }

        Color Console::colorForLevel(Lindo::Core::LogLevel level) const {
            switch (level) {
            case Lindo::Core::LogLevel::Debug:    return Color(0.60f, 0.60f, 0.70f, 1.0f);
            case Lindo::Core::LogLevel::Info:     return Color(0.40f, 0.80f, 1.00f, 1.0f);
            case Lindo::Core::LogLevel::Warning:  return Color(1.00f, 0.75f, 0.25f, 1.0f);
            case Lindo::Core::LogLevel::Error:    return Color(1.00f, 0.45f, 0.45f, 1.0f);
            case Lindo::Core::LogLevel::Critical: return Color(1.00f, 0.20f, 0.60f, 1.0f);
            default: return Color(1.0f, 1.0f, 1.0f, 1.0f);
            }
        }

        void Console::hookLogger(Console* console) {
            Lindo::Core::DebugLogger::SetConsoleWidget([console](Lindo::Core::LogLevel level, const std::string& msg) {
                if (console) console->addLog(level, msg);
                });
        }

    }
}