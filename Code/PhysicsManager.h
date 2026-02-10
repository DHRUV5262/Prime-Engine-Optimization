// PhysicsManager.h
#pragma once
#include "PrimeEngine/Game/Common/GameContext.h" // For PE::GameContext
#include "PrimeEngine/Utils/Array/Array.h"       // For Array
#include "PhysicsComponent.h"                    // For PhysicsComponent

namespace PE {
    class PhysicsManager
    {
    public:
        static void Initialize(PE::GameContext& context, PE::MemoryArena arena);
        static PhysicsManager* Instance();

        void addComponent(PE::Components::PhysicsComponent* pComp);
        void removeComponent(PE::Components::PhysicsComponent* pComp);
        void update(float dt);

        bool checkCollision(const PE::Components::PhysicsComponent::Shape& a, const PE::Components::PhysicsComponent::Shape& b);
        void resolveCollision(PE::Components::PhysicsComponent* a, PE::Components::PhysicsComponent* b);

    private:
        static PhysicsManager* s_instance;
        PhysicsManager(PE::GameContext& context, PE::MemoryArena arena);
        Array<PE::Components::PhysicsComponent*> m_components;
    };
}