#include "UIManager.h"
#include "core/AssetManager.h"
#include <debug/DebugLogger.h>
#include <debug/Console.h>
#include <vector>

namespace Lindo {
    namespace Graphics {
        namespace UI {

            static std::string generateCharset() {
                std::string charset;
                charset.reserve(512);

                auto addCP = [&](uint32_t cp) {
                    if (cp <= 0x7F) {
                        charset += static_cast<char>(cp);
                    }
                    else if (cp <= 0x7FF) {
                        charset += static_cast<char>(0xC0 | (cp >> 6));
                        charset += static_cast<char>(0x80 | (cp & 0x3F));
                    }
                    else if (cp <= 0xFFFF) {
                        charset += static_cast<char>(0xE0 | (cp >> 12));
                        charset += static_cast<char>(0x80 | ((cp >> 6) & 0x3F));
                        charset += static_cast<char>(0x80 | (cp & 0x3F));
                    }
                    };

                // 1. ��� ����������� ASCII ������� (���������� �������, �����, ����� ���������� �� 32 �� 126)
                for (uint32_t i = 32; i <= 126; ++i) {
                    addCP(i);
                }

                // 2. ���� ������ (�)
                addCP(0x2116);

                // 3. ��������� (�-� � �-�)
                for (uint32_t i = 0x0410; i <= 0x044F; ++i) {
                    addCP(i);
                }

                // 4. ����� � � �
                addCP(0x0401);
                addCP(0x0451);

                return charset;
            }

            UIManager::UIManager() = default;
            UIManager::~UIManager() = default;

            void UIManager::clearDynamicWidgets() {
                if (m_rootPanel) m_rootPanel->clearChildren();
            }

            void UIManager::init(int width, int height) {
                LOG_INFO("UIManager::init() started");

                m_renderer = std::make_unique<UIRenderer>();
                m_renderer->init();

                std::string charset = generateCharset();

                // ��� ������� 24px ���� 512x512 ����� ��� ��������� ����� (����� ���������)
                // ����� ��������� � ���������� ����� AssetManager, ��� � ��������� � ��������.
                m_font = AssetManager::get().loadFont("ui/fonts/Europeana One.ttf", 24.0f, 512, 512, charset);

                if (!m_font) {
                    LOG_ERROR("Font loading failed!");
                }
                else {
                    LOG_INFO("Font loaded successfully");
                }

                m_rootPanel = std::make_shared<UIPanel>();
                m_rootPanel->SetColor(Color(0.0f, 0.0f, 0.0f, 0.0f));
                m_rootPanel->setSize(static_cast<float>(width), static_cast<float>(height));
                m_rootPanel->setPosition(0, 0);
                m_rootPanel->updateLayout(0.0f, 0.0f,
                    static_cast<float>(width), static_cast<float>(height));

                // ������������� ������� - ���������� ��� �� �����, ��� � ��������� UI
                m_console = std::make_unique<Lindo::Debug::Console>();
                m_console->init(m_font, width, height);
                Lindo::Debug::Console::hookLogger(m_console.get()); // ��� LOG_INFO/LOG_WARN/... ������ ����� � � �������

                m_initialized = true;
                LOG_INFO("UIManager::init() finished");
            }

            void UIManager::update() {
                if (m_console) {
                    m_console->update();
                }
            }

            void UIManager::render() {
                if (!m_initialized) return;
                m_renderer->beginFrame(static_cast<int>(m_rootPanel->getWidth()), static_cast<int>(m_rootPanel->getHeight()));
                m_rootPanel->render(*m_renderer, m_font);

                // ������� �������� ��������� - ������ ����� ���������� UI
                if (m_console) {
                    m_console->render(*m_renderer);
                }

                m_renderer->endFrame();
            }

            void UIManager::onResize(int width, int height) {
                if (m_rootPanel) {
                    m_rootPanel->setSize(static_cast<float>(width), static_cast<float>(height));
                    // root всегда в (0,0), его размер = размер окна
                    m_rootPanel->updateLayout(
                        0.0f, 0.0f,
                        static_cast<float>(width), static_cast<float>(height));
                }
                if (m_console) {
                    m_console->onResize(width, height);
                }
            }

            void UIManager::onMouseMove(float x, float y) {
                // ���� ������� ������� - ��� ������������� ����, ��� ��� ������ �� ������ �����������
                if (m_console && m_console->isVisible()) return;

                if (m_rootPanel) {
                    m_rootPanel->onMouseMove(x, y);
                }
            }

            void UIManager::onMouseButton(float x, float y, int button, bool pressed) {
                if (m_console && m_console->isVisible()) return;

                if (m_rootPanel) {
                    m_rootPanel->onMouseButton(x, y, button, pressed);
                }
            }

            void UIManager::onChar(unsigned int codepoint) {
                if (m_console && m_console->isVisible()) return;
                        
                if (m_rootPanel) {
                    m_rootPanel->onChar(codepoint);
                }
            }
            
            void UIManager::onKey(int key, int scancode, int action, int mods) {
                if (m_console && m_console->isVisible()) return;
            
                if (m_rootPanel) {
                    m_rootPanel->onKey(key, scancode, action, mods);
                }
            }
        }
    }
}