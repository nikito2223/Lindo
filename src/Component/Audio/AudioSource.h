#pragma once
#include <Component/Component.h>
#include <string>
#include <AL/al.h>
namespace Lindo {
    namespace Components {
        namespace Audio {
            class AudioSource : public Lindo::World::Component{
            public:
                AudioSource();
                void printDebugInfo() const;
                virtual ~AudioSource();

                void OnUpdate() override;
                void OnDestroy() override;

                // Загрузка звука
                void setSound(const std::string& path);
                void setSound(ALuint buffer);

                // Управление воспроизведением
                void play();
                void stop();
                void pause();
                bool isPlaying() const;

                // Настройки
                void setLooping(bool loop);
                void setVolume(float volume);
                void setPitch(float pitch);
                void setSpatial(bool spatial);  // 3D звук или нет
                void setMinDistance(float dist);
                void setMaxDistance(float dist);
                void setRolloffFactor(float factor);
                void setPosition(const glm::vec3& pos);

                // Геттеры
                float getVolume() const { return m_volume; }
                float getPitch() const { return m_pitch; }
                bool isLooping() const { return m_loop; }

            private:
                ALuint m_sourceId = 0;
                ALuint m_bufferId = 0;

                std::string m_soundPath;

                bool m_loop = false;
                float m_volume = 1.0f;
                float m_pitch = 1.0f;
                bool m_spatial = true;
                float m_minDistance = 1.0f;
                float m_maxDistance = 50.0f;
                float m_rolloffFactor = 1.0f;

                glm::vec3 m_lastPosition{ 0.0f };

                void updatePosition();
                void applyParameters();
                void clearSource();
                void clear();
            };
        }
    }
}