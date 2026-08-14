#include "Collider.h"
#include "BoxCollider.h"
#include "SphereCollider.h"
#include "CapsuleCollider.h"
#include <Physics/PhysicsSystem.h>
#include <Component/GameObject/GameObject.h>
#include <algorithm>

namespace Lindo {
    namespace Components {
        namespace Physics {

            glm::vec3 Collider::GetWorldPosition() const {
                if (owner) {
                    return owner->getWorldPosition() + offset;
                }
                return offset;
            }

            glm::vec3 Collider::GetWorldCenter() const {
                return GetWorldPosition();
            }

            glm::vec3 Collider::GetWorldScale() const {
                if (useOwnerScale && owner) {
                    glm::vec3 ownerScale = owner->transform.scale;
                    return ownerScale * colliderScale;
                }
                return colliderScale;
            }

            glm::vec3 Collider::GetColliderScale() const {
                return colliderScale;
            }

            Collider::~Collider() {
                PhysicsSystem::GetInstance().UnregisterCollider(this);
            }

            void Collider::OnStart() {
                PhysicsSystem::GetInstance().RegisterCollider(this);
            }

            void Collider::OnDestroy() {
                PhysicsSystem::GetInstance().UnregisterCollider(this);
            }

            void Collider::OnCollisionEnter(Collider* other, const CollisionInfo& info) {
                if (onCollisionEnterCallback) {
                    onCollisionEnterCallback(info);
                }
                AddCollision(other);
            }

            void Collider::OnCollisionStay(Collider* other, const CollisionInfo& info) {
                if (onCollisionStayCallback) {
                    onCollisionStayCallback(info);
                }
            }

            void Collider::OnCollisionExit(Collider* other) {
                if (onCollisionExitCallback) {
                    CollisionInfo dummy;
                    dummy.other = other;
                    onCollisionExitCallback(dummy);
                }
                RemoveCollision(other);
            }

            void Collider::OnTriggerEnter(Collider* other) {
                if (onTriggerEnterCallback) {
                    onTriggerEnterCallback(other);
                }
                AddTrigger(other);
            }

            void Collider::OnTriggerStay(Collider* other) {
                if (onTriggerStayCallback) {
                    onTriggerStayCallback(other);
                }
            }

            void Collider::OnTriggerExit(Collider* other) {
                if (onTriggerExitCallback) {
                    onTriggerExitCallback(other);
                }
                RemoveTrigger(other);
            }

            void Collider::SetCollisionCallback(CollisionEvent event, std::function<void(const CollisionInfo&)> callback) {
                switch (event) {
                case CollisionEvent::Enter:
                    onCollisionEnterCallback = callback;
                    break;
                case CollisionEvent::Stay:
                    onCollisionStayCallback = callback;
                    break;
                case CollisionEvent::Exit:
                    onCollisionExitCallback = callback;
                    break;
                }
            }

            void Collider::SetTriggerCallback(CollisionEvent event, std::function<void(Collider*)> callback) {
                switch (event) {
                case CollisionEvent::Enter:
                    onTriggerEnterCallback = callback;
                    break;
                case CollisionEvent::Stay:
                    onTriggerStayCallback = callback;
                    break;
                case CollisionEvent::Exit:
                    onTriggerExitCallback = callback;
                    break;
                }
            }

            void Collider::AddCollision(Collider* other) {
                if (std::find(currentCollisions.begin(), currentCollisions.end(), other) == currentCollisions.end()) {
                    currentCollisions.push_back(other);
                }
            }

            void Collider::RemoveCollision(Collider* other) {
                auto it = std::find(currentCollisions.begin(), currentCollisions.end(), other);
                if (it != currentCollisions.end()) {
                    currentCollisions.erase(it);
                }
            }

            void Collider::AddTrigger(Collider* other) {
                if (std::find(currentTriggers.begin(), currentTriggers.end(), other) == currentTriggers.end()) {
                    currentTriggers.push_back(other);
                }
            }

            void Collider::RemoveTrigger(Collider* other) {
                auto it = std::find(currentTriggers.begin(), currentTriggers.end(), other);
                if (it != currentTriggers.end()) {
                    currentTriggers.erase(it);
                }
            }

        }
    }
}
