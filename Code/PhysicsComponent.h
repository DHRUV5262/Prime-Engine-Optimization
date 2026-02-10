#pragma once
#include "PrimeEngine/Events/Component.h"
#include "PrimeEngine/PrimitiveTypes/PrimitiveTypes.h"
#include "PrimeEngine/Math/Vector3.h"
#include "PrimeEngine/Scene/SceneNode.h"

namespace PE {
namespace Components {

    class PhysicsComponent : public Component
    {
    public:
        enum ShapeType { Sphere, Box };

        struct Shape {
            ShapeType type;
            Vector3 center;
            float radius;      // for sphere
            Vector3 boxMin;    // for box
            Vector3 boxMax;    // for box
        };

        PhysicsComponent(PE::GameContext& context, PE::MemoryArena arena, Handle hMyself);

        void setSphere(const Vector3& center, float radius);
        void setBox(const Vector3& min, const Vector3& max);
        void setStatic(bool isStatic);
        void setVelocity(const Vector3& v);

        // Called by PhysicsManager
        void integrate(float dt);
        void applyGravity(float dt);
        void syncToSceneNode();
        void syncFromSceneNode();

        // Members
        Shape m_shape;
        Vector3 m_velocity;
        bool m_isStatic;
        SceneNode* m_pOwner;
        bool m_positionChangedByPhysics = false;
    };

} // namespace Components
} // namespace PE
