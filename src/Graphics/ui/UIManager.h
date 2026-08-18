#pragma once
#include <memory>
#include "UIRenderer.h"
#include "UIFont.h"
#include "UIWidget.h"

namespace Lindo {
    namespace Debug {
        class Console; // объ€влена в debug/Console.h
    }
}

namespace Lindo {
    namespace Graphics {
        namespace UI {
            class UIManager {
            public:
                UIManager();
                ~UIManager();

                void init(int width, int height);

                // ќбновление анимаций/состо€ни€ (в т.ч. консоли) - звать раз в кадр перед render()
                void update();

                void render();
                void onResize(int width, int height);
                void onMouseMove(float x, float y);
                void onMouseButton(float x, float y, int button, bool pressed);

                UIFont* getFont() const { return m_font.get(); }
                UIRenderer* getRenderer() const { return m_renderer.get(); } // публичный доступ к рендереру

                // »грова€ консоль (лог + ввод команд), доступна после init()
                Lindo::Debug::Console* getConsole() const { return m_console.get(); }

            private:

                std::unique_ptr<UIRenderer> m_renderer;
                std::unique_ptr<UIFont> m_font;
                std::shared_ptr<UIPanel> m_rootPanel;
                std::unique_ptr<Lindo::Debug::Console> m_console;
                bool m_initialized = false;
            };
        }
    }
}