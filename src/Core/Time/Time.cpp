#include "Time.h"
#include "Core/Types/Settings.h" // Подключаем настройки DisplaySettings
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

        // 1. Обновляем фиксированный шаг из настроек
        s_fixedDeltaTime = display.fixedTimestep;

        // 2. Если включен принудительный фиксированный шаг (useFixedTimestep == true)
        if (display.useFixedTimestep) {
            s_deltaTime = display.fixedTimestep;
        }
        else {
            std::chrono::duration<float> delta = currentTime - s_lastFrameTime;
            s_deltaTime = delta.count();

            // Защита от просадок кадров (ограничиваем дельту максимум ~0.1 сек / 10 FPS)
            s_deltaTime = std::min(s_deltaTime, 0.1f);
        }

        s_lastFrameTime = currentTime;

        // 3. Обновляем счетчики времени
        s_unscaledTotalTime = std::chrono::duration<float>(currentTime - s_startTime).count();
        s_totalTime += s_deltaTime * s_timeScale;

        s_frameCount++;
    }

}