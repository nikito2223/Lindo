#include "UIManager.h"
#include <debug/DebugLogger.h> // Подключаем твой логгер
#include <core/Globals.h>
#include <vector>

namespace Lindo {
    namespace Graphics {
        namespace UI {

            static std::string generateCharset() {
                // Базовая латиница и знаки
                std::string charset = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789 .,!?-+*/=()[]{}<>:;\"'%@#&";

                // Добавляем кириллицу (А-Я, а-я, Ё, ё) через UTF-8
                auto addCP = [&](uint32_t cp) {
                    if (cp <= 0x7FF) {
                        charset += (char)(0xC0 | (cp >> 6));
                        charset += (char)(0x80 | (cp & 0x3F));
                    }
                    };

                for (uint32_t i = 0x0410; i <= 0x044F; ++i) addCP(i);
                addCP(0x0401);
                addCP(0x0451);

                return charset;
            }

            UIManager::UIManager() = default;
            UIManager::~UIManager() = default;

            void UIManager::init() {
                LOG_INFO("UIManager::init() started");

                LOG_INFO("Creating UIRenderer...");
                m_renderer = std::make_unique<UIRenderer>();

                LOG_INFO("Initializing UIRenderer...");
                m_renderer->init();
                LOG_INFO("UIRenderer initialized successfully");

                LOG_INFO("Creating UIFont...");
                m_font = std::make_unique<UIFont>();

                std::string charset = generateCharset();
                LOG_INFO("Loading font: C:/Windows/Fonts/arial.ttf (Size: 24.0, Atlas: 1024x1024)");

                bool fontLoaded = m_font->loadFromFile("C:/Windows/Fonts/arial.ttf", 24.0f, 1024, 1024, charset);

                if (!fontLoaded) {
                    LOG_ERROR("Font loading failed! Check if file exists or atlas size is sufficient.");
                }
                else {
                    LOG_INFO("Font loaded successfully");
                    // Можно раскомментировать для проверки атласа, если "кубики" не исчезнут
                    // m_font->saveAtlas("debug_font_atlas.ppm");
                }

                LOG_INFO("Creating UI root panel...");
                m_rootPanel = std::make_shared<UIPanel>();

                // Устанавливаем прозрачный фон для корневой панели
                m_rootPanel->SetColor(Color(0.0f, 0.0f, 0.0f, 0.0f));
                m_rootPanel->setSize(static_cast<float>(Globals::screenWidth), static_cast<float>(Globals::screenHeight));
                m_rootPanel->setPosition(0, 0);

                m_initialized = true;
                LOG_INFO("UIManager::init() finished");
            }

            void UIManager::render() {
                if (!m_initialized) return;
                m_renderer->beginFrame(m_rootPanel->getWidth(), m_rootPanel->getHeight());
                m_rootPanel->render(*m_renderer, m_font.get());
                m_renderer->endFrame();
            }

            void UIManager::onResize(int width, int height) {
                if (m_rootPanel) {
                    LOG_INFO("UI Resizing to: " + std::to_string(width) + "x" + std::to_string(height));
                    m_rootPanel->setSize(static_cast<float>(width), static_cast<float>(height));
                }
            }

            void UIManager::onMouseMove(float x, float y) {
                if (m_rootPanel) {
                    m_rootPanel->onMouseMove(x, y);
                }
            }

            void UIManager::onMouseButton(float x, float y, int button, bool pressed) {
                if (m_rootPanel) {
                    m_rootPanel->onMouseButton(x, y, button, pressed);
                }
            }
        }
    }
}