#include "Application.h"
#include "Window/glfw/Window.h"
#include "Input.h"
#include "utils/SceneManager.h"
#include "Graphics/ui/UIManager.h"
#include "Graphics/render/PostProcessor.h"
#include "debug/DebugOverlay.h"
#include "Graphics/core/Renderer.h"
#include "core/Globals.h" // для SCR_WIDTH, SCR_HEIGHT

Application::Application() {
    m_window = std::make_unique<Window>(SCR_WIDTH, SCR_HEIGHT, "Lingo");
    m_input = std::make_unique<Input>();
    m_sceneManager = std::make_unique<SceneManager>();
    m_uiManager = std::make_unique<UIManager>();
    m_postProcessor = std::make_unique<PostProcessor>();
    m_debugOverlay = std::make_unique<DebugOverlay>();

    m_renderer = std::make_unique<Renderer>(m_sceneManager.get(), m_uiManager.get(),
        m_postProcessor.get(), m_debugOverlay.get());
}

Application::~Application() = default;

void Application::onResize(int width, int height) {
    // Обновляем viewport
    glViewport(0, 0, width, height);

    // Обновляем соотношение сторон для проекционных матриц
    if (m_sceneManager) {
        m_sceneManager->onResize(width, height);
    }

    // Обновляем пост-процессор
    if (m_postProcessor) {
        m_postProcessor->resize(width, height);
    }

    // Обновляем UI
    if (m_uiManager) {
        m_uiManager->onResize(width, height);
    }

    // Обновляем дебаг-оверлей
    if (m_debugOverlay) {
        m_debugOverlay->onResize(width, height);
    }

    // Обновляем рендерер
    if (m_renderer) {
        m_renderer->onResize(width, height);
    }
}

void Application::run() {
    // Инициализация компонентов
    std::cout << "Initializing window callbacks..." << std::endl;
    m_window->setCallbacks(m_input.get(), m_uiManager.get());

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        throw std::runtime_error("Failed to initialize GLAD");
    }

    glfwSetWindowSizeCallback(m_window->getGLFWWindow(), [](GLFWwindow* window, int width, int height) {
        // Получаем указатель на Application из данных окна
        Application* app = static_cast<Application*>(glfwGetWindowUserPointer(window));
        if (app) {
            app->onResize(width, height);
        }
    });

    glfwSetWindowUserPointer(m_window->getGLFWWindow(), this);

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glClearColor(0.2f, 0.3f, 0.4f, 1.0f); // яркий серо-синий цвет

    std::cout << "Initializing UI..." << std::endl;
    m_uiManager->init();

    std::cout << "Initializing post-processor..." << std::endl;
    m_postProcessor->init(SCR_WIDTH, SCR_HEIGHT);

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