// Player.h
#pragma once

#include "render/objects/Object.h"
#include "FirstPersonCamera.h"
#include "render/Collision/Collider.h"
#include "camera/FirstPersonCamera.h"
#include <render/Collision/RigidBody.h>

class Player : public Object {
public:
    Player(Model* model, const Transform& transform = Transform());
    Player(Mesh* mesh, const Transform& transform = Transform());
    ~Player();

    virtual void updateCollider() override;

    void update(float deltaTime);
    void handleCollision(const CollisionInfo& info);

    // Геттеры для камеры
    FirstPersonCamera& getCamera() { return camera; }
    const FirstPersonCamera& getCamera() const { return camera; }

    // Геттер для коллайдера
    std::shared_ptr<Collider> getCollider() const { return collider; }

    // Движение и физика
    void move(const glm::vec3& movement);
    void jump(float force = 9.0f);

    void setCrouching(bool crouch);
    bool isCrouching() const { return bCrouching; }

    // Скорости
    float normalSpeed = 5.0f;        // скорость при ходьбе
    float movementSpeed = 5.0f;      // обычная скорость
    float crouchSpeed = 2.5f;        // скорость при приседании

    std::shared_ptr<RigidBody> getRigidBody() { return rigidBody; }

    std::shared_ptr<RigidBody> rigidBody;

    // Геттеры состояния
    glm::vec3 getVelocity() const { return velocity; }
    bool isGrounded() const { return grounded; }

private:
    void createCollider();

    FirstPersonCamera camera;
    glm::vec3 velocity = glm::vec3(0.0f);
    bool grounded = false;
    float jumpForce = 5.0f;
    float gravity = 9.81f;

    bool bCrouching = false;
    float standHeight = 2.0f;        // исходная высота капсулы
    float crouchHeight = 1.2f;        // высота при приседании
};