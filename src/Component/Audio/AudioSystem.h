#pragma once
#include <AL/al.h>
#include <AL/alc.h>
#include <string>
#include <vector>
#include <memory>
#include <glm/glm.hpp>

namespace Lindo {
    namespace Components {
        namespace Audio {
            class AudioSource;
            class AudioListener;
        }
    }
}
namespace Lindo {
    namespace Components {
        namespace Audio {
            class AudioSystem {
            public:
                static AudioSystem& getInstance();

                bool init();
                void shutdown();
                void update();

                // Управление слушателем
                void setListener(AudioListener* listener);
                AudioListener* getListener() const { return m_currentListener; }

                // Глобальные настройки
                void setMasterVolume(float volume);
                float getMasterVolume() const { return m_masterVolume; }

                // Проверка состояния
                bool isInitialized() const { return m_initialized; }

                // В AudioSystem.h добавьте:
                ALCcontext* getContext() const { return m_context; }
                bool isContextCurrent() const { return alcGetCurrentContext() == m_context; }

            private:
                AudioSystem() = default;
                ~AudioSystem() = default;

                bool m_initialized = false;
                ALCdevice* m_device = nullptr;
                ALCcontext* m_context = nullptr;
                AudioListener* m_currentListener = nullptr;
                float m_masterVolume = 1.0f;
            };
        }
    }
}