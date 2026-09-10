#pragma once
#include <vector>
#include <string>
#include <memory>
#include <glad/glad.h>
#include <glm/glm.hpp>
#include <Graphics/core/Shader.h>

namespace Lindo {
    namespace Graphics {
        class Skybox {
        public:
            // Загрузка классического скайбокса из 6 PNG картинок
            Skybox(std::vector<std::string> faces);

            // Загрузка HDR скайбокса из файла (.hdr или 16-bit PNG)
            Skybox(const std::string& hdrFile, unsigned int resolution = 512);

            // Создание HDR скайбокса из массива байт в памяти
            static std::unique_ptr<Skybox> CreateFromHDRData(const std::vector<char>& data, unsigned int resolution);

            ~Skybox();

            // Автономный рендер: сам активирует шейдер и передает юниформы
            void Draw(const glm::mat4& viewMatrix, const glm::mat4& projectionMatrix, float time = 0.0f, const glm::vec3& cameraPos = glm::vec3(0.0f));

            unsigned int getCubemapTexture() const { return cubemapTexture; }

        private:
            Skybox() : isHDR(false), hdrResolution(512), VAO(0), VBO(0), cubemapTexture(0) {}

            void setupBuffers();
            void initShader(); // Загрузка шейдера скайбокса
            unsigned int loadHDRTexture(const std::string& path);
            static unsigned int loadHDRTextureFromData(const std::vector<char>& data);

            // Хелпер для устранения дублирования кода генерации Cubemap из Equirectangular
            unsigned int generateCubemapFromEquirectangular(unsigned int hdrTexture, unsigned int resolution);

            unsigned int VAO, VBO;
            unsigned int cubemapTexture;
            bool isHDR = false;
            unsigned int hdrResolution;

            std::unique_ptr<Shader> m_shader; // Собственный шейдер скайбокса
        };
    }
}