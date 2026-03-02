#include "SceneManager.h"
#include "world/Scene.h"      // предположим, что тут объявлены initScene, renderScene, cleanupScene, getPlayer, getCharacterPosition
#include "Objects/Player.h"
#include "core/Input.h"

SceneManager::SceneManager() = default;

SceneManager::~SceneManager() = default;

void SceneManager::init() {
    initScene(); // глобальная функция из Scene.h
}

void SceneManager::update(float deltaTime, Input * input) {
    Player* player = getPlayer();
    if (!player) return;

    // Движение на основе ввода
    Camera& cam = player->getCamera();
    glm::vec3 moveDir(0.0f);
    if (input->isForwardPressed())
        moveDir += glm::vec3(cam.getFront().x, 0.0f, cam.getFront().z);
    if (input->isBackwardPressed())
        moveDir -= glm::vec3(cam.getFront().x, 0.0f, cam.getFront().z);
    if (input->isLeftPressed())
        moveDir -= glm::vec3(cam.getRight().x, 0.0f, cam.getRight().z);
    if (input->isRightPressed())
        moveDir += glm::vec3(cam.getRight().x, 0.0f, cam.getRight().z);
    if (glm::length(moveDir) > 0.0f)
        moveDir = glm::normalize(moveDir);
    player->move(moveDir);

    // Прыжок (однократное нажатие)
    if (input->isJumpPressed()) {
        player->jump();
    }

    glm::vec2 mouseDelta = input->getMouseDelta();

    if (mouseDelta.x != 0.0f || mouseDelta.y != 0.0f) {
        player->getCamera().processMouseMovement(mouseDelta.x, mouseDelta.y);
    }

    // После использования обязательно сбросить
    input->resetMouseDelta();

    // Приседание (удержание)
    player->setCrouching(input->isCrouchPressed());

    // Обновление игрока (физика, камера)
    player->update(deltaTime);
}

void SceneManager::onResize(int width, int height) {
    float aspectRatio = (float)width / (float)height;
    if (m_camera) {
        // Обновляем проекционную матрицу камеры
        m_camera->setAspectRatio(aspectRatio);
    }
}

void SceneManager::render(Shader& lightingShader,float deltaTime, bool debugMode, bool showLightIcons, float lightIconRadius)
{
    lightingShader.use();
    lightingShader.setBool("debugMode", debugMode);
    lightingShader.setBool("showLightIcons", showLightIcons);
    lightingShader.setFloat("lightIconRadius", lightIconRadius);

    renderScene(lightingShader, deltaTime, nullptr);
}

void SceneManager::cleanup() {
    cleanupScene();
}

Player* SceneManager::getPlayer() const {
    return ::getPlayer(); // глобальная функция
}

glm::vec3 SceneManager::getCharacterPosition() const {
    return ::getCharacterPosition(); // глобальная функция
}