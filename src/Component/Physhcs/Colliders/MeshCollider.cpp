#include "MeshCollider.h"
#include "BoxCollider.h"
#include "SphereCollider.h"
#include "CapsuleCollider.h"
#include <Physics/PhysicsSystem.h>
#include <Component/GameObject/GameObject.h>
#include <glm/gtc/matrix_transform.hpp>
#include <algorithm>
#include <limits>
#include <cmath>
#include "Graphics/core/DebugDraw.h"

namespace Lindo {
    namespace Components {
        namespace Physics {

            // ════════════════════════════════════════════════════════════
            //  Lifecycle
            // ════════════════════════════════════════════════════════════

            void MeshCollider::OnStart() {
                UpdateFromMeshRenderer();
                RebuildWorldCache();
                Collider::OnStart();
            }

            void MeshCollider::OnDestroy() {
                Collider::OnDestroy();
            }

            // ════════════════════════════════════════════════════════════
            //  Source data
            // ════════════════════════════════════════════════════════════

            void MeshCollider::SetMesh(const Lindo::Graphics::Mesh& mesh) {
                localVertices.clear();
                localVertices.reserve(mesh.vertices.size());
                for (const auto& v : mesh.vertices)
                    localVertices.push_back(v.Position);

                localIndices = mesh.indices;
                cacheDirty = true;
            }

            void MeshCollider::SetMeshData(
                const std::vector<Lindo::Graphics::Mesh::Vertex>& vertices,
                const std::vector<unsigned int>& indices)
            {
                localVertices.clear();
                localVertices.reserve(vertices.size());
                for (const auto& v : vertices)
                    localVertices.push_back(v.Position);

                localIndices = indices;
                cacheDirty = true;
            }

            void MeshCollider::UpdateFromMeshRenderer() {
                if (!gameObject) return;
            
                auto* mr = gameObject->getComponent<Lindo::Components::Physics::MeshRenderer>();
                if (!mr) return;
            
                localVertices.clear();
                localIndices.clear();
            
                // ─────────────────────────────────────────────
                // Одиночный Mesh
                // ─────────────────────────────────────────────
                if (mr->mesh) {
                    SetMesh(*mr->mesh);
                    return;
                }
            
                // ─────────────────────────────────────────────
                // Model, состоящий из нескольких Mesh
                // ─────────────────────────────────────────────
                if (mr->model) {
                    const auto& meshes = mr->model->getMeshes();
                
                    for (const auto& mesh : meshes) {
                        unsigned int base =
                            static_cast<unsigned int>(localVertices.size());
                    
                        for (const auto& vertex : mesh.vertices) {
                            localVertices.push_back(vertex.Position);
                        }
                    
                        for (unsigned int index : mesh.indices) {
                            localIndices.push_back(base + index);
                        }
                    }
                
                    cacheDirty = true;
                    return;
                }
            
                cacheDirty = true;
            }

            // ════════════════════════════════════════════════════════════
            //  World-space cache
            // ════════════════════════════════════════════════════════════

            void MeshCollider::RebuildWorldCache() {
                BuildWorldTriangles();
                BuildWorldAABB();
                cacheDirty = false;
            }

            void MeshCollider::BuildWorldTriangles() {
                worldTriangles.clear();

                if (localVertices.empty() || localIndices.size() < 3) return;

                // Build transform matrix: gameObject world transform + collider offset.
                glm::mat4 matrix(1.0f);
                if (gameObject) {
                    matrix = gameObject->transform.getMatrix();
                }
                // Apply offset as an additional translation.
                matrix = glm::translate(matrix, offset);

                worldTriangles.reserve(localIndices.size() / 3);

                for (size_t i = 0; i + 2 < localIndices.size(); i += 3) {
                    unsigned int i0 = localIndices[i];
                    unsigned int i1 = localIndices[i + 1];
                    unsigned int i2 = localIndices[i + 2];

                    if (i0 >= localVertices.size() ||
                        i1 >= localVertices.size() ||
                        i2 >= localVertices.size()) continue;

                    MeshTriangle tri;
                    tri.v0 = glm::vec3(matrix * glm::vec4(localVertices[i0], 1.0f));
                    tri.v1 = glm::vec3(matrix * glm::vec4(localVertices[i1], 1.0f));
                    tri.v2 = glm::vec3(matrix * glm::vec4(localVertices[i2], 1.0f));

                    glm::vec3 edge1 = tri.v1 - tri.v0;
                    glm::vec3 edge2 = tri.v2 - tri.v0;
                    glm::vec3 cross = glm::cross(edge1, edge2);
                    float len = glm::length(cross);
                    tri.normal = (len > 1e-8f) ? cross / len : glm::vec3(0, 1, 0);

                    worldTriangles.push_back(tri);
                }
            }

