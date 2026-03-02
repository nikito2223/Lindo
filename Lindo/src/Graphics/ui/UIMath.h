#pragma once
#include <glm/glm.hpp>

struct Vec2 {
    float x, y;
    Vec2(float x_ = 0, float y_ = 0) : x(x_), y(y_) {}
};

struct Rect {
    float x, y, w, h;
    Rect(float x_ = 0, float y_ = 0, float w_ = 0, float h_ = 0) : x(x_), y(y_), w(w_), h(h_) {}
    bool contains(float px, float py) const {
        return px >= x && px <= x + w && py >= y && py <= y + h;
    }
};

struct Color {
    float r, g, b, a;
    Color(float r_ = 1, float g_ = 1, float b_ = 1, float a_ = 1) : r(r_), g(g_), b(b_), a(a_) {}
};