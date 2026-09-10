#include "Settings.h"
#include <fstream>
#include <sstream>
#include <filesystem>
#include <iostream>
#include "Debug/DebugLogger.h" // Äëÿ LOG_INFO / LOG_ERROR

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
        std::filesystem::path path(filepath);
        std::filesystem::path absPath = std::filesystem::absolute(path);

        LOG_INFO("[Settings] Attempting to save config to: " + absPath.string());

        if (path.has_parent_path()) {
            std::error_code ec;
            std::filesystem::create_directories(path.parent_path(), ec);
            if (ec) {
                LOG_ERROR("[Settings] Failed to create directories: " + ec.message());
            }
        }

        std::ofstream outFile(filepath, std::ios::out | std::ios::trunc);
        if (!outFile.is_open()) {
            LOG_ERROR("[Settings] Failed to open file for writing: " + absPath.string());
            return;
        }

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

        outFile.flush();
        outFile.close();

        if (outFile.fail()) {
            LOG_ERROR("[Settings] Error occurred during writing to file: " + absPath.string());
        }
        else {
            LOG_INFO("[Settings] Config saved successfully to: " + absPath.string());
        }
    }

    void Settings::loadFromFile(const std::string& filepath) {
        std::filesystem::path absPath = std::filesystem::absolute(filepath);
        LOG_INFO("[Settings] Loading config from: " + absPath.string());

        if (!std::filesystem::exists(filepath)) {
            LOG_INFO("[Settings] Config file not found at " + absPath.string() + ". Using defaults and creating file.");
            saveToFile(filepath);
            return;
        }

        std::ifstream inFile(filepath);
        if (!inFile.is_open()) {
            LOG_ERROR("[Settings] Failed to open file for reading: " + absPath.string());
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
                    catch (...) {}
                }
            }
        }
        inFile.close();
        LOG_INFO("[Settings] Config loaded successfully.");
    }

    Settings& Settings::getInstance() {
        static Settings instance;
        return instance;
    }

    // --- DisplaySettings Implementation ---

    void DisplaySettings::saveToFile(const std::string& filepath) {
        std::filesystem::path path(filepath);
        std::filesystem::path absPath = std::filesystem::absolute(path);

        LOG_INFO("[DisplaySettings] Attempting to save display config to: " + absPath.string());

        if (path.has_parent_path()) {
            std::error_code ec;
            std::filesystem::create_directories(path.parent_path(), ec);
            if (ec) {
                LOG_ERROR("[DisplaySettings] Failed to create directories: " + ec.message());
            }
        }

        std::ofstream outFile(filepath, std::ios::out | std::ios::trunc);
        if (!outFile.is_open()) {
            LOG_ERROR("[DisplaySettings] Failed to open file for writing: " + absPath.string());
            return;
        }

        outFile << "windowWidth=" << windowWidth << "\n";
        outFile << "windowHeight=" << windowHeight << "\n";
        outFile << "fullscreen=" << fullscreen << "\n";
        outFile << "borderless=" << borderless << "\n";
        outFile << "vsync=" << vsync << "\n";
        outFile << "targetFPS=" << targetFPS << "\n";
        outFile << "useFixedTimestep=" << useFixedTimestep << "\n";
        outFile << "fixedTimestep=" << fixedTimestep << "\n";

        outFile.flush();
        outFile.close();

        if (outFile.fail()) {
            LOG_ERROR("[DisplaySettings] Error occurred during writing to file: " + absPath.string());
        }
        else {
            LOG_INFO("[DisplaySettings] Config saved successfully (fullscreen=" + std::to_string(fullscreen) + ") to: " + absPath.string());
        }
    }

    void DisplaySettings::loadFromFile(const std::string& filepath) {
        std::filesystem::path absPath = std::filesystem::absolute(filepath);
        LOG_INFO("[DisplaySettings] Loading config from: " + absPath.string());

        if (!std::filesystem::exists(filepath)) {
            LOG_INFO("[DisplaySettings] Config file not found at " + absPath.string() + ". Using defaults and creating file.");
            saveToFile(filepath);
            return;
        }

        std::ifstream inFile(filepath);
        if (!inFile.is_open()) {
            LOG_ERROR("[DisplaySettings] Failed to open file for reading: " + absPath.string());
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
        LOG_INFO("[DisplaySettings] Config loaded successfully.");
    }

    DisplaySettings& DisplaySettings::getInstance() {
        static DisplaySettings instance;
        return instance;
    }
}