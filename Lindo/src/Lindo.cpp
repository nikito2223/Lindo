#pragma execution_character_set("utf-8")
#include "core/OGL.h"
#include "core/Input.h"
#include "core/WindowCallbacks.h"

#include <iostream>
#include <fstream>
#include <utils/model.h>
#include <render/Scene/Scene.h>
#include <render/skybox/skybox.h>
#include <core/Globals.h>

#include <array>
#include <string>
#include <render/UI/UIRenderer.h>
#include "debug/DebugOverlay.h"

static DebugOverlay g_debugOverlay;

bool fileExists(const std::string& path) {
    std::ifstream file(path);
    return file.good();
}

// Пост-обработка (Depth of Field)
static unsigned int framebuffer = 0;
static unsigned int texColor = 0;
static unsigned int texDepth = 0;
static unsigned int quadVAO = 0;
static unsigned int quadVBO = 0;
static Shader* postShader = nullptr;

static UIRenderer g_uiRenderer;
static UIFont* g_font = nullptr;
static std::shared_ptr<UIPanel> g_rootPanel;

void initUI() {
    g_uiRenderer.init();

    std::string charset =
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789 "
        ".,!?-+*/=()[]{}<>:;\"'%@#&"
        "АБВГДЕЁЖЗИЙКЛМНОПРСТУФХЦЧШЩЪЫЬЭЮЯ"
        "абвгдеёжзийклмнопрстуфхцчшщъыьэюя";

    // Добавляем русские буквы через их UTF-8 последовательности
    // Заглавные русские буквы А-Я (кроме Ё)
    for (int c = 0x410; c <= 0x42F; ++c) {
        char utf8[4] = { 0 };
        if (c < 0x80) {
            utf8[0] = c;
        }
        else if (c < 0x800) {
            utf8[0] = 0xC0 | (c >> 6);
            utf8[1] = 0x80 | (c & 0x3F);
        }
        else {
            utf8[0] = 0xE0 | (c >> 12);
            utf8[1] = 0x80 | ((c >> 6) & 0x3F);
            utf8[2] = 0x80 | (c & 0x3F);
        }
        charset += utf8;
    }
    // Добавляем Ё (код 0x401)
    charset += "\xD0\x81"; // UTF-8 для Ё
    // Строчные русские буквы а-я (кроме ё)
    for (int c = 0x430; c <= 0x44F; ++c) {
        char utf8[4] = { 0 };
        if (c < 0x80) {
            utf8[0] = c;
        }
        else if (c < 0x800) {
            utf8[0] = 0xC0 | (c >> 6);
            utf8[1] = 0x80 | (c & 0x3F);
        }
        else {
            utf8[0] = 0xE0 | (c >> 12);
            utf8[1] = 0x80 | ((c >> 6) & 0x3F);
            utf8[2] = 0x80 | (c & 0x3F);
        }
        charset += utf8;
    }
    // Добавляем ё (код 0x451)
    charset += "\xD1\x91"; // UTF-8 для ё

    // Загружаем шрифт (путь к ttf)
    g_font = new UIFont();
    if (!g_font->isLoaded()) {
        if (!g_font->loadFromFile("C:/Windows/Fonts/arial.ttf", 24.0f, 512, 512, charset)) {
            std::cerr << "Failed to load font!" << std::endl;
        }
    }

    g_font->debugPrintGlyphs();
    //// Создаём корневую панель на весь экран
    g_rootPanel = std::make_shared<UIPanel>();

    g_rootPanel->setPosition(0, 0);
    g_rootPanel->setSize(SCR_WIDTH, SCR_HEIGHT);


    //// Добавляем элементы
    //auto btn = std::make_shared<UIButton>("Нажми меня><", []() {
    //    std::cout << "Button clicked!" << std::endl;
    //    });
    //btn->setPosition(100, 100);
    //btn->setSize(120, 30);
    //btn->setTextSize(16.0f); // большой текст
    //g_rootPanel->addChild(btn);

    //auto label = std::make_shared<UILabel>("Hello, UI!");
    //label->setPosition(100, 150);
    //g_rootPanel->addChild(label);

    g_debugOverlay.init(g_font);
    g_debugOverlay.setVisible(false); // по умолчанию выключен
}

void renderUI() {
    if (!g_rootPanel) return;
    g_uiRenderer.beginFrame(SCR_WIDTH, SCR_HEIGHT);
    g_rootPanel->render(g_uiRenderer, g_font);
    g_uiRenderer.endFrame();
}


