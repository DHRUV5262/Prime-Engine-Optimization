#include "PhysicsComponent.h"
#include "PrimeEngine/Scene/SceneNode.h"

namespace PE {
namespace Components {

    PhysicsComponent::PhysicsComponent(PE::GameContext& context, PE::MemoryArena arena, Handle hMyself)
        : Component(context, arena, hMyself)
        , m_velocity(0.0f, 0.0f, 0.0f)
        , m_isStatic(false)
        , m_pOwner(nullptr)
    {
        m_shape.type = Sphere;
        m_shape.center = Vector3(0.0f, 0.0f, 0.0f);
        m_shape.radius = 1.0f;
        m_shape.boxMin = Vector3(0.0f, 0.0f, 0.0f);
        m_shape.boxMax = Vector3(0.0f, 0.0f, 0.0f);
    }

    void PhysicsComponent::setSphere(const Vector3& center, float radius)
    {
        m_shape.type = Sphere;
        m_shape.center = center;
        m_shape.radius = radius;
    }

    void PhysicsComponent::setBox(const Vector3& min, const Vector3& max)
    {
        m_shape.type = Box;
        m_shape.boxMin = min;
        m_shape.boxMax = max;
    }

    void PhysicsComponent::setStatic(bool isStatic)
    {
        m_isStatic = isStatic;
    }

    void PhysicsComponent::setVelocity(const Vector3& v)
    {
        m_velocity = v;
    }

    void PhysicsComponent::integrate(float dt)
    {
        if (m_isStatic || !m_pOwner)
            return;
        
        if (!m_isStatic) {
            m_shape.center += m_velocity * dt;
            m_positionChangedByPhysics = true;
        }
    }

    void PhysicsComponent::applyGravity(float dt)
    {
        if (m_isStatic)
            return;

        // sample gravity
        m_velocity += Vector3(0.0f, -9.81f, 0.0f) * dt;
    }

    void PhysicsComponent::syncToSceneNode()
    {
        if (m_pOwner)
        {
            m_pOwner->m_base.setPos(m_shape.center);
        }
    }

    void PhysicsComponent::syncFromSceneNode()
    {
        if (m_pOwner)
        {
            m_shape.center = m_pOwner->m_base.getPos();
        }
    }

} // namespace Components
} // namespace PE
