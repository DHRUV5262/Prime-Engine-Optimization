#include "PartialAnimation.h"

#include "CharacterControl/Characters/SoldierNPC.h"
#include "CharacterControl/Characters/SoldierNPCBehaviorSM.h"
#include "CharacterControl/Characters/SoldierNPCAnimationSM.h"


#include "PrimeEngine/Scene/SkeletonInstance.h"
#include "PrimeEngine/Events/StandardEvents.h"
#include "PrimeEngine/Lua/LuaEnvironment.h" // <--- ADD THIS INCLUDE


using namespace PE::Components;
using namespace PE::Events;

namespace CharacterControl {
	namespace Components {

		PE_IMPLEMENT_CLASS1(PartialAnimationSM, Component);

		PartialAnimationSM::PartialAnimationSM(PE::GameContext& context, PE::MemoryArena arena, PE::Handle hMyself)
			: Component(context, arena, hMyself)
			, m_animSetIndex(0)
			, m_animIndex(0)
			, m_animationType(0)
			, m_isActive(false)
			, m_firstActiveSlot(2) // reserved for partials
			, m_lastActiveSlot(2)
			, m_firstFadingSlot(6)
			, m_lastFadingSlot(6)
			, m_weight(1.0f)
			, m_activeAnimSetIndex(UINT32_MAX)
			, m_activeAnimIndex(UINT32_MAX)
			, m_isAdditiveActive(false)
			, m_firstAdditiveActiveSlot(3) // reserve a separate slot for additive
			, m_lastAdditiveActiveSlot(3)
			, m_firstAdditiveFadingSlot(7)
			, m_lastAdditiveFadingSlot(7)
			, m_additiveWeight(1.0f)
			, m_activeAdditiveAnimSetIndex(UINT32_MAX)
			, m_activeAdditiveAnimIndex(UINT32_MAX)
			, m_isFullBlendActive(false)
			, m_firstFullSlot(0)
			, m_secondFullSlot(1)
			, m_firstFullFadingSlot(4)
			, m_secondFullFadingSlot(5)
			, m_fullBlendWeight0(0.5f)
			, m_fullBlendWeight1(0.5f)
			, m_activeFullAnimSetIndex0(UINT32_MAX)
			, m_activeFullAnimIndex0(UINT32_MAX)
			, m_activeFullAnimSetIndex1(UINT32_MAX)
			, m_activeFullAnimIndex1(UINT32_MAX)
		{
		}

		void PartialAnimationSM::addDefaultComponents()
		{
			Component::addDefaultComponents();
			PE_REGISTER_EVENT_HANDLER(Event_UPDATE, PartialAnimationSM::do_UPDATE);
		}