            void MeshCollider::BuildWorldAABB() {
                glm::vec3 minB(std::numeric_limits<float>::max());
                glm::vec3 maxB(std::numeric_limits<float>::lowest());

                for (const auto& tri : worldTriangles) {
                    minB = glm::min(minB, glm::min(tri.v0, glm::min(tri.v1, tri.v2)));
                    maxB = glm::max(maxB, glm::max(tri.v0, glm::max(tri.v1, tri.v2)));
                }

                if (worldTriangles.empty()) {
                    minB = maxB = GetWorldPosition();
                }

                worldAABB = Lindo::Math::AABB(minB, maxB);
            }

            // ════════════════════════════════════════════════════════════
            //  Collider interface
            // ════════════════════════════════════════════════════════════

            Lindo::Math::AABB MeshCollider::GetAABB() const {
                return worldAABB;
            }

            // Dispatch to the other collider's CheckCollision(MeshCollider*).
            // Since the base Collider interface doesn't have CheckCollision(MeshCollider*),
            // we implement narrow-phase here directly for each known type.
            bool MeshCollider::CheckCollision(Collider* other, CollisionInfo& outInfo) const {
                if (!other) return false;

                // Добавляем проверку на другой MeshCollider
                if (auto* mesh = dynamic_cast<MeshCollider*>(other))
                    return CheckCollision(mesh, outInfo);

                // Let the other collider handle it via double-dispatch.
                if (auto* box = dynamic_cast<BoxCollider*>(other))
                    return CheckCollision(box, outInfo);
                if (auto* sphere = dynamic_cast<SphereCollider*>(other))
                    return CheckCollision(sphere, outInfo);
                if (auto* capsule = dynamic_cast<CapsuleCollider*>(other))
                    return CheckCollision(capsule, outInfo);
                return false;
            }

            // ── Mesh vs Mesh ──────────────────────────────────────────────
            bool MeshCollider::CheckCollision(const MeshCollider* other, CollisionInfo& outInfo) const {
                if (!other) return false;
            
                // Broad-phase: AABB vs AABB
                if (!worldAABB.intersectAABB(other->GetAABB())) return false;
            
                float minDepth = std::numeric_limits<float>::max();
                glm::vec3 bestNormal(0.0f);
                glm::vec3 bestContact(0.0f);
                bool hit = false;
            
                CollisionInfo triInfo;

                // Narrow-phase: перебираем треугольники
                const auto& otherTriangles = other->GetWorldTriangles();
                for (const auto& triA : worldTriangles) {
                    for (const auto& triB : otherTriangles) {
                        // Простая оптимизация: отсечение по нормалям (если треугольники смотрят в одну сторону,
                        // они вряд ли столкнутся корректно, но для сложной геометрии это можно закомментировать)
                        if (glm::dot(triA.normal, triB.normal) > 0.0f) continue;
                    
                        if (TriVsTri(triA, triB, triInfo)) {
                            if (triInfo.penetrationDepth < minDepth) {
                                minDepth = triInfo.penetrationDepth;
                                bestNormal = triInfo.contactNormal;
                                bestContact = triInfo.contactPoint;
                            }
                            hit = true;
                        }
                    }
                }
            
                if (hit) {
                    outInfo.other = const_cast<MeshCollider*>(other);
                    outInfo.contactNormal = bestNormal;
                    outInfo.penetrationDepth = minDepth;
                    outInfo.contactPoint = bestContact;
                    outInfo.relativeVelocity = 0.0f;
                }
                return hit;
            }

