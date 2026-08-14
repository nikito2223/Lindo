#include "UIFont.h"
#include <glad/glad.h>
#include <iostream>
#include <fstream>  // ОБЯЗАТЕЛЬНО для std::ofstream
#include <vector>
#include <algorithm>
#include <map>
#include <cstring>  // ОБЯЗАТЕЛЬНО для memcpy

// Подключаем FreeType
#include <ft2build.h>
#include FT_FREETYPE_H

namespace Lindo {
    namespace Graphics {
        namespace UI {

            static std::vector<uint32_t> decodeUTF8(const std::string& str) {
                std::vector<uint32_t> result;
                const char* ptr = str.data();
                const char* end = ptr + str.size();
                while (ptr < end) {
                    uint32_t cp = 0;
                    unsigned char lead = static_cast<unsigned char>(*ptr++);
                    if (lead < 0x80) cp = lead;
                    else if ((lead >> 5) == 0x6) cp = ((lead & 0x1F) << 6) | (static_cast<unsigned char>(*ptr++) & 0x3F);
                    else if ((lead >> 4) == 0xE) cp = ((lead & 0x0F) << 12) | ((static_cast<unsigned char>(*ptr++) & 0x3F) << 6) | (static_cast<unsigned char>(*ptr++) & 0x3F);
                    else if ((lead >> 3) == 0x1E) cp = ((lead & 0x07) << 18) | ((static_cast<unsigned char>(*ptr++) & 0x3F) << 12) | ((static_cast<unsigned char>(*ptr++) & 0x3F) << 6) | (static_cast<unsigned char>(*ptr++) & 0x3F);
                    result.push_back(cp);
                }
                return result;
            }

            UIFont::UIFont() : m_texture(0) {}

            UIFont::~UIFont() {
                if (m_texture) glDeleteTextures(1, &m_texture);
            }

            float UIFont::getStringWidth(const std::string& text) const {
                float width = 0.0f;
                std::vector<uint32_t> codepoints = decodeUTF8(text);
                for (uint32_t c : codepoints) {
                    auto it = m_glyphs.find(c);
                    if (it != m_glyphs.end()) {
                        width += it->second.advance;
                    }
                }
                return width;
            }

            bool UIFont::loadFromFile(const std::string& ttfPath, float fontSize, int atlasWidth, int atlasHeight, const std::string& charset) {
                FT_Library ft;
                if (FT_Init_FreeType(&ft)) {
                    std::cerr << "ERROR::FREETYPE: Could not init FreeType Library" << std::endl;
                    return false;
                }

                FT_Face face;
                if (FT_New_Face(ft, ttfPath.c_str(), 0, &face)) {
                    std::cerr << "ERROR::FREETYPE: Failed to load font: " << ttfPath << std::endl;
                    FT_Done_FreeType(ft);
                    return false;
                }

                // Устанавливаем размер шрифта
                FT_Set_Pixel_Sizes(face, 0, static_cast<FT_UInt>(fontSize));

                m_glyphs.clear();
                m_fontSize = fontSize;
                m_atlasWidth = atlasWidth;
                m_atlasHeight = atlasHeight;

                // Метрики шрифта
                m_ascent = face->size->metrics.ascender >> 6;
                m_descent = face->size->metrics.descender >> 6;
                m_lineHeight = face->size->metrics.height >> 6;

                // Подготовка буфера атласа (одноканальный GL_RED)
                std::vector<unsigned char> bitmap(atlasWidth * atlasHeight, 0);

                std::vector<uint32_t> codepoints = charset.empty() ?
                    std::vector<uint32_t>() : decodeUTF8(charset);

                if (charset.empty()) {
                    for (uint32_t i = 32; i < 128; ++i) codepoints.push_back(i);
                    // Добавим кириллицу по умолчанию, если нужно
                    for (uint32_t i = 0x0400; i <= 0x04FF; ++i) codepoints.push_back(i);
                }

                int curX = 1, curY = 1;
                int maxHeight = 0;

                // Отключаем ограничение выравнивания для работы с 1-байтными данными
                glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

                for (uint32_t cp : codepoints) {
                    if (FT_Load_Char(face, cp, FT_LOAD_RENDER)) {
                        continue;
                    }

                    FT_GlyphSlot g = face->glyph;
                    int w = g->bitmap.width;
                    int h = g->bitmap.rows;

                    // Переход на новую строку атласа
                    if (curX + w + 1 >= atlasWidth) {
                        curX = 1;
                        curY += maxHeight + 1;
                        maxHeight = 0;
                    }

                    if (curY + h + 1 >= atlasHeight) {
                        std::cerr << "Font Atlas full!" << std::endl;
                        break;
                    }

                    // Копируем битмап глифа в атлас
                    for (int row = 0; row < h; ++row) {
                        memcpy(&bitmap[(curY + row) * atlasWidth + curX],
                            &g->bitmap.buffer[row * w], w);
                    }

                    // Заполняем инфо о глифе
                    GlyphInfo info;
                    info.x0 = (float)curX / atlasWidth;
                    info.y0 = (float)curY / atlasHeight;
                    info.x1 = (float)(curX + w) / atlasWidth;
                    info.y1 = (float)(curY + h) / atlasHeight;
                    info.width = (float)w;
                    info.height = (float)h;
                    info.bearingX = (float)g->bitmap_left;
                    info.bearingY = (float)-g->bitmap_top; // Инвертируем для UI координат
                    info.advance = (float)(g->advance.x >> 6);

                    m_glyphs[cp] = info;

                    curX += w + 1;
                    maxHeight = std::max(maxHeight, h);
                }

                createTexture(bitmap.data());
                m_atlasBitmap = std::move(bitmap);

                FT_Done_Face(face);
                FT_Done_FreeType(ft);

                return true;
            }

