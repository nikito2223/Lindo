#include "Renderer.h"
#include "utils/SceneManager.h"
#include "graphics/ui/UIManager.h"
#include "debug/DebugOverlay.h"
#include "graphics/render/skybox.h"
#include "core/Globals.h"
#include "Objects/Player.h"
#include <glm/gtc/matrix_transform.hpp>
#include <iostream>
#include <world/Scene.h>

// Вспомогательная функция проверки существования файла (можно вынести в utils)
static bool fileExists(const std::string& path) {
    std::ifstream f(path);
    return f.good();
}

Renderer::Renderer(SceneManager* scene, UIManager* ui, DebugOverlay* debug)
    : m_sceneManager(scene), m_uiManager(ui), m_debugOverlay(debug) {}

Renderer::~Renderer() {
    delete m_lightingShader;
    delete m_skyboxShader;
    delete m_skybox;
}

void Renderer::init() {
    m_lightingShader = new Shader(PathData + "shaders/VertexShader.vert", PathData + "shaders/FragmentShader.frag");
    m_skyboxShader = new Shader(PathData + "shaders/skybox/skybox.vs", PathData + "shaders/skybox/skybox.fs");

    // Загрузка skybox
    std::string hdrPath = PathData + "textures/skybox/1.hdr";
    if (fileExists(hdrPath)) {
        if (useRawResources) {
            std::cout << "Loading HDR skybox from: " << hdrPath << std::endl;
            m_skybox = new Skybox(hdrPath, 512);
        }
        else {
            std::cout << "Loading encrypted HDR skybox from: " << hdrPath << std::endl;
            try {
                std::vector<char> hdrData = CryptoUtils::decryptFileBinary(hdrPath);
                m_skybox = Skybox::CreateFromHDRData(hdrData, 512);
            }
            catch (const std::exception& e) {
                std::cerr << "Failed to decrypt/load HDR: " << e.what() << std::endl;
                m_skybox = nullptr;
            }
        }
    }

    if (!m_skybox) {
        std::cout << "Loading LDR cubemap..." << std::endl;
        m_skybox = new Skybox({
            PathData + "textures/skybox/right.jpg",
            PathData + "textures/skybox/left.jpg",
            PathData + "textures/skybox/top.jpg",
            PathData + "textures/skybox/bottom.jpg",
            PathData + "textures/skybox/front.jpg",
            PathData + "textures/skybox/back.jpg"
            });
    }
}

void Renderer::render(float deltaTime) {
    // Очистка буферов
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    Globals::Time += deltaTime;

    // Рендер сцены
    if (m_sceneManager && m_lightingShader) {
        m_sceneManager->render(*m_lightingShader, deltaTime, m_debugOverlay->isVisible(), m_showLightIcons, m_lightIconRadius);
    }

    DebugMode = m_debugOverlay->isVisible();

    // Рендер skybox
    Player* player = m_sceneManager ? m_sceneManager->getPlayer() : nullptr;
    if (!player) std::cerr << "ERROR: No player in scene!" << std::endl;
    if (player && m_skybox && m_skyboxShader) {
        auto* cam = player->owner->getComponent<Camera>();
        glm::mat4 view =       cam->getViewMatrix();
        glm::mat4 skyboxView = glm::mat4(glm::mat3(view)); // убираем трансляцию
        glm::mat4 projection = cam->getProjectionMatrix();
        
        // Сохраняем состояние OpenGL
        GLboolean depthMask;
        glGetBooleanv(GL_DEPTH_WRITEMASK, &depthMask);
        GLint cullFace;
        glGetIntegerv(GL_CULL_FACE_MODE, &cullFace);
        GLboolean cullEnabled;
        glGetBooleanv(GL_CULL_FACE, &cullEnabled);

        glDepthFunc(GL_LEQUAL);
        glDepthMask(GL_FALSE);
        glDisable(GL_CULL_FACE);

        //if (hasDirectionalLight()) {
        //    m_skyboxShader->use();
        //    m_skyboxShader->setMat4("view", skyboxView);
        //    m_skyboxShader->setMat4("projection", projection);
        //    m_skybox->Draw(*m_skyboxShader);
        //}
        //else {
        //    // Можно очистить экран черным цветом
        //    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        //}

        m_skyboxShader->use();
        m_skyboxShader->setMat4("view", skyboxView);
        m_skyboxShader->setMat4("projection", projection);
        m_skybox->Draw(*m_skyboxShader);

        // Восстанавливаем
        glDepthFunc(GL_LESS);
        glDepthMask(depthMask);
        if (cullEnabled) glEnable(GL_CULL_FACE);
        glCullFace(cullFace);
    }

    // Рендер UI (поверх всего)
    if (m_uiManager) {
        // Отключаем тест глубины для UI
        glDisable(GL_DEPTH_TEST);
        m_uiManager->render();
        glEnable(GL_DEPTH_TEST);
    }

    // Рендер дебаг-оверлея
    if (m_debugOverlay && m_debugOverlay->isVisible()) {
        if (m_uiManager && m_uiManager->getRenderer()) {
            m_debugOverlay->render(*m_uiManager->getRenderer());
        }
        else {
            std::cerr << "Warning: UIManager or its renderer is null, cannot render debug overlay" << std::endl;
        }
    }
}
void Renderer::onResize(int width, int height) {
    SCR_WIDTH = width;
    SCR_HEIGHT = height;

    // Защита от нуля
    if (height == 0) height = 1;

    if (m_sceneManager) {
        Player* player = m_sceneManager->getPlayer();
        if (player) {
            auto* cam = player->owner->getComponent<Camera>();
            cam->setAspectRatio((float)width / (float)height);
            cam->setNearFar(0.1f, 100.0f);
        }
    }

    glViewport(0, 0, width, height); // чтобы точно синхронизировать
}