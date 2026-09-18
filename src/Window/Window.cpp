#include "Window.h"
#include "core/Input.h"
#include "Graphics/ui/UIManager.h"
#include "Core/Types/Settings.h"
#include <glad/glad.h>
#include "Core/RenderCommand.h"
#include "debug/DebugLogger.h"
#include <iostream>
#include <debug/Console.h>

namespace Lindo {

    #ifdef _WIN32
    // Должен совпадать с ID в res/app.rc
    static constexpr int LINDO_ICON_RESOURCE_ID = 101;

    // Достаёт иконку из ресурсов .exe (тот самый .ico, что зашит через .rc)
    // и ставит её как иконку окна GLFW.
    static void applyWindowIconFromResource(GLFWwindow* window) {
        if (!window) return;

        const int sizes[] = { 16, 32, 48, 256 };
        std::vector<GLFWimage>              images;
        std::vector<std::vector<uint8_t>>   buffers;
        std::vector<HICON>                  icons;

        images.reserve(std::size(sizes));
        buffers.reserve(std::size(sizes));
        icons.reserve(std::size(sizes));

        for (int sz : sizes) {
            HICON hIcon = static_cast<HICON>(LoadImageW(
                GetModuleHandleW(nullptr),
                MAKEINTRESOURCEW(LINDO_ICON_RESOURCE_ID),
                IMAGE_ICON, sz, sz, LR_DEFAULTCOLOR));
            if (!hIcon) continue;
            icons.push_back(hIcon);

            ICONINFO ii{};
            if (!GetIconInfo(hIcon, &ii)) continue;

            BITMAP bm{};
            GetObject(ii.hbmColor, sizeof(BITMAP), &bm);

            const int w = bm.bmWidth;
            const int h = bm.bmHeight;
            if (w <= 0 || h <= 0) {
                DeleteObject(ii.hbmColor);
                DeleteObject(ii.hbmMask);
                continue;
            }

            BITMAPINFO bmi{};
            bmi.bmiHeader.biSize        = sizeof(BITMAPINFOHEADER);
            bmi.bmiHeader.biWidth       = w;
            bmi.bmiHeader.biHeight      = -h; // top-down
            bmi.bmiHeader.biPlanes      = 1;
            bmi.bmiHeader.biBitCount    = 32;
            bmi.bmiHeader.biCompression = BI_RGB;

            std::vector<uint8_t> pixels(static_cast<size_t>(w) * h * 4);
            HDC hdc = GetDC(nullptr);
            GetDIBits(hdc, ii.hbmColor, 0, h, pixels.data(), &bmi, DIB_RGB_COLORS);
            ReleaseDC(nullptr, hdc);

            // BGRA -> RGBA
            for (int i = 0; i < w * h; ++i) {
                std::swap(pixels[i * 4 + 0], pixels[i * 4 + 2]);
            }

            buffers.push_back(std::move(pixels));

            GLFWimage img;
            img.width  = w;
            img.height = h;
            img.pixels = buffers.back().data(); // стабильно: vector переносит владение буфером
            images.push_back(img);

            DeleteObject(ii.hbmColor);
            DeleteObject(ii.hbmMask);
        }

        if (!images.empty()) {
            glfwSetWindowIcon(window, static_cast<int>(images.size()), images.data());
            LOG_INFO("[Window] Window icon applied from .exe resources.");
        } else {
            LOG_WARN("[Window] No icon resource loaded (ID=101) — window uses default icon.");
        }

        for (HICON h : icons) DestroyIcon(h);
    }
#endif

