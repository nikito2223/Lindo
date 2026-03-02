#include "core/OGL.h"
#include "Scene.h"
#include "core/Globals.h"
#include "Graphics/core/Texture.h"
#include "Graphics/core/mesh.h"
#include "Graphics/core/model.h"
//#include "render/mesh/primitives.h"
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <vector>
#include "Graphics/core/ShadowMap.h"
#include "Objects/transform.h"
#include "antires/CryptoUtils.h"

// ===== ГЛОБАЛЬНЫЕ ПЕРЕМЕННЫЕ =====
static Player* player = nullptr;

static unsigned int diffuseMap;
static unsigned int specularMap;

static Light* directionalLight = nullptr;
static const int NR_POINT_LIGHTS = 1;
static Light* pointLights[NR_POINT_LIGHTS] = { nullptr };
static Light* spotLight = nullptr;

static ShadowMap* shadowMap = nullptr;
static Shader* shadowDepthShader = nullptr;
static bool shadowsEnabled = true;

static std::vector<Light*> lights;
static std::vector<Object*> sceneObjects;
static Object* planeObject = nullptr;

unsigned int lightVAO = 0;
unsigned int lightVBO;

// ===== ВСПОМОГАТЕЛЬНЫЕ ФУНКЦИИ =====

void initLightCube() {
    float vertices[] = {
        -0.5f,-0.5f,-0.5f,  0.5f,-0.5f,-0.5f,  0.5f, 0.5f,-0.5f,
         0.5f, 0.5f,-0.5f, -0.5f, 0.5f,-0.5f, -0.5f,-0.5f,-0.5f,
        -0.5f,-0.5f, 0.5f,  0.5f,-0.5f, 0.5f,  0.5f, 0.5f, 0.5f,
         0.5f, 0.5f, 0.5f, -0.5f, 0.5f, 0.5f, -0.5f,-0.5f, 0.5f,
        -0.5f, 0.5f, 0.5f, -0.5f, 0.5f,-0.5f, -0.5f,-0.5f,-0.5f,
        -0.5f,-0.5f,-0.5f, -0.5f,-0.5f, 0.5f, -0.5f, 0.5f, 0.5f,
         0.5f, 0.5f, 0.5f,  0.5f, 0.5f,-0.5f,  0.5f,-0.5f,-0.5f,
         0.5f,-0.5f,-0.5f,  0.5f,-0.5f, 0.5f,  0.5f, 0.5f, 0.5f,
        -0.5f,-0.5f,-0.5f,  0.5f,-0.5f,-0.5f,  0.5f,-0.5f, 0.5f,
         0.5f,-0.5f, 0.5f, -0.5f,-0.5f, 0.5f, -0.5f,-0.5f,-0.5f,
        -0.5f, 0.5f,-0.5f,  0.5f, 0.5f,-0.5f,  0.5f, 0.5f, 0.5f,
         0.5f, 0.5f, 0.5f, -0.5f, 0.5f, 0.5f, -0.5f, 0.5f,-0.5f
    };

    glGenVertexArrays(1, &lightVAO);
    glGenBuffers(1, &lightVBO);

    glBindVertexArray(lightVAO);
    glBindBuffer(GL_ARRAY_BUFFER, lightVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
}

Mesh* createCapsuleMesh(float radius = 0.4f, float height = 1.6f, int segments = 16) {
    std::vector<Vertex> vertices;
    std::vector<unsigned int> indices;

    float halfHeight = height * 0.5f;

    // Верхняя полусфера
    for (int i = 0; i <= segments; ++i) {
        float phi = glm::pi<float>() * 0.5f * i / segments; // от 0 до 90 градусов
        float y = halfHeight + radius * sin(phi);
        float r = radius * cos(phi);

        for (int j = 0; j <= segments; ++j) {
            float theta = 2.0f * glm::pi<float>() * j / segments;
            float x = r * cos(theta);
            float z = r * sin(theta);

            glm::vec3 pos(x, y, z);
            glm::vec3 norm = glm::normalize(glm::vec3(x, y - halfHeight, z));
            glm::vec2 uv((float)j / segments, (float)i / segments);
            vertices.push_back({ pos, norm, uv });
        }
    }

    // Цилиндрическая часть
    for (int i = 0; i <= segments; ++i) {
        float y = halfHeight - height * i / segments;
        for (int j = 0; j <= segments; ++j) {
            float theta = 2.0f * glm::pi<float>() * j / segments;
            float x = radius * cos(theta);
            float z = radius * sin(theta);

            glm::vec3 pos(x, y, z);
            glm::vec3 norm = glm::normalize(glm::vec3(x, 0.0f, z));
            glm::vec2 uv((float)j / segments, (float)(i + segments) / (2 * segments));
            vertices.push_back({ pos, norm, uv });
        }
    }

    // Нижняя полусфера
    for (int i = 0; i <= segments; ++i) {
        float phi = -glm::pi<float>() * 0.5f * i / segments; // от -90 до 0
        float y = -halfHeight + radius * sin(phi);
        float r = radius * cos(phi);

        for (int j = 0; j <= segments; ++j) {
            float theta = 2.0f * glm::pi<float>() * j / segments;
            float x = r * cos(theta);
            float z = r * sin(theta);

            glm::vec3 pos(x, y, z);
            glm::vec3 norm = glm::normalize(glm::vec3(x, y + halfHeight, z));
            glm::vec2 uv((float)j / segments, (float)(i + 2 * segments) / (3 * segments));
            vertices.push_back({ pos, norm, uv });
        }
    }

    // Индексы (для каждого квадрата два треугольника)
    int rows = 3 * segments; // всего рядов: верх/цилиндр/низ
    for (int i = 0; i < rows; ++i) {
        for (int j = 0; j < segments; ++j) {
            int nextRow = (i + 1) % rows;
            int nextCol = (j + 1) % (segments + 1);

            int a = i * (segments + 1) + j;
            int b = i * (segments + 1) + nextCol;
            int c = nextRow * (segments + 1) + j;
            int d = nextRow * (segments + 1) + nextCol;

            indices.push_back(a);
            indices.push_back(b);
            indices.push_back(c);

            indices.push_back(b);
            indices.push_back(d);
            indices.push_back(c);
        }
    }

    return new Mesh(vertices, indices, std::vector<Texture>{});
}

// ===== НАСТРОЙКА КОЛЛИЗИЙ (без создания глобальных коллайдеров) =====
void setupCollisionLayers() {
    auto& collisionSystem = CollisionSystem::getInstance();

    // Настраиваем слои коллизий
    collisionSystem.setCollisionLayer("platform", 1);
    collisionSystem.setCollisionLayer("player", 2);
    collisionSystem.setCollisionLayer("parkour_block", 4);

    collisionSystem.setLayerCollision(1, 2, true);  // Платформа-Игрок
    collisionSystem.setLayerCollision(2, 4, true);  // Игрок-Блоки

    // Коллбэки для отладки
    collisionSystem.registerCollisionCallback("player", "platform",
        [](const CollisionEvent& event) {
            std::cout << "Player collided with platform! Enter=" << event.isEnter
                << " depth=" << event.info.depth << std::endl;
        });
}

// ===== ОТРИСОВКА КОЛЛАЙДЕРОВ =====
void drawColliders(Shader& shader) {
    CollisionSystem::getInstance().drawDebug(shader);
}

struct ParkourBlock {
    glm::vec3 position;
    glm::vec3 size;
};

std::vector<ParkourBlock> blocks = {
    { { -5.0f, 1.0f,  0.0f }, { 2.0f, 0.5f, 2.0f } },
    { {  2.0f, 2.0f, -4.0f }, { 2.0f, 0.5f, 2.0f } },
    { {  7.0f, 3.0f,  3.0f }, { 2.0f, 0.5f, 2.0f } },
    { { -8.0f, 4.0f, -6.0f }, { 2.0f, 0.5f, 2.0f } },
    { {  0.0f, 5.0f,  8.0f }, { 2.0f, 0.5f, 2.0f } }
};

// ===== ИНИЦИАЛИЗАЦИЯ СЦЕНЫ =====
void initScene() {
    initLightCube();

    // ==== Тени ====
    shadowMap = new ShadowMap();
    if (!shadowMap->Init(2048, 2048)) {
        std::cout << "Failed to initialize shadow map!" << std::endl;
    }
    shadowDepthShader = new Shader(PathData + "shaders/Light/shadow_depth.vs", PathData + "shaders/Light/shadow_depth.fs");

    // ==== Освещение ====
    directionalLight = new Light(LightType::Directional);
    directionalLight->direction = glm::vec3(-0.2f, -1.0f, -0.3f);
    directionalLight->enabled = false;
    directionalLight->ambient = glm::vec3(0.2f, 0.2f, 0.2f);
    directionalLight->diffuse = glm::vec3(0.5f, 0.5f, 0.5f);
    directionalLight->specular = glm::vec3(1.0f, 1.0f, 1.0f);
    lights.push_back(directionalLight);

    for (int i = 0; i < NR_POINT_LIGHTS; i++) {
        pointLights[i] = new Light(LightType::Point);
        lights.push_back(pointLights[i]);
    }
    pointLights[0]->transform.position = glm::vec3(0.0f, 2.0f, 0.0f);
    pointLights[0]->constant = 1.0f;
    pointLights[0]->enabled = true;
    pointLights[0]->ambient = glm::vec3(1, 0, 0);
    pointLights[0]->diffuse = glm::vec3(1, 0, 0);
    pointLights[0]->specular = glm::vec3(1, 0, 0);
    pointLights[0]->SetBaseColor(glm::vec3(1.0f, 1.0f, 1.0f));
    pointLights[0]->linear = 0.09f;
    pointLights[0]->quadratic = 0.032f;


    diffuseMap = loadTexture(PathData + "textures/textures.png");
    specularMap = loadTexture(PathData + "textures/textures.png");

    // ==== Платформа ====
    Mesh* planeMesh = new Mesh(
        std::vector<Vertex>{
            { {-15, 0, -15}, { 0,1,0 }, { 0,0 } },
            { { 15, 0, -15}, {0,1,0}, {30,0} },
            { { 15, 0,  15}, {0,1,0}, {30,30} },
            { {-15, 0,  15}, {0,1,0}, {0,30} }
    },
        std::vector<unsigned int>{0, 1, 2, 2, 3, 0},
                std::vector<Texture>{}
            );
    planeObject = new Object(planeMesh);
    planeObject->transform.position = glm::vec3(0.0f, 0.0f, 0.0f);
    // Создаём коллайдер прямо у объекта
    planeObject->collider = std::make_shared<BoxCollider>(glm::vec3(30.0f, 0.1f, 30.0f));
    planeObject->collider->setDebugColor(glm::vec3(0.2f, 0.8f, 0.2f));
    planeObject->hasCollider = true;
    CollisionSystem::getInstance().addCollider(planeObject->collider, "platform");
    sceneObjects.push_back(planeObject);

    Mesh* capsuleMesh = createCapsuleMesh(0.4f, 2.0f);
    Player* playerObj = new Player(capsuleMesh);
    playerObj->transform.position = glm::vec3(0.0f, 1.2f, 0.0f);
    playerObj->castsShadows = true;
    player = playerObj;
    // Игрок не добавляем в sceneObjects, т.к. рисуем отдельно (или можно добавить, но тогда будет дважды)
    sceneObjects.push_back(playerObj);

    Object* capsuleObject = new Object();  // без меша, только коллайдер
    capsuleObject->transform.position = glm::vec3(3.0f, 1.0f, 5.0f); // Пример позиции
    capsuleObject->castsShadows = true;

    // Создаём капсульный коллайдер: радиус 0.5, высота 2.0 (расстояние между центрами полусфер)
    auto capsuleCollider = std::make_shared<CapsuleCollider>(0.4f, 2.0f);
    capsuleCollider->setDebugColor(glm::vec3(0.2f, 0.5f, 0.9f)); // Синий цвет для отладки
    capsuleObject->collider = capsuleCollider;
    capsuleObject->hasCollider = true;

    // Добавляем коллайдер в систему коллизий с нужным слоем (например, "parkour_block")
    CollisionSystem::getInstance().addCollider(capsuleObject->collider, "capsule_obstacle");

    // Если нужна физика, создаём тело (статическое или динамическое)
    auto capsuleBody = std::make_shared<RigidBody>(&capsuleObject->transform, 3.0f); // масса 1 кг
    capsuleBody->collider = capsuleObject->collider;
    capsuleBody->useGravity = true;
    PhysicsSystem::getInstance().addRigidBody(capsuleBody, "capsule_body");

    sceneObjects.push_back(capsuleObject);

    // ==== Паркур-блоки ====
    int blockIndex = 0;
    for (const auto& block : blocks) {
        float w = block.size.x * 0.5f;
        float h = block.size.y * 0.5f;
        float d = block.size.z * 0.5f;

        std::vector<Vertex> cubeVerts = {
            // нижняя грань
            { {-w, -h, -d}, { 0, -1, 0 }, {0,0} },
            { { w, -h, -d}, {0, -1, 0}, {1,0} },
            { { w, -h,  d}, {0, -1, 0}, {1,1} },
            { {-w, -h,  d}, {0, -1, 0}, {0,1} },
            // верхняя грань
            { {-w,  h, -d}, {0, 1, 0}, {0,0} },
            { { w,  h, -d}, {0, 1, 0}, {1,0} },
            { { w,  h,  d}, {0, 1, 0}, {1,1} },
            { {-w,  h,  d}, {0, 1, 0}, {0,1} }
        };
        std::vector<unsigned int> cubeIndices = {
            0,1,2, 2,3,0,     // низ
            4,5,6, 6,7,4,     // верх
            3,2,6, 6,7,3,     // перед
            0,1,5, 5,4,0,     // зад
            0,3,7, 7,4,0,     // лево
            1,2,6, 6,5,1      // право
        };
        Mesh* blockMesh = new Mesh(cubeVerts, cubeIndices, std::vector<Texture>{});

        Object* blockObj = new Object(blockMesh);
        blockObj->transform.position = block.position;
        blockObj->castsShadows = true;

        // Создаём коллайдер и привязываем к объекту
        blockObj->collider = std::make_shared<BoxCollider>(block.size);
        blockObj->collider->setDebugColor(glm::vec3(0.8f, 0.2f, 0.2f));
        blockObj->hasCollider = true;
        CollisionSystem::getInstance().addCollider(blockObj->collider, "parkour_block_" + std::to_string(blockIndex));

        sceneObjects.push_back(blockObj);

        // Физическое тело (статическое)
        auto blockBody = std::make_shared<RigidBody>(&blockObj->transform, 0.0f);
        blockBody->collider = blockObj->collider;
        PhysicsSystem::getInstance().addRigidBody(blockBody, "parkour_block_body_" + std::to_string(blockIndex));

        blockIndex++;
    }

    // ==== Физические тела ====
    // Платформа (статическое)
    auto platformBody = std::make_shared<RigidBody>(&planeObject->transform, 0.0f);
    platformBody->collider = planeObject->collider;
    PhysicsSystem::getInstance().addRigidBody(platformBody, "platform");

    // Игрок (динамическое)
    player->rigidBody = std::make_shared<RigidBody>(&player->transform, 70.0f);
    player->rigidBody->collider = player->collider;
    player->rigidBody->useGravity = true;
    player->rigidBody->restitution = 0.0f;
    PhysicsSystem::getInstance().addRigidBody(player->rigidBody, "player");

    // Настройка слоёв коллизий
    setupCollisionLayers();

    /*std::vector<unsigned char> key(16);*/

    // Используем направление directionalLight
    //key[0] = static_cast<unsigned char>(std::abs(directionalLight->direction.x) * 10) ^ 0xA5;
    //key[1] = static_cast<unsigned char>(std::abs(directionalLight->direction.y) * 10) ^ 0x5A;

    //// Ambient составляющая directionalLight
    //key[2] = static_cast<unsigned char>(directionalLight->ambient.r * 255) ^ 0x3C;
    //key[3] = static_cast<unsigned char>(directionalLight->ambient.g * 255) ^ 0xC3;

    //// Позиция и цвет первого точечного источника
    //if (pointLights[0]) {
    //    key[4] = static_cast<unsigned char>(pointLights[0]->transform.position.x * 10) ^ 0x69;
    //    key[5] = static_cast<unsigned char>(pointLights[0]->transform.position.y * 10) ^ 0x96;
    //    key[6] = static_cast<unsigned char>(pointLights[0]->diffuse.r * 255) ^ 0x12;
    //    key[7] = static_cast<unsigned char>(pointLights[0]->diffuse.g * 255) ^ 0x21;
    //}

    //// Параметры первого паркур-блока
    //if (!blocks.empty()) {
    //    key[8] = static_cast<unsigned char>(blocks[0].position.x + 10) ^ 0x34;
    //    key[9] = static_cast<unsigned char>(blocks[0].position.y * 5) ^ 0x43;
    //    key[10] = static_cast<unsigned char>(blocks[0].position.z + 5) ^ 0x56;
    //    key[11] = static_cast<unsigned char>(blocks[0].size.x * 10) ^ 0x65;
    //}

    //// Коэффициенты затухания pointLights[0]
    //key[12] = static_cast<unsigned char>(pointLights[0]->linear * 100) ^ 0x78;
    //key[13] = static_cast<unsigned char>(pointLights[0]->quadratic * 100) ^ 0x87;

    //// Specular составляющая directionalLight
    //key[14] = static_cast<unsigned char>(directionalLight->specular.r * 255) ^ 0x9A;
    //key[15] = static_cast<unsigned char>(directionalLight->specular.g * 255) ^ 0xBC;

    //// Устанавливаем ключ в CryptoUtils
    //CryptoUtils::setKey(key);

    //if (useRawResources) {
    //    std::cout << "=== ENCRYPTION KEY FOR ASSET TOOL ===" << std::endl;
    //    std::cout << "const std::vector<unsigned char> KEY = {" << std::endl;
    //    std::cout << "    ";
    //    for (size_t i = 0; i < key.size(); ++i) {
    //        printf("0x%02X", key[i]);
    //        if (i < key.size() - 1) {
    //            std::cout << ", ";
    //        }
    //        if ((i + 1) % 8 == 0 && i < key.size() - 1) {
    //            std::cout << "\n    ";
    //        }
    //    }
    //    std::cout << "\n};" << std::endl;
    //    std::cout << "=====================================" << std::endl;
    //}

    std::cout << "Scene initialized. Objects count: " << sceneObjects.size() << std::endl;
}

// ===== РЕНДЕР КАРТЫ ГЛУБИНЫ ДЛЯ ТЕНЕЙ =====
void renderShadowDepth()
{
    //if (!shadowMap || !shadowDepthShader || !directionalLight || !directionalLight->enabled || !shadowsEnabled)
    //    return;

    // Вычисляем матрицы для источника света
    glm::mat4 lightProjection = glm::ortho(-10.0f, 10.0f, -10.0f, 10.0f, 1.0f, 20.0f);
    glm::mat4 lightView = glm::lookAt(
        directionalLight->direction * -10.0f,
        glm::vec3(0.0f, 0.0f, 0.0f),
        glm::vec3(0.0f, 1.0f, 0.0f)
    );

    shadowMap->SetLightMatrices(lightView, lightProjection);
    glm::mat4 lightSpaceMatrix = lightProjection * lightView;

    // Начинаем рендеринг в карту теней
    shadowMap->BeginRender();

    // Рендерим все объекты для карты глубины
    shadowDepthShader->use();
    shadowDepthShader->setMat4("lightSpaceMatrix", lightSpaceMatrix);

    for (auto* obj : sceneObjects) {
        if (!obj->castsShadows) continue;

        shadowDepthShader->setMat4("model", obj->transform.getMatrixQuat());

        if (obj->mesh) {
            obj->mesh->Draw(*shadowDepthShader);
        }
        else if (obj->model) {
            obj->model->Draw(*shadowDepthShader);
        }
    }

    shadowMap->EndRender(SCR_WIDTH, SCR_HEIGHT);
}

// ===== РЕНДЕР СЦЕНЫ =====
void renderScene(Shader& shader, float deltaTime, Shader* debugShader) {
    // Синхронизация коллайдеров с трансформацией объектов
    for (auto* obj : sceneObjects) {
        if (obj->hasCollider) {
            obj->updateCollider();
        }
    }

    // Обновление физики и игрока
    PhysicsSystem::getInstance().update(deltaTime);
    if (player) player->update(deltaTime);

    if (shadowsEnabled) renderShadowDepth();

    Camera& cam = player->getCamera();

    // Матрицы проекции и вида
    glm::mat4 projection = glm::perspective(
        glm::radians(cam.getZoom()),
        (float)SCR_WIDTH / SCR_HEIGHT,
        0.1f, 100.0f
    );
    glm::mat4 view = cam.getViewMatrix();

    shader.use();
    if (shadowsEnabled && shadowMap && directionalLight && directionalLight->enabled) {
        shader.setBool("shadowsEnabled", true);
        shader.setMat4("lightSpaceMatrix", shadowMap->GetLightSpaceMatrix());
        glActiveTexture(GL_TEXTURE2);
        glBindTexture(GL_TEXTURE_2D, shadowMap->GetTextureID());
        shader.setInt("shadowMap", 2);
    }
    else {
        shader.setBool("shadowsEnabled", false);
    }

    shader.setMat4("projection", projection);
    shader.setMat4("view", view);
    shader.setVec3("viewPos", cam.getPosition());

    shader.setInt("material.diffuse", 0);
    shader.setInt("material.specular", 1);
    shader.setFloat("material.shininess", 32.0f);


    if (directionalLight) directionalLight->ApplyToShader(shader, "dirLight");
    for (int i = 0; i < NR_POINT_LIGHTS; i++) {
        if (pointLights[i]) {
            pointLights[i]->ApplyToShader(shader, "pointLights[" + std::to_string(i) + "]");
        }
    }
    if (spotLight) {
        spotLight->transform.position = cam.getPosition();
        spotLight->direction = cam.getFront();
        spotLight->ApplyToShader(shader, "spotLight");
    }

    // Отладочная отрисовка источников света
    if (debugShader) {
        debugShader->use();
        for (Light* light : lights) {
            if (!light->enabled) continue;
            glm::mat4 model = glm::mat4(1.0f);
            if (light->type != LightType::Directional)
                model = glm::translate(model, light->transform.position);
            model = glm::scale(model, glm::vec3(0.2f));
            glm::mat4 mvp = projection * view * model;
            debugShader->setMat4("MVP", mvp);
            debugShader->setVec3("lightColor", light->GetFinalColor());
            glBindVertexArray(lightVAO);
            glDrawArrays(GL_TRIANGLES, 0, 36);
        }
        shader.use();
    }

    // Привязка текстур
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, diffuseMap);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, specularMap);

    // Отрисовка объектов сцены (кроме игрока, если он не в списке)
    for (auto* obj : sceneObjects) {
        if (obj == player) continue; // на случай, если player добавлен
        obj->Draw(shader);
    }
    // Отрисовка коллайдеров (отладка)
    static Shader* colliderShader = nullptr;
    if (!colliderShader) {
        colliderShader = new Shader(PathData + "shaders/debug_collider.vert", PathData + "shaders/debug_collider.frag");
    }

    colliderShader->use();
    colliderShader->setMat4("projection", projection);
    colliderShader->setMat4("view", view);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDisable(GL_CULL_FACE);

    if(DebugMode) drawColliders(*colliderShader);

    glEnable(GL_CULL_FACE);
    glDisable(GL_BLEND);
}

