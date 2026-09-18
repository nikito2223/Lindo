#pragma once
#include <unordered_map>
#include <string>
#include <memory>
#include <filesystem>

#include <Graphics/core/model.h>
#include <Graphics/core/Texture.h>

#include <AL/al.h>
#include <AL/alc.h>

#include "Graphics/ui/UIFont.h"

namespace Lindo {

    /**
     * @brief ���������� �������� �������� ������ Lindo.
     *
     * ��������� ������� Singleton. �������� �� ��������, �����������
     * � ���������� �������� ������� (OpenGL), 3D-������� � �������� ������� (OpenAL).
     * ������������� ������������ �������� � ������.
     */
    class AssetManager {
    public:
        /**
         * @brief �������� ������������ ��������� ��������� ��������.
         * @return ������ �� ���������� ������ AssetManager.
         */
        static AssetManager& get();

        /**
         * @brief ������������� ������� ���������� ��� ������ ���� ��������.
         * @param path ���� � ����� � ��������� (�� ��������� "../res").
         */
        void setBasePath(const std::filesystem::path& path) { m_basePath = path; }

        /**
         * @brief ���������� ������� ������� ���������� ��������.
         * @return ����������� ������ �� ����.
         */
        const std::filesystem::path& getBasePath() const { return m_basePath; }

        /**
         * @brief ����������� ������������� ���� � ������, �������� ������� ���������� � ��������.
         * @param relativePath ������������� ���� � ����� (��������, "player.png").
         * @param subDir ������������� ������ �������� ���� (��������, "textures").
         * @return ������ � ��������������� ������ ����� � �����.
         */
        std::string resolvePath(const std::string& relativePath, const std::string& subDir = "") const;

        // ===== �������� =====

        /**
         * @brief ������������� ��������� �������� � �������� � � ���.
         * @param path ���� � ����� ��������.
         * @return ������������� �������� OpenGL (ID). ���� �������� �� �������, ������ 0.
         */
        unsigned int loadTexture(const std::string& path);

        /**
         * @brief ���������� �������� �� ����. ���� � ��� ���, ������������� �������� loadTexture().
         * @param path ���� � ����� ��������.
         * @return ������������� �������� OpenGL (ID).
         */
        unsigned int getTexture(const std::string& path);

        // ===== ������ =====

        /**
         * @brief ������������� ��������� 3D-������ � �������� � � ���.
         * @param path ���� � ����� ������.
         * @return ��������� �� ����������� ������ (Lindo::Graphics::Model). ������ nullptr ��� ������.
         */
        Lindo::Graphics::Model* loadModel(const std::string& path);

        /**
         * @brief ���������� ������ �� ����. ���� � ��� ���, ������������� �������� loadModel().
         * @param path ���� � ����� ������.
         * @return ��������� �� ������ ������.
         */
        Lindo::Graphics::Model* getModel(const std::string& path);

        // ===== ����� =====

        /**
         * @brief ��������� ��������� (WAV) � ������� ��� ���� ����� OpenAL.
         * @param path ���� � ����������.
         * @return ������������� ������ OpenAL (ALuint). ������ 0 ��� ������ �������� ��� ��������.
         */
        ALuint loadSound(const std::string& path);

        /**
         * @brief ���������� �������� ����� �� ����. ���� ��� ��� ���, ������������� �������� loadSound().
         * @param path ���� � ����������.
         * @return ������������� ������ OpenAL (ALuint).
         */
        ALuint getSound(const std::string& path);

        // ===== Fonts =====

        /**
         * @brief Loads a TTF font into a UIFont (glyph atlas) and caches it.
         *
         * Cache key is (path + fontSize). If a font with the same path
         * and size is already cached, the cached instance is returned.
         * @param path Path to the TTF file (absolute paths are honored as-is).
         * @param fontSize Font size in pixels.
         * @param atlasWidth Glyph atlas width (default 512).
         * @param atlasHeight Glyph atlas height (default 512).
         * @param charset Set of characters to bake into the atlas (empty = default set).
         * @return Pointer to the cached UIFont, or nullptr on failure.
         */
        Lindo::Graphics::UI::UIFont* loadFont(const std::string& path, float fontSize = 24.0f,
            int atlasWidth = 512, int atlasHeight = 512, const std::string& charset = "");

        /**
         * @brief Returns a cached font. If missing, loads it via loadFont().
         * @param path Path to the TTF file.
         * @param fontSize Font size in pixels.
         * @param atlasWidth Glyph atlas width (default 512), used only if a load is triggered.
         * @param atlasHeight Glyph atlas height (default 512), used only if a load is triggered.
         * @param charset Charset used only if a load is triggered.
         * @return Pointer to the cached UIFont.
         */
        Lindo::Graphics::UI::UIFont* getFont(const std::string& path, float fontSize = 24.0f,
            int atlasWidth = 512, int atlasHeight = 512, const std::string& charset = "");

        // ===== ������� =====

        /**
         * @brief ��������������� ����� ��� ��������� ������� ���� � ����� �������.
         * @param shaderName ��� ����� ������� (��������, "main.vert").
         * @return ������ � ������ ����� � �������.
         */
        std::string getShaderPath(const std::string& shaderName) const;

        // ===== ������� =====

        /**
         * @brief ��������� ������� ��� ���������.
         *
         * ������� ��� �������� �� ����������� (glDeleteTextures), ��������� 3D-������
         * � ������� �������� ������ (alDeleteBuffers). ������������� �������� ��� ����� ���� ��� �������� ����.
         */
        void clear();

    private:
        AssetManager() = default;

        std::filesystem::path m_basePath = "../assets";

        std::unordered_map<std::string, unsigned int> textures;
        std::unordered_map<std::string, std::unique_ptr<Lindo::Graphics::Model>> models;
        std::unordered_map<std::string, ALuint> sounds;
        std::unordered_map<std::string, std::unique_ptr<Lindo::Graphics::UI::UIFont>> fonts;
    };
}