#pragma once

#include "PrimeEngine/Events/Component.h"
#include "PrimeEngine/Scene/DefaultAnimationSM.h"
#include "CharacterControl/Characters/SoldierNPCBehaviorSM.h"
#include "CharacterControl/Characters/SoldierNPCAnimationSM.h"


namespace CharacterControl {
	namespace Components {

		struct PartialAnimationSM : public PE::Components::Component
		{
			PE_DECLARE_CLASS(PartialAnimationSM);

			PartialAnimationSM(PE::GameContext& context, PE::MemoryArena arena, PE::Handle hMyself);

			virtual void addDefaultComponents();

			PE_DECLARE_IMPLEMENT_EVENT_HANDLER_WRAPPER(do_UPDATE)
			virtual void do_UPDATE(PE::Events::Event* pEvt);

			// Animation control API (called from SoldierNPCBehaviorSM)
			void playPartial(PrimitiveTypes::UInt32 animSetIndex, PrimitiveTypes::UInt32 animIndex,
				PrimitiveTypes::Float32 weight = 1.0f, PrimitiveTypes::Bool looping = false);
			void stopPartial();
			void setPartialWeight(PrimitiveTypes::Float32 weight);
			bool isPartialActive() const { return m_isActive; }

			// Full-body blend API 
			void playFullBlend(PrimitiveTypes::UInt32 animSetIndex0, PrimitiveTypes::UInt32 animIndex0,
				PrimitiveTypes::UInt32 animSetIndex1, PrimitiveTypes::UInt32 animIndex1,
				PrimitiveTypes::Float32 weight0 = 0.5f, PrimitiveTypes::Float32 weight1 = 0.5f,
				PrimitiveTypes::Bool looping = false);
			void setFullBlendWeights(PrimitiveTypes::Float32 weight0, PrimitiveTypes::Float32 weight1);
			void stopFullBlend();

			// Additive animation API 
			void playAdditive(PrimitiveTypes::UInt32 animSetIndex, PrimitiveTypes::UInt32 animIndex,
				PrimitiveTypes::Float32 weight = 1.0f, PrimitiveTypes::Bool looping = false);
			void stopAdditive();
			void setAdditiveWeight(PrimitiveTypes::Float32 weight);
			bool isAdditiveActive() const { return m_isAdditiveActive; }

			// animation to play by default (animSetIndex, animIndex)
			PrimitiveTypes::UInt32 m_animSetIndex;
			PrimitiveTypes::UInt32 m_animIndex;
			// animation mode coming from spawn (0 = full blend, 1 = partial, 2 = additive)
			PrimitiveTypes::Int32 m_animationType;

			bool m_isActive; // whether the partial slot is currently active
			PrimitiveTypes::UInt32 m_firstActiveSlot;
			PrimitiveTypes::UInt32 m_lastActiveSlot;
			PrimitiveTypes::UInt32 m_firstFadingSlot;
			PrimitiveTypes::UInt32 m_lastFadingSlot;
			PrimitiveTypes::Float32 m_weight;

			// remember current active animation so stopPartial can disable the correct anim
			PrimitiveTypes::UInt32 m_activeAnimSetIndex;
			PrimitiveTypes::UInt32 m_activeAnimIndex;

			// Additive-specific members
			bool m_isAdditiveActive;
			PrimitiveTypes::UInt32 m_firstAdditiveActiveSlot;
			PrimitiveTypes::UInt32 m_lastAdditiveActiveSlot;
			PrimitiveTypes::UInt32 m_firstAdditiveFadingSlot;
			PrimitiveTypes::UInt32 m_lastAdditiveFadingSlot;
			PrimitiveTypes::Float32 m_additiveWeight;
			PrimitiveTypes::UInt32 m_activeAdditiveAnimSetIndex;
			PrimitiveTypes::UInt32 m_activeAdditiveAnimIndex;

			// Full-body blend (two separate slots) members (kept private)
			bool m_isFullBlendActive;
			PrimitiveTypes::UInt32 m_firstFullSlot; // slot index for first full-body anim (e.g. 0)
			PrimitiveTypes::UInt32 m_secondFullSlot; // slot index for second full-body anim (e.g. 1)
			PrimitiveTypes::UInt32 m_firstFullFadingSlot;
			PrimitiveTypes::UInt32 m_secondFullFadingSlot;
			PrimitiveTypes::Float32 m_fullBlendWeight0;
			PrimitiveTypes::Float32 m_fullBlendWeight1;
			PrimitiveTypes::UInt32 m_activeFullAnimSetIndex0;
			PrimitiveTypes::UInt32 m_activeFullAnimIndex0;
			PrimitiveTypes::UInt32 m_activeFullAnimSetIndex1;
			PrimitiveTypes::UInt32 m_activeFullAnimIndex1;
		};

	}; // namespace Components
}; // namespace CharacterControl