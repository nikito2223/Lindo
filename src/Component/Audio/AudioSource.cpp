#include "AudioSource.h"
#include "core/AssetManager.h"
#include <AL/al.h>
#include <iostream>
#include <Component/Audio/AudioSystem.h>
#include <Component/GameObject/GameObject.h>

namespace Lindo {
    namespace Components {
        namespace Audio {

            AudioSource::AudioSource() {
                // Активируем контекст OpenAL
                if (AudioSystem::getInstance().isInitialized()) {
                    alcMakeContextCurrent(AudioSystem::getInstance().getContext());
                }

                alGenSources(1, &m_sourceId);
                if (alGetError() != AL_NO_ERROR || m_sourceId == 0) {
                    std::cerr << "Failed to generate audio source" << std::endl;
                    m_sourceId = 0;
                    return;
                }

                std::cout << "AudioSource created with ID: " << m_sourceId << std::endl;

                // Убеждаемся, что источник не имеет привязанного буфера
                alSourcei(m_sourceId, AL_BUFFER, 0);

                // Устанавливаем параметры по умолчанию
                alSourcef(m_sourceId, AL_GAIN, m_volume);
                alSourcef(m_sourceId, AL_PITCH, m_pitch);
                alSourcei(m_sourceId, AL_LOOPING, AL_FALSE);
                alSourcei(m_sourceId, AL_SOURCE_RELATIVE, AL_FALSE);
                alSourcef(m_sourceId, AL_REFERENCE_DISTANCE, m_minDistance);
                alSourcef(m_sourceId, AL_MAX_DISTANCE, m_maxDistance);
                alSourcef(m_sourceId, AL_ROLLOFF_FACTOR, m_rolloffFactor);
            }

            void AudioSource::printDebugInfo() const {
                std::cout << "=== AudioSource Debug ===" << std::endl;
                std::cout << "Source ID: " << m_sourceId << std::endl;
                std::cout << "Buffer ID: " << m_bufferId << std::endl;
                std::cout << "Sound path: " << m_soundPath << std::endl;
                std::cout << "Looping: " << (m_loop ? "Yes" : "No") << std::endl;
                std::cout << "Volume: " << m_volume << std::endl;
                std::cout << "Pitch: " << m_pitch << std::endl;

                if (m_sourceId) {
                    ALint state;
                    alGetSourcei(m_sourceId, AL_SOURCE_STATE, &state);
                    std::cout << "State: ";
                    switch (state) {
                    case AL_INITIAL: std::cout << "INITIAL"; break;
                    case AL_PLAYING: std::cout << "PLAYING"; break;
                    case AL_PAUSED: std::cout << "PAUSED"; break;
                    case AL_STOPPED: std::cout << "STOPPED"; break;
                    default: std::cout << "UNKNOWN"; break;
                    }
                    std::cout << std::endl;
                }
                std::cout << "=========================" << std::endl;
            }

            AudioSource::~AudioSource() {
                clearSource();
            }

            void AudioSource::OnUpdate() {
                if (!m_sourceId || !m_spatial) return;

                // Обновляем позицию только если объект двигается
                glm::vec3 currentPos = gameObject->transform.position;
                if (currentPos != m_lastPosition) {
                    updatePosition();
                    m_lastPosition = currentPos;
                }
            }

            void AudioSource::OnDestroy() {
                stop();
                clearSource();
            }

            void AudioSource::setSound(const std::string& path) {
                if (path.empty()) return;

                ALuint buffer = AssetManager::get().getSound(path);
                if (buffer) {
                    setSound(buffer);
                    m_soundPath = path;
                }
                else {
                    std::cerr << "Failed to load sound: " << path << std::endl;
                }
            }

            void AudioSource::setSound(ALuint buffer) {
                // Активируем контекст OpenAL
                if (AudioSystem::getInstance().isInitialized()) {
                    alcMakeContextCurrent(AudioSystem::getInstance().getContext());
                }

                if (!m_sourceId) {
                    std::cerr << "AudioSource: No source ID (source not created)" << std::endl;
                    return;
                }

                if (buffer == 0) {
                    std::cerr << "AudioSource: Invalid buffer (0)" << std::endl;
                    return;
                }

                std::cout << "AudioSource::setSound buffer=" << buffer << std::endl;

                // Полная очистка источника перед установкой нового буфера
                alSourceStop(m_sourceId);
                alSourcei(m_sourceId, AL_BUFFER, 0); // Отвязываем текущий буфер

                // Проверяем ошибки после очистки
                ALenum error = alGetError();
                if (error != AL_NO_ERROR) {
                    std::cerr << "  Warning: Error during source cleanup: " << error << std::endl;
                }

                // Теперь устанавливаем новый буфер
                m_bufferId = buffer;
                alSourcei(m_sourceId, AL_BUFFER, m_bufferId);

                error = alGetError();
                if (error != AL_NO_ERROR) {
                    std::cerr << "  Failed to set sound buffer, error: " << error << std::endl;
                    std::cerr << "  Source ID: " << m_sourceId << ", Buffer ID: " << buffer << std::endl;
                    m_bufferId = 0;
                    return;
                }

                std::cout << "  Buffer set successfully" << std::endl;
                applyParameters();
            }

