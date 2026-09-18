#pragma once
#include <memory>
#include "UIRenderer.h"
#include "UIFont.h"
#include "UIWidget.h"

namespace Lindo {
    namespace Debug {
        class Console; // ��������� � debug/Console.h
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

                // ���������� ��������/��������� (� �.�. �������) - ����� ��� � ���� ����� render()
                void update();

                void render();
                void onResize(int width, int height);
                void onMouseMove(float x, float y);
                void onMouseButton(float x, float y, int button, bool pressed);
                void clearDynamicWidgets();

                void onChar(unsigned int codepoint);
                void onKey(int key, int scancode, int action, int mods);

                UIFont* getFont() const { return m_font; }
                UIRenderer* getRenderer() const { return m_renderer.get(); } // ��������� ������ � ���������
                // ������ ��� � UIManager.h ������ ������ UIManager:
                std::shared_ptr<UIPanel> getRootPanel() const { return m_rootPanel; }
                // ������� ������� (��� + ���� ������), �������� ����� init()
                Lindo::Debug::Console* getConsole() const { return m_console.get(); }

            private:

                std::unique_ptr<UIRenderer> m_renderer;
                UIFont* m_font = nullptr; // ���������: ��������� ��������� AssetManager, UIManager �� ��������
                std::shared_ptr<UIPanel> m_rootPanel;
                std::unique_ptr<Lindo::Debug::Console> m_console;
                bool m_initialized = false;
            };
        }
    }
}