            void UIFont::createTexture(const unsigned char* bitmap) {
                if (m_texture) glDeleteTextures(1, &m_texture);

                glGenTextures(1, &m_texture);
                glBindTexture(GL_TEXTURE_2D, m_texture);

                glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, m_atlasWidth, m_atlasHeight,
                    0, GL_RED, GL_UNSIGNED_BYTE, bitmap);

                // Обязательно Swizzle Mask, чтобы красный канал работал как Alpha
                // Это сделает шрифт белым с прозрачностью, а не красным на черном
                GLint swizzleMask[] = { GL_ONE, GL_ONE, GL_ONE, GL_RED };
                glTexParameteriv(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_RGBA, swizzleMask);

                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            }

            void UIFont::getTextVertices(const std::string& text, float x, float y,
                const Color& color,
                std::vector<UIRenderer::Vertex>& outVertices) const {
                float cursorX = x;
                float cursorY = y; // базовая линия

                auto codepoints = decodeUTF8(text);
                for (uint32_t c : codepoints) {
                    auto it = m_glyphs.find(c);
                    if (it == m_glyphs.end()) continue;

                    const GlyphInfo& g = it->second;

                    float charX = cursorX + g.bearingX;
                    float charY = cursorY + g.bearingY; // исправленный знак

                    Rect rect(charX, charY, static_cast<float>(g.width), static_cast<float>(g.height));

                    float texLeft = g.x0;
                    float texRight = g.x1;
                    float texTop = g.y0;    // верх глифа в атласе
                    float texBottom = g.y1; // низ глифа

                    UIRenderer::Vertex v0{ {rect.x, rect.y}, {texLeft,  texTop},    {color.r, color.g, color.b, color.a} };
                    UIRenderer::Vertex v1{ {rect.x + rect.w, rect.y}, {texRight, texTop},    {color.r, color.g, color.b, color.a} };
                    UIRenderer::Vertex v2{ {rect.x + rect.w, rect.y + rect.h}, {texRight, texBottom}, {color.r, color.g, color.b, color.a} };
                    UIRenderer::Vertex v3{ {rect.x, rect.y + rect.h}, {texLeft,  texBottom}, {color.r, color.g, color.b, color.a} };

                    outVertices.push_back(v0);
                    outVertices.push_back(v1);
                    outVertices.push_back(v2);
                    outVertices.push_back(v0);
                    outVertices.push_back(v2);
                    outVertices.push_back(v3);

                    cursorX += g.advance;
                }
            }

