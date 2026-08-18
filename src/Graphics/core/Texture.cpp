#include "./core/OGL.h"
#include "Texture.h"
#include "core/Types/Settings.h" // Подключаем наши настройки
#include "debug/DebugLogger.h"   // Подключаем логгер
#include <iostream>
#include <cstdio>
#include <vector>
#include <png.h>

namespace Lindo {
    namespace Graphics {
        unsigned int loadTexture(const std::string& path) {
            LOG_DEBUG("[Texture] Attempting to open PNG file: " + path);

            // 1. Открываем файл в бинарном режиме
            FILE* fp = fopen(path.c_str(), "rb");
            if (!fp) {
                LOG_ERROR("[Texture] Couldn't open file: " + path);
                return 0;
            }

            // 2. Проверяем сигнатуру PNG (первые 8 байт)
            png_byte header[8];
            if (fread(header, 1, 8, fp) != 8 || png_sig_cmp(header, 0, 8)) {
                LOG_ERROR("[Texture] File is not a valid PNG format: " + path);
                fclose(fp);
                return 0;
            }

            // 3. Инициализируем структуры libpng
            png_structp png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, nullptr, nullptr, nullptr);
            if (!png_ptr) {
                LOG_CRITICAL("[Texture] Failed to allocate png_ptr for: " + path);
                fclose(fp);
                return 0;
            }

            png_infop info_ptr = png_create_info_struct(png_ptr);
            if (!info_ptr) {
                LOG_CRITICAL("[Texture] Failed to allocate info_ptr for: " + path);
                png_destroy_read_struct(&png_ptr, nullptr, nullptr);
                fclose(fp);
                return 0;
            }

            // Обработка ошибок libpng через setjmp
            if (setjmp(png_jmpbuf(png_ptr))) {
                LOG_ERROR("[Texture] LIBPNG_ERROR encountered while processing: " + path);
                png_destroy_read_struct(&png_ptr, &info_ptr, nullptr);
                fclose(fp);
                return 0;
            }

            png_init_io(png_ptr, fp);
            png_set_sig_bytes(png_ptr, 8);
            png_read_info(png_ptr, info_ptr);

            png_uint_32 width = png_get_image_width(png_ptr, info_ptr);
            png_uint_32 height = png_get_image_height(png_ptr, info_ptr);
            png_byte color_type = png_get_color_type(png_ptr, info_ptr);
            png_byte bit_depth = png_get_bit_depth(png_ptr, info_ptr);

            // 4. Нормализация форматов
            if (color_type == PNG_COLOR_TYPE_PALETTE)
                png_set_palette_to_rgb(png_ptr);

            if (color_type == PNG_COLOR_TYPE_GRAY && bit_depth < 8)
                png_set_expand_gray_1_2_4_to_8(png_ptr);

            if (png_get_valid(png_ptr, info_ptr, PNG_INFO_tRNS))
                png_set_tRNS_to_alpha(png_ptr);

            if (bit_depth == 16)
                png_set_strip_16(png_ptr);

            if (color_type == PNG_COLOR_TYPE_GRAY || color_type == PNG_COLOR_TYPE_GRAY_ALPHA)
                png_set_gray_to_rgb(png_ptr);

            png_read_update_info(png_ptr, info_ptr);

            // 5. Выделение памяти и чтение строк
            size_t rowbytes = png_get_rowbytes(png_ptr, info_ptr);
            std::vector<unsigned char> image_data(rowbytes * height);

            std::vector<png_bytep> row_pointers(height);
            for (png_uint_32 i = 0; i < height; ++i) {
                row_pointers[height - 1 - i] = &image_data[i * rowbytes]; // Инверсия Y для OpenGL
            }

            png_read_image(png_ptr, row_pointers.data());

            png_destroy_read_struct(&png_ptr, &info_ptr, nullptr);
            fclose(fp);

            GLenum format = (color_type & PNG_COLOR_MASK_ALPHA) ? GL_RGBA : GL_RGB;
            glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

            // 6. Создаем OpenGL текстуру
            unsigned int textureID;
            glGenTextures(1, &textureID);
            glBindTexture(GL_TEXTURE_2D, textureID);

            glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, image_data.data());
            glGenerateMipmap(GL_TEXTURE_2D);

            // 7. Интеграция параметров из Settings
            auto& settings = Settings::getInstance();
            LOG_DEBUG("[Texture] Applying configuration settings for texture ID: " + std::to_string(textureID));

            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

            // Настройка фильтрации в зависимости от выбора в Settings
            switch (settings.textureFiltering) {
            case TextureFiltering::Bilinear:
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_NEAREST);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
                break;
            case TextureFiltering::Trilinear:
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
                break;
            case TextureFiltering::Anisotropic2x:
            case TextureFiltering::Anisotropic8x:
            case TextureFiltering::Anisotropic16x: {
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

                GLfloat anisoFactor = 2.0f;
                if (settings.textureFiltering == TextureFiltering::Anisotropic8x) anisoFactor = 8.0f;
                if (settings.textureFiltering == TextureFiltering::Anisotropic16x) anisoFactor = 16.0f;

                // Проверка на поддержку анизотропии
                GLfloat maxAniso = 0.0f;
                glGetFloatv(GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, &maxAniso);
                if (maxAniso > 0.0f) {
                    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, std::min(anisoFactor, maxAniso));
                }
                break;
            }
            }

            LOG_INFO("[Texture] Loaded successfully: " + path + " (" + std::to_string(width) + "x" + std::to_string(height) + ", ID: " + std::to_string(textureID) + ")");

            return textureID;
        }
    }
}