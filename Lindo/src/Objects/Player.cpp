#include "Player.h"
#include <iostream>
#include <Physics/Collider/CollisionSystem.h>
#include <Physics/Collider/CapsuleCollider.h>
#include <Physics/PhysicsSystem.h>

Player::Player(Model* model, const Transform& transform)
    : Object(model)
{
    this->transform = transform;
    createCollider(); // коллайдер создаётся и добавляется в систему
    // rigidBody НЕ создаём здесь
    camera.setPlayerTransform(this->transform);

}

Player::Player(Mesh* mesh, const Transform& transform)
    : Object(mesh)
{
    this->transform = transform;
    createCollider();
    camera.setPlayerTransform(this->transform);
}

Player::~Player() {
    if (collider) {
        auto& collisionSystem = CollisionSystem::getInstance();
        collisionSystem.removeCollider(collider);
    }
}

void Player::update(float deltaTime) {
    // Автовставание при приседе
    if (bCrouching && blocked) {
        float oldHeight = crouchHeight;
        float newHeight = standHeight;
        float heightDiff = newHeight - oldHeight;

        float safetyMargin = 0.1f;

        glm::vec3 rayOrigin = transform.position + glm::vec3(0.0f, oldHeight * 0.5f, 0.0f);
        glm::vec3 rayDir = glm::vec3(0.0f, 1.0f, 0.0f);

        RaycastHit hit;
        blocked = PhysicsSystem::getInstance().raycast(rayOrigin, rayDir, heightDiff + safetyMargin, hit, "", nullptr);

        if (!blocked) {
            setCrouching(false);
        }
    }

    // Обновляем камеру с учётом высоты капсулы
    auto capsuleCollider = std::dynamic_pointer_cast<CapsuleCollider>(collider);
    if (capsuleCollider) {
        float eyeHeightFromTop = 0.2f; // расстояние от макушки до уровня глаз
        float headHeight = capsuleCollider->getHeight() * 0.5f - eyeHeightFromTop;
        camera.setHeightOffset(headHeight);
    }

    camera.update(deltaTime);
}

void Player::handleCollision(const CollisionInfo& info) {
    // Здесь можно добавить звуки, эффекты и т.п.
    // Физическая реакция уже обработана в PhysicsSystem
}

void Player::move(const glm::vec3& direction) {
    if (rigidBody) {
        // Устанавливаем горизонтальную скорость
        rigidBody->velocity.x = direction.x * movementSpeed;
        rigidBody->velocity.z = direction.z * movementSpeed;
    }
}

void Player::jump(float force) {
    if (rigidBody && rigidBody->isGrounded && canJump) {
        rigidBody->velocity.y = force;
    }
}

void Player::createCollider() {
    Transform colTransform;
    colTransform.position = transform.position;
    colTransform.rotation = glm::vec3(0.0f);
    colTransform.scale = transform.scale;
    // Было: CapsuleCollider(0.4f, 2.0f, colTransform)
    Object::collider = std::make_shared<CapsuleCollider>(0.4f, standHeight, colTransform);
    Object::collider->setDebugColor(glm::vec3(1.0f, 0.0f, 0.0f));
    Object::collider->setVisible(true);
    Object::hasCollider = true;
    auto& collisionSystem = CollisionSystem::getInstance();
    collisionSystem.addCollider(Object::collider, "player");
}

void Player::setCrouching(bool crouch) {
    auto capsuleCollider = std::dynamic_pointer_cast<CapsuleCollider>(collider);
    if (!capsuleCollider) return;

    if (crouch == bCrouching) return;

    if (crouch) {
        // Приседаем
        float oldHeight = capsuleCollider->getHeight();
        float newHeight = crouchHeight;
        float heightDiff = oldHeight - newHeight;

        capsuleCollider->setHeight(newHeight);
        transform.position.y -= heightDiff * 0.5f; // опускаем центр

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

        glm::vec3 rayOrigin = transform.position + glm::vec3(0.0f, oldHeight * 0.5f, 0.0f);
        glm::vec3 rayDir = glm::vec3(0.0f, 1.0f, 0.0f);

        RaycastHit hit;
        blocked = PhysicsSystem::getInstance().raycast(rayOrigin, rayDir, heightDiff, hit, "", nullptr);

        if (blocked) {
            // Над головой что-то есть — нельзя вставать
            return;
        }

        // Свободно — можно вставать
        capsuleCollider->setHeight(newHeight);
        transform.position.y += heightDiff * 0.5f;

        movementSpeed = normalSpeed;
        bCrouching = false;

        // Включаем прыжки обратно
        canJump = true;
    }
}


void Player::updateCollider() {
    if (collider) {
        collider->getTransform().position = transform.position;
        collider->getTransform().scale = transform.scale;
        // Поворот не трогаем – остаётся нулевым
    }
}