		void PartialAnimationSM::do_UPDATE(PE::Events::Event* pEvt)
		{
			(void)pEvt;
		}

#ifndef PARTIAL_BODY_ANIMATION
#define PARTIAL_BODY_ANIMATION 0x01
#endif

void PartialAnimationSM::playPartial(PrimitiveTypes::UInt32 animSetIndex, PrimitiveTypes::UInt32 animIndex,
	PrimitiveTypes::Float32 weight, PrimitiveTypes::Bool looping)
{
	char dbg[256];
	sprintf(dbg, "PartialAnimationSM::playPartial called - animSet=%u anim=%u weight=%.2f type=%d\n",
		(unsigned)animSetIndex, (unsigned)animIndex, (double)weight, (int)m_animationType);
	OutputDebugStringA(dbg);

	OutputDebugStringA("PartialAnimationSM::playPartial\n");

	// Find the DefaultAnimationSM instance (prefer SkeletonInstance child)
	DefaultAnimationSM* pAnimSM = nullptr;

	PE::Handle hSkelInst = getFirstParentByType<SkeletonInstance>();
	if (hSkelInst.isValid())
	{
		pAnimSM = hSkelInst.getObject<SkeletonInstance>()->getFirstComponent<DefaultAnimationSM>();
	}

	if (!pAnimSM)
	{
		SoldierNPC* pSol = getFirstParentByTypePtr<SoldierNPC>();
		if (pSol)
		{
			SkeletonInstance* pSkelInstChild = NULL;
			int idxSkel = -1;
			while (pSol->getFirstComponentIP<SkeletonInstance>(idxSkel + 1, idxSkel, pSkelInstChild))
			{
				if (pSkelInstChild)
				{
					pAnimSM = pSkelInstChild->getFirstComponent<DefaultAnimationSM>();
					if (pAnimSM)
						break;
				}
			}
		}
	}

	if (!pAnimSM)
	{
		SoldierNPC* pSol2 = getFirstParentByTypePtr<SoldierNPC>();
		if (pSol2)
		{
			pAnimSM = pSol2->getFirstComponent<SoldierNPCAnimationSM>();
			if (!pAnimSM)
				pAnimSM = pSol2->getFirstComponent<DefaultAnimationSM>();
		}
	}

	if (!pAnimSM) return;

	PrimitiveTypes::UInt32 flags = PARTIAL_BODY_ANIMATION;
	if (looping) flags |= PE::LOOPING;

	if (m_isActive && m_activeAnimSetIndex == animSetIndex && m_activeAnimIndex == animIndex)
	{
		setPartialWeight(weight);
		return;
	}

	auto pSlot = pAnimSM->setAnimation(animSetIndex, animIndex,
		m_firstActiveSlot, m_lastActiveSlot,
		m_firstFadingSlot, m_lastFadingSlot,
		flags, weight);

	if (pSlot)
	{
		// Partial Joint Range - keep your discovered values here
		PrimitiveTypes::UInt32 upperStartJoint = 7;
		PrimitiveTypes::UInt32 upperEndJoint   = 30;

		pSlot->m_startJoint = upperStartJoint;
		pSlot->m_endJoint   = upperEndJoint;

		pSlot->m_flags |= PARTIAL_BODY_ANIMATION;

		char dbg[128];
		sprintf(dbg, "DEBUG_FORCE_PARTIAL: slot set anim %u forced joints %u..%u flags 0x%x\n",
			(unsigned)pSlot->m_animationIndex, (unsigned)upperStartJoint, (unsigned)upperEndJoint, (unsigned)pSlot->m_flags);
		OutputDebugStringA(dbg);
	}

	m_isActive = true;
	m_weight = weight;
	m_activeAnimSetIndex = animSetIndex;
	m_activeAnimIndex = animIndex;

	OutputDebugStringA("PartialAnimationSM::playPartial done\n");
}

void PartialAnimationSM::stopPartial()
{
	if (!m_isActive)
		return;

	DefaultAnimationSM* pAnimSM = nullptr;
	PE::Handle hSkelInst = getFirstParentByType<SkeletonInstance>();
	if (hSkelInst.isValid())
	{
		pAnimSM = hSkelInst.getObject<SkeletonInstance>()->getFirstComponent<DefaultAnimationSM>();
	}
	if (!pAnimSM)
	{
		SoldierNPC* pSol = getFirstParentByTypePtr<SoldierNPC>();
		if (!pSol) return;
		pAnimSM = pSol->getFirstComponent<SoldierNPCAnimationSM>();
		if (!pAnimSM)
			pAnimSM = pSol->getFirstComponent<DefaultAnimationSM>();
	}
	if (!pAnimSM) return;

	pAnimSM->disableAnimation(m_activeAnimSetIndex, m_activeAnimIndex, m_firstActiveSlot, m_lastActiveSlot);

	m_isActive = false;
	m_activeAnimSetIndex = UINT32_MAX;
	m_activeAnimIndex = UINT32_MAX;
	m_weight = 0.0f;
}

void PartialAnimationSM::setPartialWeight(PrimitiveTypes::Float32 weight)
{
	char dbg[128];
	sprintf(dbg, "PartialAnimationSM::setPartialWeight called - weight=%.2f active=%d\n", (double)weight, (int)m_isActive);
	OutputDebugStringA(dbg);

	if (!m_isActive)
	{
		m_weight = weight; // store for next play
		return;
	}

	DefaultAnimationSM* pAnimSM = nullptr;
	PE::Handle hSkelInst = getFirstParentByType<SkeletonInstance>();
	if (hSkelInst.isValid())
		pAnimSM = hSkelInst.getObject<SkeletonInstance>()->getFirstComponent<DefaultAnimationSM>();

	if (!pAnimSM)
	{
		SoldierNPC* pSol = getFirstParentByTypePtr<SoldierNPC>();
		if (!pSol) return;
		pAnimSM = pSol->getFirstComponent<SoldierNPCAnimationSM>();
		if (!pAnimSM)
			pAnimSM = pSol->getFirstComponent<DefaultAnimationSM>();
	}
	if (!pAnimSM) return;

	pAnimSM->setAnimationWeight(m_activeAnimSetIndex, m_activeAnimIndex, m_firstActiveSlot, m_lastActiveSlot, weight);
	m_weight = weight;
}


