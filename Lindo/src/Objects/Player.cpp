#include "Player.h"
#include <iostream>
#include <Physics/Collider/CollisionSystem.h>
#include <Physics/Collider/CapsuleCollider.h>
#include <Physics/PhysicsSystem.h>

Player::Player(Model* model) {
    createCollider();
}

Player::Player(Mesh* mesh) {
    createCollider();
}

Player::~Player() {
    if (auto* col = owner->getComponent<CapsuleCollider>()) { 
        auto& collisionSystem = CollisionSystem::getInstance(); 
        collisionSystem.removeCollider(col);
    }
}

void Player::OnStart() {
    std::cout << "Player::OnStart() called, owner: " << owner << std::endl;
    if (!owner) {
        std::cerr << "ERROR: Player owner is nullptr in OnStart!" << std::endl;
        return;
    }
}

void Player::OnUpdate(float deltaTime) {
    if (!owner) {
        std::cerr << "ERROR: Player owner is nullptr in OnUpdate!" << std::endl;
        return;
    }

    auto* cam = owner->getComponent<Camera>();
    if (!cam) return;

    // Автовставание при приседе
    if (bCrouching && blocked) {
        float oldHeight = crouchHeight;
        float newHeight = standHeight;
        float heightDiff = newHeight - oldHeight;

        float safetyMargin = 0.1f;

        glm::vec3 rayOrigin = owner->transform.position + glm::vec3(0.0f, oldHeight * 0.5f, 0.0f);
        glm::vec3 rayDir = glm::vec3(0.0f, 1.0f, 0.0f);

        RaycastHit hit;
        blocked = PhysicsSystem::getInstance().raycast(rayOrigin, rayDir, heightDiff + safetyMargin, hit, "", nullptr);

        if (!blocked) {
            setCrouching(false);
        }
    }

    // Обновляем камеру с учётом высоты капсулы
    auto capsuleCollider = owner->getComponent<CapsuleCollider>();
    if (capsuleCollider && cam) {
        float eyeHeightFromTop = 0.2f;
        float headHeight = capsuleCollider->getHeight() * 0.5f - eyeHeightFromTop;
        cam->setHeightOffset(headHeight);
    }
}

void Player::move(const glm::vec3& direction) {
    auto rb = owner->getComponent<RigidBody>();
    if (rb) {
        // Устанавливаем горизонтальную скорость
        rb->velocity.x = direction.x * movementSpeed;
        rb->velocity.z = direction.z * movementSpeed;
    }
}

void Player::jump(float baseForce) {
    auto rb = owner->getComponent<RigidBody>();
    if (!rb || !rb->isGrounded || !canJump) return;

    auto* cam = owner->getComponent<Camera>();
    if (!cam) return;

    glm::vec3 jumpVelocity = glm::vec3(0.0f, baseForce, 0.0f);

    // Направление взгляда камеры
    glm::vec3 forward = cam->getFront();
    forward.y = 0.0f;
    if (glm::length(forward) > 0.001f) {
        forward = glm::normalize(forward);
        float forwardFactor = 0.5f;
        jumpVelocity += forward * baseForce * forwardFactor;
    }

    float lookUpFactor = glm::clamp(cam->getPitch() / 90.0f, 0.0f, 1.0f);
    jumpVelocity += forward * baseForce * lookUpFactor;

    rb->velocity += jumpVelocity;
    rb->isGrounded = false;
}

void Player::createCollider() {
    Transform colTransform;
    colTransform.position = owner->transform.position;
    colTransform.rotation = glm::vec3(0.0f);
    colTransform.scale = owner->transform.scale;
    // Было: CapsuleCollider(0.4f, 2.0f, colTransform) 
    auto* col = owner->addComponent<CapsuleCollider>(0.4f, standHeight);
    col->setDebugColor(glm::vec3(1.0f, 0.0f, 0.0f));
    col->setVisible(true);
    auto& collisionSystem = CollisionSystem::getInstance(); 
    collisionSystem.addCollider(col, "player");
}

void Player::setCrouching(bool crouch) {
    auto capsuleCollider = owner->getComponent<CapsuleCollider>();
    if (crouch == bCrouching) return;

    if (crouch) {
        // Приседаем
        float oldHeight = capsuleCollider->getHeight();
        float newHeight = crouchHeight;
        float heightDiff = oldHeight - newHeight;

        capsuleCollider->setHeight(newHeight);
        owner->transform.position.y -= heightDiff * 0.5f; // опускаем центр

        movementSpeed = crouchSpeed;
        bCrouching = true;

        // Запрещаем прыжки
        canJump = false;
    }
    else {
        // Встаём — проверяем место сверху
        float oldHeight = capsuleCollider->getHeight();
        float newHeight = standHeight;
        float heightDiff = newHeight - oldHeight;

        glm::vec3 rayOrigin = owner->transform.position + glm::vec3(0.0f, oldHeight * 0.5f, 0.0f);
        glm::vec3 rayDir = glm::vec3(0.0f, 1.0f, 0.0f);

        RaycastHit hit;
        blocked = PhysicsSystem::getInstance().raycast(rayOrigin, rayDir, heightDiff, hit, "", nullptr);

        if (blocked) {
            // Над головой что-то есть — нельзя вставать
            return;
        }

        // Свободно — можно вставать
        capsuleCollider->setHeight(newHeight);
        owner->transform.position.y += heightDiff * 0.5f;

        movementSpeed = normalSpeed;
        bCrouching = false;

        // Включаем прыжки обратно
        canJump = true;
    }
}