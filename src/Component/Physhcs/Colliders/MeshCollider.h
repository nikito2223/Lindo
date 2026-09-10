#pragma once

#include "Collider.h"
#include <Physics/Math/AABB.h>
#include <Graphics/core/mesh.h>
#include <Component/Physhcs/MeshRenderer.h>
#include <glm/glm.hpp>
#include <vector>

namespace Lindo {
    namespace Components {
        namespace Physics {

            // Triangle in world space (cached after transform)
            struct MeshTriangle {
                glm::vec3 v0, v1, v2;
                glm::vec3 normal; // face normal, precomputed
            };

            // MeshCollider stores the actual mesh triangles and performs
            // per-triangle narrow-phase collision detection.
            // It does NOT inherit BoxCollider — it IS its own collider type.
            class MeshCollider : public Collider {
            public:
                MeshCollider() = default;
                virtual ~MeshCollider() = default;

                void OnStart()   override;
                void OnDestroy() override;

                // ── Source data ──────────────────────────────────────────

                // Populate triangles from a Graphics::Mesh directly.
                void SetMesh(const Lindo::Graphics::Mesh& mesh);

                // Populate triangles from a raw vertex/index list
                // (matches the Mesh::Vertex layout — only Position is used).
                void SetMeshData(const std::vector<Lindo::Graphics::Mesh::Vertex>& vertices,
                                 const std::vector<unsigned int>& indices);

                // Pull mesh data automatically from MeshRenderer on the same GameObject.
                void UpdateFromMeshRenderer();

                // Rebuild the world-space triangle cache and AABB.
                // Call this whenever the GameObject transform changes.
                void RebuildWorldCache();

                // ── Accessors ────────────────────────────────────────────

                const std::vector<MeshTriangle>& GetWorldTriangles() const { return worldTriangles; }
                size_t GetTriangleCount() const { return worldTriangles.size(); }

                bool IsConvex() const  { return convex; }
                void SetConvex(bool c) { convex = c; }

                // ── Collider interface ───────────────────────────────────

                Lindo::Math::AABB GetAABB() const override;

                bool CheckCollision(Collider*              other, CollisionInfo& outInfo) const override;
                bool CheckCollision(const BoxCollider*     other, CollisionInfo& outInfo) const override;
                bool CheckCollision(const SphereCollider*  other, CollisionInfo& outInfo) const override;
                bool CheckCollision(const CapsuleCollider* other, CollisionInfo& outInfo) const override;
                bool CheckCollision(const MeshCollider*    other, CollisionInfo& outInfo) const override;

                void OnDrawGizmos() override;

            private:
                // ── Local-space mesh data ────────────────────────────────
                std::vector<glm::vec3>    localVertices;
                std::vector<unsigned int> localIndices;

                // ── World-space cache (rebuilt on RebuildWorldCache) ─────
                std::vector<MeshTriangle> worldTriangles;
                Lindo::Math::AABB         worldAABB;
                bool                      cacheDirty = true;
                
                static bool TriVsTri(const MeshTriangle& triA,
                     const MeshTriangle& triB,
                     CollisionInfo& outInfo);

                // When true the mesh is treated as convex (cheaper queries).
                bool convex = false;

                // ── Helpers ──────────────────────────────────────────────

                void BuildWorldTriangles();
                void BuildWorldAABB();

                // Closest point on a triangle to a given point.
                static glm::vec3 ClosestPointOnTriangle(const glm::vec3& p,
                                                        const glm::vec3& a,
                                                        const glm::vec3& b,
                                                        const glm::vec3& c);

                // Narrow-phase: sphere vs single triangle.
                static bool SphereVsTriangle(const glm::vec3& sphereCenter,
                                             float             sphereRadius,
                                             const MeshTriangle& tri,
                                             CollisionInfo&    outInfo);

                // Narrow-phase: AABB vs single triangle (SAT, 13 axes).
                static bool AABBVsTriangle(const glm::vec3& boxCenter,
                                           const glm::vec3& boxHalfExtents,
                                           const MeshTriangle& tri,
                                           CollisionInfo&    outInfo);
            };

        }
    }
}