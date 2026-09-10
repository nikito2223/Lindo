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

    /**
     * @brief Глобальный менеджер ресурсов движка Lindo.
     *
     * Реализует паттерн Singleton. Отвечает за загрузку, кэширование
     * и корректную выгрузку текстур (OpenGL), 3D-моделей и звуковых буферов (OpenAL).
     * Предотвращает дублирование ресурсов в памяти.
     */
    class AssetManager {
    public:
        /**
         * @brief Получает единственный экземпляр менеджера ресурсов.
         * @return Ссылка на глобальный объект AssetManager.
         */
        static AssetManager& get();

        /**
         * @brief Устанавливает базовую директорию для поиска всех ресурсов.
         * @param path Путь к папке с ресурсами (по умолчанию "../res").
         */
        void setBasePath(const std::filesystem::path& path) { m_basePath = path; }

        /**
         * @brief Возвращает текущую базовую директорию ресурсов.
         * @return Константная ссылка на путь.
         */
        const std::filesystem::path& getBasePath() const { return m_basePath; }

        /**
         * @brief Преобразует относительный путь в полный, учитывая базовую директорию и подпапку.
         * @param relativePath Относительный путь к файлу (например, "player.png").
         * @param subDir Поддиректория внутри базового пути (например, "textures").
         * @return Строка с нормализованным полным путем к файлу.
         */
        std::string resolvePath(const std::string& relativePath, const std::string& subDir = "") const;

        // ===== ТЕКСТУРЫ =====

        /**
         * @brief Принудительно загружает текстуру и помещает её в кэш.
         * @param path Путь к файлу текстуры.
         * @return Идентификатор текстуры OpenGL (ID). Если загрузка не удалась, вернет 0.
         */
        unsigned int loadTexture(const std::string& path);

        /**
         * @brief Возвращает текстуру из кэша. Если её там нет, автоматически вызывает loadTexture().
         * @param path Путь к файлу текстуры.
         * @return Идентификатор текстуры OpenGL (ID).
         */
        unsigned int getTexture(const std::string& path);

        // ===== МОДЕЛИ =====

        /**
         * @brief Принудительно загружает 3D-модель и помещает её в кэш.
         * @param path Путь к файлу модели.
         * @return Указатель на загруженную модель (Lindo::Graphics::Model). Вернет nullptr при ошибке.
         */
        Lindo::Graphics::Model* loadModel(const std::string& path);

        /**
         * @brief Возвращает модель из кэша. Если её там нет, автоматически вызывает loadModel().
         * @param path Путь к файлу модели.
         * @return Указатель на объект модели.
         */
        Lindo::Graphics::Model* getModel(const std::string& path);

        // ===== ЗВУКИ =====

        /**
         * @brief Загружает аудиофайл (WAV) и создает для него буфер OpenAL.
         * @param path Путь к аудиофайлу.
         * @return Идентификатор буфера OpenAL (ALuint). Вернет 0 при ошибке парсинга или загрузки.
         */
        ALuint loadSound(const std::string& path);

        /**
         * @brief Возвращает звуковой буфер из кэша. Если его там нет, автоматически вызывает loadSound().
         * @param path Путь к аудиофайлу.
         * @return Идентификатор буфера OpenAL (ALuint).
         */
        ALuint getSound(const std::string& path);

        // ===== ШЕЙДЕРЫ =====

        /**
         * @brief Вспомогательный метод для получения полного пути к файлу шейдера.
         * @param shaderName Имя файла шейдера (например, "main.vert").
         * @return Строка с полным путем к шейдеру.
         */
        std::string getShaderPath(const std::string& shaderName) const;

        // ===== ОЧИСТКА =====

        /**
         * @brief Полностью очищает кэш менеджера.
         *
         * Удаляет все текстуры из видеопамяти (glDeleteTextures), выгружает 3D-модели
         * и удаляет звуковые буферы (alDeleteBuffers). Рекомендуется вызывать при смене сцен или закрытии игры.
         */
        void clear();

    private:
        AssetManager() = default;

        std::filesystem::path m_basePath = "../res";

        std::unordered_map<std::string, unsigned int> textures;
        std::unordered_map<std::string, std::unique_ptr<Lindo::Graphics::Model>> models;
        std::unordered_map<std::string, ALuint> sounds;
    };
}