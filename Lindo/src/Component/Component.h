#pragma once
#include <glm/glm.hpp>
#include <vector>

class GameObject;
class Collider;
class Shader;

class Component {
public:
    GameObject* owner = nullptr;

    Component() = default;
    virtual ~Component() = default;

    virtual void OnStart() {}
    virtual void OnUpdate(float deltaTime) {}
    virtual void OnDraw(Shader& shader) {}
    virtual void OnDestroy() {}

    virtual void OnCollisionEnter(Collider* other) {}
    virtual void OnCollisionStay(Collider* other) {}
    virtual void OnCollisionExit(Collider* other) {}

    virtual void OnTriggerEnter(Collider* other) {}
    virtual void OnTriggerStay(Collider* other) {}
    virtual void OnTriggerExit(Collider* other) {}

    virtual void OnDrawGizmos() {}

    glm::vec3 getPosition() const;
    void setPosition(const glm::vec3& pos);
};