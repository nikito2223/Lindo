#pragma once
#include <Component/Component.h>
#include "core/OGL.h"
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <Component/GameObject/GameObject.h>

namespace Lindo {
    struct Settings;

    namespace Components {
        namespace Rendering {
            /**
             * @brief Компонент камеры сцены. Поддерживает режимы FirstPerson и Free, а также покачивание головы (head bobbing).
             */
            class Camera : public Lindo::World::Component {
            public:
                Camera() = default;
                virtual ~Camera() = default;

                /**
                 * @brief Инициализация параметров камеры из настроек при старте.
                 */
                void OnStart() override;

                /**
                 * @brief Обновление позиционирования и векторов камеры каждый кадр.
                 */
                void OnUpdate() override;

                /**
                 * @brief Обработка клавиатурного ввода движения камеры.
                 * @param key Нажатая клавиша.
                 * @param action Состояние действия.
                 */
                void processKeyboardInput(int key, int action);

                /**
                 * @brief Обработка вращения камеры с помощью мыши.
                 * @param xOffset Дельта движения по оси X.
                 * @param yOffset Дельта движения по оси Y.
                 * @param constrainPitch Ограничивать ли угол тангажа (Pitch).
                 */
                void processMouseMovement(float xOffset, float yOffset, bool constrainPitch = true);

                /**
                 * @brief Обработка приближения/удаления (FOV) с помощью скролла.
                 * @param yOffset Дельта скролла.
                 */
                void processMouseScroll(float yOffset);

                /**
                 * @brief Вычисляет и возвращает матрицу вида (View Matrix).
                 * @return Матрица вида glm::mat4.
                 */
                glm::mat4 getViewMatrix() const;

                /**
                 * @brief Вычисляет и возвращает матрицу проекции (Projection Matrix).
                 * @return Перспективная матрица проекции glm::mat4.
                 */
                glm::mat4 getProjectionMatrix() const;

                void applySettings(const Lindo::Settings& settings);

                /**
                 * @brief Возвращает абсолютную позицию камеры с учетом смещения высоты.
                 * @return Вектор позиции glm::vec3.
                 */
                glm::vec3 getPosition() const {
                    return gameObject ? (gameObject->transform.position + glm::vec3(0.0f, heightOffset, 0.0f)) : glm::vec3(0.0f);
                }

                /**
                 * @brief Возвращает вектор направления "вперед".
                 * @return Вектор glm::vec3.
                 */
                glm::vec3 getFront() const { return front; }

                /**
                 * @brief Возвращает вектор направления "вверх".
                 * @return Вектор glm::vec3.
                 */
                glm::vec3 getUp() const { return up; }

                /**
                 * @brief Возвращает вектор направления "вправо".
                 * @return Вектор glm::vec3.
                 */
                glm::vec3 getRight() const { return right; }

                /**
                 * @brief Возвращает текущее значение зума (FOV).
                 * @return Угол обзора в градусах.
                 */
                float getZoom() const { return zoom; }

                /**
                 * @brief Возвращает угол рыскания (Yaw).
                 * @return Угол в градусах.
                 */
                float getYaw() const { return yaw; }

                /**
                 * @brief Возвращает угол тангажа (Pitch).
                 * @return Угол в градусах.
                 */
                float getPitch() const { return pitch; }

                /**
                 * @brief Возвращает дистанцию ближней плоскости отсечения.
                 * @return Дистанция near plane.
                 */
                float getNearPlane() { return m_near; }

                /**
                 * @brief Возвращает дистанцию дальней плоскости отсечения.
                 * @return Дистанция far plane.
                 */
                float getFarPlane() { return m_far; }

                /**
                 * @brief Задает вертикальное смещение точки обзора от центра объекта.
                 * @param offset Вышина смещения в метрах/юнитах.
                 */
                void setHeightOffset(float offset) { heightOffset = offset; }

                /**
                 * @brief Задает скорость свободного перемещения камеры.
                 * @param speed Новая скорость.
                 */
                void setMovementSpeed(float speed) { movementSpeed = speed; }

                /**
                 * @brief Задает чувствительность мыши.
                 * @param sensitivity Коэффициент чувствительности.
                 */
                void setMouseSensitivity(float sensitivity) { mouseSensitivity = sensitivity; }

