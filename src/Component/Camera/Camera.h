#pragma once
#include <Component/Component.h>
#include "core/OGL.h"
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <Component/GameObject/GameObject.h>
namespace Lindo {
    namespace Components {
        namespace Rendering {
            class Camera : public Lindo::World::Component {
            public:
                Camera() = default;
                virtual ~Camera() = default;

                // �������������� ������ Component
                void OnStart() override;
                void OnUpdate() override;

                // ��������� �����
                void processKeyboardInput(int key, int action);
                void processMouseMovement(float xOffset, float yOffset, bool constrainPitch = true);
                void processMouseScroll(float yOffset);

                // �������
                glm::mat4 getViewMatrix() const;
                glm::mat4 getProjectionMatrix() const;

                // ��������: getPosition() ������ ���������� gameObject �� Component
                // gameObject - ��� ��������� �� GameObject, ������� ���� � ���� �����������
                glm::vec3 getPosition() const {
                    return gameObject ? (gameObject->transform.position + glm::vec3(0.0f, heightOffset, 0.0f)) : glm::vec3(0.0f);
                }

                glm::vec3 getFront() const { return front; }
                glm::vec3 getUp() const { return up; }
                glm::vec3 getRight() const { return right; }
                float getZoom() const { return zoom; }
                float getYaw() const { return yaw; }
                float getPitch() const { return pitch; }
                float getNearPlane() { return m_near; }
                float getFarPlane() { return m_far; }

                // �������
                void setHeightOffset(float offset) { heightOffset = offset; }
                void setMovementSpeed(float speed) { movementSpeed = speed; }
                void setMouseSensitivity(float sensitivity) { mouseSensitivity = sensitivity; }
                void setWorldUp(const glm::vec3& worldUp) { this->worldUp = worldUp; updateCameraVectors(); }
                void setFront(const glm::vec3& newFront);
                void setAspectRatio(float aspect) { m_aspect = aspect; }
                void setNearFar(float nearPlane, float farPlane) { m_near = nearPlane; m_far = farPlane; }
                void setGamma(float g) { gamma = g; }


                float movementSpeed = 5.0f;
                float mouseSensitivity = 0.1f;
                float zoom = 90.0f;
                float maxPitch = 89.0f;
                float minPitch = -89.0f;
                bool invertY = false;
                float heightOffset = 1.8f; 
                bool clampToGround = false;
                float groundHeight = 0.0f;
                float bobAmount = 0.05f;   
                float bobSpeed = 10.0f;    
                float gamma = 2.2f;



                enum class Mode {
                    FirstPerson,
                    Free        
                };

                void setMode(Mode newMode) { mode = newMode; }
                Mode getMode() const { return mode; }

            private:
                void updateCameraVectors();
                void updateBob();

                // ��������� ������
                glm::vec3 front = glm::vec3(0.0f, 0.0f, -1.0f);
                glm::vec3 up = glm::vec3(0.0f, 1.0f, 0.0f);
                glm::vec3 right = glm::vec3(1.0f, 0.0f, 0.0f);
                glm::vec3 worldUp = glm::vec3(0.0f, 1.0f, 0.0f);

                // ���� ������
                float yaw = -90.0f;
                float pitch = 0.0f;

                // ��������� �����
                struct {
                    bool forward = false;
                    bool backward = false;
                    bool left = false;
                    bool right = false;
                    bool up = false;
                    bool down = false;
                } movementState;

                // ��������� ��������
                float m_near = 0.1f;
                float m_far = 1000.0f;
                float m_aspect = 16.0f / 9.0f;

                // �������
                float bobTimer = 0.0f;
                bool isBobbing = false;
                glm::vec3 bobOffset = glm::vec3(0.0f);

                // ����� ������
                Mode mode = Mode::FirstPerson;
            };
        }
    }
}