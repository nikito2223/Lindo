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
#include <iostream>
#include <chrono>
#include <iomanip>
#include "Graphics/core/ShadowMap.h"
#include "Objects/transform.h"
#include "antires/CryptoUtils.h"
#include <Component/GameObject/GameObject.h>
#include <Component/Component.h>
#include <Component/Graphics/Light.h>
#include <Component/Physhcs/MeshRenderer.h>

// ===== ГЛОБАЛЬНЫЕ ПЕРЕМЕННЫЕ =====

static Player* player = nullptr;

static unsigned int diffuseMap = 0;
static unsigned int specularMap = 0;

static const int NR_POINT_LIGHTS = 1;

unsigned int lightVAO = 0;
unsigned int lightVBO = 0;

static std::vector<GameObject*> sceneObjects;

Light* getDirectionalLight() {
    for (auto* obj : sceneObjects) {
        if (!obj) continue;

        auto* light = obj->getComponent<Light>();
        if (light && light->type == LightType::Directional) {
            return light;
        }
    }
    return nullptr;
}

//Player* getPlayer() {
//    for (auto* obj : sceneObjects) {
//        if (!obj) continue;
//
//        auto* pl = obj->getComponent<Player>();
//        if (pl) {
//            return pl;
//        }
//    }
//    return nullptr;
//}

Player* getPlayer() { return player; }

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

    glBindVertexArray(0);
}

Mesh* createCubeMesh(const glm::vec3& size) {
    float w = size.x * 0.5f;
    float h = size.y * 0.5f;
    float d = size.z * 0.5f;

    std::vector<Vertex> vertices = {
        // передняя грань
        { {-w, -h,  d}, { 0, 0, 1 }, {0,0} },
        { { w, -h,  d}, { 0, 0, 1 }, {1,0} },
        { { w,  h,  d}, { 0, 0, 1 }, {1,1} },
        { {-w,  h,  d}, { 0, 0, 1 }, {0,1} },
        // задняя грань
        { {-w, -h, -d}, { 0, 0,-1 }, {0,0} },
        { { w, -h, -d}, { 0, 0,-1 }, {1,0} },
        { { w,  h, -d}, { 0, 0,-1 }, {1,1} },
        { {-w,  h, -d}, { 0, 0,-1 }, {0,1} },
        // левая грань
        { {-w, -h, -d}, {-1, 0, 0 }, {0,0} },
        { {-w, -h,  d}, {-1, 0, 0 }, {1,0} },
        { {-w,  h,  d}, {-1, 0, 0 }, {1,1} },
        { {-w,  h, -d}, {-1, 0, 0 }, {0,1} },
        // правая грань
        { { w, -h, -d}, { 1, 0, 0 }, {0,0} },
        { { w, -h,  d}, { 1, 0, 0 }, {1,0} },
        { { w,  h,  d}, { 1, 0, 0 }, {1,1} },
        { { w,  h, -d}, { 1, 0, 0 }, {0,1} },
        // нижняя грань
        { {-w, -h, -d}, { 0,-1, 0 }, {0,0} },
        { { w, -h, -d}, { 0,-1, 0 }, {1,0} },
        { { w, -h,  d}, { 0,-1, 0 }, {1,1} },
        { {-w, -h,  d}, { 0,-1, 0 }, {0,1} },
        // верхняя грань
        { {-w,  h, -d}, { 0, 1, 0 }, {0,0} },
        { { w,  h, -d}, { 0, 1, 0 }, {1,0} },
        { { w,  h,  d}, { 0, 1, 0 }, {1,1} },
        { {-w,  h,  d}, { 0, 1, 0 }, {0,1} }
    };

    std::vector<unsigned int> indices = {
        0,1,2, 2,3,0,       // перед
        4,5,6, 6,7,4,       // зад
        8,9,10, 10,11,8,    // лево
        12,13,14, 14,15,12, // право
        16,17,18, 18,19,16, // низ
        20,21,22, 22,23,20  // верх
    };

    return new Mesh(vertices, indices, std::vector<Texture>{});
}