            // В AudioSource::play():
            void AudioSource::play() {
                // Активируем контекст OpenAL
                if (AudioSystem::getInstance().isInitialized()) {
                    alcMakeContextCurrent(AudioSystem::getInstance().getContext());
                }

                if (!m_sourceId) {
                    std::cerr << "AudioSource: No source ID!" << std::endl;
                    return;
                }
                if (!m_bufferId) {
                    std::cerr << "AudioSource: No buffer ID for sound: " << m_soundPath << std::endl;
                    return;
                }

                std::cout << "Playing sound: " << m_soundPath << std::endl;
                alSourcePlay(m_sourceId);

                ALenum error = alGetError();
                if (error != AL_NO_ERROR) {
                    std::cerr << "Failed to play sound, error: " << error << std::endl;
                }
            }

            void AudioSource::stop() {
                if (AudioSystem::getInstance().isInitialized()) {
                    alcMakeContextCurrent(AudioSystem::getInstance().getContext());
                }
                if (!m_sourceId) return;
                alSourceStop(m_sourceId);
            }


            void AudioSource::pause() {
                if (!m_sourceId) return;
                alSourcePause(m_sourceId);
            }

            bool AudioSource::isPlaying() const {
                if (!m_sourceId) return false;

                ALint state;
                alGetSourcei(m_sourceId, AL_SOURCE_STATE, &state);
                return state == AL_PLAYING;
            }

            void AudioSource::setLooping(bool loop) {
                if (AudioSystem::getInstance().isInitialized()) {
                    alcMakeContextCurrent(AudioSystem::getInstance().getContext());
                }
                m_loop = loop;
                if (m_sourceId) {
                    alSourcei(m_sourceId, AL_LOOPING, loop ? AL_TRUE : AL_FALSE);
                }
            }

            void AudioSource::setVolume(float volume) {
                if (AudioSystem::getInstance().isInitialized()) {
                    alcMakeContextCurrent(AudioSystem::getInstance().getContext());
                }
                m_volume = glm::clamp(volume, 0.0f, 1.0f);
                if (m_sourceId) {
                    alSourcef(m_sourceId, AL_GAIN, m_volume);
                }
            }

            void AudioSource::setPitch(float pitch) {
                m_pitch = pitch;
                if (m_sourceId) {
                    alSourcef(m_sourceId, AL_PITCH, m_pitch);
                }
            }

            void AudioSource::setSpatial(bool spatial) {
                m_spatial = spatial;
                if (m_sourceId) {
                    alSourcei(m_sourceId, AL_SOURCE_RELATIVE, spatial ? AL_FALSE : AL_TRUE);
                    if (spatial) {
                        updatePosition();
                    }
                    else {
                        alSource3f(m_sourceId, AL_POSITION, 0.0f, 0.0f, 0.0f);
                    }
                }
            }

            void AudioSource::setMinDistance(float dist) {
                m_minDistance = dist;
                if (m_sourceId) {
                    alSourcef(m_sourceId, AL_REFERENCE_DISTANCE, m_minDistance);
                }
            }

            void AudioSource::setMaxDistance(float dist) {
                m_maxDistance = dist;
                if (m_sourceId) {
                    alSourcef(m_sourceId, AL_MAX_DISTANCE, m_maxDistance);
                }
            }

            void AudioSource::setRolloffFactor(float factor) {
                m_rolloffFactor = factor;
                if (m_sourceId) {
                    alSourcef(m_sourceId, AL_ROLLOFF_FACTOR, m_rolloffFactor);
                }
            }

            void AudioSource::setPosition(const glm::vec3& pos) {
                if (m_sourceId && m_spatial) {
                    alSource3f(m_sourceId, AL_POSITION, pos.x, pos.y, pos.z);
                    m_lastPosition = pos;
                }
            }

            void AudioSource::updatePosition() {
                if (!m_sourceId || !gameObject) return;

                glm::vec3 pos = gameObject->transform.position;
                alSource3f(m_sourceId, AL_POSITION, pos.x, pos.y, pos.z);
            }

            void AudioSource::applyParameters() {
                setLooping(m_loop);
                setVolume(m_volume);
                setPitch(m_pitch);
                setSpatial(m_spatial);
                setMinDistance(m_minDistance);
                setMaxDistance(m_maxDistance);
                setRolloffFactor(m_rolloffFactor);
                updatePosition();
            }

            void AudioSource::clearSource() {
                if (!m_sourceId) return;

                alSourceStop(m_sourceId);
                alSourcei(m_sourceId, AL_BUFFER, 0);

                m_bufferId = 0;
                m_soundPath.clear();
            }
        }
    }
}