            // ── Mesh vs Box ──────────────────────────────────────────────
            bool MeshCollider::CheckCollision(const BoxCollider* other, CollisionInfo& outInfo) const {
                if (!other) return false;

                // Broad-phase: AABB vs AABB.
                if (!worldAABB.intersectAABB(other->GetAABB())) return false;

                glm::vec3 boxCenter = other->GetWorldCenter();
                glm::vec3 boxHalf   = other->GetWorldHalfExtents();

                float     minDepth = std::numeric_limits<float>::max();
                glm::vec3 bestNormal(0.0f);
                glm::vec3 bestContact(0.0f);
                bool      hit = false;

                CollisionInfo triInfo;
                for (const auto& tri : worldTriangles) {
                    if (AABBVsTriangle(boxCenter, boxHalf, tri, triInfo)) {
                        if (triInfo.penetrationDepth < minDepth) {
                            minDepth    = triInfo.penetrationDepth;
                            bestNormal  = triInfo.contactNormal;
                            bestContact = triInfo.contactPoint;
                        }
                        hit = true;
                    }
                }

                if (hit) {
                    outInfo.other            = const_cast<BoxCollider*>(other);
                    outInfo.contactNormal    = bestNormal;
                    outInfo.penetrationDepth = minDepth;
                    outInfo.contactPoint     = bestContact;
                    outInfo.relativeVelocity = 0.0f;
                }
                return hit;
            }

            // ── Mesh vs Sphere ───────────────────────────────────────────
            bool MeshCollider::CheckCollision(const SphereCollider* other, CollisionInfo& outInfo) const {
                if (!other) return false;

                // Broad-phase.
                if (!worldAABB.intersectAABB(other->GetAABB())) return false;

                glm::vec3 sphereCenter = other->GetWorldCenter();
                float     sphereRadius = other->GetWorldRadius();

                float     minDepth = std::numeric_limits<float>::max();
                glm::vec3 bestNormal(0.0f);
                glm::vec3 bestContact(0.0f);
                bool      hit = false;

                CollisionInfo triInfo;
                for (const auto& tri : worldTriangles) {
                    if (SphereVsTriangle(sphereCenter, sphereRadius, tri, triInfo)) {
                        if (triInfo.penetrationDepth < minDepth) {
                            minDepth    = triInfo.penetrationDepth;
                            bestNormal  = triInfo.contactNormal;
                            bestContact = triInfo.contactPoint;
                        }
                        hit = true;
                    }
                }

                if (hit) {
                    outInfo.other            = const_cast<SphereCollider*>(other);
                    outInfo.contactNormal    = bestNormal;
                    outInfo.penetrationDepth = minDepth;
                    outInfo.contactPoint     = bestContact;
                    outInfo.relativeVelocity = 0.0f;
                }
                return hit;
            }

