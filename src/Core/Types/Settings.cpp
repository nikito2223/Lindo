#include "Settings.h"
#include <fstream>
#include <sstream>
#include <filesystem>
#include <iostream>

namespace Lindo {

    // --- Settings Implementation ---

    void Settings::resetToDefaults() {
        debugMode = false;
        showFPS = true;
        wireframeMode = false;
        msaaSamples = 4;
        shadowQuality = ShadowQuality::High;
        textureFiltering = TextureFiltering::Anisotropic8x;
        enableShadows = true;
        enableBloom = true;
        enableFXAA = false;
        enableSSAO = false;
        gamma = 2.2f;
        exposure = 1.0f;
        fov = 60.0f;
        nearPlane = 0.1f;
        farPlane = 400.0f;
        masterVolume = 1.0f;
        musicVolume = 0.8f;
        sfxVolume = 1.0f;
        muteAudio = false;
    }

    void Settings::saveToFile(const std::string& filepath) {
        // Убеждаемся, что директория существует (например, config/)
        std::filesystem::path path(filepath);
        if (path.has_parent_path()) {
            std::filesystem::create_directories(path.parent_path());
        }

        std::ofstream outFile(filepath);
        if (!outFile.is_open()) {
            std::cerr << "[Settings] Failed to open file for writing: " << filepath << std::endl;
            return;
        }

        // Самый простой и надежный формат ключ = значение (INI-like)
        outFile << "debugMode=" << debugMode << "\n";
        outFile << "showFPS=" << showFPS << "\n";
        outFile << "wireframeMode=" << wireframeMode << "\n";
        outFile << "msaaSamples=" << msaaSamples << "\n";
        outFile << "shadowQuality=" << static_cast<int>(shadowQuality) << "\n";
        outFile << "textureFiltering=" << static_cast<int>(textureFiltering) << "\n";
        outFile << "enableShadows=" << enableShadows << "\n";
        outFile << "enableBloom=" << enableBloom << "\n";
        outFile << "enableFXAA=" << enableFXAA << "\n";
        outFile << "enableSSAO=" << enableSSAO << "\n";
        outFile << "gamma=" << gamma << "\n";
        outFile << "exposure=" << exposure << "\n";
        outFile << "fov=" << fov << "\n";
        outFile << "nearPlane=" << nearPlane << "\n";
        outFile << "farPlane=" << farPlane << "\n";
        outFile << "masterVolume=" << masterVolume << "\n";
        outFile << "musicVolume=" << musicVolume << "\n";
        outFile << "sfxVolume=" << sfxVolume << "\n";
        outFile << "muteAudio=" << muteAudio << "\n";

        outFile.close();
    }

    void Settings::loadFromFile(const std::string& filepath) {
        if (!std::filesystem::exists(filepath)) {
            std::cout << "[Settings] Config file not found at " << filepath << ". Using defaults and creating file.\n";
            saveToFile(filepath); // Создаем файл с дефолтными значениями, если его нет
            return;
        }

        std::ifstream inFile(filepath);
        if (!inFile.is_open()) {
            std::cerr << "[Settings] Failed to open file for reading: " << filepath << std::endl;
            return;
        }

        std::string line;
        while (std::getline(inFile, line)) {
            std::istringstream ss(line);
            std::string key;
            if (std::getline(ss, key, '=')) {
                std::string value;
                if (std::getline(ss, value)) {
                    try {
                        if (key == "debugMode") debugMode = (value == "1" || value == "true");
                        else if (key == "showFPS") showFPS = (value == "1" || value == "true");
                        else if (key == "wireframeMode") wireframeMode = (value == "1" || value == "true");
                        else if (key == "msaaSamples") msaaSamples = std::stoi(value);
                        else if (key == "shadowQuality") shadowQuality = static_cast<ShadowQuality>(std::stoi(value));
                        else if (key == "textureFiltering") textureFiltering = static_cast<TextureFiltering>(std::stoi(value));
                        else if (key == "enableShadows") enableShadows = (value == "1" || value == "true");
                        else if (key == "enableBloom") enableBloom = (value == "1" || value == "true");
                        else if (key == "enableFXAA") enableFXAA = (value == "1" || value == "true");
                        else if (key == "enableSSAO") enableSSAO = (value == "1" || value == "true");
                        else if (key == "gamma") gamma = std::stof(value);
                        else if (key == "exposure") exposure = std::stof(value);
                        else if (key == "fov") fov = std::stof(value);
                        else if (key == "nearPlane") nearPlane = std::stof(value);
                        else if (key == "farPlane") farPlane = std::stof(value);
                        else if (key == "masterVolume") masterVolume = std::stof(value);
                        else if (key == "musicVolume") musicVolume = std::stof(value);
                        else if (key == "sfxVolume") sfxVolume = std::stof(value);
                        else if (key == "muteAudio") muteAudio = (value == "1" || value == "true");
                    }
                    catch (...) {
                        // Игнорируем ошибки парсинга отдельных строк
                    }
                }
            }
        }
        inFile.close();
    }

    Settings& Settings::getInstance() {
        static Settings instance;
        return instance;
    }

    // --- DisplaySettings Implementation ---

    void DisplaySettings::saveToFile(const std::string& filepath) {
        std::filesystem::path path(filepath);
        if (path.has_parent_path()) {
            std::filesystem::create_directories(path.parent_path());
        }

        std::ofstream outFile(filepath);
        if (!outFile.is_open()) return;

        outFile << "windowWidth=" << windowWidth << "\n";
        outFile << "windowHeight=" << windowHeight << "\n";
        outFile << "fullscreen=" << fullscreen << "\n";
        outFile << "borderless=" << borderless << "\n";
        outFile << "vsync=" << vsync << "\n";
        outFile << "targetFPS=" << targetFPS << "\n";
        outFile << "useFixedTimestep=" << useFixedTimestep << "\n";
        outFile << "fixedTimestep=" << fixedTimestep << "\n";

        outFile.close();
    }

    void DisplaySettings::loadFromFile(const std::string& filepath) {
        if (!std::filesystem::exists(filepath)) {
            saveToFile(filepath);
            return;
        }

        std::ifstream inFile(filepath);
        if (!inFile.is_open()) return;

        std::string line;
        while (std::getline(inFile, line)) {
            std::istringstream ss(line);
            std::string key;
            if (std::getline(ss, key, '=')) {
                std::string value;
                if (std::getline(ss, value)) {
                    try {
                        if (key == "windowWidth") windowWidth = std::stoi(value);
                        else if (key == "windowHeight") windowHeight = std::stoi(value);
                        else if (key == "fullscreen") fullscreen = (value == "1" || value == "true");
                        else if (key == "borderless") borderless = (value == "1" || value == "true");
                        else if (key == "vsync") vsync = (value == "1" || value == "true");
                        else if (key == "targetFPS") targetFPS = std::stoi(value);
                        else if (key == "useFixedTimestep") useFixedTimestep = (value == "1" || value == "true");
                        else if (key == "fixedTimestep") fixedTimestep = std::stof(value);
                    }
                    catch (...) {}
                }
            }
        }
        inFile.close();
    }

    DisplaySettings& DisplaySettings::getInstance() {
        static DisplaySettings instance;
        return instance;
    }
}