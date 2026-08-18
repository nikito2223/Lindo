#include "FrameManager.h"
#include <thread>

#ifdef _WIN32
#include <windows.h>
#include <timeapi.h>
#pragma comment(lib, "winmm.lib")
#endif

namespace Lindo {

    void FrameManager::Init(int targetFPS) {
#ifdef _WIN32
        // Принудительно поднимаем точность таймера Windows до 1 миллисекунды
        timeBeginPeriod(1);
#endif
        SetTargetFPS(targetFPS);
        s_frameStart = std::chrono::high_resolution_clock::now();
    }

    void FrameManager::SetTargetFPS(int targetFPS) {
        s_targetFPS = targetFPS;
        s_targetFrameTimeMs = (targetFPS > 0) ? (1000.0f / static_cast<float>(targetFPS)) : 0.0f;
    }

    void FrameManager::BeginFrame() {
        s_frameStart = std::chrono::high_resolution_clock::now();
    }

    void FrameManager::EndFrame() {
        if (s_targetFPS <= 0) {
            auto now = std::chrono::high_resolution_clock::now();
            s_frameTimeMs = std::chrono::duration<float, std::milli>(now - s_frameStart).count();
            if (s_frameTimeMs > 0.0f) s_currentFPS = 1000.0f / s_frameTimeMs;
            return;
        }

        auto now = std::chrono::high_resolution_clock::now();
        float elapsedMs = std::chrono::duration<float, std::milli>(now - s_frameStart).count();

        // Держим поток в гибридном ожидании
        while (elapsedMs < s_targetFrameTimeMs) {
            float timeToWait = s_targetFrameTimeMs - elapsedMs;

            // Засыпаем только если до конца кадра больше 3 мс (учитывая погрешность OS)
            if (timeToWait > 3.0f) {
                std::this_thread::sleep_for(std::chrono::milliseconds(1));
            }
            else {
                // Микросекундный спин-лок для идеальной точности в конце кадра
                std::this_thread::yield();
            }

            now = std::chrono::high_resolution_clock::now();
            elapsedMs = std::chrono::duration<float, std::milli>(now - s_frameStart).count();
        }

        s_frameTimeMs = elapsedMs;

        // Расчет сглаженного FPS
        if (s_frameTimeMs > 0.0f) {
            s_fpsBuffer[s_bufferIndex] = 1000.0f / s_frameTimeMs;
            s_bufferIndex = (s_bufferIndex + 1) % 30;

            float sum = 0.0f;
            for (float fps : s_fpsBuffer) sum += fps;
            s_currentFPS = sum / 30.0f;
        }
    }

}