            // ── Mesh vs Capsule ──────────────────────────────────────────
            // A capsule = two spheres swept along a segment.
            // We test each triangle against both the segment and each cap sphere.
            bool MeshCollider::CheckCollision(const CapsuleCollider* other, CollisionInfo& outInfo) const {
                if (!other) return false;

                if (!worldAABB.intersectAABB(other->GetAABB())) return false;

                // Decompose capsule into top/bottom sphere centers + radius.
                glm::vec3 capTop;
                glm::vec3 capBottom;
                other->GetEndpoints(capTop, capBottom);

                float     radius    = other->GetWorldRadius();

                float     minDepth = std::numeric_limits<float>::max();
                glm::vec3 bestNormal(0.0f);
                glm::vec3 bestContact(0.0f);
                bool      hit = false;

                for (const auto& tri : worldTriangles) {
                    // Test the segment (capBottom -> capTop) against the triangle
                    // by finding the point on the segment closest to the triangle,
                    // then running a sphere test.
                    glm::vec3 closestOnTri, closestOnSeg;

                    // Closest point on segment to triangle (iterate: clamp point on
                    // segment to triangle then re-clamp back).
                    auto clampSeg = [&](const glm::vec3& p) -> glm::vec3 {
                        glm::vec3 d = capTop - capBottom;
                        float t = glm::dot(p - capBottom, d) / glm::dot(d, d);
                        t = glm::clamp(t, 0.0f, 1.0f);
                        return capBottom + t * d;
                    };

                    closestOnTri = ClosestPointOnTriangle(capBottom, tri.v0, tri.v1, tri.v2);
                    closestOnSeg = clampSeg(closestOnTri);
                    closestOnTri = ClosestPointOnTriangle(closestOnSeg, tri.v0, tri.v1, tri.v2);

                    glm::vec3 delta = closestOnSeg - closestOnTri;
                    float dist = glm::length(delta);

                    if (dist < radius) {
                        float depth = radius - dist;
                        glm::vec3 normal = (dist > 1e-8f)
                            ? delta / dist
                            : tri.normal;

                        if (depth < minDepth) {
                            minDepth    = depth;
                            bestNormal  = normal;
                            bestContact = closestOnTri;
                        }
                        hit = true;
                    }
                }

                if (hit) {
                    outInfo.other            = const_cast<CapsuleCollider*>(other);
                    outInfo.contactNormal    = bestNormal;
                    outInfo.penetrationDepth = minDepth;
                    outInfo.contactPoint     = bestContact;
                    outInfo.relativeVelocity = 0.0f;
                }
                return hit;
            }

            // ════════════════════════════════════════════════════════════
            //  Static geometry helpers
            // ════════════════════════════════════════════════════════════

            glm::vec3 MeshCollider::ClosestPointOnTriangle(
                const glm::vec3& p,
                const glm::vec3& a,
                const glm::vec3& b,
                const glm::vec3& c)
            {
                glm::vec3 ab = b - a;
                glm::vec3 ac = c - a;
                glm::vec3 ap = p - a;

                float d1 = glm::dot(ab, ap);
                float d2 = glm::dot(ac, ap);
                if (d1 <= 0.0f && d2 <= 0.0f) return a;

                glm::vec3 bp = p - b;
                float d3 = glm::dot(ab, bp);
                float d4 = glm::dot(ac, bp);
                if (d3 >= 0.0f && d4 <= d3) return b;

                float vc = d1 * d4 - d3 * d2;
                if (vc <= 0.0f && d1 >= 0.0f && d3 <= 0.0f) {
                    float v = d1 / (d1 - d3);
                    return a + v * ab;
                }

                glm::vec3 cp = p - c;
                float d5 = glm::dot(ab, cp);
                float d6 = glm::dot(ac, cp);
                if (d6 >= 0.0f && d5 <= d6) return c;

                float vb = d5 * d2 - d1 * d6;
                if (vb <= 0.0f && d2 >= 0.0f && d6 <= 0.0f) {
                    float w = d2 / (d2 - d6);
                    return a + w * ac;
                }

                float va = d3 * d6 - d5 * d4;
                if (va <= 0.0f && (d4 - d3) >= 0.0f && (d5 - d6) >= 0.0f) {
                    float w = (d4 - d3) / ((d4 - d3) + (d5 - d6));
                    return b + w * (c - b);
                }

                float denom = 1.0f / (va + vb + vc);
                float v = vb * denom;
                float w = vc * denom;
                return a + v * ab + w * ac;
            }

