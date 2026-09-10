#include "AudioListener.h"
#include "AudioSystem.h"
#include <AL/al.h>
#include <Component/GameObject/GameObject.h>

namespace Lindo::Components::Audio {

AudioListener::~AudioListener() {
    if (AudioSystem::getInstance().getListener() == this) {
        AudioSystem::getInstance().setListener(nullptr);
    }
}

void AudioListener::OnUpdate() {
    if (!m_enabled || !gameObject || !AudioSystem::getInstance().isInitialized()) return;

    if (AudioSystem::getInstance().getListener() != this) {
        AudioSystem::getInstance().setListener(this);
    }

    glm::vec3 pos = gameObject->transform.position;
    alListener3f(AL_POSITION, pos.x, pos.y, pos.z);
    alListener3f(AL_VELOCITY, 0.0f, 0.0f, 0.0f);

    glm::vec3 forward = gameObject->transform.getForward();
    glm::vec3 up = gameObject->transform.getUp();

    ALfloat orientation[] = { forward.x, forward.y, forward.z, up.x, up.y, up.z };
    alListenerfv(AL_ORIENTATION, orientation);
}

void AudioListener::OnDestroy() {
    if (AudioSystem::getInstance().getListener() == this) {
        AudioSystem::getInstance().setListener(nullptr);
    }
}

} // namespace Lindo::Components::Audio