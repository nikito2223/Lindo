#include "./core/OGL.h"
#include "Texture.h"
#include "core/Globals.h"  
#include <iostream>
#include <cstdio>
#include <vector>
#include <png.h> // Подключаем libpng

namespace Lindo {
    namespace Graphics {
        unsigned int loadTexture(const std::string& path) {
            // 1. Открываем файл в бинарном режиме
            FILE* fp = fopen(path.c_str(), "rb");
            if (!fp) {
                std::cerr << "ERROR::TEXTURE::LOAD_FAILED: Couldn't open file " << path << std::endl;
                return 0;
            }

            // 2. Проверяем сигнатуру PNG (первые 8 байт)
            png_byte header[8];
            if (fread(header, 1, 8, fp) != 8 || png_sig_cmp(header, 0, 8)) {
                std::cerr << "ERROR::TEXTURE::LOAD_FAILED: " << path << " is not a valid PNG file!" << std::endl;
                fclose(fp);
                return 0;
            }

            // 3. Инициализируем структуры libpng
            png_structp png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, nullptr, nullptr, nullptr);
            if (!png_ptr) {
                fclose(fp);
                return 0;
            }

            png_infop info_ptr = png_create_info_struct(png_ptr);
            if (!info_ptr) {
                png_destroy_read_struct(&png_ptr, nullptr, nullptr);
                fclose(fp);
                return 0;
            }

            // Обработка ошибок libpng через setjmp (стандартный C-механизм библиотеки)
            if (setjmp(png_jmpbuf(png_ptr))) {
                png_destroy_read_struct(&png_ptr, &info_ptr, nullptr);
                fclose(fp);
                std::cerr << "ERROR::TEXTURE::LIBPNG_ERROR while processing " << path << std::endl;
                return 0;
            }

            // Передаём файловый указатель и сообщаем, что 8 байт сигнатуры уже прочитаны
            png_init_io(png_ptr, fp);
            png_set_sig_bytes(png_ptr, 8);

            // Читаем метаданные изображения
            png_read_info(png_ptr, info_ptr);

            png_uint_32 width = png_get_image_width(png_ptr, info_ptr);
            png_uint_32 height = png_get_image_height(png_ptr, info_ptr);
            png_byte color_type = png_get_color_type(png_ptr, info_ptr);
            png_byte bit_depth = png_get_bit_depth(png_ptr, info_ptr);

            // 4. Нормализуем форматы в стандартизированный RGBA / RGB 8-bit

            // Преобразуем палитровые изображения в RGB
            if (color_type == PNG_COLOR_TYPE_PALETTE)
                png_set_palette_to_rgb(png_ptr);

            // Градации серого менее 8 бит приводим к 8 битам
            if (color_type == PNG_COLOR_TYPE_GRAY && bit_depth < 8)
                png_set_expand_gray_1_2_4_to_8(png_ptr);

            // Прозрачность (tRNS chunk) приводим к полноценному альфа-каналу
            if (png_get_valid(png_ptr, info_ptr, PNG_INFO_tRNS))
                png_set_tRNS_to_alpha(png_ptr);

            // 16-битные изображения снижаем до 8-битных (для стандартного OpenGL пиксельного формата)
            if (bit_depth == 16)
                png_set_strip_16(png_ptr);

            // Градации серого конвертируем в RGB
            if (color_type == PNG_COLOR_TYPE_GRAY || color_type == PNG_COLOR_TYPE_GRAY_ALPHA)
                png_set_gray_to_rgb(png_ptr);

            // Обновляем информацию после трансформаций
            png_read_update_info(png_ptr, info_ptr);

            // 5. Выделяем память под данные пикселей
            size_t rowbytes = png_get_rowbytes(png_ptr, info_ptr);
            std::vector<unsigned char> image_data(rowbytes * height);

            // Массив указателей на строки для libpng
            std::vector<png_bytep> row_pointers(height);
            for (png_uint_32 i = 0; i < height; ++i) {
                // Опционально: отзеркаливаем по вертикали, так как в OpenGL координата Y идет снизу вверх
                row_pointers[height - 1 - i] = &image_data[i * rowbytes];
            }

            // Выполняем чтение
            png_read_image(png_ptr, row_pointers.data());

            // Закрываем файл и очищаем структуры libpng
            png_destroy_read_struct(&png_ptr, &info_ptr, nullptr);
            fclose(fp);

            // 6. Определяем OpenGL формат (GL_RGB или GL_RGBA)
            GLenum format = (color_type & PNG_COLOR_MASK_ALPHA) ? GL_RGBA : GL_RGB;

            // Выравнивание строк в OpenGL (для RGB данных без альфа-канала с нечетной шириной)
            glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

            // 7. Создаем OpenGL текстуру
            unsigned int textureID;
            glGenTextures(1, &textureID);
            glBindTexture(GL_TEXTURE_2D, textureID);

            glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, image_data.data());
            glGenerateMipmap(GL_TEXTURE_2D);

            // Настройка фильтрации и повторения
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

            std::cout << "Texture loaded successfully (via libpng): " << path << " (" << width << "x" << height << ")" << std::endl;

            return textureID;
        }
    }
}