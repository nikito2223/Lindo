#pragma once

#include <glm/glm.hpp>
#include <cmath>
#include <string>

namespace Lindo::Math {

    // ==========================================
    // Vector2
    // ==========================================
    struct Vector2 {
        float x = 0.0f;
        float y = 0.0f;

        Vector2() = default;
        Vector2(float fill) : x(fill), y(fill) {}
        Vector2(float x, float y) : x(x), y(y) {}
        Vector2(const glm::vec2& v) : x(v.x), y(v.y) {}

        operator glm::vec2() const { return glm::vec2(x, y); }

        Vector2 operator+(const Vector2& rhs) const { return Vector2(x + rhs.x, y + rhs.y); }
        Vector2 operator-(const Vector2& rhs) const { return Vector2(x - rhs.x, y - rhs.y); }
        Vector2 operator*(float scalar) const { return Vector2(x * scalar, y * scalar); }
        Vector2 operator/(float scalar) const { return Vector2(x / scalar, y / scalar); }
        Vector2 operator-() const { return Vector2(-x, -y); }

        float length() const { return std::sqrt(x * x + y * y); }
        Vector2 normalized() const {
            float len = length();
            return len > 0.00001f ? *this / len : Vector2(0.0f);
        }
        float dot(const Vector2& rhs) const { return x * rhs.x + y * rhs.y; }

        static float distance(const Vector2& a, const Vector2& b) { return (a - b).length(); }
    };

    // ==========================================
    // Vector3
    // ==========================================
    struct Vector3 {
        float x = 0.0f;
        float y = 0.0f;
        float z = 0.0f;

        Vector3() = default;
        Vector3(float fill) : x(fill), y(fill), z(fill) {}
        Vector3(float x, float y, float z) : x(x), y(y), z(z) {}
        Vector3(const glm::vec3& v) : x(v.x), y(v.y), z(v.z) {}

        operator glm::vec3() const { return glm::vec3(x, y, z); }

        Vector3 operator+(const Vector3& rhs) const { return Vector3(x + rhs.x, y + rhs.y, z + rhs.z); }
        Vector3 operator-(const Vector3& rhs) const { return Vector3(x - rhs.x, y - rhs.y, z - rhs.z); }
        Vector3 operator*(float scalar) const { return Vector3(x * scalar, y * scalar, z * scalar); }
        Vector3 operator/(float scalar) const { return Vector3(x / scalar, y / scalar, z / scalar); }
        Vector3 operator-() const { return Vector3(-x, -y, -z); }

        float length() const { return std::sqrt(x * x + y * y + z * z); }
        Vector3 normalized() const {
            float len = length();
            return len > 0.00001f ? *this / len : Vector3(0.0f);
        }
        float dot(const Vector3& rhs) const { return x * rhs.x + y * rhs.y + z * rhs.z; }
        Vector3 cross(const Vector3& rhs) const {
            return Vector3(
                y * rhs.z - z * rhs.y,
                z * rhs.x - x * rhs.z,
                x * rhs.y - y * rhs.x
            );
        }

        static float distance(const Vector3& a, const Vector3& b) { return (a - b).length(); }

        // Удобные константы
        static Vector3 zero()    { return Vector3(0.0f, 0.0f, 0.0f); }
        static Vector3 one()     { return Vector3(1.0f, 1.0f, 1.0f); }
        static Vector3 up()      { return Vector3(0.0f, 1.0f, 0.0f); }
        static Vector3 down()    { return Vector3(0.0f, -1.0f, 0.0f); }
        static Vector3 forward() { return Vector3(0.0f, 0.0f, 1.0f); }
        static Vector3 back()    { return Vector3(0.0f, 0.0f, -1.0f); }
        static Vector3 right()   { return Vector3(1.0f, 0.0f, 0.0f); }
        static Vector3 left()    { return Vector3(-1.0f, 0.0f, 0.0f); }
    };

    // ==========================================
    // Vector4
    // ==========================================
    struct Vector4 {
        float x = 0.0f;
        float y = 0.0f;
        float z = 0.0f;
        float w = 0.0f;

        Vector4() = default;
        Vector4(float fill) : x(fill), y(fill), z(fill), w(fill) {}
        Vector4(float x, float y, float z, float w) : x(x), y(y), z(z), w(w) {}
        Vector4(const glm::vec4& v) : x(v.x), y(v.y), z(v.z), w(v.w) {}

        operator glm::vec4() const { return glm::vec4(x, y, z, w); }

        Vector4 operator+(const Vector4& rhs) const { return Vector4(x + rhs.x, y + rhs.y, z + rhs.z, w + rhs.w); }
        Vector4 operator-(const Vector4& rhs) const { return Vector4(x - rhs.x, y - rhs.y, z - rhs.z, w - rhs.w); }
        Vector4 operator*(float scalar) const { return Vector4(x * scalar, y * scalar, z * scalar, w * scalar); }
        Vector4 operator/(float scalar) const { return Vector4(x / scalar, y / scalar, z / scalar, w / scalar); }
        Vector4 operator-() const { return Vector4(-x, -y, -z, -w); }

        float length() const { return std::sqrt(x * x + y * y + z * z + w * w); }
        Vector4 normalized() const {
            float len = length();
            return len > 0.00001f ? *this / len : Vector4(0.0f);
        }
        float dot(const Vector4& rhs) const { return x * rhs.x + y * rhs.y + z * rhs.z + w * rhs.w; }
    };

} // namespace Lindo::Math