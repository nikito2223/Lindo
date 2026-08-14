#ifndef GLOBALS_H
#define GLOBALS_H

#include <glm/glm.hpp>
#include <string_view> // Легковесные строки
#include <memory>

// Forward declarations
namespace Lindo {
    namespace Components {
        namespace Rendering {
            class Camera;
        }
        namespace Controller {
            class Player;
        }
    }
    namespace Graphics {
        namespace UI {
            class UIPanel;
        }
    }
}

namespace Globals {
    // Константы экрана (используем соразмерные типы)
    inline int screenWidth{ 1280 };
    inline int screenHeight{ 720 };
    inline std::string_view engineName = "Lindo Engine";

    inline Lindo::Components::Rendering::Camera* camera = nullptr;

    // Время и физика
    inline float time{ 0.0f };
    inline float deltaTime{ 0.0f };
    inline float lastFrame{ 0.0f };

    // Управление
    inline glm::vec2 lastMousePos{ 400.0f, 300.0f }; // Сразу в вектор, так удобнее считать смещение
    inline bool firstMouse{ true };

    // Системные флаги
    inline bool useRawResources{ false };
    inline bool debugMode{ false };
    inline bool renderPhysicsDebug{ false };
    inline bool uiActive{ false };

    // Пути (string_view идеален для константных путей в C++20)
    using namespace std::literals;
    inline constexpr std::string_view pathData = "res/";

    // Глобальные ссылки (через умные указатели)
    inline std::shared_ptr<Lindo::Graphics::UI::UIPanel> rootPanel{ nullptr };
}

// Интерфейсные функции (их лучше оставить вне namespace или в нем же, по вкусу)
[[nodiscard]] Lindo::Components::Rendering::Camera* getActiveCamera(); // [[nodiscard]] заставит компилятор ругаться, если ты вызвал функцию и забыл результат
[[nodiscard]] Lindo::Components::Controller::Player* getPlayer();

#endif

