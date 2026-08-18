#include "skybox.h"
#include "core/AssetManager.h"
#include "debug/DebugLogger.h"
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <iostream>
#include <fstream>
#include <sstream>
#include <cmath>
#include <filesystem>
#include <png.h>

namespace Lindo {
    namespace Graphics {

        static float skyboxVertices[] = {
            -1,-1, 1,  1,-1, 1,  1, 1, 1,  1, 1, 1, -1, 1, 1, -1,-1, 1,
            -1,-1,-1, -1, 1,-1,  1, 1,-1,  1, 1,-1,  1,-1,-1, -1,-1,-1,
            -1, 1,-1, -1, 1, 1,  1, 1, 1,  1, 1, 1,  1, 1,-1, -1, 1,-1,
            -1,-1,-1,  1,-1,-1,  1,-1, 1,  1,-1, 1, -1,-1, 1, -1,-1,-1,
             1,-1,-1,  1, 1,-1,  1, 1, 1,  1, 1, 1,  1,-1, 1,  1,-1,-1,
            -1,-1,-1, -1,-1, 1, -1, 1, 1, -1, 1, 1, -1, 1,-1, -1,-1,-1
        };

        static glm::mat4 captureViews[] = {
            glm::lookAt(glm::vec3(0.0f), glm::vec3(1.0f,  0.0f,  0.0f), glm::vec3(0.0f, -1.0f,  0.0f)),
            glm::lookAt(glm::vec3(0.0f), glm::vec3(-1.0f,  0.0f,  0.0f), glm::vec3(0.0f, -1.0f,  0.0f)),
            glm::lookAt(glm::vec3(0.0f), glm::vec3(0.0f,  1.0f,  0.0f), glm::vec3(0.0f,  0.0f,  1.0f)),
            glm::lookAt(glm::vec3(0.0f), glm::vec3(0.0f, -1.0f,  0.0f), glm::vec3(0.0f,  0.0f, -1.0f)),
            glm::lookAt(glm::vec3(0.0f), glm::vec3(0.0f,  0.0f,  1.0f), glm::vec3(0.0f, -1.0f,  0.0f)),
            glm::lookAt(glm::vec3(0.0f), glm::vec3(0.0f,  0.0f, -1.0f), glm::vec3(0.0f, -1.0f,  0.0f))
        };

        static bool loadPNGFace(const std::string& path, int& width, int& height, int& channels, std::vector<unsigned char>& outData) {
            FILE* fp = fopen(path.c_str(), "rb");
            if (!fp) return false;

            png_byte header[8];
            if (fread(header, 1, 8, fp) != 8 || png_sig_cmp(header, 0, 8)) {
                fclose(fp);
                return false;
            }

            png_structp png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, nullptr, nullptr, nullptr);
            if (!png_ptr) { fclose(fp); return false; }

            png_infop info_ptr = png_create_info_struct(png_ptr);
            if (!info_ptr) { png_destroy_read_struct(&png_ptr, nullptr, nullptr); fclose(fp); return false; }

            if (setjmp(png_jmpbuf(png_ptr))) {
                png_destroy_read_struct(&png_ptr, &info_ptr, nullptr);
                fclose(fp);
                return false;
            }

            png_init_io(png_ptr, fp);
            png_set_sig_bytes(png_ptr, 8);
            png_read_info(png_ptr, info_ptr);

            width = png_get_image_width(png_ptr, info_ptr);
            height = png_get_image_height(png_ptr, info_ptr);
            png_byte color_type = png_get_color_type(png_ptr, info_ptr);
            png_byte bit_depth = png_get_bit_depth(png_ptr, info_ptr);

            if (color_type == PNG_COLOR_TYPE_PALETTE) png_set_palette_to_rgb(png_ptr);
            if (color_type == PNG_COLOR_TYPE_GRAY && bit_depth < 8) png_set_expand_gray_1_2_4_to_8(png_ptr);
            if (png_get_valid(png_ptr, info_ptr, PNG_INFO_tRNS)) png_set_tRNS_to_alpha(png_ptr);
            if (bit_depth == 16) png_set_strip_16(png_ptr);
            if (color_type == PNG_COLOR_TYPE_GRAY || color_type == PNG_COLOR_TYPE_GRAY_ALPHA) png_set_gray_to_rgb(png_ptr);

