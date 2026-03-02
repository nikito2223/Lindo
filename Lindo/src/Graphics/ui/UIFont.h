#pragma once
#include "UIRenderer.h"
#include <string>
#include <unordered_map>
#include <vector>

struct GlyphInfo {
    float x0, y0, x1, y1;  // координаты в атласе (нормализованные)
    float advance;          // смещение до следующего символа
    float width, height;    // размер глифа в пикселях
    float bearingX, bearingY; // смещение относительно базовой линии
};

class UIFont {
public:
    UIFont();
    ~UIFont();

    void saveAtlas(const std::string& filename) const;  // сохранить атлас в PPM
    void debugPrintGlyphs() const;                       // вывести информацию о глифах в консоль

    bool loadFromFile(const std::string& ttfPath, float fontSize,
        int atlasWidth, int atlasHeight,
        const std::string& charset = "");  // добавлен charset
    void getTextVertices(const std::string& text, float x, float y, const Color& color, std::vector<UIRenderer::Vertex>& outVertices) const;

    unsigned int getTextureID() const { return m_texture; }
    float getLineHeight() const { return m_lineHeight; }

    void getTextVerticesWithSize(const std::string& text, float x, float y,
        float fontSize, const Color& color,
        std::vector<UIRenderer::Vertex>& outVertices) const;

    // Вспомогательный метод для получения ширины текста с указанным размером
    float getStringWidthWithSize(const std::string& text, float fontSize) const;

    float getStringWidth(const std::string& text) const;
    float getAscent() const { return m_ascent; }
    float getDescent() const { return m_descent; }
    bool isLoaded() const { return !m_glyphs.empty(); }


private:
    unsigned int m_texture = 0;
    int m_atlasWidth, m_atlasHeight;
    float m_fontSize;
    float m_lineHeight;
    std::unordered_map<uint32_t, GlyphInfo> m_glyphs;
    void createTexture(const unsigned char* bitmap);
    std::vector<unsigned char> m_atlasBitmap;            // копия атласа
    float m_ascent = 0.0f;
    float m_descent = 0.0f;

};