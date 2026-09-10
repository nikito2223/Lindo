#pragma once
#include <string>
#include <functional>
#include <vector>

namespace Lindo {

    enum class ShadowQuality {
        Off,
        Low,
        Medium,
        High,
        Ultra
    };

    enum class TextureFiltering {
        Bilinear,
        Trilinear,
        Anisotropic2x,
        Anisotropic8x,
        Anisotropic16x
    };

    struct Settings {
        // --- Debug & Engine ---
        bool debugMode = false;
        bool showFPS = true;
        bool wireframeMode = false;

        // --- Graphics & Quality ---
        int msaaSamples = 4;
        ShadowQuality shadowQuality = ShadowQuality::High;
        TextureFiltering textureFiltering = TextureFiltering::Anisotropic8x;
        bool enableShadows = true;

        // --- Camera & Render Limits ---
        float fov = 75.0f;
        float nearPlane = 0.1f;
        float farPlane = 400.0f;

        // --- Post-Processing & Color ---
        bool enableBloom = true;
        bool enableFXAA = false;
        bool enableSSAO = false;
        float gamma = 2.2f;
        float exposure = 1.0f;

        // --- Audio ---
        float masterVolume = 1.0f;
        float musicVolume = 0.8f;
        float sfxVolume = 1.0f;
        bool muteAudio = false;

        using SettingsChangedCallback = std::function<void(const Settings&)>;
        
        void addOnChangedCallback(SettingsChangedCallback callback) {
            m_onChangeCallbacks.push_back(callback);
        }

        void apply(const std::string& filepath = "../config/settings.ini") {
            saveToFile(filepath);
            for (auto& cb : m_onChangeCallbacks) {
                if (cb) cb(*this);
            }
        }

        // --- Getters / Setters ---
        bool isDebugDrawEnabled() const { return debugMode; }
        void setDebugDrawEnabled(bool value) { debugMode = value; }

        void resetToDefaults();

        // ������ ��� ������ � �������
        void saveToFile(const std::string& filepath = "../config/settings.ini");
        void loadFromFile(const std::string& filepath = "../config/settings.ini");

        static Settings& getInstance();
        private: 
            std::vector<SettingsChangedCallback> m_onChangeCallbacks;
    };

    struct DisplaySettings {
        int windowWidth = 1280;
        int windowHeight = 720;
        bool fullscreen = true;
        bool borderless = false;
        bool vsync = true;
        int targetFPS = 0;
        bool useFixedTimestep = true;
        float fixedTimestep = 1.0f / 60.0f;

        float getAspectRatio() const {
            return windowHeight > 0 ? static_cast<float>(windowWidth) / static_cast<float>(windowHeight) : 16.0f / 9.0f;
        }

        using DisplayChangedCallback = std::function<void(const DisplaySettings&)>;

        void addOnChangedCallback(DisplayChangedCallback callback) {
            m_onChangeCallbacks.push_back(callback);
        }

        void apply(const std::string& filepath = "../config/display.ini") {
            saveToFile(filepath);
            for (auto& cb : m_onChangeCallbacks) {
                if (cb) cb(*this);
            }
        }

        void saveToFile(const std::string& filepath = "../config/display.ini");
        void loadFromFile(const std::string& filepath = "../config/display.ini");

        static DisplaySettings& getInstance();

        private:
            std::vector<DisplayChangedCallback> m_onChangeCallbacks;
    };
}