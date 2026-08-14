#include "AudioListener.h"
#include "AudioSystem.h"
#include <AL/al.h>
#include <iostream>
#include <Component/GameObject/GameObject.h>

namespace Lindo {
    namespace Components {
        namespace Audio {
            AudioListener::~AudioListener() {
                if (AudioSystem::getInstance().getListener() == this) {
                    AudioSystem::getInstance().setListener(nullptr);
                }
            }

            void AudioListener::OnUpdate(float deltaTime) {
                if (!m_enabled || !owner) return;

                // Убеждаемся, что этот слушатель активен
                if (AudioSystem::getInstance().getListener() != this) {
                    AudioSystem::getInstance().setListener(this);
                }

                // Получаем позицию из Transform
                glm::vec3 pos = owner->transform.position;
                alListener3f(AL_POSITION, pos.x, pos.y, pos.z);

                // Скорость (пока 0)
                alListener3f(AL_VELOCITY, 0.0f, 0.0f, 0.0f);

                // Ориентация: forward и up
                glm::vec3 forward = owner->transform.getForward();
                glm::vec3 up = owner->transform.getUp();

                ALfloat orientation[] = {
                    forward.x, forward.y, forward.z,
                    up.x, up.y, up.z
                };
                alListenerfv(AL_ORIENTATION, orientation);

                // Проверка на ошибки
                ALenum error = alGetError();
                if (error != AL_NO_ERROR) {
                    std::cerr << "OpenAL Listener error: " << error << std::endl;
                }
            }

            void AudioListener::OnDestroy() {
                if (AudioSystem::getInstance().getListener() == this) {
                    AudioSystem::getInstance().setListener(nullptr);
                }
            }
        }
    }
}