std::vector<unsigned char> generateEncryptionKey() {
    // Стабильные факторы, не зависящие от сцены
    const std::string gameName = "Lindo";
    const std::string version = "1.0.0";   // меняйте только при необходимости перешифровать ресурсы
    const std::string salt = "G4m3D3v"; // дополнительная соль

    std::string combined = gameName + version + salt;

    // Простой XOR-хеш для получения 16 байт
    std::array<unsigned char, 16> key = {};
    for (size_t i = 0; i < combined.size(); ++i) {
        key[i % 16] ^= static_cast<unsigned char>(combined[i]);
    }

    // Дополнительное смешивание с фиксированными константами для надёжности
    const unsigned char fixed[] = {
        0xA5, 0x5A, 0x3C, 0xC3, 0x69, 0x96, 0x12, 0x21,
        0x34, 0x43, 0x56, 0x65, 0x78, 0x87, 0x9A, 0xBC
    };
    for (int i = 0; i < 16; ++i) {
        key[i] ^= fixed[i];
    }

    return std::vector<unsigned char>(key.begin(), key.end());
}

void initPostProcessing() {
    // Создаём FBO
    glGenFramebuffers(1, &framebuffer);
    glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);

    // Текстура цвета
    glGenTextures(1, &texColor);
    glBindTexture(GL_TEXTURE_2D, texColor);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, SCR_WIDTH, SCR_HEIGHT, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, texColor, 0);

    // Текстура глубины
    glGenTextures(1, &texDepth);
    glBindTexture(GL_TEXTURE_2D, texDepth);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, SCR_WIDTH, SCR_HEIGHT, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, texDepth, 0);

    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
        std::cout << "ERROR: Framebuffer not complete!" << std::endl;

    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    // Полноэкранный квад
    float quadVertices[] = {
        // positions   // texCoords
        -1.0f,  1.0f,  0.0f, 1.0f,
        -1.0f, -1.0f,  0.0f, 0.0f,
         1.0f, -1.0f,  1.0f, 0.0f,

        -1.0f,  1.0f,  0.0f, 1.0f,
         1.0f, -1.0f,  1.0f, 0.0f,
         1.0f,  1.0f,  1.0f, 1.0f
    };
    glGenVertexArrays(1, &quadVAO);
    glGenBuffers(1, &quadVBO);
    glBindVertexArray(quadVAO);
    glBindBuffer(GL_ARRAY_BUFFER, quadVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), quadVertices, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));
    glBindVertexArray(0);

    // Шейдер пост-обработки
    postShader = new Shader(PathData + "shaders/PostProcess/postprocess.vert", PathData + "shaders/PostProcess/postprocess.frag");
}