                /**
                 * @brief Задает глобальный вектор "вверх" и пересчитывает ориентацию.
                 * @param worldUp Глобальный вектор вверх.
                 */
                void setWorldUp(const glm::vec3& worldUp) { this->worldUp = worldUp; updateCameraVectors(); }

                /**
                 * @brief Задает вектор направления передней оси и вычисляет углы Yaw/Pitch.
                 * @param newFront Новый вектор направления.
                 */
                void setFront(const glm::vec3& newFront);

                /**
                 * @brief Задает соотношение сторон экрана.
                 * @param aspect Coотношение сторон (Width / Height).
                 */
                void setAspectRatio(float aspect) { m_aspect = aspect; }

                /**
                 * @brief Устанавливает дистанции плоскостей отсечения.
                 * @param nearPlane Ближняя плоскость.
                 * @param farPlane Дальняя плоскость.
                 */
                void setNearFar(float nearPlane, float farPlane) { m_near = nearPlane; m_far = farPlane; }

                /**
                 * @brief Задает значение гамма-коррекции.
                 * @param g Значение гаммы.
                 */
                void setGamma(float g) { gamma = g; }

                float movementSpeed = 5.0f;    ///< Скорость передвижения камеры.
                float mouseSensitivity = 0.1f; ///< Чувствительность ввода мыши.
                float zoom = 90.0f;             ///< Поле зрения (FOV).
                float maxPitch = 89.0f;         ///< Максимальный угол наклона вверх.
                float minPitch = -89.0f;        ///< Максимальный угол наклона вниз.
                bool invertY = false;           ///< Инверсия оси Y для мыши.
                float heightOffset = 1.8f;      ///< Высота точки обзора игрока.
                bool clampToGround = false;     ///< Флаг привязки к поверхности земли.
                float groundHeight = 0.0f;      ///< Минимальная высота над уровнем земли.
                float bobAmount = 0.05f;        ///< Амплитуда покачивания головы.
                float bobSpeed = 10.0f;         ///< Скорость покачивания головы.
                float gamma = 2.2f;             ///< Коэффициент гаммы.

                /**
                 * @brief Режим работы камеры.
                 */
                enum class Mode {
                    FirstPerson, ///< Режим привязки к объекту (от первого лица).
                    Free         ///< Свободный режим с полным управлением (noclip).
                };

                /**
                 * @brief Переключает текущий режим работы камеры.
                 * @param newMode Новый режим работы.
                 */
                void setMode(Mode newMode) { mode = newMode; }

                /**
                 * @brief Возвращает текущий режим камеры.
                 * @return Экземпляр enum Mode.
                 */
                Mode getMode() const { return mode; }

            private:
                void updateCameraVectors();
                void updateBob();

                glm::vec3 front = glm::vec3(0.0f, 0.0f, -1.0f); ///< Вектор "вперед".
                glm::vec3 up = glm::vec3(0.0f, 1.0f, 0.0f);    ///< Вектор "вверх".
                glm::vec3 right = glm::vec3(1.0f, 0.0f, 0.0f); ///< Вектор "вправо".
                glm::vec3 worldUp = glm::vec3(0.0f, 1.0f, 0.0f);///< Глобальный вектор вверх.

                float yaw = -90.0f; ///< Угол поворота вокруг вертикальной оси Y.
                float pitch = 0.0f;  ///< Угол наклона вокруг горизонтальной оси X.

                /**
                 * @brief Внутреннее состояние удержания клавиш движения.
                 */
                struct {
                    bool forward = false;
                    bool backward = false;
                    bool left = false;
                    bool right = false;
                    bool up = false;
                    bool down = false;
                } movementState;

                float m_near = 0.1f;               ///< Ближняя плоскость отсечения.
                float m_far = 1000.0f;             ///< Дальняя плоскость отсечения.
                float m_aspect = 16.0f / 9.0f;     ///< Соотношение сторон экранной области.

                float bobTimer = 0.0f;             ///< Таймер анимации покачивания.
                bool isBobbing = false;            ///< Флаг выполнения покачивания в кадре.
                glm::vec3 bobOffset = glm::vec3(0.0f); ///< Рассчитанное смещение точки обзора от покачивания.

                Mode mode = Mode::FirstPerson;     ///< Активный режим работы камеры.
            };
        }
    }
}