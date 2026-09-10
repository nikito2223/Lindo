#include "AudioSource.h"
#include "core/AssetManager.h"
#include <AL/al.h>
#include <iostream>
#include <Component/Audio/AudioSystem.h>
#include <Component/GameObject/GameObject.h>

namespace Lindo::Components::Audio {

AudioSource::AudioSource() {
    auto& audio = AudioSystem::getInstance();
    if (!audio.isInitialized()) return;

    alcMakeContextCurrent(audio.getContext());
    alGenSources(1, &m_sourceId);
    if (alGetError() != AL_NO_ERROR || m_sourceId == 0) {
        m_sourceId = 0;
        return;
    }

    alSourcei(m_sourceId, AL_BUFFER, 0);
    applyParameters();
}

AudioSource::~AudioSource() {
    clearSource();
}

void AudioSource::play() {
    auto& audio = AudioSystem::getInstance();
    if (!audio.isInitialized() || !m_sourceId) return;

    // Попытка ленивого создания источника, если инициализация завершилась позже конструктора
    if (!m_sourceId) {
        alGenSources(1, &m_sourceId);
        if (!m_sourceId) return;
        applyParameters();
    }

    alcMakeContextCurrent(audio.getContext());
    if (!m_bufferId) return;

    alSourcePlay(m_sourceId);
}

void AudioSource::stop() {
    auto& audio = AudioSystem::getInstance();
    if (!audio.isInitialized() || !m_sourceId) return;

    alcMakeContextCurrent(audio.getContext());
    alSourceStop(m_sourceId);
}

void AudioSource::pause() {
    auto& audio = AudioSystem::getInstance();
    if (!audio.isInitialized() || !m_sourceId) return;

    alcMakeContextCurrent(audio.getContext());
    alSourcePause(m_sourceId);
}

bool AudioSource::isPlaying() const {
    auto& audio = AudioSystem::getInstance();
    if (!audio.isInitialized() || !m_sourceId) return false;

    ALint state;
    alGetSourcei(m_sourceId, AL_SOURCE_STATE, &state);
    return state == AL_PLAYING;
}

void AudioSource::setSound(const std::string& path) {
    if (path.empty()) return;
    m_soundPath = path;

    if (!AudioSystem::getInstance().isInitialized()) return;

    ALuint buffer = AssetManager::get().getSound(path);
    if (buffer) {
        setSound(buffer);
    }
}

void AudioSource::setSound(ALuint buffer) {
    auto& audio = AudioSystem::getInstance();
    if (!audio.isInitialized() || buffer == 0) return;

    alcMakeContextCurrent(audio.getContext());

    if (!m_sourceId) {
        alGenSources(1, &m_sourceId);
        if (!m_sourceId) return;
    }

    alSourceStop(m_sourceId);
    m_bufferId = buffer;
    alSourcei(m_sourceId, AL_BUFFER, m_bufferId);
    applyParameters();
}

void AudioSource::OnUpdate() {
    if (!m_sourceId || !m_spatial || !AudioSystem::getInstance().isInitialized()) return;

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

void AudioSource::updatePosition() {
    if (!m_sourceId || !gameObject || !AudioSystem::getInstance().isInitialized()) return;
    glm::vec3 pos = gameObject->transform.position;
    alSource3f(m_sourceId, AL_POSITION, pos.x, pos.y, pos.z);
}

void AudioSource::clearSource() {
    if (!m_sourceId || !AudioSystem::getInstance().isInitialized()) return;
    alSourceStop(m_sourceId);
    alSourcei(m_sourceId, AL_BUFFER, 0);
    alDeleteSources(1, &m_sourceId);
    m_sourceId = 0;
    m_bufferId = 0;
}

void AudioSource::applyParameters() {
    if (!m_sourceId || !AudioSystem::getInstance().isInitialized()) return;
    alSourcei(m_sourceId, AL_LOOPING, m_loop ? AL_TRUE : AL_FALSE);
    alSourcef(m_sourceId, AL_GAIN, m_volume);
    alSourcef(m_sourceId, AL_PITCH, m_pitch);
    alSourcei(m_sourceId, AL_SOURCE_RELATIVE, m_spatial ? AL_FALSE : AL_TRUE);
    alSourcef(m_sourceId, AL_REFERENCE_DISTANCE, m_minDistance);
    alSourcef(m_sourceId, AL_MAX_DISTANCE, m_maxDistance);
    alSourcef(m_sourceId, AL_ROLLOFF_FACTOR, m_rolloffFactor);
    updatePosition();
}

void AudioSource::setLooping(bool loop) { m_loop = loop; applyParameters(); }
void AudioSource::setVolume(float volume) { m_volume = glm::clamp(volume, 0.0f, 1.0f); applyParameters(); }
void AudioSource::setPitch(float pitch) { m_pitch = pitch; applyParameters(); }
void AudioSource::setSpatial(bool spatial) { m_spatial = spatial; applyParameters(); }
void AudioSource::setMinDistance(float dist) { m_minDistance = dist; applyParameters(); }
void AudioSource::setMaxDistance(float dist) { m_maxDistance = dist; applyParameters(); }
void AudioSource::setRolloffFactor(float factor) { m_rolloffFactor = factor; applyParameters(); }
void AudioSource::setPosition(const glm::vec3& pos) { m_lastPosition = pos; updatePosition(); }

} // namespace Lindo::Components::Audio