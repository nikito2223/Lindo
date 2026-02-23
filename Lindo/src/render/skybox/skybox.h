#pragma once
#include <vector>
#include <string>
#include <glad/glad.h>
#include <shader/shader_s.h>

class Skybox {
public:
    // Конструктор для загрузки 6 изображений (стандартный кубическая карта)
    Skybox(std::vector<std::string> faces);

    // Конструктор для загрузки HDR изображения (equirectangular map)
    Skybox(const std::string& hdrFile, unsigned int resolution = 512);

    void Draw(Shader& shader);
    unsigned int getCubemapTexture() const { return cubemapTexture; }

private:
    void setupBuffers();
    unsigned int loadHDRTexture(const std::string& path);

    unsigned int VAO, VBO;
    unsigned int cubemapTexture;
    bool isHDR = false;
    unsigned int hdrResolution;
};