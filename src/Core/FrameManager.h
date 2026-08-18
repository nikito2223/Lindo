#pragma once

#include <chrono>

namespace Lindo {

    class FrameManager {
    public:
        // Инициализация менеджера кадров
        static void Init(int targetFPS = 60);

        // Фиксация начала кадра
        static void BeginFrame();

        // Завершение кадра, ограничение FPS и расчет статистики
        static void EndFrame();

        // Установка целевого лимита FPS (0 — без ограничений)
        static void SetTargetFPS(int targetFPS);
        static int GetTargetFPS() { return s_targetFPS; }

        // Метрики
        static float GetFPS() { return s_currentFPS; }
        static float GetFrameTimeMs() { return s_frameTimeMs; }

    private:
        static inline int s_targetFPS = 60;
        static inline float s_targetFrameTimeMs = 1000.0f / 60.0f;

        static inline float s_currentFPS = 0.0f;
        static inline float s_frameTimeMs = 0.0f;

        static inline std::chrono::high_resolution_clock::time_point s_frameStart;

        // Скользящее среднее для сглаживания показаний FPS
        static inline float s_fpsBuffer[30] = { 0.0f };
        static inline int s_bufferIndex = 0;
    };

}