#pragma once
#include <vector>
#include "mesh.h"

class Primitives {
public:
    // Создание куба
    static Mesh createCube(float size = 1.0f);

    // Создание плоскости
    static Mesh createPlane(float width = 5.0f, float height = 5.0f);

    // Создание сферы (упрощенная)
    static Mesh createSphere(float radius = 1.0f, int sectors = 36, int stacks = 18);

    // Создание цилиндра
    static Mesh createCylinder(float radius = 0.5f, float height = 2.0f,
        int segments = 32);

private:
    // Вспомогательные функции
    static void addTextureCoordinates(std::vector<Vertex>& vertices,
        const std::vector<Texture>& textures);
};