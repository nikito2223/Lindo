#include "UIFont.h"
#define STB_TRUETYPE_IMPLEMENTATION
#include "stb_truetype.h"
#include <glad/glad.h>
#include <iostream>
#include <fstream>
#include <vector>
#include <cstdint>
#include <algorithm>
#include <cstring> // для memcpy
#include <unordered_set>
#include <iomanip>  // для std::hex, std::dec
// Вспомогательная функция декодирования UTF-8 (оставляем как есть)
static std::vector<uint32_t> decodeUTF8(const std::string& str) {
    std::vector<uint32_t> result;
    const char* ptr = str.data();
    const char* end = ptr + str.size();
    while (ptr < end) {
        uint32_t cp = 0;
        unsigned char lead = static_cast<unsigned char>(*ptr++);
        if (lead < 0x80) {
            cp = lead;
        }
        else if ((lead >> 5) == 0x6) {
            if (ptr >= end) break;
            cp = ((lead & 0x1F) << 6) | (static_cast<unsigned char>(*ptr++) & 0x3F);
        }
        else if ((lead >> 4) == 0xE) {
            if (ptr + 1 >= end) break;
            cp = ((lead & 0x0F) << 12) | ((static_cast<unsigned char>(*ptr++) & 0x3F) << 6) | (static_cast<unsigned char>(*ptr++) & 0x3F);
        }
        else if ((lead >> 3) == 0x1E) {
            if (ptr + 2 >= end) break;
            cp = ((lead & 0x07) << 18) | ((static_cast<unsigned char>(*ptr++) & 0x3F) << 12) | ((static_cast<unsigned char>(*ptr++) & 0x3F) << 6) | (static_cast<unsigned char>(*ptr++) & 0x3F);
        }
        result.push_back(cp);
    }
    return result;
}

UIFont::UIFont() {}

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