int main()
{
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWmonitor* primaryMonitor = glfwGetPrimaryMonitor();
    const GLFWvidmode* videoMode = glfwGetVideoMode(primaryMonitor);

    GLFWwindow* window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "Lingo", nullptr, nullptr);
    if (!window) {
        glfwTerminate();
        return -1;
    }

    uiActive = true;
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);

    auto toggleFullscreen = [&]() {
        static bool isFullscreen = false;
        static int windowedXPos = 100, windowedYPos = 100;
        static int windowedWidth = SCR_WIDTH, windowedHeight = SCR_HEIGHT;

        if (!isFullscreen) {
            glfwGetWindowPos(window, &windowedXPos, &windowedYPos);
            glfwGetWindowSize(window, &windowedWidth, &windowedHeight);
            glfwSetWindowMonitor(window, primaryMonitor, 0, 0,
                videoMode->width, videoMode->height,
                videoMode->refreshRate);
            SCR_WIDTH = videoMode->width;
            SCR_HEIGHT = videoMode->height;
            isFullscreen = true;
            if (g_rootPanel) {
                g_rootPanel->setSize(SCR_WIDTH, SCR_HEIGHT);
            }

            std::cout << "Fullscreen: " << SCR_WIDTH << "x" << SCR_HEIGHT << std::endl;
        }
        else {
            glfwSetWindowMonitor(window, nullptr, windowedXPos, windowedYPos,
                windowedWidth, windowedHeight, 0);
            SCR_WIDTH = windowedWidth;
            SCR_HEIGHT = windowedHeight;
            isFullscreen = false;
            std::cout << "Windowed: " << SCR_WIDTH << "x" << SCR_HEIGHT << std::endl;
        }
        lastX = SCR_WIDTH / 2.0f;
        lastY = SCR_HEIGHT / 2.0f;
        firstMouse = true;
    };

    glfwMakeContextCurrent(window);

    glfwSetFramebufferSizeCallback(window, [](GLFWwindow* window, int width, int height) {
        SCR_WIDTH = width;
        SCR_HEIGHT = height;
        glViewport(0, 0, width, height);
        lastX = width / 2.0f;
        lastY = height / 2.0f;
        firstMouse = true;
        if (g_rootPanel) {
            g_rootPanel->setSize(width, height);
        }
        });

    glfwSetCursorPosCallback(window, [](GLFWwindow* window, double xpos, double ypos) {
        mouse_callback(window, xpos, ypos); // если тебе нужно для камеры

        if (uiActive && g_rootPanel) {
            g_rootPanel->onMouseMove((float)xpos, (float)ypos);
        }
        });
    glfwSetMouseButtonCallback(window, [](GLFWwindow* window, int button, int action, int mods) {
        double x, y;
        glfwGetCursorPos(window, &x, &y);
        if (uiActive && g_rootPanel) {
            g_rootPanel->onMouseButton((float)x, (float)y, button, action == GLFW_PRESS);
        }
        });
    glfwSetScrollCallback(window, scroll_callback);
    glfwSetInputMode(window, GLFW_CURSOR, uiActive ? GLFW_CURSOR_NORMAL : GLFW_CURSOR_DISABLED);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        glfwTerminate();
        return -1;
    }

    glEnable(GL_DEPTH_TEST);
    glClearColor(0.1f, 0.1f, 0.15f, 1.0f);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    auto encryptionKey = generateEncryptionKey();

    if (useRawResources) {
        std::cout << "=== ENCRYPTION KEY FOR ASSET TOOL ===" << std::endl;
        std::cout << "const std::vector<unsigned char> KEY = {" << std::endl;
        std::cout << "    ";
        for (size_t i = 0; i < encryptionKey.size(); ++i) {
            printf("0x%02X", encryptionKey[i]);
            if (i < encryptionKey.size() - 1) {
                std::cout << ", ";
            }
            if ((i + 1) % 8 == 0 && i < encryptionKey.size() - 1) {
                std::cout << "\n    ";
            }
        }
        std::cout << "\n};" << std::endl;
        std::cout << "=====================================" << std::endl;
    }

    CryptoUtils::setKey(encryptionKey);


    bool showLightIcons = true;
    float lightIconRadius = 0.3f;

    // ==== Загрузка шейдеров ====
    Shader lightingShader(PathData + "shaders/VertexShader.vs", PathData + "shaders/FragmentShader.fs");
    Shader skyboxShader(PathData + "shaders/skybox/skybox.vs", PathData + "shaders/skybox/skybox.fs");

    std::cout << PathData << std::endl;

    Skybox* skybox = nullptr;
    std::string hdrPath = PathData + "textures/skybox/1.hdr";
    if (fileExists(hdrPath)) {
        if (useRawResources) {
            // Режим разработки: читаем файл напрямую
            std::cout << "Loading HDR skybox from: " << hdrPath << std::endl;
            skybox = new Skybox(hdrPath, 512);
        }
        else {
            // Режим продакшена: файл должен быть зашифрован
            std::cout << "Loading encrypted HDR skybox from: " << hdrPath << std::endl;
            try {
                std::vector<char> hdrData = CryptoUtils::decryptFileBinary(hdrPath);
                skybox = Skybox::CreateFromHDRData(hdrData, 512);
            }
            catch (const std::exception& e) {
                std::cout << "Failed to decrypt/load HDR: " << e.what() << std::endl;
                // fallback на LDR
                skybox = nullptr; // чтобы перейти к LDR
            }
        }
    }
    else {
        std::cout << "HDR file not found, loading LDR cubemap..." << std::endl;
        skybox = new Skybox({
            PathData + "textures/skybox/right.jpg",
            PathData + "textures/skybox/left.jpg",
            PathData + "textures/skybox/top.jpg",
            PathData + "textures/skybox/bottom.jpg",
            PathData + "textures/skybox/front.jpg",
            PathData + "textures/skybox/back.jpg"
            });
    }

    // Инициализация сцены
    initScene();
    initUI();
    initPostProcessing();

    bool f11Pressed = false;

    while (!glfwWindowShouldClose(window))
    {
        float time = (float)glfwGetTime();
        float deltaTime = time - lastFrame;
        if (deltaTime > 0.05f) deltaTime = 0.05f;
        lastFrame = time;

        processInput(window);
        // Обработка F3
        static bool f3Pressed = false;
        if (glfwGetKey(window, GLFW_KEY_F3) == GLFW_PRESS && !f3Pressed) {
            DebugMode = !DebugMode;
            g_debugOverlay.toggle(); // Переключаем видимость оверлея
            f3Pressed = true;
            std::cout << "Debug mode: " << (DebugMode ? "ON" : "OFF") << std::endl;
        }
        if (glfwGetKey(window, GLFW_KEY_F3) == GLFW_RELEASE) {
            f3Pressed = false;
        }

        // Обработка F11
        if (glfwGetKey(window, GLFW_KEY_F11) == GLFW_PRESS && !f11Pressed) {
            toggleFullscreen();
            f11Pressed = true;
        }
        if (glfwGetKey(window, GLFW_KEY_F11) == GLFW_RELEASE) {
            f11Pressed = false;
        }

        static bool escapePressed = false;
        if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS && !escapePressed) {
            uiActive = !uiActive;
            if (uiActive) {
                glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
            }
            else {
                glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
            }
            escapePressed = true;
        }
        if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_RELEASE) {
            escapePressed = false;
        }

        // Очистка буферов
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // ==== Рендер сцены ====
        lightingShader.use();
        lightingShader.setBool("debugMode", DebugMode);
        lightingShader.setBool("showLightIcons", showLightIcons);
        lightingShader.setFloat("lightIconRadius", lightIconRadius);


        renderScene(lightingShader, deltaTime, nullptr);


        // ==== Рендер skybox ====
        FirstPersonCamera* cam = getActiveCamera();
        if (cam) {
            glm::mat4 view = cam->getViewMatrix();
            glm::mat4 skyboxView = glm::mat4(glm::mat3(view));
            glm::mat4 projection = glm::perspective(
                glm::radians(cam->getZoom()),
                (float)SCR_WIDTH / SCR_HEIGHT,
                0.1f, 100.0f
            );

            Player* player = getPlayer();

            if (cam && player) {
                // Движение
                glm::vec3 moveDir(0.0f);
                if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
                    moveDir += glm::vec3(cam->getFront().x, 0.0f, cam->getFront().z);
                if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
                    moveDir -= glm::vec3(cam->getFront().x, 0.0f, cam->getFront().z);
                if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
                    moveDir -= glm::vec3(cam->getRight().x, 0.0f, cam->getRight().z);
                if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
                    moveDir += glm::vec3(cam->getRight().x, 0.0f, cam->getRight().z);
                if (glm::length(moveDir) > 0.0f)
                    moveDir = glm::normalize(moveDir);
                player->move(moveDir);

                // Прыжок
                static bool spacePressed = false;
                if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS && !spacePressed) {
                    player->jump();
                    spacePressed = true;
                }
                if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_RELEASE)
                    spacePressed = false;

                // Приседание
                static bool shiftPressed = false;
                if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS && !shiftPressed) {
                    player->setCrouching(true);
                    shiftPressed = true;
                }
                if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_RELEASE && shiftPressed) {
                    player->setCrouching(false);
                    shiftPressed = false;
                }
            }

            if (skybox) {
                GLboolean depthMaskEnabled;
                glGetBooleanv(GL_DEPTH_WRITEMASK, &depthMaskEnabled);
                GLint cullFaceMode;
                glGetIntegerv(GL_CULL_FACE_MODE, &cullFaceMode);
                GLboolean cullFaceEnabled;
                glGetBooleanv(GL_CULL_FACE, &cullFaceEnabled);

                glDepthFunc(GL_LEQUAL);
                glDepthMask(GL_FALSE);
                glDisable(GL_CULL_FACE);

                skyboxShader.use();
                skyboxShader.setMat4("view", skyboxView);
                skyboxShader.setMat4("projection", projection);
                skybox->Draw(skyboxShader);

                glDepthFunc(GL_LESS);
                glDepthMask(depthMaskEnabled);
                if (cullFaceEnabled) glEnable(GL_CULL_FACE);
                glCullFace(cullFaceMode);
            }
        }


        // Обновление заголовка окна
        static float fpsTimer = 0.0f;
        static int frameCount = 0;
        fpsTimer += deltaTime;
        frameCount++;

        glm::vec3 pos = getCharacterPosition();
        
        if (fpsTimer >= 1.0f) {
            // Обновляем отладочный оверлей
            g_debugOverlay.update(fpsTimer, frameCount, pos, DebugMode);

            fpsTimer = 0.0f;
            frameCount = 0;
        }

        // Сохраняем состояние
        GLboolean depthTestEnabled;
        glGetBooleanv(GL_DEPTH_TEST, &depthTestEnabled);

        // Отключаем тест глубины для UI (или оставляем, но отключаем запись)
        glDisable(GL_DEPTH_TEST);  // Проще всего отключить полностью

        renderUI();

        g_debugOverlay.render(g_uiRenderer);

        // Восстанавливаем состояние, если нужно для дальнейшего рендера
        if (depthTestEnabled) {
            glEnable(GL_DEPTH_TEST);
        }
        
        glGenerateMipmap(GL_TEXTURE_2D);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY, 16.0f); // если доступно

        glfwSwapBuffers(window);
        glfwPollEvents();

    }

    // Очистка
    cleanupScene();
    if (skybox) delete skybox;

    glfwTerminate();
    return 0;
}