#include "AssetManager.h"
#include "Core/RenderAPI.h"
#include "debug/DebugLogger.h"
#include <fstream>
#include <vector>
#include <cstring>

namespace Lindo {

    AssetManager& AssetManager::get() {
        static AssetManager instance;
        return instance;
    }

    std::string AssetManager::resolvePath(const std::string& relativePath, const std::string& subDir) const {
        std::filesystem::path path(relativePath);

        // ���� ���� ��� ���������� ��� ��� �������� ������� ���������� � ���������� ��� ����
        if (path.is_absolute() || (path.has_parent_path() && path.begin()->string() == m_basePath.string())) {
            return path.string();
        }

        // ���� ������� ������������� (�������� "textures") � � ��� ��� � ����, ��������� �
        if (!subDir.empty() && path.parent_path().string().find(subDir) == std::string::npos) {
            return (m_basePath / subDir / path).lexically_normal().string();
        }

        // ����� ������ �������� ���� �� ������� �����
        return (m_basePath / path).lexically_normal().string();
    }

    std::string AssetManager::getShaderPath(const std::string& shaderName) const {
        const std::string name = std::filesystem::path(shaderName).filename().string();
        std::string apiFolder = "";

        switch (Lindo::Graphics::IRenderAPI::GetAPI()) {
        case Lindo::Graphics::GraphicsAPI::OpenGL:
        case Lindo::Graphics::GraphicsAPI::None:
        default:
            apiFolder = "Shaders/OpenGL";
            break;
        }

        std::vector<std::filesystem::path> candidates = {
            m_basePath / apiFolder / name,
            m_basePath / "Shaders" / apiFolder / name,
            m_basePath / "shaders" / name,
            m_basePath / "shaders" / apiFolder / name,
            std::filesystem::path(shaderName),
            resolvePath(shaderName, "shaders")
        };

        for (const auto& candidate : candidates) {
            if (std::filesystem::exists(candidate)) {
                std::string result = candidate.lexically_normal().string();
                LOG_DEBUG("[AssetManager] Resolved shader path for '" + shaderName + "': " + result);
                return result;
            }
        }

        std::string fallback = resolvePath(shaderName, "shaders");
        LOG_WARN("[AssetManager] Shader not found in API-specific folders, falling back to legacy path: " + fallback);
        return fallback;
    }

    // ===== �������� =====
    unsigned int AssetManager::loadTexture(const std::string& path) {
        std::string fullPath = resolvePath(path, "textures");

        // �������� ���� �� ��������� ������������ ������� � ������
        if (textures.count(fullPath)) {
            LOG_DEBUG("[AssetManager] Texture already cached: " + fullPath);
            return textures[fullPath];
        }

        LOG_DEBUG("[AssetManager] Requesting texture load: " + fullPath);
        unsigned int tex = Lindo::Graphics::loadTexture(fullPath);
        if (tex != 0) {
            textures[fullPath] = tex;
            LOG_INFO("[AssetManager] Texture cached successfully: " + fullPath + " [ID: " + std::to_string(tex) + "]");
        }
        else {
            LOG_ERROR("[AssetManager] Failed to load texture through loader: " + fullPath);
        }
        return tex;
    }

    unsigned int AssetManager::getTexture(const std::string& path) {
        std::string fullPath = resolvePath(path, "textures");
        if (textures.count(fullPath)) {
            LOG_DEBUG("[AssetManager] Fetching texture from cache: " + fullPath);
            return textures[fullPath];
        }
        LOG_WARN("[AssetManager] Texture not found in cache, triggering load: " + fullPath);
        return loadTexture(path);
    }

    // ===== ������ =====
    Lindo::Graphics::Model* AssetManager::loadModel(const std::string& path) {
        std::string fullPath = resolvePath(path, "models");

        if (models.count(fullPath)) {
            LOG_DEBUG("[AssetManager] Model already cached: " + fullPath);
            return models[fullPath].get();
        }

        LOG_INFO("[AssetManager] Loading 3D model: " + fullPath);
        try {
            models[fullPath] = std::make_unique<Lindo::Graphics::Model>(fullPath);
            LOG_INFO("[AssetManager] Model loaded and cached successfully: " + fullPath);
            return models[fullPath].get();
        }
        catch (const std::exception& e) {
            LOG_ERROR("[AssetManager] Exception caught while loading model " + fullPath + " - " + e.what());
            return nullptr;
        }
    }