Mesh* createCapsuleMesh(float radius = 0.4f, float height = 1.6f, int segments = 16) {
    std::vector<Vertex> vertices;
    std::vector<unsigned int> indices;

    float halfHeight = height * 0.5f;

    // Верхняя полусфера
    for (int i = 0; i <= segments; ++i) {
        float phi = glm::pi<float>() * 0.5f * i / segments;
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
        float phi = -glm::pi<float>() * 0.5f * i / segments;
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

    // Индексы
    int rows = 3 * segments;
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

// ===== НАСТРОЙКА КОЛЛИЗИЙ =====
void setupCollisionLayers() {
    auto& collisionSystem = CollisionSystem::getInstance();

    collisionSystem.setCollisionLayer("platform", 1);
    collisionSystem.setCollisionLayer("player", 2);
    collisionSystem.setCollisionLayer("parkour_block", 4);

    collisionSystem.setLayerCollision(1, 2, true);
    collisionSystem.setLayerCollision(2, 4, true);

    collisionSystem.registerCollisionCallback("player", "platform",
        [](const CollisionEvent& event) {});
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
    std::cout << "Initializing scene..." << std::endl;

    initLightCube();

    // Directional Light как компонент
    GameObject* directionalLightObj = new GameObject();
    directionalLightObj->name = "Directional Light";
    auto* dirLight = directionalLightObj->addComponent<Light>();

    dirLight->type = LightType::Directional;
    dirLight->direction = glm::vec3(-0.2f, -1.0f, -0.3f);
    dirLight->enabled = true;
    dirLight->ambient = glm::vec3(0.2f);
    dirLight->diffuse = glm::vec3(0.5f);
    dirLight->specular = glm::vec3(1.0f);

    dirLight->InitShadowResources(2048, 2048);
    sceneObjects.push_back(directionalLightObj);

    diffuseMap = loadTexture(PathData + "textures/textures.png");
    if (diffuseMap == 0) {
        std::cerr << "Failed to load diffuse texture" << std::endl;
    }

    specularMap = loadTexture(PathData + "textures/textures.png");
    if (specularMap == 0) {
        std::cerr << "Failed to load specular texture" << std::endl;
    }

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

    GameObject* planeObject = new GameObject();
    planeObject->name = "Plane";
    planeObject->tag = "platform";
    planeObject->transform.position = glm::vec3(0.0f, 0.0f, 0.0f);

    auto* planeRenderer = planeObject->addComponent<MeshRenderer>();
    if (planeRenderer) {
        planeRenderer->mesh = planeMesh;
    }

    auto* planeCollider = planeObject->addComponent<BoxCollider>();
    if (planeCollider) {
        planeCollider->setSize(glm::vec3(30.0f, 0.1f, 30.0f));
        planeCollider->setDebugColor(glm::vec3(0.2f, 0.8f, 0.2f));
    }
    CollisionSystem::getInstance().addCollider(planeCollider, "Default");

    auto* planeBody = planeObject->addComponent<RigidBody>(0.0f);
    if (planeBody) {
        planeBody->collider = planeCollider;
    }

    PhysicsSystem::getInstance().addRigidBody(planeBody);
    sceneObjects.push_back(planeObject);

    Mesh* capsuleMesh = createCapsuleMesh(0.4f, 2.0f);

    GameObject* playerObj = new GameObject("Player", "player");
    playerObj->castsShadows = true;
    playerObj->transform.position = glm::vec3(0.0f, 1.2f, 0.0f);
    auto* playerRenderer = playerObj->addComponent<MeshRenderer>();
    if (playerRenderer) {
        playerRenderer->mesh = capsuleMesh;
    }

    auto* playerCollider = playerObj->addComponent<CapsuleCollider>();
    auto* playerComponent = playerObj->addComponent<Player>();
    if (playerCollider) {
        playerCollider->setDebugColor(glm::vec3(0.2f, 0.2f, 0.8f));
    }
    CollisionSystem::getInstance().addCollider(playerCollider, "Default");
    auto* playerCamera = playerObj->addComponent<Camera>();
    playerCamera->heightOffset = 0.8f; // Высота глаз
    playerCamera->setMode(Camera::Mode::FirstPerson);

    player = playerObj->getComponent<Player>();

    auto* playerBody = playerObj->addComponent<RigidBody>(70.0f);
    if (playerBody) {
        playerBody->useGravity = true;
        playerBody->restitution = 0.0f;
        playerBody->collider = playerCollider;
    }
    PhysicsSystem::getInstance().addRigidBody(playerBody);
    sceneObjects.push_back(playerObj);

    GameObject* zoneObject = new GameObject();
    zoneObject->name = "Trigger Zone";
    zoneObject->tag = "trigger";
    zoneObject->transform.position = glm::vec3(5.0f, 1.0f, 0.0f);

    auto* zoneCollider = zoneObject->addComponent<BoxCollider>();
    if (zoneCollider) {
        zoneCollider->setSize(glm::vec3(2.0f, 2.0f, 2.0f));
        zoneCollider->setTrigger(true);
        zoneCollider->setDebugColor(glm::vec3(1.0f, 1.0f, 0.0f));
    }
    CollisionSystem::getInstance().addCollider(zoneCollider, "Default");

    sceneObjects.push_back(zoneObject);

    int blockIndex = 0;
    for (const auto& block : blocks) {
        Mesh* blockMesh = createCubeMesh(block.size);

        GameObject* blockObject = new GameObject();
        blockObject->name = "Parkour Block " + std::to_string(blockIndex);
        blockObject->tag = "parkour_block";
        blockObject->castsShadows = true;
        blockObject->transform.position = block.position;

        auto* blockRenderer = blockObject->addComponent<MeshRenderer>();
        if (blockRenderer) {
            blockRenderer->mesh = blockMesh;
        }

        auto* blockCollider = blockObject->addComponent<BoxCollider>();
        if (blockCollider) {
            blockCollider->setSize(block.size);
            blockCollider->setDebugColor(glm::vec3(0.8f, 0.2f, 0.2f));
        }
        CollisionSystem::getInstance().addCollider(blockCollider, "Default");

        auto* blockBody = blockObject->addComponent<RigidBody>(0.0f);
        PhysicsSystem::getInstance().addRigidBody(blockBody);
        if (blockBody) {
            blockBody->collider = blockCollider;
        }

        sceneObjects.push_back(blockObject);
        blockIndex++;
    }

    setupCollisionLayers();

    std::cout << "Scene initialized with " << sceneObjects.size() << " objects" << std::endl;
}

// ===== РЕНДЕР СЦЕНЫ =====
void renderScene(Shader& shader, float deltaTime, Shader* debugShader) {
    PhysicsSystem::getInstance().update(deltaTime);

    // Рендер теней от источников света
    for (auto* obj : sceneObjects) {
        if (!obj) continue;

        auto* light = obj->getComponent<Light>();
        if (light && light->enabled && light->castShadows) {
            light->RenderShadows(sceneObjects, SCR_WIDTH, SCR_HEIGHT);
        }
    }

    auto* player = getPlayer();
    if (!player) return;
    auto* cam = player->owner->getComponent<Camera>();
    if (!cam) return;

    glm::mat4 projection = cam->getProjectionMatrix();
    glm::mat4 view = cam->getViewMatrix();

    shader.use();

    Light* mainShadowLight = nullptr;
    for (auto* obj : sceneObjects) {
        if (!obj) continue;

        auto* light = obj->getComponent<Light>();
        if (light && light->type == LightType::Directional && light->enabled && light->castShadows) {
            mainShadowLight = light;
            break;
        }
    }

    if (mainShadowLight) {
        shader.setBool("shadowsEnabled", true);
        shader.setMat4("lightSpaceMatrix", mainShadowLight->GetLightSpaceMatrix());

        glActiveTexture(GL_TEXTURE2);
        unsigned int shadowTexID = mainShadowLight->shadowMap->GetTextureID();
        glBindTexture(GL_TEXTURE_2D, shadowTexID);
        shader.setInt("shadowMap", 2);
    }
    else {
        shader.setBool("shadowsEnabled", false);
    }

    shader.setMat4("projection", projection);
    shader.setMat4("view", view);
    shader.setVec3("viewPos", cam->getPosition());

    shader.setInt("material.diffuse", 0);
    shader.setInt("material.specular", 1);
    shader.setFloat("material.shininess", 32.0f);

    int pointLightIndex = 0;
    for (auto* obj : sceneObjects) {
        if (!obj) continue;

        auto* light = obj->getComponent<Light>();
        if (!light || !light->enabled) continue;

        if (light->type == LightType::Directional) {
            light->ApplyToShader(shader, "dirLight");
        }
        else if (light->type == LightType::Point) {
            std::string uniformName = "pointLights[" + std::to_string(pointLightIndex++) + "]";
            light->ApplyToShader(shader, uniformName);
        }
        else if (light->type == LightType::Spot) {
            light->ApplyToShader(shader, "spotLight");
        }
    }

    // Отладочная отрисовка источников света
    if (debugShader) {
        debugShader->use();
        for (auto* obj : sceneObjects) {
            auto* light = obj->getComponent<Light>();
            if (!light || !light->enabled) continue;

            glm::mat4 model = glm::mat4(1.0f);
            if (light->type != LightType::Directional)
                model = glm::translate(model, light->owner->transform.position);
            model = glm::scale(model, glm::vec3(0.2f));

            glm::mat4 mvp = projection * view * model;
            debugShader->setMat4("MVP", mvp);
            debugShader->setVec3("lightColor", light->GetFinalColor());

            glBindVertexArray(lightVAO);
            glDrawArrays(GL_TRIANGLES, 0, 36);
        }
        shader.use();
    }

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, diffuseMap);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, specularMap);

    for (auto* obj : sceneObjects) {
        if (!obj) continue;
        if (obj->tag == "player") continue; // пропускаем игрока
        obj->Draw(shader);
    }

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

    if (DebugMode) {
        drawColliders(*colliderShader);
    }

    glEnable(GL_CULL_FACE);
    glDisable(GL_BLEND);
}

// ===== ОЧИСТКА =====
void cleanupScene() {
    std::cout << "Cleaning up scene..." << std::endl;

    for (auto* obj : sceneObjects) {
        if (obj) {
            delete obj;
        }
    }
    sceneObjects.clear();

    CollisionSystem::getInstance().clearColliders();

    glDeleteVertexArrays(1, &lightVAO);
    glDeleteBuffers(1, &lightVBO);
    glDeleteTextures(1, &diffuseMap);
    glDeleteTextures(1, &specularMap);

    std::cout << "Scene cleanup completed" << std::endl;
}

glm::vec3 getCharacterPosition() {
    for (auto* obj : sceneObjects) {
        if (!obj) continue;

        auto* pl = obj->getComponent<Player>();
        if (pl && pl->owner) {
            return pl->owner->transform.position;
        }
    }
    return glm::vec3(0.0f);
}