            void UIFont::saveAtlas(const std::string& filename) const {
                if (m_atlasBitmap.empty()) {
                    std::cerr << "Atlas bitmap is empty, cannot save." << std::endl;
                    return;
                }

                std::ofstream file(filename, std::ios::binary);
                if (!file) {
                    std::cerr << "Cannot open file for writing: " << filename << std::endl;
                    return;
                }

                file << "P6\n" << m_atlasWidth << " " << m_atlasHeight << "\n255\n";
                for (unsigned char pixel : m_atlasBitmap) {
                    file.put(pixel);
                    file.put(pixel);
                    file.put(pixel);
                }

                std::cout << "Atlas saved to " << filename << std::endl;
            }

            void UIFont::debugPrintGlyphs() const {
                std::cout << "Glyph atlas: " << m_atlasWidth << "x" << m_atlasHeight << "\n";
                std::cout << "Font size: " << m_fontSize << ", line height: " << m_lineHeight << "\n";
                for (const auto& pair : m_glyphs) {
                    std::cout << "Char code " << pair.first << ": "
                        << "pos=(" << pair.second.x0 << "," << pair.second.y0 << ")-("
                        << pair.second.x1 << "," << pair.second.y1 << ") "
                        << "size=" << pair.second.width << "x" << pair.second.height << " "
                        << "bearing=(" << pair.second.bearingX << "," << pair.second.bearingY << ") "
                        << "advance=" << pair.second.advance << "\n";
                }
            }

            float UIFont::getStringWidthWithSize(const std::string& text, float fontSize) const {
                float scale = fontSize / m_fontSize; // отношение желаемого размера к базовому
                float width = 0.0f;
                std::vector<uint32_t> codepoints = decodeUTF8(text);
                for (uint32_t c : codepoints) {
                    auto it = m_glyphs.find(c);
                    if (it != m_glyphs.end()) {
                        width += it->second.advance * scale;
                    }
                }
                return width;
            }

            void UIFont::getTextVerticesWithSize(const std::string& text, float x, float y,
                float fontSize, const Color& color,
                std::vector<UIRenderer::Vertex>& outVertices) const {
                float scale = fontSize / m_fontSize; // масштаб относительно базового размера шрифта
                float cursorX = x;
                float cursorY = y;

                auto codepoints = decodeUTF8(text);
                for (uint32_t c : codepoints) {
                    auto it = m_glyphs.find(c);
                    if (it == m_glyphs.end()) continue;

                    const GlyphInfo& g = it->second;

                    // Масштабируем все метрики
                    float scaledBearingX = g.bearingX * scale;
                    float scaledBearingY = g.bearingY * scale;
                    float scaledWidth = g.width * scale;
                    float scaledHeight = g.height * scale;
                    float scaledAdvance = g.advance * scale;

                    // Создаём вершины только для видимых символов
                    if (g.width > 0 && g.height > 0) {
                        float charX = cursorX + scaledBearingX;
                        float charY = cursorY + scaledBearingY;

                        Rect rect(charX, charY, scaledWidth, scaledHeight);

                        // Текстурные координаты не масштабируются - они всегда в диапазоне [0,1]
                        float texLeft = g.x0;
                        float texRight = g.x1;
                        float texTop = g.y0;
                        float texBottom = g.y1;

                        UIRenderer::Vertex v0{ {rect.x, rect.y}, {texLeft,  texTop},    {color.r, color.g, color.b, color.a} };
                        UIRenderer::Vertex v1{ {rect.x + rect.w, rect.y}, {texRight, texTop},    {color.r, color.g, color.b, color.a} };
                        UIRenderer::Vertex v2{ {rect.x + rect.w, rect.y + rect.h}, {texRight, texBottom}, {color.r, color.g, color.b, color.a} };
                        UIRenderer::Vertex v3{ {rect.x, rect.y + rect.h}, {texLeft,  texBottom}, {color.r, color.g, color.b, color.a} };

                        outVertices.push_back(v0);
                        outVertices.push_back(v1);
                        outVertices.push_back(v2);
                        outVertices.push_back(v0);
                        outVertices.push_back(v2);
                        outVertices.push_back(v3);
                    }

                    cursorX += scaledAdvance;
                }
            }
        }
    }
}