    Lindo::Graphics::Model* AssetManager::getModel(const std::string& path) {
        std::string fullPath = resolvePath(path, "models");
        if (models.count(fullPath)) {
            return models[fullPath].get();
        }
        LOG_WARN("[AssetManager] Model missing in cache, loading: " + fullPath);
        return loadModel(path);
    }

    // ===== ����� =====
    ALuint AssetManager::loadSound(const std::string& path) {
        std::string fullPath = resolvePath(path, "sounds");
        LOG_INFO("[AssetManager] Loading audio file (WAV): " + fullPath);

        std::ifstream file(fullPath, std::ios::binary);
        if (!file.is_open()) {
            LOG_ERROR("[AssetManager] Cannot open sound file stream: " + fullPath);
            return 0;
        }

        // ������� WAV-���������
        char riff[4];
        file.read(riff, 4);
        if (std::strncmp(riff, "RIFF", 4) != 0) {
            LOG_ERROR("[AssetManager] Invalid RIFF header in audio file: " + fullPath);
            return 0;
        }

        file.seekg(8);
        char wave[4];
        file.read(wave, 4);
        if (std::strncmp(wave, "WAVE", 4) != 0) {
            LOG_ERROR("[AssetManager] Invalid WAVE header in audio file: " + fullPath);
            return 0;
        }

        // ���������� ���������� ����� (������, �������, ��������)
        file.seekg(22);
        short channels;
        file.read(reinterpret_cast<char*>(&channels), sizeof(short));

        file.seekg(24);
        int sampleRate;
        file.read(reinterpret_cast<char*>(&sampleRate), sizeof(int));

        file.seekg(34);
        short bitsPerSample;
        file.read(reinterpret_cast<char*>(&bitsPerSample), sizeof(short));

        file.seekg(40);
        int dataSize;
        file.read(reinterpret_cast<char*>(&dataSize), sizeof(int));

        // ������ ����� �����������
        std::vector<char> buffer(dataSize);
        file.read(buffer.data(), dataSize);

        // ����������� ������� ��� OpenAL
        ALenum format = 0;
        if (channels == 1 && bitsPerSample == 16) {
            format = AL_FORMAT_MONO16;
        }
        else if (channels == 2 && bitsPerSample == 16) {
            format = AL_FORMAT_STEREO16;
        }
        else {
            LOG_ERROR("[AssetManager] Unsupported audio format (Channels: " + std::to_string(channels) + ", Bits: " + std::to_string(bitsPerSample) + ") in: " + fullPath);
            return 0;
        }

        // �������� ������ OpenAL � �������� � ���� ������
        ALuint alBuffer;
        alGenBuffers(1, &alBuffer);
        if (alGetError() != AL_NO_ERROR) {
            LOG_ERROR("[AssetManager] OpenAL error generated during buffer creation for: " + fullPath);
            return 0;
        }

        alBufferData(alBuffer, format, buffer.data(), dataSize, sampleRate);
        if (alGetError() != AL_NO_ERROR) {
            LOG_ERROR("[AssetManager] OpenAL error while uploading buffer data for: " + fullPath);
            alDeleteBuffers(1, &alBuffer);
            return 0;
        }

        sounds[fullPath] = alBuffer;
        LOG_INFO("[AssetManager] Sound loaded successfully: " + fullPath + " [Buffer ID: " + std::to_string(alBuffer) + "]");
        return alBuffer;
    }

    ALuint AssetManager::getSound(const std::string& path) {
        std::string fullPath = resolvePath(path, "sounds");
        if (sounds.count(fullPath)) {
            return sounds[fullPath];
        }
        return loadSound(path);
    }

    // ===== ������� =====
    void AssetManager::clear() {
        LOG_INFO("[AssetManager] Clearing and releasing all cached assets...");

        // ����������� ������ ����������
        if (!textures.empty()) {
            LOG_DEBUG("[AssetManager] Deleting " + std::to_string(textures.size()) + " textures from GPU memory.");
            for (auto& [path, tex] : textures) {
                glDeleteTextures(1, &tex);
            }
            textures.clear();
        }

        // Unique_ptr ������������� ������� ������ ��� ������ clear
        if (!models.empty()) {
            LOG_DEBUG("[AssetManager] Unloading " + std::to_string(models.size()) + " 3D models.");
            models.clear();
        }

        // ����������� ������� OpenAL
        if (!sounds.empty()) {
            LOG_DEBUG("[AssetManager] Deleting " + std::to_string(sounds.size()) + " OpenAL audio buffers.");
            for (auto& [path, buf] : sounds) {
                alDeleteBuffers(1, &buf);
            }
            sounds.clear();
        }

        LOG_INFO("[AssetManager] All assets successfully cleared.");
    }
}