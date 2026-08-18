#pragma once
#include <unordered_map>
#include <string>
#include <memory>
#include <filesystem>

#include <Graphics/core/model.h>
#include <Graphics/core/Texture.h>

#include <AL/al.h>
#include <AL/alc.h>

namespace Lindo {
    class AssetManager {
    public:
        static AssetManager& get();

        // Ќастройка базового пути к ресурсам (по умолчанию "res/")
        void setBasePath(const std::filesystem::path& path) { m_basePath = path; }
        const std::filesystem::path& getBasePath() const { return m_basePath; }

        // ѕреобразование любого пути в полный относительный/абсолютный путь
        std::string resolvePath(const std::string& relativePath, const std::string& subDir = "") const;

        // ===== TEXTURES =====
        unsigned int loadTexture(const std::string& path);
        unsigned int getTexture(const std::string& path);

        // ===== MODELS =====
        Lindo::Graphics::Model* loadModel(const std::string& path);
        Lindo::Graphics::Model* getModel(const std::string& path);

        // ===== SOUND =====
        ALuint loadSound(const std::string& path);
        ALuint getSound(const std::string& path);

        // ===== SHADERS (¬спомогательный метод) =====
        std::string getShaderPath(const std::string& shaderName) const;

        // ===== CLEAR =====
        void clear();

    private:
        AssetManager() = default;

        std::filesystem::path m_basePath = "../res";

        std::unordered_map<std::string, unsigned int> textures;
        std::unordered_map<std::string, std::unique_ptr<Lindo::Graphics::Model>> models;
        std::unordered_map<std::string, ALuint> sounds;
    };
}