#include "Time.h"
#include "Core/Types/Settings.h" // ���������� ��������� DisplaySettings
#include <algorithm>

namespace Lindo {

    void Time::Update() {
        auto currentTime = std::chrono::high_resolution_clock::now();

        if (!s_initialized) {
            s_startTime = currentTime;
            s_lastFrameTime = currentTime;
            s_initialized = true;
            return;
        }

        const auto& display = DisplaySettings::getInstance();

        // 1. ��������� ������������� ��� �� ��������
        s_fixedDeltaTime = display.fixedTimestep;

        // Render/gameplay time must always represent real elapsed time.
        // Fixed-step simulation is accumulated and consumed by PhysicsSystem.
        std::chrono::duration<float> delta = currentTime - s_lastFrameTime;
        s_deltaTime = delta.count();

        // Prevent a long pause or debugger break from producing a huge jump.
        s_deltaTime = std::clamp(s_deltaTime, 0.0f, 0.1f);

        s_lastFrameTime = currentTime;

        // 3. ��������� �������� �������
        s_unscaledTotalTime = std::chrono::duration<float>(currentTime - s_startTime).count();
        s_totalTime += s_deltaTime * s_timeScale;

        s_frameCount++;
    }

}