#include "primitives.h"
#include <glm/glm.hpp>
#include <vector>

Mesh Primitives::createCube(float size)
{
    std::vector<Vertex> vertices;
    std::vector<unsigned int> indices;
    std::vector<Texture> textures;

    float half = size / 2.0f;

    Vertex verts[] = {
        // Передняя грань
        {{-half,-half, half},{0,0,1},{0,0}},
        {{ half,-half, half},{0,0,1},{1,0}},
        {{ half, half, half},{0,0,1},{1,1}},
        {{-half, half, half},{0,0,1},{0,1}},
        // Задняя грань
        {{ half,-half,-half},{0,0,-1},{0,0}},
        {{-half,-half,-half},{0,0,-1},{1,0}},
        {{-half, half,-half},{0,0,-1},{1,1}},
        {{ half, half,-half},{0,0,-1},{0,1}},
        // Верх
        {{-half, half, half},{0,1,0},{0,0}},
        {{ half, half, half},{0,1,0},{1,0}},
        {{ half, half,-half},{0,1,0},{1,1}},
        {{-half, half,-half},{0,1,0},{0,1}},
        // Низ
        {{-half,-half,-half},{0,-1,0},{0,0}},
        {{ half,-half,-half},{0,-1,0},{1,0}},
        {{ half,-half, half},{0,-1,0},{1,1}},
        {{-half,-half, half},{0,-1,0},{0,1}},
        // Правая
        {{ half,-half, half},{1,0,0},{0,0}},
        {{ half,-half,-half},{1,0,0},{1,0}},
        {{ half, half,-half},{1,0,0},{1,1}},
        {{ half, half, half},{1,0,0},{0,1}},
        // Левая
        {{-half,-half,-half},{-1,0,0},{0,0}},
        {{-half,-half, half},{-1,0,0},{1,0}},
        {{-half, half, half},{-1,0,0},{1,1}},
        {{-half, half,-half},{-1,0,0},{0,1}}
    };

    unsigned int inds[] = {
        0,1,2,2,3,0,       // перед
        4,5,6,6,7,4,       // зад
        8,9,10,10,11,8,    // верх
        12,13,14,14,15,12, // низ
        16,17,18,18,19,16, // правая
        20,21,22,22,23,20  // левая
    };

    vertices.assign(verts, verts + 24);
    indices.assign(inds, inds + 36);

    return Mesh(vertices, indices, textures); // позиция/масштаб через SceneObject
}

Mesh Primitives::createPlane(float width, float height)
{
    std::vector<Vertex> vertices;
    std::vector<unsigned int> indices;
    std::vector<Texture> textures;

    float hw = width / 2.0f;
    float hh = height / 2.0f;

    Vertex verts[] = {
        {{-hw,0,-hh},{0,1,0},{0,0}},
        {{ hw,0,-hh},{0,1,0},{1,0}},
        {{ hw,0, hh},{0,1,0},{1,1}},
        {{-hw,0, hh},{0,1,0},{0,1}}
    };

    unsigned int inds[] = { 0,1,2, 2,3,0 };

    vertices.assign(verts, verts + 4);
    indices.assign(inds, inds + 6);

    return Mesh(vertices, indices, textures);
}