            bool MeshCollider::TriVsTri(const MeshTriangle& triA, 
                            const MeshTriangle& triB, 
                            CollisionInfo& outInfo) 
            {
                glm::vec3 edgesA[3] = {
                    triA.v1 - triA.v0,
                    triA.v2 - triA.v1,
                    triA.v0 - triA.v2
                };
                glm::vec3 edgesB[3] = {
                    triB.v1 - triB.v0,
                    triB.v2 - triB.v1,
                    triB.v0 - triB.v2
                };
            
                glm::vec3 axes[11];
                axes[0] = triA.normal;
                axes[1] = triB.normal;
                int axisCount = 2;
            
                for (int i = 0; i < 3; ++i) {
                    for (int j = 0; j < 3; ++j) {
                        glm::vec3 cross = glm::cross(edgesA[i], edgesB[j]);
                        float len = glm::length(cross);
                        if (len > 1e-6f) {
                            axes[axisCount++] = cross / len;
                        }
                    }
                }
            
                float minOverlap = std::numeric_limits<float>::max();
                glm::vec3 bestAxis(0.0f);
            
                for (int i = 0; i < axisCount; ++i) {
                    glm::vec3 axis = axes[i];
                    if (glm::length(axis) < 1e-6f) continue;
                    axis = glm::normalize(axis);
                
                    float pA0 = glm::dot(triA.v0, axis);
                    float pA1 = glm::dot(triA.v1, axis);
                    float pA2 = glm::dot(triA.v2, axis);
                    float minA = std::min({pA0, pA1, pA2});
                    float maxA = std::max({pA0, pA1, pA2});
                
                    float pB0 = glm::dot(triB.v0, axis);
                    float pB1 = glm::dot(triB.v1, axis);
                    float pB2 = glm::dot(triB.v2, axis);
                    float minB = std::min({pB0, pB1, pB2});
                    float maxB = std::max({pB0, pB1, pB2});
                
                    if (minA > maxB || minB > maxA) {
                        return false; // Найдена разделяющая ось, пересечения нет
                    }
                
                    float overlap = std::min(maxA - minB, maxB - minA);
                    if (overlap < minOverlap) {
                        minOverlap = overlap;
                        bestAxis = axis;
                    }
                }
            
                // Нормаль выталкивания должна указывать от B (земли) к A (проваливающемуся объекту)
                glm::vec3 centerA = (triA.v0 + triA.v1 + triA.v2) / 3.0f;
                glm::vec3 centerB = (triB.v0 + triB.v1 + triB.v2) / 3.0f;
                if (glm::dot(bestAxis, centerA - centerB) < 0.0f) {
                    bestAxis = -bestAxis;
                }
            
                outInfo.contactNormal = bestAxis;
                outInfo.penetrationDepth = minOverlap;
                // Аппроксимация точки контакта (для честной нужно делать полигональный клиппинг)
                outInfo.contactPoint = (centerA + centerB) * 0.5f; 
                return true;
            }

            bool MeshCollider::SphereVsTriangle(
                const glm::vec3& center,
                float             radius,
                const MeshTriangle& tri,
                CollisionInfo&    outInfo)
            {
                glm::vec3 closest = ClosestPointOnTriangle(center, tri.v0, tri.v1, tri.v2);
                glm::vec3 delta   = center - closest;
                float     dist    = glm::length(delta);

                if (dist >= radius) return false;

                float     depth  = radius - dist;
                glm::vec3 normal = (dist > 1e-8f) ? delta / dist : tri.normal;

                outInfo.contactPoint     = closest;
                outInfo.contactNormal    = normal;
                outInfo.penetrationDepth = depth;
                return true;
            }

