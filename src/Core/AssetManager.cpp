#include "AssetManager.h"
#include "core/Globals.h"
#include "debug/DebugLogger.h"  // Добавляем подключение логгера

namespace Lindo {
    // ===== SINGLETON =====
    AssetManager& AssetManager::get() {
        static AssetManager instance;
        return instance;
    }

    // ===== TEXTURES =====
    unsigned int AssetManager::loadTexture(const std::string& path) {
        LOG_DEBUG("Loading texture: " + path);
        
        if (textures.count(path)) {
            LOG_DEBUG("Texture already loaded: " + path + ", returning cached");
            return textures[path];
        }

        unsigned int tex = Lindo::Graphics::loadTexture(path);
        if (tex != 0) {
            textures[path] = tex;
            LOG_INFO("Texture loaded successfully: " + path + " (ID: " + std::to_string(tex) + ")");
        } else {
            LOG_ERROR("Failed to load texture: " + path);
        }
        return tex;
    }

    unsigned int AssetManager::getTexture(const std::string& path) {
        if (textures.count(path)) {
            LOG_DEBUG("Texture found in cache: " + path);
            return textures[path];
        }

        LOG_DEBUG("Texture not in cache, loading: " + path);
        return loadTexture(path);
    }

    // ===== MODELS =====
    Lindo::Graphics::Model* AssetManager::loadModel(const std::string& path) {
        LOG_DEBUG("Loading model: " + path);
        
        if (models.count(path)) {
            LOG_DEBUG("Model already loaded: " + path + ", returning cached");
            return models[path].get();
        }

        try {
            models[path] = std::make_unique<Lindo::Graphics::Model>(path);
            LOG_INFO("Model loaded successfully: " + path);
            return models[path].get();
        } catch (const std::exception& e) {
            LOG_ERROR("Failed to load model: " + path + " - " + e.what());
            return nullptr;
        }
    }

    Lindo::Graphics::Model* AssetManager::getModel(const std::string& path) {
        if (models.count(path)) {
            LOG_DEBUG("Model found in cache: " + path);
            return models[path].get();
        }

        LOG_DEBUG("Model not in cache, loading: " + path);
        return loadModel(path);
    }

    // ===== AUDIO =====
    ALuint AssetManager::loadSound(const std::string& path) {
        LOG_INFO("Loading sound: " + path);

        std::ifstream file(path, std::ios::binary);
        if (!file.is_open()) {
            LOG_ERROR("Cannot open sound file: " + path);
            return 0;
        }

        // Чтение и проверка заголовка WAV
        char riff[4]; 
        file.read(riff, 4);
        if (strncmp(riff, "RIFF", 4) != 0) {
            LOG_ERROR("Invalid RIFF header in: " + path);
            return 0;
        }

        // Пропуск размера файла
        file.seekg(8);
        char wave[4]; 
        file.read(wave, 4);
        if (strncmp(wave, "WAVE", 4) != 0) {
            LOG_ERROR("Invalid WAVE header in: " + path);
            return 0;
        }

        // Чтение параметров аудио
        file.seekg(22); // numChannels
        short channels; 
        file.read(reinterpret_cast<char*>(&channels), sizeof(short));
        LOG_DEBUG("  Sound channels: " + std::to_string(channels));

        file.seekg(24); // sampleRate
        int sampleRate; 
        file.read(reinterpret_cast<char*>(&sampleRate), sizeof(int));
        LOG_DEBUG("  Sample rate: " + std::to_string(sampleRate) + " Hz");

        file.seekg(34); // bitsPerSample
        short bitsPerSample; 
        file.read(reinterpret_cast<char*>(&bitsPerSample), sizeof(short));
        LOG_DEBUG("  Bits per sample: " + std::to_string(bitsPerSample));

        file.seekg(40); // data size
        int dataSize; 
        file.read(reinterpret_cast<char*>(&dataSize), sizeof(int));
        LOG_DEBUG("  Data size: " + std::to_string(dataSize) + " bytes");

        // Чтение аудиоданных
        std::vector<char> buffer(dataSize);
        file.read(buffer.data(), dataSize);

        // Определение формата OpenAL
        ALenum format = 0;
        if (channels == 1 && bitsPerSample == 16) {
            format = AL_FORMAT_MONO16;
            LOG_DEBUG("  Format: MONO16");
        } else if (channels == 2 && bitsPerSample == 16) {
            format = AL_FORMAT_STEREO16;
            LOG_DEBUG("  Format: STEREO16");
        } else {
            LOG_ERROR("Unsupported audio format: channels=" + std::to_string(channels) +
                     ", bitsPerSample=" + std::to_string(bitsPerSample) + " in: " + path);
            return 0;
        }

        // Создание буфера OpenAL
        ALuint alBuffer;
        alGenBuffers(1, &alBuffer);
        ALenum error = alGetError();
        if (error != AL_NO_ERROR) {
            LOG_ERROR("Failed to generate OpenAL buffer for: " + path + 
                     ", error code: " + std::to_string(error));
            return 0;
        }

        // Загрузка данных в буфер
        alBufferData(alBuffer, format, buffer.data(), dataSize, sampleRate);
        error = alGetError();
        if (error != AL_NO_ERROR) {
            LOG_ERROR("Failed to set OpenAL buffer data for: " + path + 
                     ", error code: " + std::to_string(error));
            alDeleteBuffers(1, &alBuffer);
            return 0;
        }

        sounds[path] = alBuffer;
        LOG_INFO("Sound loaded successfully: " + path + " (Buffer ID: " + std::to_string(alBuffer) + ")");
        return alBuffer;
    }

    ALuint AssetManager::getSound(const std::string& path) {
        if (sounds.count(path)) {
            LOG_DEBUG("Sound found in cache: " + path);
            return sounds[path];
        }

        LOG_DEBUG("Sound not in cache, loading: " + path);
        return loadSound(path);
    }

    // ===== CLEAR =====
    void AssetManager::clear() {
        LOG_INFO("Clearing all assets...");
        
        // Очистка текстур
        if (!textures.empty()) {
            LOG_DEBUG("Deleting " + std::to_string(textures.size()) + " textures");
            for (auto& [path, tex] : textures) {
                glDeleteTextures(1, &tex);
                LOG_DEBUG("Deleted texture: " + path + " (ID: " + std::to_string(tex) + ")");
            }
            textures.clear();
            LOG_INFO("All textures cleared");
        }

        // Очистка моделей
        if (!models.empty()) {
            LOG_DEBUG("Clearing " + std::to_string(models.size()) + " models");
            models.clear();
            LOG_INFO("All models cleared");
        }

        // Очистка звуков
        if (!sounds.empty()) {
            LOG_DEBUG("Deleting " + std::to_string(sounds.size()) + " sounds");
            for (auto& [path, buf] : sounds) {
                alDeleteBuffers(1, &buf);
                LOG_DEBUG("Deleted sound: " + path + " (Buffer ID: " + std::to_string(buf) + ")");
            }
            sounds.clear();
            LOG_INFO("All sounds cleared");
        }

        LOG_INFO("Asset cleanup completed");
    }
}