            png_read_update_info(png_ptr, info_ptr);

            channels = (color_type & PNG_COLOR_MASK_ALPHA) ? 4 : 3;
            size_t rowbytes = png_get_rowbytes(png_ptr, info_ptr);
            outData.resize(rowbytes * height);

            std::vector<png_bytep> row_pointers(height);
            for (int i = 0; i < height; ++i) {
                row_pointers[i] = &outData[i * rowbytes];
            }

            png_read_image(png_ptr, row_pointers.data());
            png_destroy_read_struct(&png_ptr, &info_ptr, nullptr);
            fclose(fp);
            return true;
        }

        static float* parseRGBEStream(std::istream& stream, int& width, int& height) {
            std::string line;
            bool headerOk = false;

            while (std::getline(stream, line)) {
                if (!line.empty() && line.back() == '\r') line.pop_back();
                if (line.empty()) break;
                if (line.find("#?RADIANCE") == 0 || line.find("#?RGBE") == 0) {
                    headerOk = true;
                }
            }

            if (!std::getline(stream, line)) return nullptr;
            if (!line.empty() && line.back() == '\r') line.pop_back();

            int w = 0, h = 0;
            if (sscanf(line.c_str(), "-Y %d +X %d", &h, &w) != 2 &&
                sscanf(line.c_str(), "+Y %d -X %d", &h, &w) != 2 &&
                sscanf(line.c_str(), "+Y %d +X %d", &h, &w) != 2 &&
                sscanf(line.c_str(), "-Y %d -X %d", &h, &w) != 2) {
                return nullptr;
            }

            width = w;
            height = h;
            float* pixels = new float[width * height * 3];

            for (int y = 0; y < height; ++y) {
                int r = stream.get();
                int g = stream.get();
                int b = stream.get();
                int e = stream.get();

                if (stream.fail()) {
                    delete[] pixels;
                    return nullptr;
                }

                std::vector<unsigned char> scanline(width * 4);

                if (r == 2 && g == 2 && (b & 128) == 0) {
                    int scanline_width = (b << 8) | e;
                    if (scanline_width != width) {
                        delete[] pixels;
                        return nullptr;
                    }

                    for (int i = 0; i < 4; ++i) {
                        int ptr = 0;
                        while (ptr < width) {
                            int count = stream.get();
                            if (count > 128) {
                                count &= 127;
                                int val = stream.get();
                                for (int j = 0; j < count; ++j) {
                                    scanline[ptr++ * 4 + i] = val;
                                }
                            }
                            else {
                                for (int j = 0; j < count; ++j) {
                                    scanline[ptr++ * 4 + i] = stream.get();
                                }
                            }
                        }
                    }
                }
                else {
                    stream.unget(); stream.unget(); stream.unget(); stream.unget();
                    stream.read(reinterpret_cast<char*>(scanline.data()), width * 4);
                }

                for (int x = 0; x < width; ++x) {
                    unsigned char rgbe[4] = {
                        scanline[x * 4 + 0], scanline[x * 4 + 1],
                        scanline[x * 4 + 2], scanline[x * 4 + 3]
                    };

                    int idx = (height - 1 - y) * width + x;
                    if (rgbe[3] > 0) {
                        float f = std::ldexp(1.0f, rgbe[3] - (128 + 8));
                        pixels[idx * 3 + 0] = rgbe[0] * f;
                        pixels[idx * 3 + 1] = rgbe[1] * f;
                        pixels[idx * 3 + 2] = rgbe[2] * f;
                    }
                    else {
                        pixels[idx * 3 + 0] = pixels[idx * 3 + 1] = pixels[idx * 3 + 2] = 0.0f;
                    }
                }
            }

            return pixels;
        }