// ===== ОЧИСТКА =====
void cleanupScene() {
    std::cout << "Cleaning up scene objects..." << std::endl;
    for (auto* obj : sceneObjects) {
        delete obj->mesh;
        delete obj;
    }
    sceneObjects.clear();
    std::cout << "Scene objects cleared." << std::endl;

    std::cout << "Deleting lights..." << std::endl;
    for (auto* light : lights) {
        delete light;
    }
    lights.clear();
    std::cout << "Lights deleted." << std::endl;

    std::cout << "Clearing colliders..." << std::endl;
    CollisionSystem::getInstance().clearColliders();
    std::cout << "Colliders cleared." << std::endl;

    directionalLight = nullptr;
    spotLight = nullptr;
    for (int i = 0; i < NR_POINT_LIGHTS; i++) pointLights[i] = nullptr;

    if (shadowMap) {
        delete shadowMap;
        shadowMap = nullptr;
    }
    if (shadowDepthShader) {
        delete shadowDepthShader;
        shadowDepthShader = nullptr;
    }

    glDeleteVertexArrays(1, &lightVAO);
    glDeleteBuffers(1, &lightVBO);
    glDeleteTextures(1, &diffuseMap);
    glDeleteTextures(1, &specularMap);

    std::cout << "Cleanup finished." << std::endl;
}