    Window::Window(int width, int height, const char* title) {
        DisplaySettings& displaySettings = DisplaySettings::getInstance();
        Settings& settings = Settings::getInstance();

        LOG_INFO("[Window] Initializing Window creation...");
        LOG_INFO(std::string("[Window] Target Resolution from Settings: ") + std::to_string(displaySettings.windowWidth) + "x" + std::to_string(displaySettings.windowHeight));
        LOG_INFO(std::string("[Window] VSync setting is: ") + (displaySettings.vsync ? "ENABLED (1)" : "DISABLED (0)"));
        LOG_INFO(std::string("[Window] MSAA Samples: ") + std::to_string(settings.msaaSamples));
        LOG_INFO(std::string("[Window] Fullscreen mode: ") + (displaySettings.fullscreen ? "ON" : "OFF"));
        LOG_INFO(std::string("[Window] Borderless mode: ") + (displaySettings.borderless ? "ON" : "OFF"));

        m_width = displaySettings.windowWidth;
        m_height = displaySettings.windowHeight;

        if (!glfwInit()) {
            LOG_ERROR("[Window] Failed to initialize GLFW!");
            std::cerr << "Failed to initialize GLFW" << std::endl;
            throw std::runtime_error("GLFW init failed");
        }
        LOG_INFO("[Window] GLFW initialized successfully.");

        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
        glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
        glfwWindowHint(GLFW_SAMPLES, settings.msaaSamples);

        if (displaySettings.borderless) {
            LOG_INFO("[Window] Applying window hint: Borderless (GLFW_DECORATED = FALSE)");
            glfwWindowHint(GLFW_DECORATED, GLFW_FALSE);
        }

        GLFWmonitor* monitor = displaySettings.fullscreen ? glfwGetPrimaryMonitor() : nullptr;
        m_window = glfwCreateWindow(m_width, m_height, title, monitor, nullptr);

        if (!m_window) {
            LOG_ERROR("[Window] Failed to create GLFW window!");
            glfwTerminate();
            throw std::runtime_error("Failed to create GLFW window");
        }
        LOG_INFO("[Window] GLFW window created successfully.");
    
#ifdef _WIN32
        applyWindowIconFromResource(m_window);
#endif

        glfwSetWindowUserPointer(m_window, this);
        glfwMakeContextCurrent(m_window);
        LOG_INFO("[Window] OpenGL context made current.");

        // meshes, textures, and framebuffers. Load GLAD before any subsystem
        // creates those resources, regardless of the selected render API.
        if (!gladLoadGLLoader(reinterpret_cast<GLADloadproc>(glfwGetProcAddress))) {
            LOG_CRITICAL("[Window] GLAD initialization failed after creating the OpenGL context.");
            glfwDestroyWindow(m_window);
            m_window = nullptr;
            glfwTerminate();
            throw std::runtime_error("GLAD initialization failed");
        }
        const char* glVersion = reinterpret_cast<const char*>(glGetString(GL_VERSION));
        LOG_INFO(std::string("[Window] GLAD initialized. OpenGL version: ") +
            (glVersion ? glVersion : "unknown"));

        // ��������� VSync
        setVSync(displaySettings.vsync);

        glfwSetInputMode(m_window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
        LOG_INFO("[Window] Input mode set to CURSOR_DISABLED.");
    }

    Window::~Window() {
        LOG_INFO("[Window] Destroying window and terminating GLFW...");
        // ��������� ������� ��������� ����� ���������
        DisplaySettings::getInstance().apply();
        if (m_window) {
            glfwDestroyWindow(m_window);
        }
        glfwTerminate();
        LOG_INFO("[Window] Window destroyed.");
    }

    bool Window::shouldClose() const {
        return glfwWindowShouldClose(m_window);
    }

    void Window::swapBuffers() {
        glfwSwapBuffers(m_window);
    }

    void Window::pollEvents() {
        glfwPollEvents();
    }

    void Window::setVSync(bool enabled) {
        DisplaySettings::getInstance().vsync = enabled;
        int swapInterval = enabled ? 1 : 0;
        glfwSwapInterval(swapInterval);
        LOG_INFO(std::string("[Window] glfwSwapInterval updated: ") + std::to_string(swapInterval));
    }

    void Window::setFullscreen(bool fullscreen) {
        LOG_INFO(std::string("[Window] setFullscreen called: ") + (fullscreen ? "true" : "false"));
        GLFWmonitor* monitor = glfwGetPrimaryMonitor();
        const GLFWvidmode* mode = glfwGetVideoMode(monitor);
        DisplaySettings& displaySettings = DisplaySettings::getInstance();

        if (fullscreen && !m_isFullscreen) {
            LOG_INFO("[Window] Switching to Fullscreen mode.");

            // ���������� ������������ ��������� ���� �� �������� � ������������� �����
            m_windowedW = displaySettings.windowWidth;
            m_windowedH = displaySettings.windowHeight;
            glfwGetWindowPos(m_window, &m_windowedX, &m_windowedY);

            glfwSetWindowMonitor(m_window, monitor, 0, 0, mode->width, mode->height, mode->refreshRate);
            m_width = mode->width;
            m_height = mode->height;
            m_isFullscreen = true;
            displaySettings.fullscreen = true;
        }
        else if (!fullscreen && m_isFullscreen) {
            LOG_INFO("[Window] Restoring Windowed mode.");

            glfwSetWindowMonitor(m_window, nullptr, m_windowedX, m_windowedY, m_windowedW, m_windowedH, 0);
            m_width = m_windowedW;
            m_height = m_windowedH;
            m_isFullscreen = false;
            displaySettings.fullscreen = false;

            // ��������������� ������������ �������� � ���������� ����
            displaySettings.windowWidth = m_windowedW;
            displaySettings.windowHeight = m_windowedH;
        }

        setVSync(displaySettings.vsync);

        // ROOT CAUSE FIX: glfwSetWindowMonitor() *should* fire the framebuffer
        // size callback, but on some platforms/drivers it can be delayed by a
        // frame (or not fire before the next render happens), leaving the GL
        // viewport pointed at the pre-switch size for one frame. That stale,
        // smaller viewport is exactly what produces the grey bars along the
        // edges seen when entering fullscreen (and, symmetrically, when a
        // resolution change is applied). Force the viewport to the real,
        // current framebuffer size immediately so nothing is left stale.
        int fbWidth = 0, fbHeight = 0;
        glfwGetFramebufferSize(m_window, &fbWidth, &fbHeight);
        glViewport(0, 0, fbWidth, fbHeight);

        // ��� ������ ��������� ������ ���� ��� ��� ������������ ������
        displaySettings.apply();
    }

    void Window::toggleFullscreen() {
        LOG_INFO("[Window] toggleFullscreen triggered.");
        setFullscreen(!m_isFullscreen);
        framebufferSizeCallback(m_window, m_width, m_height);
    }

    void Window::setCallbacks(Lindo::Input::Input* input, Lindo::Graphics::UI::UIManager* uiManager) {
        LOG_INFO("[Window] Registering window callbacks...");
        m_input = input;
        m_uiManager = uiManager;

        glfwSetFramebufferSizeCallback(m_window, framebufferSizeCallback);
        glfwSetCursorPosCallback(m_window, cursorPosCallback);
        glfwSetMouseButtonCallback(m_window, mouseButtonCallback);
        glfwSetScrollCallback(m_window, scrollCallback);
        glfwSetKeyCallback(m_window, keyCallback);

        glfwSetCharCallback(m_window, [](GLFWwindow* window, unsigned int codepoint) {
            Window* win = static_cast<Window*>(glfwGetWindowUserPointer(window));
            if (win && win->m_input) {
                win->m_input->onChar(codepoint);
            }
            });

        LOG_INFO("[Window] Callbacks registered successfully.");
    }

    void Window::framebufferSizeCallback(GLFWwindow* window, int w, int h) {
        if (h == 0) h = 1;

        Window* win = static_cast<Window*>(glfwGetWindowUserPointer(window));
        if (!win) return;

        DisplaySettings& displaySettings = DisplaySettings::getInstance();

        // ��������� ������� �������� ��� �������, �� �� ��������� � ����
        if (displaySettings.windowWidth != w || displaySettings.windowHeight != h) {
            displaySettings.windowWidth = w;
            displaySettings.windowHeight = h;
        }

        win->m_width = w;
        win->m_height = h;

        // ROOT CAUSE FIX: previously nothing here touched the GL viewport, so
        // it kept whatever dimensions the last bound framebuffer (offscreen or
        // default) had left it at. Sync it to the new size the instant we
        // learn about a resize, instead of only relying on downstream
        // consumers (UI manager / resize callback) to eventually do it.
        glViewport(0, 0, w, h);

        if (win->m_uiManager) win->m_uiManager->onResize(w, h);
        if (win->m_resizeCallback) win->m_resizeCallback(w, h);
    }

    void Window::cursorPosCallback(GLFWwindow* window, double x, double y) {
        Window* win = static_cast<Window*>(glfwGetWindowUserPointer(window));
        if (!win) return;

        if (win->m_input) win->m_input->onMouseMove(x, y);
        if (win->m_uiManager) win->m_uiManager->onMouseMove((float)x, (float)y);
    }

    void Window::mouseButtonCallback(GLFWwindow* window, int button, int action, int mods) {
        Window* win = static_cast<Window*>(glfwGetWindowUserPointer(window));
        if (!win) return;

        if (win->m_input) win->m_input->onMouseButton(button, action);
        if (win->m_uiManager) {
            double x, y;
            glfwGetCursorPos(window, &x, &y);
            win->m_uiManager->onMouseButton((float)x, (float)y, button, action == GLFW_PRESS);
        }
    }

    void Window::scrollCallback(GLFWwindow* window, double xoffset, double yoffset) {
        Window* win = static_cast<Window*>(glfwGetWindowUserPointer(window));
        if (win && win->m_input) {
            win->m_input->onScroll(yoffset);
        }
    }

    void Window::keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods) {
        Window* win = static_cast<Window*>(glfwGetWindowUserPointer(window));
        if (!win) return;

        if (win->m_input) {
            win->m_input->onKey(key, action);
        }

        if (action == GLFW_PRESS || action == GLFW_REPEAT) {
            bool ctrlPressed = (mods & GLFW_MOD_CONTROL) != 0;

            if (win->m_uiManager) {
                auto* console = win->m_uiManager->getConsole();
                if (console && console->isVisible()) {
                    if (ctrlPressed) {
                        if (key == GLFW_KEY_A) console->onSelectAll();
                        else if (key == GLFW_KEY_C) console->onCopy();
                        else if (key == GLFW_KEY_V) console->onPaste();
                        else if (key == GLFW_KEY_X) console->onCut();
                    }
                    else if (key == GLFW_KEY_BACKSPACE) console->onBackspace();
                    else if (key == GLFW_KEY_DELETE) console->onDeleteForward();
                    else if (key == GLFW_KEY_ENTER || key == GLFW_KEY_KP_ENTER) console->onEnter();
                    else if (key == GLFW_KEY_TAB) console->onTab();
                    else if (key == GLFW_KEY_ESCAPE) console->onEscape();
                    else if (key == GLFW_KEY_HOME) console->onHome();
                    else if (key == GLFW_KEY_END) console->onEnd();
                    else if (key == GLFW_KEY_LEFT) console->onMoveCursorLeft();
                    else if (key == GLFW_KEY_RIGHT) console->onMoveCursorRight();
                    else if (key == GLFW_KEY_UP) console->onHistoryUp();
                    else if (key == GLFW_KEY_DOWN) console->onHistoryDown();
                }
            }
        }
    }

    void Window::showSplashScreen(int width, int height, const std::string& title) {
        LOG_INFO("[Window] Showing splash screen...");
        glfwWindowHint(GLFW_DECORATED, GLFW_FALSE);
        GLFWwindow* splash = glfwCreateWindow(width, height, title.c_str(), nullptr, nullptr);
        if (!splash) {
            LOG_ERROR("[Window] Failed to create splash screen window!");
            return;
        }

        glfwMakeContextCurrent(splash);

        Lindo::Graphics::RenderCommand::SetClearColor(glm::vec4(0.15f, 0.15f, 0.15f, 1.0f));
        Lindo::Graphics::RenderCommand::Clear(true, true);
        glfwSwapBuffers(splash);
        glfwPollEvents();

        glfwMakeContextCurrent(nullptr);
        glfwDestroyWindow(splash);
        glfwDefaultWindowHints();
        LOG_INFO("[Window] Splash screen closed.");
    }

    void Window::setResizeCallback(std::function<void(int, int)> callback) {
        m_resizeCallback = callback;
    }
}