#include "AudioSystem.h"
#include <iostream>
#include <AL/al.h>
#include <AL/alc.h>

namespace Lindo {
    namespace Components {
        namespace Audio {

            AudioSystem& AudioSystem::getInstance() {
                static AudioSystem instance;
                return instance;
            }

            bool AudioSystem::init() {
                if (m_initialized) return true;

                // Открываем устройство по умолчанию
                m_device = alcOpenDevice(nullptr);
                if (!m_device) {
                    std::cerr << "Failed to open OpenAL device" << std::endl;
                    return false;
                }

                // Создаем контекст
                m_context = alcCreateContext(m_device, nullptr);
                if (!m_context) {
                    std::cerr << "Failed to create OpenAL context" << std::endl;
                    alcCloseDevice(m_device);
                    m_device = nullptr;
                    return false;
                }

                // Делаем контекст текущим
                if (!alcMakeContextCurrent(m_context)) {
                    std::cerr << "Failed to make OpenAL context current" << std::endl;
                    alcDestroyContext(m_context);
                    alcCloseDevice(m_device);
                    m_context = nullptr;
                    m_device = nullptr;
                    return false;
                }

                // Проверяем версию OpenAL
                std::cout << "OpenAL initialized. Vendor: " << alGetString(AL_VENDOR)
                    << ", Version: " << alGetString(AL_VERSION) << std::endl;

                // Устанавливаем расстояние по умолчанию (метры)
                alDistanceModel(AL_INVERSE_DISTANCE_CLAMPED);

                m_initialized = true;
                return true;
            }

            void AudioSystem::shutdown() {
                if (!m_initialized) return;

                alcMakeContextCurrent(nullptr);

                if (m_context) {
                    alcDestroyContext(m_context);
                    m_context = nullptr;
                }

                if (m_device) {
                    alcCloseDevice(m_device);
                    m_device = nullptr;
                }

                m_initialized = false;
                m_currentListener = nullptr;
            }

            void AudioSystem::update(float deltaTime) {
                if (!m_initialized) return;

                if (alcGetCurrentContext() != m_context) {
                    std::cerr << "AudioSystem: Context lost, reactivating..." << std::endl;
                    alcMakeContextCurrent(m_context);
                }

                // Обновляем позицию слушателя, если он есть
                if (m_currentListener) {
                    // Слушатель обновляет себя сам в своем OnUpdate
                }

                // Здесь можно добавить глобальную обработку звуков
            }

            void AudioSystem::setListener(AudioListener* listener) {
                m_currentListener = listener;
            }

            void AudioSystem::setMasterVolume(float volume) {
                m_masterVolume = glm::clamp(volume, 0.0f, 1.0f);
                alListenerf(AL_GAIN, m_masterVolume);
            }
        }
    }
}