// ===== ВСПОМОГАТЕЛЬНЫЕ ФУНКЦИИ ДЛЯ СВЕТА =====
void setDirectionalLight(const glm::vec3& direction, const glm::vec3& ambient,
    const glm::vec3& diffuse, const glm::vec3& specular) {
    if (directionalLight) {
        directionalLight->direction = glm::normalize(direction);
        directionalLight->ambient = ambient;
        directionalLight->diffuse = diffuse;
        directionalLight->specular = specular;
    }
}

void setPointLight(int index, const glm::vec3& position, const glm::vec3& ambient,
    const glm::vec3& diffuse, const glm::vec3& specular,
    float constant, float linear, float quadratic) {
    if (index >= 0 && index < NR_POINT_LIGHTS && pointLights[index]) {
        pointLights[index]->transform.position = position;
        pointLights[index]->ambient = ambient;
        pointLights[index]->diffuse = diffuse;
        pointLights[index]->specular = specular;
        pointLights[index]->constant = constant;
        pointLights[index]->linear = linear;
        pointLights[index]->quadratic = quadratic;
    }
}

void setSpotLight(const glm::vec3& position, const glm::vec3& direction,
    float cutOff, float outerCutOff, const glm::vec3& ambient,
    const glm::vec3& diffuse, const glm::vec3& specular) {
    if (spotLight) {
        spotLight->transform.position = position;
        spotLight->direction = direction;
        spotLight->cutOff = cutOff;
        spotLight->outerCutOff = outerCutOff;
        spotLight->ambient = ambient;
        spotLight->diffuse = diffuse;
        spotLight->specular = specular;
    }
}

glm::vec3 getPointLightPosition(int index) {
    if (index >= 0 && index < NR_POINT_LIGHTS && pointLights[index])
        return pointLights[index]->transform.position;
    return glm::vec3(0.0f);
}

void setPointLightPosition(int index, const glm::vec3& position) {
    if (index >= 0 && index < NR_POINT_LIGHTS && pointLights[index])
        pointLights[index]->transform.position = position;
}

void updateSpotLightWithCamera() {
    Camera& cam = player->getCamera();

    if (spotLight) {
        spotLight->transform.position = cam.getPosition();
        spotLight->direction = cam.getFront();
    }
}

std::vector<Light*>& getLights() { return lights; }

// ===== ФУНКЦИИ ДЛЯ УПРАВЛЕНИЯ ИГРОКОМ =====
void setCharacterPosition(const glm::vec3& position) {
    if (player) player->transform.position = position;
}

glm::vec3 getCharacterPosition() {
    return player ? player->transform.position : glm::vec3(0.0f);
}

Player* getPlayer() { return player; }