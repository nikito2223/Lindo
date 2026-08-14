#pragma once
#include <unordered_map>
#include <string>
#include <memory>

#include <Graphics/core/model.h>
#include <Graphics/core/Texture.h>

#include <AL/al.h>
#include <AL/alc.h>
namespace Lindo {
    class AssetManager {
    public:
        static AssetManager& get();

        // ===== TEXTURES =====
        unsigned int loadTexture(const std::string& path);
        unsigned int getTexture(const std::string& path);

        // ===== MODELS =====
        Lindo::Graphics::Model* loadModel(const std::string& path);
        Lindo::Graphics::Model* getModel(const std::string& path);

        // ===== SOUND =====
        ALuint loadSound(const std::string& path);
        ALuint getSound(const std::string& path);

        // ===== CLEAR =====
        void clear();

    private:
        AssetManager() = default;

        std::unordered_map<std::string, unsigned int> textures;
        std::unordered_map<std::string, std::unique_ptr<Lindo::Graphics::Model>> models;
        std::unordered_map<std::string, ALuint> sounds;
    };
}