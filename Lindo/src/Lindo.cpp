#include "core/OGL.h"
#include "core/Globals.h"
#include "core/Input.h"
#include "core/WindowCallbacks.h"

#include <iostream>
#include <fstream>
#include <utils/model.h>
#include <render/Scene/Scene.h>
#include <render/skybox/skybox.h>

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
    postShader = new Shader("res/shaders/PostProcess/postprocess.vert", "res/shaders/PostProcess/postprocess.frag");
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
        });

    glfwSetCursorPosCallback(window, mouse_callback);
    glfwSetScrollCallback(window, scroll_callback);
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    cursorMode = GLFW_CURSOR_DISABLED;

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        glfwTerminate();
        return -1;
    }

    glEnable(GL_DEPTH_TEST);
    glClearColor(0.1f, 0.1f, 0.15f, 1.0f);


    bool debugMode = false;
    bool showLightIcons = true;
    float lightIconRadius = 0.3f;

    // ==== Загрузка шейдеров ====
    Shader lightingShader("res/shaders/VertexShader.vs", "res/shaders/FragmentShader.fs");
    Shader skyboxShader("res/shaders/skybox/skybox.vs", "res/shaders/skybox/skybox.fs");

    Skybox* skybox = nullptr;
    std::string hdrPath = "res/textures/skybox/1.hdr";
    if (fileExists(hdrPath)) {
        std::cout << "Loading HDR skybox from: " << hdrPath << std::endl;
        skybox = new Skybox(hdrPath, 512);
    }
    else {
        std::cout << "HDR file not found, loading LDR cubemap..." << std::endl;
        skybox = new Skybox({
            "res/textures/skybox/right.jpg",
            "res/textures/skybox/left.jpg",
            "res/textures/skybox/top.jpg",
            "res/textures/skybox/bottom.jpg",
            "res/textures/skybox/front.jpg",
            "res/textures/skybox/back.jpg"
            });
    }
    // Инициализация сцены
    initScene();
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
            debugMode = !debugMode;
            f3Pressed = true;
            std::cout << "Debug mode: " << (debugMode ? "ON" : "OFF") << std::endl;
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

        // Очистка буферов
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // ==== Рендер сцены ====
        lightingShader.use();
        lightingShader.setBool("debugMode", debugMode);
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
            std::string title = "Lingo - " + std::to_string(SCR_WIDTH) + "x" + std::to_string(SCR_HEIGHT) +
                " | FPS: " + std::to_string((int)frameCount) +
                " | Pos: (" + std::to_string(pos.x) + ", " + std::to_string(pos.y) + ", " + std::to_string(pos.z) + ")";
            if (debugMode) title += " [DEBUG]";
            glfwSetWindowTitle(window, title.c_str());

            fpsTimer = 0.0f;
            frameCount = 0;
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