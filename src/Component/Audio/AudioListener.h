#pragma once
#include <Component/Component.h>

class GameObject;

namespace Lindo {
    namespace Components {
        namespace Audio {
            class AudioListener : public Lindo::World::Component {
            public:
                AudioListener() = default;
                virtual ~AudioListener();

                void OnUpdate(float deltaTime) override;
                void OnDestroy() override;

                // Настройки слушателя
                void setEnabled(bool enabled) { m_enabled = enabled; }
                bool isEnabled() const { return m_enabled; }

            private:
                bool m_enabled = true;
            };
        }
    }
}