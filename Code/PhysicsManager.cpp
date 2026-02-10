#include "PhysicsManager.h"
#include "PhysicsComponent.h" 

#include "PrimeEngine/Scene/DebugRenderer.h"

#include<algorithm>
#include<cmath>

using namespace PE::Components;

namespace PE {

    PhysicsManager* PhysicsManager::s_instance = nullptr;
    void PhysicsManager::Initialize(PE::GameContext& context, PE::MemoryArena arena) {
        if (!s_instance)
            s_instance = new PhysicsManager(context, arena);
    }
    PhysicsManager* PhysicsManager::Instance() {
        assert(s_instance);
        return s_instance;
    }
    PhysicsManager::PhysicsManager(PE::GameContext& context, PE::MemoryArena arena)
        : m_components(context, arena, 25) {
    }

    void PhysicsManager::addComponent(PhysicsComponent* pComp)
    {
        // Avoid duplicates
        for (PrimitiveTypes::UInt32 i = 0; i < m_components.m_size; ++i) {
            if (m_components[i] == pComp)
                return;
        }
        char buf[256];
        sprintf(buf, "PhysicsComponent added: %p, isStatic: %d, owner: %p\n", pComp, pComp->m_isStatic, pComp->m_pOwner);
        OutputDebugStringA(buf);
        m_components.add(pComp);
    }

    void PhysicsManager::removeComponent(PhysicsComponent* pComp)
    {
        for (PrimitiveTypes::UInt32 i = 0; i < m_components.m_size; ++i) {
            if (m_components[i] == pComp) {
                m_components.remove(i);
                return;
            }
        }
    }

    // Helper: Clamp value between min and max
    static float clamp(float val, float minVal, float maxVal) {
        return std::max(minVal, std::min(val, maxVal));
    }


    inline float dot(const Vector3& a, const Vector3& b) {
        return a.m_x * b.m_x + a.m_y * b.m_y + a.m_z * b.m_z;
    }

    inline float length(const Vector3& v) {
        return std::sqrt(dot(v, v));
    }

    inline Vector3 normalized(const Vector3& v) {
        float len = length(v);
        if (len > 0.00001f)
            return Vector3(v.m_x / len, v.m_y / len, v.m_z / len);
        return Vector3(0, 0, 0);
    }

    void PhysicsManager::resolveCollision(PhysicsComponent* a, PhysicsComponent* b)
    {
        // Only handle sphere-sphere and sphere-box for simplicity
        if (a->m_shape.type == PhysicsComponent::Sphere) {
            Vector3 correction(0, 0, 0);
            if (b->m_shape.type == PhysicsComponent::Sphere) {
                Vector3 delta = a->m_shape.center - b->m_shape.center;
                float dist = delta.length();
                float minDist = a->m_shape.radius + b->m_shape.radius;
                if (dist < minDist && dist > 0.0001f) {
                    correction = normalized(delta) * (minDist - dist);
                    a->m_shape.center += correction;
					a->m_positionChangedByPhysics = true;
                    // Zero velocity along collision normal
                    Vector3 normal = normalized(delta);
                    float vDotN = dot(a->m_velocity,normal);
                    if (vDotN < 0) a->m_velocity -= normal * vDotN;
                }
            }
            else if (b->m_shape.type == PhysicsComponent::Box) {
                // Find closest point on box to sphere center
                float cx = clamp(a->m_shape.center.m_x, b->m_shape.boxMin.m_x, b->m_shape.boxMax.m_x);
                float cy = clamp(a->m_shape.center.m_y, b->m_shape.boxMin.m_y, b->m_shape.boxMax.m_y);
                float cz = clamp(a->m_shape.center.m_z, b->m_shape.boxMin.m_z, b->m_shape.boxMax.m_z);
                Vector3 closest(cx, cy, cz);
                Vector3 delta = a->m_shape.center - closest;
                float dist = delta.length();
                if (dist < a->m_shape.radius && dist > 0.0001f) {
                    correction = normalized(delta) * (a->m_shape.radius - dist);
                    a->m_shape.center += correction;
					a->m_positionChangedByPhysics = true;
                    // Zero velocity along collision normal
                    Vector3 normal = normalized(delta);
                    float vDotN = dot(a->m_velocity, normal);
                    if (vDotN < 0) a->m_velocity -= normal * vDotN;
                }
            }
            // After correction, sync to scene node
            a->syncToSceneNode();
        }
    }