            // SAT: AABB vs triangle — 13 potential separating axes.
            bool MeshCollider::AABBVsTriangle(
                const glm::vec3& boxCenter,
                const glm::vec3& half,
                const MeshTriangle& tri,
                CollisionInfo&    outInfo)
            {
                // Translate triangle to box-local space.
                glm::vec3 v0 = tri.v0 - boxCenter;
                glm::vec3 v1 = tri.v1 - boxCenter;
                glm::vec3 v2 = tri.v2 - boxCenter;

                glm::vec3 e0 = v1 - v0;
                glm::vec3 e1 = v2 - v1;
                glm::vec3 e2 = v0 - v2;

                // AABB face normals.
                const glm::vec3 boxAxes[3] = {
                    glm::vec3(1, 0, 0),
                    glm::vec3(0, 1, 0),
                    glm::vec3(0, 0, 1)
                };

                float     minOverlap = std::numeric_limits<float>::max();
                glm::vec3 bestAxis(0.0f);

                auto testAxis = [&](glm::vec3 axis) -> bool {
                    float axisLen = glm::length(axis);
                    if (axisLen < 1e-8f) return true; // degenerate, skip
                    axis /= axisLen;

                    float p0 = glm::dot(v0, axis);
                    float p1 = glm::dot(v1, axis);
                    float p2 = glm::dot(v2, axis);

                    float triMin = std::min({p0, p1, p2});
                    float triMax = std::max({p0, p1, p2});

                    float boxR = half.x * std::abs(axis.x) +
                                 half.y * std::abs(axis.y) +
                                 half.z * std::abs(axis.z);

                    if (triMin > boxR || triMax < -boxR) return false; // gap found

                    float overlap = std::min(boxR - triMin, triMax + boxR);
                    if (overlap < minOverlap) {
                        minOverlap = overlap;
                        bestAxis   = axis;
                    }
                    return true;
                };

                // 3 AABB face normals.
                for (const auto& ax : boxAxes)
                    if (!testAxis(ax)) return false;

                // 1 triangle face normal.
                if (!testAxis(tri.normal)) return false;

                // 9 edge cross products.
                for (const auto& triEdge : { e0, e1, e2 })
                    for (const auto& ax : boxAxes)
                        if (!testAxis(glm::cross(triEdge, ax))) return false;

                // Ensure normal points from triangle toward box center (away from mesh).
                if (glm::dot(bestAxis, boxCenter - tri.v0) < 0.0f)
                    bestAxis = -bestAxis;

                outInfo.contactNormal    = bestAxis;
                outInfo.penetrationDepth = minOverlap;
                outInfo.contactPoint     = (tri.v0 + tri.v1 + tri.v2) / 3.0f; // triangle centroid
                return true;
            }

            void MeshCollider::OnDrawGizmos() {
                if (!gameObject || !enabledGizmos) return;

                auto& debugDraw = Lindo::Graphics::DebugDraw::GetInstance();

                glm::mat4 world = gameObject->getWorldMatrix();
                glm::vec3 pos = GetWorldPosition();

                // 1. Отрисовка локальных осей координат
                glm::vec3 xAxis = glm::vec3(world[0]) * 0.5f;
                glm::vec3 yAxis = glm::vec3(world[1]) * 0.5f;
                glm::vec3 zAxis = glm::vec3(world[2]) * 0.5f;

                debugDraw.DrawLine(pos, pos + xAxis, glm::vec3(1.0f, 0.0f, 0.0f));
                debugDraw.DrawLine(pos, pos + yAxis, glm::vec3(0.0f, 1.0f, 0.0f));
                debugDraw.DrawLine(pos, pos + zAxis, glm::vec3(0.0f, 0.0f, 1.0f));

                // 2. Отрисовка всех треугольников меша
                const glm::vec3 meshColor(1.0f, 0.0f, 1.0f); // Пурпурный цвет
                const auto& triangles = GetWorldTriangles();

                for (const auto& tri : triangles) {
                    debugDraw.DrawLine(tri.v0, tri.v1, meshColor);
                    debugDraw.DrawLine(tri.v1, tri.v2, meshColor);
                    debugDraw.DrawLine(tri.v2, tri.v0, meshColor);
                }

                // 3. (Опционально) Отрисовка AABB для Broad-phase
                /*
                const auto aabb = GetAABB();
                glm::mat4 transform = glm::translate(glm::mat4(1.0f), (aabb.min + aabb.max) * 0.5f);
                transform = glm::scale(transform, aabb.max - aabb.min);
                debugDraw.DrawWireBox(transform, glm::vec3(0.3f, 0.0f, 0.3f));
                */
            }

        }
    }
}