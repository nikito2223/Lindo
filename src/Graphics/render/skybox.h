#pragma once
#include <vector>
#include <string>
#include <glad/glad.h>
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
            static Skybox* CreateFromHDRData(const std::vector<char>& data, unsigned int resolution);

            ~Skybox();

            void Draw(Shader& shader);
            unsigned int getCubemapTexture() const { return cubemapTexture; }

        private:
            Skybox() : isHDR(false), hdrResolution(512), VAO(0), VBO(0), cubemapTexture(0) {}

            void setupBuffers();
            unsigned int loadHDRTexture(const std::string& path);
            static unsigned int loadHDRTextureFromData(const std::vector<char>& data);

            // Хелпер для устранения дублирования кода генерации Cubemap из Equirectangular
            unsigned int generateCubemapFromEquirectangular(unsigned int hdrTexture, unsigned int resolution);

            unsigned int VAO, VBO;
            unsigned int cubemapTexture;
            bool isHDR = false;
            unsigned int hdrResolution;
        };
    }
}