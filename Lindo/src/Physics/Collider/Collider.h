#pragma once

#include <glm/glm.hpp>
#include <memory>
#include "Objects/transform.h"
#include "ColliderType.h"
#include <Component/Component.h>
#include <Physics/RaycastHit.h>
#include <Component/GameObject/GameObject.h>
#include <functional>

class Shader;
class Component;

class Collider : public Component {
public:
    Collider(ColliderType type)
        : type(type),
        isTrigger(false),
        isVisible(true),
        debugColor(glm::vec3(0, 1, 0)),
        userData(nullptr) {}

    virtual ~Collider() = default;

    virtual bool checkCollision(const Collider* other,
        CollisionInfo* info = nullptr) const = 0;

    virtual bool checkRayCollision(const glm::vec3& origin,
        const glm::vec3& direction,
        float* distance = nullptr,
        glm::vec3* normal = nullptr) const = 0;

    virtual bool intersectRay(const glm::vec3& origin,
        const glm::vec3& dir,
        float maxDist,
        RaycastHit& hit) const = 0;

    virtual void drawDebug(Shader& shader) const = 0;

    virtual glm::vec3 getCenter() const = 0;
    virtual glm::vec3 getExtents() const = 0;

    ColliderType getType() const { return type; }

    glm::vec3 getPosition() const {
        return owner ? owner->transform.position : glm::vec3(0.0f);
    }

    void setTrigger(bool trigger) { isTrigger = trigger; }
    bool getTrigger() const { return isTrigger; }

    void setVisible(bool visible) { isVisible = visible; }
    bool getVisible() const { return isVisible; }

    void setDebugColor(const glm::vec3& color) { debugColor = color; }
    glm::vec3 getDebugColor() const { return debugColor; }

    void setUserData(void* data) { userData = data; }
    void* getUserData() const { return userData; }

    using TriggerCallback = std::function<void(Collider* other)>;

    void onTriggerEnter(TriggerCallback callback) {
        onEnter = callback;
    }

    void fireTriggerEvent(Collider* other) {
        if (onEnter) onEnter(other);
    }

protected:
    TriggerCallback onEnter = nullptr;

    ColliderType type;

    bool isTrigger;
    bool isVisible;
    glm::vec3 debugColor;
    void* userData;
};