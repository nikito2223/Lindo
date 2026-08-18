#pragma once

#include <chrono>

namespace Lindo {

    class Time {
    public:
        // Вызывается один раз в начале главного цикла
        static void Update();

        // Основное дельта-время (с учетом timeScale)
        static float GetDeltaTime() { return s_deltaTime * s_timeScale; }

        // Реальное дельта-время без учета timeScale (для UI/Сетей)
        static float GetUnscaledDeltaTime() { return s_deltaTime; }

        // Фиксированный шаг времени (например, для физики)
        static float GetFixedDeltaTime() { return s_fixedDeltaTime * s_timeScale; }
        static float GetUnscaledFixedDeltaTime() { return s_fixedDeltaTime; }

        static float GetTotalTime() { return s_totalTime; }
        static float GetUnscaledTotalTime() { return s_unscaledTotalTime; }

        static float GetTimeScale() { return s_timeScale; }
        static void SetTimeScale(float scale) { s_timeScale = scale < 0.0f ? 0.0f : scale; }

        static unsigned int GetFrameCount() { return s_frameCount; }

    private:
        static inline float s_deltaTime = 0.0f;
        static inline float s_fixedDeltaTime = 1.0f / 60.0f;
        static inline float s_totalTime = 0.0f;
        static inline float s_unscaledTotalTime = 0.0f;
        static inline float s_timeScale = 1.0f;
        static inline unsigned int s_frameCount = 0;

        static inline std::chrono::high_resolution_clock::time_point s_lastFrameTime;
        static inline std::chrono::high_resolution_clock::time_point s_startTime;
        static inline bool s_initialized = false;
    };

}