bool UIFont::loadFromFile(const std::string& ttfPath, float fontSize,
    int atlasWidth, int atlasHeight,
    const std::string& charset) {
    m_glyphs.clear();
    m_atlasBitmap.clear();

    m_fontSize = fontSize;
    m_atlasWidth = atlasWidth;
    m_atlasHeight = atlasHeight;

    std::cout << "Loading font from: " << ttfPath << " with " << charset.size() << " chars in charset" << std::endl;

    // Открываем файл шрифта
    FILE* file = nullptr;
    if (fopen_s(&file, ttfPath.c_str(), "rb") != 0) {
        std::cerr << "Failed to open font file: " << ttfPath << std::endl;
        return false;
    }
    fseek(file, 0, SEEK_END);
    long size = ftell(file);
    fseek(file, 0, SEEK_SET);
    std::vector<unsigned char> ttfBuffer(size);
    fread(ttfBuffer.data(), 1, size, file);
    fclose(file);

    // Инициализация stb_truetype
    stbtt_fontinfo info;
    if (!stbtt_InitFont(&info, ttfBuffer.data(), 0)) {
        std::cerr << "Failed to init font" << std::endl;
        return false;
    }

    // Подготовка атласа
    std::vector<unsigned char> bitmap(atlasWidth * atlasHeight, 0);
    float scale = stbtt_ScaleForPixelHeight(&info, fontSize);
    int ascent, descent, lineGap;
    stbtt_GetFontVMetrics(&info, &ascent, &descent, &lineGap);
    m_lineHeight = (ascent - descent + lineGap) * scale;
    m_ascent = ascent * scale;
    m_descent = descent * scale;

    int spaceAdvance, spaceLeftBearing;
    stbtt_GetCodepointHMetrics(&info, 32, &spaceAdvance, &spaceLeftBearing);

    GlyphInfo spaceGlyph;
    spaceGlyph.x0 = 0;
    spaceGlyph.y0 = 0;
    spaceGlyph.x1 = 0;
    spaceGlyph.y1 = 0;
    spaceGlyph.width = 0;
    spaceGlyph.height = 0;
    spaceGlyph.bearingX = 0;
    spaceGlyph.bearingY = 0;
    spaceGlyph.advance = spaceAdvance * scale;

    m_glyphs[32] = spaceGlyph;
    std::cout << "Space glyph added with advance: " << spaceGlyph.advance << std::endl;

    // Определяем список символов для загрузки
    std::vector<uint32_t> codepoints;
    if (charset.empty()) {
        for (int c = 32; c < 128; ++c)
            codepoints.push_back(c);
    }
    else {
        codepoints = decodeUTF8(charset);
        std::sort(codepoints.begin(), codepoints.end());
        codepoints.erase(std::unique(codepoints.begin(), codepoints.end()), codepoints.end());
    }

    // Размещение глифов в атласе
// Размещение глифов в атласе
    int x = 0, y = 0, maxHeight = 0;
    int loadedCount = 0;
    int missingCount = 0;

    for (uint32_t c : codepoints) {

       

        // Проверяем, есть ли глиф в шрифте
        int glyphIndex = stbtt_FindGlyphIndex(&info, c);
        if (glyphIndex == 0) {
            // Глиф отсутствует в шрифте - пропускаем
            missingCount++;

            // Для отладки выводим первые 10 отсутствующих символов
            static std::unordered_set<uint32_t> reportedMissing;
            if (reportedMissing.size() < 10 && reportedMissing.find(c) == reportedMissing.end()) {
                std::cout << "Glyph not found for code: " << c
                    << " (0x" << std::hex << c << std::dec << ")" << std::endl;
                reportedMissing.insert(c);
            }
            continue;
        }

        // Получаем горизонтальные метрики
        int advance, leftBearing;
        stbtt_GetCodepointHMetrics(&info, c, &advance, &leftBearing);

        // Получаем битмап глифа
        int width, height, xoff, yoff;
        unsigned char* glyphBitmap = stbtt_GetCodepointBitmap(&info, scale, scale, c, &width, &height, &xoff, &yoff);

        if (!glyphBitmap) {
            missingCount++;
            continue;
        }

        // Размещение в атласе - перенос строки если не влезает
        if (x + width > atlasWidth) {
            x = 0;
            y += maxHeight;
            maxHeight = 0;

            // Проверка на переполнение атласа по вертикали
            if (y + height > atlasHeight) {
                std::cerr << "Atlas overflow! Not enough space for all glyphs." << std::endl;
                stbtt_FreeBitmap(glyphBitmap, nullptr);
                break;
            }
        }

        // Копируем пиксели в атлас
        for (int row = 0; row < height; ++row) {
            memcpy(bitmap.data() + (y + row) * atlasWidth + x,
                glyphBitmap + row * width, width);
        }

        // Сохраняем метаданные глифа
        GlyphInfo g;
        g.x0 = (float)x / atlasWidth;
        g.y0 = (float)y / atlasHeight;
        g.x1 = (float)(x + width) / atlasWidth;
        g.y1 = (float)(y + height) / atlasHeight;
        g.width = width;
        g.height = height;
        g.bearingX = xoff;
        g.bearingY = yoff;
        g.advance = advance * scale;

        m_glyphs[c] = g;
        loadedCount++;


        // Обновляем позицию для следующего глифа
        x += width;
        maxHeight = std::max(maxHeight, height);

        // Освобождаем память
        stbtt_FreeBitmap(glyphBitmap, nullptr);
    }

    // Выводим статистику загрузки
    std::cout << "Font loading stats:" << std::endl;
    std::cout << "  Total requested: " << codepoints.size() << std::endl;
    std::cout << "  Loaded: " << loadedCount << std::endl;
    std::cout << "  Missing: " << missingCount << std::endl;

    // Проверяем, загрузились ли русские буквы
    int cyrillicLoaded = 0;
    for (const auto& pair : m_glyphs) {
        if (pair.first >= 0x0400 && pair.first <= 0x04FF) {
            cyrillicLoaded++;
        }
    }
    std::cout << "  Cyrillic glyphs loaded: " << cyrillicLoaded << std::endl;

    // Создаём текстуру OpenGL
    createTexture(bitmap.data());

    // Сохраняем копию для отладки
    m_atlasBitmap = std::move(bitmap);

    std::cout << "Font loaded: " << ttfPath << ", size: " << fontSize
        << ", glyphs: " << m_glyphs.size() << std::endl;
    return true;
}

void UIFont::createTexture(const unsigned char* bitmap) {
    glGenTextures(1, &m_texture);
    glBindTexture(GL_TEXTURE_2D, m_texture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, m_atlasWidth, m_atlasHeight, 0, GL_RED, GL_UNSIGNED_BYTE, bitmap);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
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