    void PhysicsManager::update(float dt)
    {
        // 1. Apply gravity and integrate
        for (PrimitiveTypes::UInt32 i = 0; i < m_components.m_size; ++i) {
            PhysicsComponent* pComp = m_components[i];
            if (!pComp || pComp->m_isStatic) continue;
            //pComp->applyGravity(dt);
            pComp->integrate(dt);
            pComp->syncToSceneNode();

			//// For debug: draw shapes
   //         for (PrimitiveTypes::UInt32 i = 0; i < m_components.m_size; ++i) {
   //             PhysicsComponent* pComp = m_components[i];
   //             if (!pComp) continue;
   //             if (pComp->m_shape.type == PhysicsComponent::Box) {
   //                 int threadMask = 0; // Use appropriate thread mask if needed
   //                 DebugRenderer::Instance()->createBoundingBox(
   //                     pComp->m_shape.boxMin,
   //                     pComp->m_shape.boxMax,
   //                     threadMask
   //                 );
   //             }
   //             if (pComp->m_shape.type == PhysicsComponent::Sphere) {
   //                 Vector3 min = pComp->m_shape.center - Vector3(pComp->m_shape.radius, pComp->m_shape.radius, pComp->m_shape.radius);
   //                 Vector3 max = pComp->m_shape.center + Vector3(pComp->m_shape.radius, pComp->m_shape.radius, pComp->m_shape.radius);
   //                 int threadMask = 0;
   //                 DebugRenderer::Instance()->createBoundingBox(min, max, threadMask);
   //             }
   //         }
        }

        // 2. Collision detection and response
        for (PrimitiveTypes::UInt32 i = 0; i < m_components.m_size; ++i) {
            PhysicsComponent* a = m_components[i];
            if (!a || a->m_isStatic) continue;

            for (PrimitiveTypes::UInt32 j = 0; j < m_components.m_size; ++j) {
                if (i == j) continue;
                PhysicsComponent* b = m_components[j];
                if (!b) continue;
                // Only check dynamic vs static or dynamic vs dynamic (once)
                if (b->m_isStatic || j > i) {
                    if (checkCollision(a->m_shape, b->m_shape)) {
                        // Simple response: move a out of collision and zero velocity
                        resolveCollision(a, b);
                    }
                }
            }
            /*if (a->m_positionChangedByPhysics) {
                a->syncToSceneNode();
                a->m_positionChangedByPhysics = false;
            }*/
            a->syncToSceneNode();

        }
    }

    bool PhysicsManager::checkCollision(const Components::PhysicsComponent::Shape& a, const Components::PhysicsComponent::Shape& b)
    {
        // Sphere-Sphere
        if (a.type == PhysicsComponent::Sphere && b.type == PhysicsComponent::Sphere) {
            float distSq = (a.center - b.center).lengthSqr();
            float rSum = a.radius + b.radius;
            return distSq < rSum * rSum;
        }
        // Box-Box (AABB)
        if (a.type == PhysicsComponent::Box && b.type == PhysicsComponent::Box) {
            return (a.boxMin.m_x <= b.boxMax.m_x && a.boxMax.m_x >= b.boxMin.m_x) &&
                (a.boxMin.m_y <= b.boxMax.m_y && a.boxMax.m_y >= b.boxMin.m_y) &&
                (a.boxMin.m_z <= b.boxMax.m_z && a.boxMax.m_z >= b.boxMin.m_z);
        }
        // Sphere-Box
        const PhysicsComponent::Shape* sphere = nullptr;
        const PhysicsComponent::Shape* box = nullptr;
        if (a.type == PhysicsComponent::Sphere && b.type == PhysicsComponent::Box) {
            sphere = &a; box = &b;
        }
        else if (a.type == PhysicsComponent::Box && b.type == PhysicsComponent::Sphere) {
            sphere = &b; box = &a;
        }
        if (sphere && box) {
            // Find closest point on box to sphere center
            float cx = clamp(sphere->center.m_x, box->boxMin.m_x, box->boxMax.m_x);
            float cy = clamp(sphere->center.m_y, box->boxMin.m_y, box->boxMax.m_y);
            float cz = clamp(sphere->center.m_z, box->boxMin.m_z, box->boxMax.m_z);
            Vector3 closest(cx, cy, cz);
            float distSq = (sphere->center - closest).lengthSqr();
            return distSq < sphere->radius * sphere->radius;
        }
        return false;
    }

} // namespace PE