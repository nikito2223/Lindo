// MeshRenderer.cpp
#include "MeshRenderer.h"
#include <algorithm>
#include <limits>

void MeshRenderer::calculateMeshBoundingBox() {
    if (!mesh) return;

    float minX = std::numeric_limits<float>::max();
    float minY = std::numeric_limits<float>::max();
    float minZ = std::numeric_limits<float>::max();
    float maxX = std::numeric_limits<float>::lowest();
    float maxY = std::numeric_limits<float>::lowest();
    float maxZ = std::numeric_limits<float>::lowest();

    // ������� min/max ���������� ������
    for (const auto& vertex : mesh->vertices) {
        minX = std::min(minX, vertex.Position.x);
        minY = std::min(minY, vertex.Position.y);
        minZ = std::min(minZ, vertex.Position.z);
        maxX = std::max(maxX, vertex.Position.x);
        maxY = std::max(maxY, vertex.Position.y);
        maxZ = std::max(maxZ, vertex.Position.z);
    }

    bboxMin = glm::vec3(minX, minY, minZ);
    bboxMax = glm::vec3(maxX, maxY, maxZ);
    hasBBox = true;

    // ����� ��������� bounding sphere
    bsphereCenter = (bboxMin + bboxMax) * 0.5f;
    bsphereRadius = glm::length(bboxMax - bboxMin) * 0.5f;
    hasBSphere = true;

    std::cout << "Mesh BBox: min(" << minX << ", " << minY << ", " << minZ
        << "), max(" << maxX << ", " << maxY << ", " << maxZ
        << "), radius: " << bsphereRadius << std::endl;
}

void MeshRenderer::calculateModelBoundingBox() {
    if (!model) return;

    bboxMin = glm::vec3(-1.0f, -1.0f, -1.0f);
    bboxMax = glm::vec3(1.0f, 1.0f, 1.0f);
    hasBBox = true;

    bsphereCenter = glm::vec3(0.0f);
    bsphereRadius = 1.0f;
    hasBSphere = true;
}