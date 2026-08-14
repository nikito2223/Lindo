#pragma once
#include <vector>
#include <string>
#include <glad/glad.h>
#include <Graphics/core/Shader.h>


class Skybox {
public:
    //  онструктор дл€ загрузки 6 изображений (стандартный кубическа€ карта)
    Skybox(std::vector<std::string> faces);

    //  онструктор дл€ загрузки HDR изображени€ (equirectangular map)
    Skybox(const std::string& hdrFile, unsigned int resolution = 512);

    static Skybox* CreateFromHDRData(const std::vector<char>& data, unsigned int resolution);

    void Draw(Shader& shader);
    unsigned int getCubemapTexture() const { return cubemapTexture; }

private:
    void setupBuffers();
    unsigned int loadHDRTexture(const std::string& path);

    Skybox() : isHDR(false), hdrResolution(512), VAO(0), VBO(0), cubemapTexture(0) {}

    // «агрузка HDR-текстуры из пам€ти
    static unsigned int loadHDRTextureFromData(const std::vector<char>& data);

    unsigned int VAO, VBO;
    unsigned int cubemapTexture;
    bool isHDR = false;
    unsigned int hdrResolution;
};