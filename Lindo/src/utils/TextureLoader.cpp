#include "./core/OGL.h"
#include "TextureLoader.h"
#include "antires/CryptoUtils.h"
#include "core/Globals.h"  // для доступа к useRawResources
#include <stb_image.h>
#include <iostream>

unsigned int loadTexture(const std::string& path) {
    int width, height, channels;
    unsigned char* image = nullptr;

    if (useRawResources) {
        // Прямая загрузка из файла (незашифрованный режим)
        image = stbi_load(path.c_str(), &width, &height, &channels, 0);
        if (!image) {
            std::cerr << "Failed to load texture: " << path << " - " << stbi_failure_reason() << std::endl;
            return 0;
        }
    }
    else {
        // Загрузка из зашифрованного файла
        std::vector<char> fileData;
        try {
            fileData = CryptoUtils::decryptFileBinary(path);
        }
        catch (const std::exception& e) {
            std::cerr << "Failed to read encrypted texture: " << path << " - " << e.what() << std::endl;
            return 0;
        }

        image = stbi_load_from_memory(
            reinterpret_cast<const unsigned char*>(fileData.data()),
            fileData.size(), &width, &height, &channels, 0);

        if (!image) {
            std::cerr << "Failed to load texture from memory: " << path << " - " << stbi_failure_reason() << std::endl;
            return 0;
        }
    }

    GLenum format = (channels == 4) ? GL_RGBA : GL_RGB;
    unsigned int textureID;
    glGenTextures(1, &textureID);
    glBindTexture(GL_TEXTURE_2D, textureID);
    glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, image);
    glGenerateMipmap(GL_TEXTURE_2D);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    stbi_image_free(image);
    return textureID;
}