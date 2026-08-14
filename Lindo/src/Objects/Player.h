#pragma once
#include <Component/Component.h>
#include <Physics/RigidBody.h>

// Предварительные объявления
class Model;
class Mesh;

class Player : public Component {
public:
    Player() = default;  // конструктор по умолчанию
    Player(Model* model);
    Player(Mesh* mesh);
    virtual ~Player();

    // Переопределяем методы Component
    void OnStart() override;
    void OnUpdate(float deltaTime) override;
    
    // Движение и физика
    void move(const glm::vec3& direction);
    void jump(float force = 9.0f);
    void setCrouching(bool crouch);
    bool isCrouching() const { return bCrouching; }

    // Скорости
    float normalSpeed = 5.0f;
    float movementSpeed = 5.0f;
    float crouchSpeed = 2.5f;
    // Геттеры состояния
    glm::vec3 getVelocity() const { return velocity; } 
    bool isGrounded() const { return grounded; }

private:
    void createCollider();
    glm::vec3 velocity = glm::vec3(0.0f);
    bool grounded = false; 
    float jumpForce = 5.0f; 
    float gravity = 9.81f;
    bool bCrouching = false;
    bool canJump = true; 
    bool blocked = false;
    float standHeight = 2.0f; // исходная высота капсулы 
    float crouchHeight = 1.5f; // высота при приседании
};