        void Skybox::initShader() {
            auto& assets = AssetManager::get();
            std::string vs = assets.getShaderPath("skybox/skybox.vs");
            std::string fs = assets.getShaderPath("skybox/skybox.fs");

            if (!std::filesystem::exists(vs) || !std::filesystem::exists(fs)) {
                LOG_ERROR("Skybox shaders not found: " + vs + " | " + fs);
            }

            m_shader = std::make_unique<Shader>(vs.c_str(), fs.c_str());
            LOG_INFO("Skybox shader loaded internally.");
        }

        void Skybox::setupBuffers() {
            glGenVertexArrays(1, &VAO);
            glGenBuffers(1, &VBO);

            glBindVertexArray(VAO);
            glBindBuffer(GL_ARRAY_BUFFER, VBO);
            glBufferData(GL_ARRAY_BUFFER, sizeof(skyboxVertices), skyboxVertices, GL_STATIC_DRAW);

            glEnableVertexAttribArray(0);
            glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
            glBindVertexArray(0);
        }

        Skybox::~Skybox() {
            if (VAO) glDeleteVertexArrays(1, &VAO);
            if (VBO) glDeleteBuffers(1, &VBO);
            if (cubemapTexture) glDeleteTextures(1, &cubemapTexture);
        }

        Skybox::Skybox(std::vector<std::string> faces) : isHDR(false) {
            setupBuffers();
            initShader();

            glGenTextures(1, &cubemapTexture);
            glBindTexture(GL_TEXTURE_CUBE_MAP, cubemapTexture);
            glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

            auto& assets = AssetManager::get();
            for (unsigned int i = 0; i < faces.size(); i++) {
                int w, h, ch;
                std::vector<unsigned char> data;
                std::string resolvedPath = assets.resolvePath(faces[i], "textures");

                if (loadPNGFace(resolvedPath, w, h, ch, data)) {
                    GLenum format = (ch == 4) ? GL_RGBA : GL_RGB;
                    glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGB, w, h, 0, format, GL_UNSIGNED_BYTE, data.data());
                }
            }

            glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
        }

        unsigned int Skybox::generateCubemapFromEquirectangular(unsigned int hdrTexture, unsigned int resolution) {
            unsigned int captureFBO, captureRBO;
            glGenFramebuffers(1, &captureFBO);
            glGenRenderbuffers(1, &captureRBO);

            glm::mat4 captureProjection = glm::perspective(glm::radians(90.0f), 1.0f, 0.1f, 10.0f);

            auto& assets = AssetManager::get();
            std::string vsPath = assets.getShaderPath("equirectangular_to_cubemap.vs");
            std::string fsPath = assets.getShaderPath("equirectangular_to_cubemap.fs");

            if (!std::filesystem::exists(vsPath) || !std::filesystem::exists(fsPath)) {
                vsPath = assets.getShaderPath("skybox/equirectangular_to_cubemap.vs");
                fsPath = assets.getShaderPath("skybox/equirectangular_to_cubemap.fs");
            }

            Shader equirectangularToCubemapShader(vsPath.c_str(), fsPath.c_str());

            unsigned int cubemap;
            glGenTextures(1, &cubemap);
            glBindTexture(GL_TEXTURE_CUBE_MAP, cubemap);
            for (unsigned int i = 0; i < 6; ++i) {
                glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGB16F,
                    resolution, resolution, 0, GL_RGB, GL_FLOAT, nullptr);
            }
            glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

            glBindFramebuffer(GL_FRAMEBUFFER, captureFBO);
            glBindRenderbuffer(GL_RENDERBUFFER, captureRBO);
            glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, resolution, resolution);

            equirectangularToCubemapShader.use();
            equirectangularToCubemapShader.setInt("equirectangularMap", 0);
            equirectangularToCubemapShader.setMat4("projection", captureProjection);
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, hdrTexture);

            glViewport(0, 0, resolution, resolution);
            for (unsigned int i = 0; i < 6; ++i) {
                equirectangularToCubemapShader.setMat4("view", captureViews[i]);
                glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                    GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, cubemap, 0);
                glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

                glBindVertexArray(VAO);
                glDrawArrays(GL_TRIANGLES, 0, 36);
            }
            glBindFramebuffer(GL_FRAMEBUFFER, 0);

            glBindTexture(GL_TEXTURE_CUBE_MAP, cubemap);
            glGenerateMipmap(GL_TEXTURE_CUBE_MAP);
            glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);

            glDeleteFramebuffers(1, &captureFBO);
            glDeleteRenderbuffers(1, &captureRBO);

            return cubemap;
        }

        Skybox::Skybox(const std::string& hdrFile, unsigned int resolution)
            : isHDR(true), hdrResolution(resolution) {
            setupBuffers();
            initShader();

            std::string resolvedHdrPath = AssetManager::get().resolvePath(hdrFile, "textures");
            unsigned int hdrTexture = loadHDRTexture(resolvedHdrPath);
            if (hdrTexture == 0) throw std::runtime_error("Failed to load HDR texture: " + resolvedHdrPath);

            cubemapTexture = generateCubemapFromEquirectangular(hdrTexture, resolution);
            glDeleteTextures(1, &hdrTexture);
        }

        Skybox* Skybox::CreateFromHDRData(const std::vector<char>& data, unsigned int resolution) {
            Skybox* skybox = new Skybox();
            skybox->isHDR = true;
            skybox->hdrResolution = resolution;
            skybox->setupBuffers();
            skybox->initShader();

            unsigned int hdrTexture = loadHDRTextureFromData(data);
            if (hdrTexture == 0) {
                delete skybox;
                return nullptr;
            }

            skybox->cubemapTexture = skybox->generateCubemapFromEquirectangular(hdrTexture, resolution);
            glDeleteTextures(1, &hdrTexture);
            return skybox;
        }

        unsigned int Skybox::loadHDRTexture(const std::string& path) {
            std::ifstream file(path, std::ios::binary);
            if (!file.is_open()) return 0;

            int width = 0, height = 0;
            float* data = parseRGBEStream(file, width, height);
            if (!data) return 0;

            unsigned int hdrTexture = 0;
            glGenTextures(1, &hdrTexture);
            glBindTexture(GL_TEXTURE_2D, hdrTexture);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F, width, height, 0, GL_RGB, GL_FLOAT, data);

            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

            delete[] data;
            return hdrTexture;
        }

        unsigned int Skybox::loadHDRTextureFromData(const std::vector<char>& data) {
            std::string strData(data.begin(), data.end());
            std::istringstream stream(strData);

            int width = 0, height = 0;
            float* pixels = parseRGBEStream(stream, width, height);
            if (!pixels) return 0;

            unsigned int hdrTexture = 0;
            glGenTextures(1, &hdrTexture);
            glBindTexture(GL_TEXTURE_2D, hdrTexture);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F, width, height, 0, GL_RGB, GL_FLOAT, pixels);

            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

            delete[] pixels;
            return hdrTexture;
        }

        void Skybox::Draw(const glm::mat4& viewMatrix, const glm::mat4& projectionMatrix, float time, const glm::vec3& cameraPos) {
            if (!m_shader) return;

            glDepthFunc(GL_LEQUAL);
            glDepthMask(GL_FALSE);
            glDisable(GL_CULL_FACE);

            m_shader->use();

            // Убираем смещение из матрицы вида, оставляя только вращение
            glm::mat4 skyboxView = glm::mat4(glm::mat3(viewMatrix));
            skyboxView = glm::rotate(skyboxView, time * 0.015f, glm::vec3(0.0f, 1.0f, 0.0f));

            m_shader->setMat4("view", skyboxView);
            m_shader->setMat4("projection", projectionMatrix);
            m_shader->setFloat("u_time", time);
            m_shader->setVec3("u_cameraPos", cameraPos);

            glBindVertexArray(VAO);
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_CUBE_MAP, cubemapTexture);
            glDrawArrays(GL_TRIANGLES, 0, 36);
            glBindVertexArray(0);

            glDepthMask(GL_TRUE);
            glEnable(GL_CULL_FACE);
            glDepthFunc(GL_LESS);
        }
    }
}