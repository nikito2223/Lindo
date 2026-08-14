#include "Application.h"
#include "Window/glfw/Window.h"
#include "Input.h"
#include "utils/SceneManager.h"
#include "Graphics/ui/UIManager.h"
#include "debug/DebugOverlay.h"
#include "Graphics/core/Renderer.h"
#include "core/Globals.h" // для SCR_WIDTH, SCR_HEIGHT
#include <iostream>

Application::Application() {
    m_window = std::make_unique<Window>(SCR_WIDTH, SCR_HEIGHT, "Lingo");
    m_input = std::make_unique<Input>();
    m_sceneManager = std::make_unique<SceneManager>();
    m_uiManager = std::make_unique<UIManager>();
    m_debugOverlay = std::make_unique<DebugOverlay>();

    m_renderer = std::make_unique<Renderer>(m_sceneManager.get(), m_uiManager.get(), m_debugOverlay.get());
}

Application::~Application() = default;

void Application::run() {
    // Инициализация компонентов
    std::cout << "Initializing window callbacks..." << std::endl;
    m_window->setCallbacks(m_input.get(), m_uiManager.get());

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        throw std::runtime_error("Failed to initialize GLAD");
    }

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glEnable(GL_MULTISAMPLE);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);

    std::cout << "Initializing UI..." << std::endl;
    m_uiManager->init();

    std::cout << "Initializing scene..." << std::endl;
    m_sceneManager->init();

    std::cout << "Initializing debug overlay..." << std::endl;
    m_debugOverlay->init(m_uiManager->getFont());

    std::cout << "Initializing renderer..." << std::endl;
    m_renderer->init();

    std::cout << "Entering main loop..." << std::endl;
    


    // Главный цикл
    while (!m_window->shouldClose()) {
        m_window->pollEvents();
        float currentTime = (float)glfwGetTime();
        float deltaTime = currentTime - m_lastFrameTime;
        if (deltaTime > 0.05f) deltaTime = 0.05f;
        m_lastFrameTime = currentTime;

        // Обновление ввода
        m_input->update(m_window.get());
        
        // Обработка горячих клавиш
        if (m_input->consumeF3()) {
            bool debugMode = m_debugOverlay->toggle(); // предположим, toggle возвращает новое состояние
            std::cout << "Debug mode: " << (debugMode ? "ON" : "OFF") << std::endl;
        }
        if (m_input->consumeF11()) {
            m_window->toggleFullscreen();
        }
        if (m_input->consumeEscape()) {
            bool uiActive = !m_input->isUIActive();
            m_input->setUIActive(uiActive);
        }

        // Обновление сцены (движение, физика)
        m_sceneManager->update(deltaTime, m_input.get());

        // Обновление дебаг-оверлея (позиция персонажа, FPS)
        glm::vec3 pos = m_sceneManager->getCharacterPosition();
        m_debugOverlay->updatePosition(pos);
        static float fpsTimer = 0.0f;
        static int frameCount = 0;
        fpsTimer += deltaTime;
        frameCount++;
        if (fpsTimer >= 1.0f) {
            m_debugOverlay->updateStats(fpsTimer, frameCount, m_debugOverlay->isVisible());
            fpsTimer = 0.0f;
            frameCount = 0;
        }

        // Рендеринг
        m_renderer->render(deltaTime);

        // Обмен буферов и обработка событий
        m_window->swapBuffers();
    }

    // Очистка
    m_sceneManager->cleanup();
}