		// ----------------------
		// Full-body blend API
		// ----------------------

void PartialAnimationSM::playFullBlend(PrimitiveTypes::UInt32 animSetIndex0, PrimitiveTypes::UInt32 animIndex0,
	PrimitiveTypes::UInt32 animSetIndex1, PrimitiveTypes::UInt32 animIndex1,
	PrimitiveTypes::Float32 weight0, PrimitiveTypes::Float32 weight1,
	PrimitiveTypes::Bool looping)
{
	// debug
	char dbg[256];
	sprintf(dbg, "PartialAnimationSM::playFullBlend called - a0=%u:%u a1=%u:%u w0=%.2f w1=%.2f type=%d\n",
		(unsigned)animSetIndex0, (unsigned)animIndex0,
		(unsigned)animSetIndex1, (unsigned)animIndex1,
		(double)weight0, (double)weight1, (int)m_animationType);
	OutputDebugStringA(dbg);

	// Find DefaultAnimationSM 
	DefaultAnimationSM* pAnimSM = nullptr;
	PE::Handle hSkelInst = getFirstParentByType<SkeletonInstance>();
	if (hSkelInst.isValid())
		pAnimSM = hSkelInst.getObject<SkeletonInstance>()->getFirstComponent<DefaultAnimationSM>();

	if (!pAnimSM)
	{
		SoldierNPC* pSol = getFirstParentByTypePtr<SoldierNPC>();
		if (pSol)
		{
			// try SoldierNPC's own components first
			pAnimSM = pSol->getFirstComponent<SoldierNPCAnimationSM>();
			if (!pAnimSM)
				pAnimSM = pSol->getFirstComponent<DefaultAnimationSM>();

			// fallback: search skeleton children
			if (!pAnimSM)
			{
				SkeletonInstance* pSkelInstChild = NULL;
				int idxSkel = -1;
				while (pSol->getFirstComponentIP<SkeletonInstance>(idxSkel + 1, idxSkel, pSkelInstChild))
				{
					if (pSkelInstChild)
					{
						pAnimSM = pSkelInstChild->getFirstComponent<DefaultAnimationSM>();
						if (pAnimSM)
							break;
					}
				}
			}
		}
	}

	if (!pAnimSM)
	{
		OutputDebugStringA("PartialAnimationSM::playFullBlend: No DefaultAnimationSM found, aborting full blend\n");
		return;
	}

	// set looping flag if requested
	PrimitiveTypes::UInt32 flags = 0;
	if (looping) flags |= PE::LOOPING;

	// compute normalized alpha for setAnimations helper (alpha is weight for second animation)
	float denom = weight0 + weight1;
	float alpha = 0.5f;
	if (denom > 1e-6f)
		alpha = weight1 / denom;
	else
	{
		// if both zero-ish, default to 50/50
		alpha = 0.5f;
	}

	pAnimSM->disableAnimations(m_firstFullSlot, m_secondFullSlot);

	// Use DefaultAnimationSM::setAnimations to place two animations into the full-body slot range.
	// This treats the two tracks as a single blended group.
	pAnimSM->setAnimations(
		animSetIndex0, animIndex0,
		animSetIndex1, animIndex1,
		alpha,
		m_firstFullSlot, m_secondFullSlot,
		m_firstFullFadingSlot, m_secondFullFadingSlot,
		flags);

	
	// setAnimationWeight searches the active slots for matching animation and updates weight.
	pAnimSM->setAnimationWeight(animSetIndex0, animIndex0, m_firstFullSlot, m_secondFullSlot, weight0);
	pAnimSM->setAnimationWeight(animSetIndex1, animIndex1, m_firstFullSlot, m_secondFullSlot, weight1);

	// store active info (for later weight changes / stop)
	m_isFullBlendActive = true;
	m_fullBlendWeight0 = weight0;
	m_fullBlendWeight1 = weight1;
	m_activeFullAnimSetIndex0 = animSetIndex0;
	m_activeFullAnimIndex0 = animIndex0;
	m_activeFullAnimSetIndex1 = animSetIndex1;
	m_activeFullAnimIndex1 = animIndex1;

	// debug: list active slots as observed in this component (DefaultAnimationSM already prints active slots each update)
	sprintf(dbg, "PartialAnimationSM::playFullBlend: requested full-blend alpha=%.3f -> slots %u..%u (fading %u..%u)\n",
		(double)alpha, (unsigned)m_firstFullSlot, (unsigned)m_secondFullSlot, (unsigned)m_firstFullFadingSlot, (unsigned)m_secondFullFadingSlot);
	OutputDebugStringA(dbg);
}

void PartialAnimationSM::setFullBlendWeights(PrimitiveTypes::Float32 weight0, PrimitiveTypes::Float32 weight1)
{
	char dbg[128];
	sprintf(dbg, "PartialAnimationSM::setFullBlendWeights called - w0=%.2f w1=%.2f active=%d\n", (double)weight0, (double)weight1, (int)m_isFullBlendActive);
	OutputDebugStringA(dbg);

	if (!m_isFullBlendActive)
	{
		m_fullBlendWeight0 = weight0;
		m_fullBlendWeight1 = weight1;
		return;
	}

	DefaultAnimationSM* pAnimSM = nullptr;
	PE::Handle hSkelInst = getFirstParentByType<SkeletonInstance>();
	if (hSkelInst.isValid())
		pAnimSM = hSkelInst.getObject<SkeletonInstance>()->getFirstComponent<DefaultAnimationSM>();
	if (!pAnimSM)
	{
		SoldierNPC* pSol = getFirstParentByTypePtr<SoldierNPC>();
		if (!pSol) return;
		pAnimSM = pSol->getFirstComponent<SoldierNPCAnimationSM>();
		if (!pAnimSM)
		 pAnimSM = pSol->getFirstComponent<DefaultAnimationSM>();
	}
	if (!pAnimSM) return;

	pAnimSM->setAnimationWeight(m_activeFullAnimSetIndex0, m_activeFullAnimIndex0, m_firstFullSlot, m_firstFullSlot, weight0);
	pAnimSM->setAnimationWeight(m_activeFullAnimSetIndex1, m_activeFullAnimIndex1, m_secondFullSlot, m_secondFullSlot, weight1);
	m_fullBlendWeight0 = weight0;
	m_fullBlendWeight1 = weight1;
}

void PartialAnimationSM::stopFullBlend()
{
	if (!m_isFullBlendActive) return;

	DefaultAnimationSM* pAnimSM = nullptr;
	PE::Handle hSkelInst = getFirstParentByType<SkeletonInstance>();
	if (hSkelInst.isValid())
		pAnimSM = hSkelInst.getObject<SkeletonInstance>()->getFirstComponent<DefaultAnimationSM>();
	if (!pAnimSM)
	{
		SoldierNPC* pSol = getFirstParentByTypePtr<SoldierNPC>();
		if (!pSol) return;
		pAnimSM = pSol->getFirstComponent<SoldierNPCAnimationSM>();
		if (!pAnimSM)
		 pAnimSM = pSol->getFirstComponent<DefaultAnimationSM>();
	}
	if (!pAnimSM) return;

	pAnimSM->disableAnimation(m_activeFullAnimSetIndex0, m_activeFullAnimIndex0, m_firstFullSlot, m_firstFullSlot);
	pAnimSM->disableAnimation(m_activeFullAnimSetIndex1, m_activeFullAnimIndex1, m_secondFullSlot, m_secondFullSlot);

	m_isFullBlendActive = false;
	m_activeFullAnimSetIndex0 = m_activeFullAnimIndex0 = UINT32_MAX;
	m_activeFullAnimSetIndex1 = m_activeFullAnimIndex1 = UINT32_MAX;
	m_fullBlendWeight0 = m_fullBlendWeight1 = 0.0f;
}

// ----------------------
// Additive API
// ----------------------

#ifndef ADDITIVE_ANIMATION
#define ADDITIVE_ANIMATION 0x040
#endif

void PartialAnimationSM::playAdditive(PrimitiveTypes::UInt32 animSetIndex, PrimitiveTypes::UInt32 animIndex,
	PrimitiveTypes::Float32 weight, PrimitiveTypes::Bool looping)
{
	char dbg[256];
	sprintf(dbg, "PartialAnimationSM::playAdditive called - animSet=%u anim=%u weight=%.2f type=%d\n",
		(unsigned)animSetIndex, (unsigned)animIndex, (double)weight, (int)m_animationType);
	OutputDebugStringA(dbg);

	OutputDebugStringA("PartialAnimationSM::playAdditive\n");

	// Find DefaultAnimationSM (same lookup)
	DefaultAnimationSM* pAnimSM = nullptr;
	PE::Handle hSkelInst = getFirstParentByType<SkeletonInstance>();
	if (hSkelInst.isValid())
	{
		pAnimSM = hSkelInst.getObject<SkeletonInstance>()->getFirstComponent<DefaultAnimationSM>();
	}
	if (!pAnimSM)
	{
		SoldierNPC* pSol = getFirstParentByTypePtr<SoldierNPC>();
		if (pSol)
		{
			SkeletonInstance* pSkelInstChild = NULL;
			int idxSkel = -1;
			while (pSol->getFirstComponentIP<SkeletonInstance>(idxSkel + 1, idxSkel, pSkelInstChild))
			{
				if (pSkelInstChild)
				{
					pAnimSM = pSkelInstChild->getFirstComponent<DefaultAnimationSM>();
					if (pAnimSM)
						break;
				}
			}
		}
	}
	if (!pAnimSM)
	{
		SoldierNPC* pSol2 = getFirstParentByTypePtr<SoldierNPC>();
		if (pSol2)
		{
			pAnimSM = pSol2->getFirstComponent<SoldierNPCAnimationSM>();
			if (!pAnimSM)
				pAnimSM = pSol2->getFirstComponent<DefaultAnimationSM>();
		}
	}

	if (!pAnimSM) return;

	PrimitiveTypes::UInt32 flags = ADDITIVE_ANIMATION;
	if (looping) flags |= PE::LOOPING;

	if (m_isAdditiveActive && m_activeAdditiveAnimSetIndex == animSetIndex && m_activeAdditiveAnimIndex == animIndex)
	{
		setAdditiveWeight(weight);
		return;
	}

	auto pSlot = pAnimSM->setAnimation(animSetIndex, animIndex,
		m_firstAdditiveActiveSlot, m_lastAdditiveActiveSlot,
		m_firstAdditiveFadingSlot, m_lastAdditiveFadingSlot,
		flags, weight);

	if (pSlot)
	{
		// By default allow additive to run on full skeleton, but you can restrict by joint range
		// For example upper body only - reuse same range if desired
		PrimitiveTypes::UInt32 addStartJoint = 7;
		PrimitiveTypes::UInt32 addEndJoint = 30;

		pSlot->m_startJoint = addStartJoint;
		pSlot->m_endJoint = addEndJoint;

		pSlot->m_flags |= ADDITIVE_ANIMATION;

		char dbg[128];
		sprintf(dbg, "DEBUG_ADDATIVE_SLOT: slot set anim %u forced joints %u..%u flags 0x%x\n",
			(unsigned)pSlot->m_animationIndex, (unsigned)addStartJoint, (unsigned)addEndJoint, (unsigned)pSlot->m_flags);
		OutputDebugStringA(dbg);
	}

	m_isAdditiveActive = true;
	m_additiveWeight = weight;
	m_activeAdditiveAnimSetIndex = animSetIndex;
	m_activeAdditiveAnimIndex = animIndex;

	OutputDebugStringA("PartialAnimationSM::playAdditive done\n");
}

void PartialAnimationSM::stopAdditive()
{
	if (!m_isAdditiveActive)
		return;

	DefaultAnimationSM* pAnimSM = nullptr;
	PE::Handle hSkelInst = getFirstParentByType<SkeletonInstance>();
	if (hSkelInst.isValid())
		pAnimSM = hSkelInst.getObject<SkeletonInstance>()->getFirstComponent<DefaultAnimationSM>();

	if (!pAnimSM)
	{
		SoldierNPC* pSol = getFirstParentByTypePtr<SoldierNPC>();
		if (!pSol) return;
		pAnimSM = pSol->getFirstComponent<SoldierNPCAnimationSM>();
		if (!pAnimSM)
			pAnimSM = pSol->getFirstComponent<DefaultAnimationSM>();
	}
	if (!pAnimSM) return;

	pAnimSM->disableAnimation(m_activeAdditiveAnimSetIndex, m_activeAdditiveAnimIndex, m_firstAdditiveActiveSlot, m_lastAdditiveActiveSlot);

	m_isAdditiveActive = false;
	m_activeAdditiveAnimSetIndex = UINT32_MAX;
	m_activeAdditiveAnimIndex = UINT32_MAX;
	m_additiveWeight = 0.0f;
}

void PartialAnimationSM::setAdditiveWeight(PrimitiveTypes::Float32 weight)
{
	char dbg[128];
	sprintf(dbg, "PartialAnimationSM::setAdditiveWeight called - weight=%.2f active=%d\n", (double)weight, (int)m_isAdditiveActive);
	OutputDebugStringA(dbg);

	if (!m_isAdditiveActive)
	{
		m_additiveWeight = weight;
		return;
	}

	DefaultAnimationSM* pAnimSM = nullptr;
	PE::Handle hSkelInst = getFirstParentByType<SkeletonInstance>();
	if (hSkelInst.isValid())
		pAnimSM = hSkelInst.getObject<SkeletonInstance>()->getFirstComponent<DefaultAnimationSM>();

	if (!pAnimSM)
	{
		SoldierNPC* pSol = getFirstParentByTypePtr<SoldierNPC>();
		if (!pSol) return;
		pAnimSM = pSol->getFirstComponent<SoldierNPCAnimationSM>();
		if (!pAnimSM)
			pAnimSM = pSol->getFirstComponent<DefaultAnimationSM>();
	}
	if (!pAnimSM) return;

	pAnimSM->setAnimationWeight(m_activeAdditiveAnimSetIndex, m_activeAdditiveAnimIndex, m_firstAdditiveActiveSlot, m_lastAdditiveActiveSlot, weight);
	m_additiveWeight = weight;
}
	} // namespace Components
} // namespace CharacterControl