#include "PrimeEngine/APIAbstraction/APIAbstractionDefines.h"

#include "PrimeEngine/Lua/LuaEnvironment.h"
#include "PrimeEngine/Scene/DebugRenderer.h"
#include "../ClientGameObjectManagerAddon.h"
#include "../CharacterControlContext.h"
#include "SoldierNPCMovementSM.h"
#include "SoldierNPCAnimationSM.h"
#include "SoldierNPCBehaviorSM.h"
#include "SoldierNPC.h"
#include "PartialAnimation.h"
#include "PrimeEngine/Scene/SkeletonInstance.h"

// Added includes to resolve AnimationSlot / AnimSetBufferGPU / AnimationSetCPU
#include "PrimeEngine/Scene/DefaultAnimationSM.h"
#include "PrimeEngine/APIAbstraction/GPUBuffers/AnimSetBufferGPU.h"
#include "PrimeEngine/Geometry/SkeletonCPU/AnimationSetCPU.h"

#include "PrimeEngine/Scene/SceneNode.h"
#include "PrimeEngine/Render/IRenderer.h"
// make core PE types (AnimationSlot, AnimSetBufferGPU, AnimationSetCPU, etc.) visible unqualified
using namespace PE;
using namespace PE::Components;
using namespace PE::Events;
using namespace CharacterControl::Events;

namespace CharacterControl{

namespace Components{

PE_IMPLEMENT_CLASS1(SoldierNPCBehaviorSM, Component);

SoldierNPCBehaviorSM::SoldierNPCBehaviorSM(PE::GameContext &context, PE::MemoryArena arena, PE::Handle hMyself, PE::Handle hMovementSM) 
: Component(context, arena, hMyself)
, m_hMovementSM(hMovementSM)
{

}



void SoldierNPCBehaviorSM::start()
{
	OutputDebugStringA("SoldierNPCBehaviorSM::start()\n");
	if (m_havePatrolWayPoint)
	{
		m_state = WAITING_FOR_WAYPOINT; // will update on next do_UPDATE()
	}
	else
	{
		OutputDebugStringA("SoldierNPCBehaviorSM::start(): no patrol waypoint, going to IDLE\n");
		m_state = IDLE; // stand in place

		PE::Handle h("SoldierNPCMovementSM_Event_STOP", sizeof(SoldierNPCMovementSM_Event_STOP));
		SoldierNPCMovementSM_Event_STOP *pEvt = new(h) SoldierNPCMovementSM_Event_STOP();

		// Send movement STOP first so movement/animation SMs initialize baseline (walk) before we apply
		// full-body blend / partial / additive logic below. If we call animations first, movement SM
		// may overwrite the slots (set walk into slot 0) immediately after and clobber our blend.
		m_hMovementSM.getObject<Component>()->handleEvent(pEvt);

		SoldierNPC* pSol = getFirstParentByTypePtr<SoldierNPC>();
		if (pSol)
		{
			// Try to find PartialAnimationSM directly on the Soldier (or on its SkeletonInstance children)
			CharacterControl::Components::PartialAnimationSM *pPartial = pSol->getFirstComponent<CharacterControl::Components::PartialAnimationSM>();

			SkeletonInstance *pSkelInstChild = NULL;
			int idxSkel = -1;
			if (!pPartial)
			{
				while (pSol->getFirstComponentIP<SkeletonInstance>(idxSkel + 1, idxSkel, pSkelInstChild))
				{
					if (pSkelInstChild)
					{
						pPartial = pSkelInstChild->getFirstComponent<CharacterControl::Components::PartialAnimationSM>();
						if (pPartial)
							break;
					}
				}
			}

			DefaultAnimationSM *pAnimSM = nullptr;
			SkeletonInstance *pSkelInstFound = NULL;

			// Prefer soldier-specific animation SM
			pAnimSM = pSol->getFirstComponent<SoldierNPCAnimationSM>();
			if (!pAnimSM)
				pAnimSM = pSol->getFirstComponent<DefaultAnimationSM>();

			// If still not found, search skeleton children and prefer the explicit m_hAnimationSM handle
			if (!pAnimSM)
			{
				idxSkel = -1;
				while (pSol->getFirstComponentIP<SkeletonInstance>(idxSkel + 1, idxSkel, pSkelInstChild))
				{
					if (!pSkelInstChild) continue;
					// try handle first (this is the reliable handle set in SkeletonInstance ctor)
					if (pSkelInstChild->m_hAnimationSM.isValid())
					{
						DefaultAnimationSM *pFromHandle = pSkelInstChild->m_hAnimationSM.getObject<DefaultAnimationSM>();
						if (pFromHandle)
						{
							pAnimSM = pFromHandle;
							pSkelInstFound = pSkelInstChild;
							break;
						}
					}
					// fallback to component query on skeleton child
					DefaultAnimationSM *pCompASM = pSkelInstChild->getFirstComponent<DefaultAnimationSM>();
					if (pCompASM)
					{
						pAnimSM = pCompASM;
						pSkelInstFound = pSkelInstChild;
						break;
					}
				}
			}

			// If we still don't have a skeleton instance for animation-set lookups (used later), pick any skeleton child
			if (!pSkelInstFound)
			{
				idxSkel = -1;
				while (pSol->getFirstComponentIP<SkeletonInstance>(idxSkel + 1, idxSkel, pSkelInstChild))
				{
					if (pSkelInstChild) { pSkelInstFound = pSkelInstChild; break; }
				}
			}

			// animation ids / set index used in this project
			PrimitiveTypes::UInt32 animSetIndex = 0;
			PrimitiveTypes::UInt32 animWalk = SoldierNPCAnimationSM::WALK;
			PrimitiveTypes::UInt32 animShoot = SoldierNPCAnimationSM::STAND_SHOOT;

			if (pPartial)
			{
				// choose behavior based on the mode stored on PartialAnimationSM
				switch (pPartial->m_animationType)
				{
				case 0: // full-body blending (blend walk + shoot at 50/50)
					OutputDebugStringA("This is an FULL-BODY BLEND (walk+shoot 50/50)\n");
					// Prefer the helper on PartialAnimationSM if available
					pPartial->playFullBlend(animSetIndex, animWalk, animSetIndex, animShoot, 0.5f, 0.5f, true);
					break;

				case 1: // partial override (upper-body shoot over walking baseline)
					OutputDebugStringA("This is PARTIAL animation (upper-body shoot)\n");
					// Ensure baseline walk playing
					if (pAnimSM)
					{
						pAnimSM->setAnimation(animSetIndex, animWalk, 0, 0, 4, 5, 0, 1.0f);
					}
					// Play partial on pPartial
					pPartial->playPartial(animSetIndex, animShoot, 1.0f, true);
					break;

				case 2: // additive (layer shoot delta on top of walk)
					OutputDebugStringA("SoldierNPCBehaviorSM: starting ADDITIVE animation (shoot as delta)\n");
					// Ensure baseline walk playing
					if (pAnimSM)
					{
						pAnimSM->setAnimation(animSetIndex, animWalk, 0, 0, 4, 5, 0, 1.0f);
					}
					// Play additive on pPartial
					pPartial->playAdditive(animSetIndex, animShoot, 1.0f, true);
					break;

				default:
					// fallback to simple partial behavior
					OutputDebugStringA("SoldierNPCBehaviorSM: unknown animationType, defaulting to PARTIAL\n");
					if (pAnimSM)
					{
						pAnimSM->setAnimation(animSetIndex, animWalk, 0, 0, 4, 5, 0, 1.0f);
					}
					pPartial->playPartial(animSetIndex, animShoot, 1.0f, true);
					break;
				}
			}
			else
			{
				// No PartialAnimationSM found at all -> perform selection using available Default/Soldier anim SM
				// Default to full-body blend if requested mode is not present (we don't have mode info here, assume full blend)
				OutputDebugStringA("SoldierNPCBehaviorSM: PartialAnimationSM not found; using DefaultAnimationSM for full blend fallback\n");
				if (pAnimSM)
				{
					// start both animations in separate slots and set 50/50 weights
					// slot 0 -> walk
					pAnimSM->setAnimation(animSetIndex, animWalk, 0, 0, 4, 4, 0, 0.5f);
					// slot 1 -> shoot
					pAnimSM->setAnimation(animSetIndex, animShoot, 1, 1, 5, 5, 0, 0.5f);
				}
				else
				{
					OutputDebugStringA("SoldierNPCBehaviorSM: No animation state machine found to play animations\n");
				}
			}
		}

		// release memory now that event is processed
		h.release();
		
	}	
}

void SoldierNPCBehaviorSM::addDefaultComponents()
{
	Component::addDefaultComponents();

	PE_REGISTER_EVENT_HANDLER(SoldierNPCMovementSM_Event_TARGET_REACHED, SoldierNPCBehaviorSM::do_SoldierNPCMovementSM_Event_TARGET_REACHED);
	PE_REGISTER_EVENT_HANDLER(Event_UPDATE, SoldierNPCBehaviorSM::do_UPDATE);

	PE_REGISTER_EVENT_HANDLER(Event_PRE_RENDER_needsRC, SoldierNPCBehaviorSM::do_PRE_RENDER_needsRC);
}

// sent my movement state machine whenever it reaches current target
//void SoldierNPCBehaviorSM::do_SoldierNPCMovementSM_Event_TARGET_REACHED(PE::Events::Event *pEvt)
//{
//	PEINFO("SoldierNPCBehaviorSM::do_SoldierNPCMovementSM_Event_TARGET_REACHED\n");
//
//	if (m_state == PATROLLING_WAYPOINTS)
//	{
//		ClientGameObjectManagerAddon *pGameObjectManagerAddon = (ClientGameObjectManagerAddon *)(m_pContext->get<CharacterControlContext>()->getGameObjectManagerAddon());
//		if (pGameObjectManagerAddon)
//		{
//			// search for waypoint object
//			WayPoint *pWP = pGameObjectManagerAddon->getWayPoint(m_curPatrolWayPoint);
//			if (pWP && StringOps::length(pWP->m_nextWayPointName) > 0)
//			{
//				// have next waypoint to go to
//				pWP = pGameObjectManagerAddon->getWayPoint(pWP->m_nextWayPointName);
//				if (pWP)
//				{
//					StringOps::writeToString(pWP->m_name, m_curPatrolWayPoint, 32);
//
//					m_state = PATROLLING_WAYPOINTS;
//					PE::Handle h("SoldierNPCMovementSM_Event_MOVE_TO", sizeof(SoldierNPCMovementSM_Event_MOVE_TO));
//					Events::SoldierNPCMovementSM_Event_MOVE_TO *pEvt = new(h) SoldierNPCMovementSM_Event_MOVE_TO(pWP->m_base.getPos());
//					pEvt->m_running = rand() % 2 > 0;
//
//					m_hMovementSM.getObject<Component>()->handleEvent(pEvt);
//					// release memory now that event is processed
//					h.release();
//				}
//			}
//			else
//			{
//				m_state = IDLE;
//				// no need to send the event. movement state machine will automatically send event to animation state machine to play idle animation
//			}
//		}
//	}
//}

// my copy of do_SoldierNPCMovementSM_Event_TARGET_REACHED
void SoldierNPCBehaviorSM::do_SoldierNPCMovementSM_Event_TARGET_REACHED(PE::Events::Event* pEvt)
{
	PEINFO("SoldierNPCBehaviorSM::do_SoldierNPCMovementSM_Event_TARGET_REACHED\n");

	if (m_state == PATROLLING_WAYPOINTS)
	{
		ClientGameObjectManagerAddon* pGameObjectManagerAddon = (ClientGameObjectManagerAddon*)(m_pContext->get<CharacterControlContext>()->getGameObjectManagerAddon());
		if (pGameObjectManagerAddon)
		{
			// search for waypoint object
			WayPoint* pWP = pGameObjectManagerAddon->getWayPoint(m_curPatrolWayPoint);
			if (pWP && pWP->m_numNextWayPoints > 0)
			{
				// Randomly selecting one of the next waypoints
				int idx = rand() % pWP->m_numNextWayPoints;

				OutputDebugStringA("The way point chosen is:");
				OutputDebugStringA(pWP->m_nextWayPointNames[idx]);

				StringOps::writeToString(pWP->m_nextWayPointNames[idx], m_curPatrolWayPoint, 32);

				WayPoint* pNextWP = pGameObjectManagerAddon->getWayPoint(m_curPatrolWayPoint);
				if (pNextWP)
				{
					m_state = PATROLLING_WAYPOINTS;
					PE::Handle h("SoldierNPCMovementSM_Event_MOVE_TO", sizeof(SoldierNPCMovementSM_Event_MOVE_TO));
					Events::SoldierNPCMovementSM_Event_MOVE_TO* pEvt = new(h) SoldierNPCMovementSM_Event_MOVE_TO(pNextWP->m_base.getPos());
					pEvt->m_running = rand() % 2 > 0;

					m_hMovementSM.getObject<Component>()->handleEvent(pEvt);
					// release memory now that event is processed
					h.release();
				}
			}
			else
			{
				m_state = IDLE;
				// no need to send the event. movement state machine will automatically send event to animation state machine to play idle animation
			}
		}
	}
}

// this event is executed when thread has RC
void SoldierNPCBehaviorSM::do_PRE_RENDER_needsRC(PE::Events::Event *pEvt)
{
	Event_PRE_RENDER_needsRC *pRealEvent = (Event_PRE_RENDER_needsRC *)(pEvt);
	if (m_havePatrolWayPoint)
	{
		char buf[80];
		sprintf(buf, "Patrol Waypoint: %s",m_curPatrolWayPoint);
		SoldierNPC *pSol = getFirstParentByTypePtr<SoldierNPC>();
		PE::Handle hSoldierSceneNode = pSol->getFirstComponentHandle<PE::Components::SceneNode>();
		Matrix4x4 base = hSoldierSceneNode.getObject<PE::Components::SceneNode>()->m_worldTransform;
		
		DebugRenderer::Instance()->createTextMesh(
			buf, false, false, true, false, 0,
			base.getPos(), 0.01f, pRealEvent->m_threadOwnershipMask);
		
		{
			//we can also construct points ourself
			bool sent = false;
			ClientGameObjectManagerAddon *pGameObjectManagerAddon = (ClientGameObjectManagerAddon *)(m_pContext->get<CharacterControlContext>()->getGameObjectManagerAddon());
			if (pGameObjectManagerAddon)
			{
				WayPoint *pWP = pGameObjectManagerAddon->getWayPoint(m_curPatrolWayPoint);
				if (pWP)
				{
					Vector3 target = pWP->m_base.getPos();
					Vector3 pos = base.getPos();
					Vector3 color(1.0f, 1.0f, 0);
					Vector3 linepts[] = {pos, color, target, color};
					
					DebugRenderer::Instance()->createLineMesh(true, base,  &linepts[0].m_x, 2, 0);// send event while the array is on the stack
					sent = true;
				}
			}
			if (!sent) // if for whatever reason we didnt retrieve waypoint info, send the event with transform only
				DebugRenderer::Instance()->createLineMesh(true, base, NULL, 0, 0);// send event while the array is on the stack
		}
	}

	// --- Floating debug text for this soldier: animation names, frames, weights ---
	// COMMENTED OUT: Only keeping Animation LOD display in DefaultAnimationSM.cpp
	/*
	{
		SoldierNPC *pSol = getFirstParentByTypePtr<SoldierNPC>();
		if (!pSol) return;

		// world position above soldier
		PE::Handle hSoldierSceneNode = pSol->getFirstComponentHandle<PE::Components::SceneNode>();
		if (!hSoldierSceneNode.isValid()) return;
		Matrix4x4 world = hSoldierSceneNode.getObject<PE::Components::SceneNode>()->m_worldTransform;
		Vector3 basePos = world.getPos();

		// find a DefaultAnimationSM (prefer one on SkeletonInstance children)
		DefaultAnimationSM *pAnimSM = nullptr;
		SkeletonInstance *pSkelInstFound = NULL;
		int idxSkel = -1;
		SkeletonInstance *pSkelInstChild = NULL;
		while (pSol->getFirstComponentIP<SkeletonInstance>(idxSkel + 1, idxSkel, pSkelInstChild))
		{
			if (pSkelInstChild)
			{
				auto p = pSkelInstChild->getFirstComponent<DefaultAnimationSM>();
				if (p)
				{
					pAnimSM = p;
					pSkelInstFound = pSkelInstChild;
					break;
				}
			}
		}
		// fallback: try DefaultAnimationSM directly on soldier
		if (!pAnimSM)
		{
			pAnimSM = pSol->getFirstComponent<DefaultAnimationSM>();
			// try to get a skeleton instance (if any) for animset lookup
			idxSkel = -1;
			while (pSol->getFirstComponentIP<SkeletonInstance>(idxSkel + 1, idxSkel, pSkelInstChild))
			{
				if (pSkelInstChild) { pSkelInstFound = pSkelInstChild; break; }
			}
		}

		// Also find PartialAnimationSM if present to show partial/additive/full blend states
		CharacterControl::Components::PartialAnimationSM *pPartial = pSol->getFirstComponent<CharacterControl::Components::PartialAnimationSM>();
		if (!pPartial)
		{
			// look in skeleton child
			idxSkel = -1;
			while (pSol->getFirstComponentIP<SkeletonInstance>(idxSkel + 1, idxSkel, pSkelInstChild))
			{
				if (pSkelInstChild)
				{
					pPartial = pSkelInstChild->getFirstComponent<CharacterControl::Components::PartialAnimationSM>();
					if (pPartial) break;
				}
			}
		}

		// start rendering text lines above the soldier
		const float lineHeight = 0.2;   // was 0.035f — larger = more space between lines
		int line = 0;
		char dbgLine[256];

		// Compute stable lateral offset per-soldier so multiple soldiers' debug blocks don't overlap.
		// Use a simple pointer-based hash to avoid needing global indexing.
		size_t idHash = (size_t)(pSol);
		int lateralSlot = (int)((idHash >> 4) & 0x7); // 0..7
		float lateralOffset = (lateralSlot - 3.5f) * 0.18f; // spread around 0
		Vector3 right = world.getU(); // local X
		Vector3 up = world.getV(); // local Y
		Vector3 forward = world.getN(); // local Z
		const float baseYOffset = 2.4f; // raise the whole block a bit if it collides with the head
		// small forward push to avoid z-fighting with geometry
		const float forwardPush = 0.05f;

		auto emitLine = [&](const char *text)
		{
			Vector3 pos = basePos + up * (baseYOffset + line * lineHeight) + right * lateralOffset + forward * forwardPush;
			DebugRenderer::Instance()->createTextMesh(text, false, false, true, false, 0,
				pos, 0.012f, pRealEvent->m_threadOwnershipMask);
			++line;
		};

		// Show mode from PartialAnimationSM if available, plus stored vs slot weights for partial/additive
		if (pPartial)
		{
	

			sprintf(dbgLine, "Animation Mode: %d | Partial: %s | Additive: %s | Full Blend: %s",
				(int)pPartial->m_animationType,
				pPartial->m_isActive ? "ON" : "OFF",
				pPartial->m_isAdditiveActive ? "ON" : "OFF",
				pPartial->m_isFullBlendActive ? "ON" : "OFF");
			emitLine(dbgLine);

			// Weight Distribution for PartialAnimationSM
			sprintf(dbgLine, "Weights → Partial: %.2f | Additive: %.2f | Blend A: %.2f | Blend B: %.2f",
				(double)pPartial->m_weight,
				(double)pPartial->m_additiveWeight,
				(double)pPartial->m_fullBlendWeight0,
				(double)pPartial->m_fullBlendWeight1);
			emitLine(dbgLine);

			// If we have an animation SM, show the actual slot weights for the active partial/additive slots
			if (pAnimSM)
			{
				// Partial slot weight
				if (pPartial->m_isActive && pPartial->m_activeAnimSetIndex != UINT32_MAX)
				{
					AnimationSlot *pSlot = pAnimSM->getSlot(pPartial->m_activeAnimSetIndex, pPartial->m_activeAnimIndex,
						pPartial->m_firstActiveSlot, pPartial->m_lastActiveSlot);
					if (pSlot)
					{
						sprintf(dbgLine, "partial slot: animSet=%u anim=%u slotWeight=%.2f frame=%.2f",
							(unsigned)pSlot->m_animationSetIndex, (unsigned)pSlot->m_animationIndex, (double)pSlot->m_weight, (double)pSlot->m_frameIndex);
					}
					else
					{
						sprintf(dbgLine, "partial slot: (not present in anim SM)");
					}
					emitLine(dbgLine);
				}
				else
				{
					emitLine("partial slot: (inactive)");
				}

				// Additive slot weight
				if (pPartial->m_isAdditiveActive && pPartial->m_activeAdditiveAnimSetIndex != UINT32_MAX)
				{
					AnimationSlot *pAddSlot = pAnimSM->getSlot(pPartial->m_activeAdditiveAnimSetIndex, pPartial->m_activeAdditiveAnimIndex,
						pPartial->m_firstAdditiveActiveSlot, pPartial->m_lastAdditiveActiveSlot);
					if (pAddSlot)
					{
						sprintf(dbgLine, "additive slot: animSet=%u anim=%u slotWeight=%.2f frame=%.2f",
							(unsigned)pAddSlot->m_animationSetIndex, (unsigned)pAddSlot->m_animationIndex, (double)pAddSlot->m_weight, (double)pAddSlot->m_frameIndex);
					}
					else
					{
						sprintf(dbgLine, "additive slot: (not present in anim SM)");
					}
					emitLine(dbgLine);
				}
				else
				{
					emitLine("additive slot: (inactive)");
				}
			}
			else
			{
				emitLine("No DefaultAnimationSM available to query slot weights");
			}
		}

		// If we have an animation SM, list active slots
		if (pAnimSM)
		{
			// Header
			sprintf(dbgLine, "AnimSlots (slot: animName idx, frame, wt, flags)");
			emitLine(dbgLine);

			// iterate through slots (m_animSlots is available on DefaultAnimationSM)
			for (PrimitiveTypes::UInt32 i = 0; i < pAnimSM->m_animSlots.m_size; ++i)
			{
				AnimationSlot &s = pAnimSM->m_animSlots[i];
				if (!s.isActive()) continue;

				const char *animName = "(unknown)";
				if (pSkelInstFound && pSkelInstFound->m_hAnimationSetGPUs.m_size > s.m_animationSetIndex)
				{
					AnimSetBufferGPU *pBuf = pSkelInstFound->m_hAnimationSetGPUs[s.m_animationSetIndex].getObject<AnimSetBufferGPU>();
					if (pBuf)
					{
						AnimationSetCPU *pAnimSetCPU = pBuf->m_hAnimationSetCPU.getObject<AnimationSetCPU>();
						if (pAnimSetCPU && s.m_animationIndex < pAnimSetCPU->m_animations.m_size)
							animName = pAnimSetCPU->m_animations[s.m_animationIndex].m_name;
					}
				}

				// compute approximate current frame (frameIndex = m_frameIndex)
				double curFrame = (double)s.m_frameIndex;
				// format line: slot 0: "a01_WalkRifle" idx=5 frame=12.3 wt=1.00 flags=0x6
				sprintf(dbgLine, "slot %u: \"%s\" idx=%u frame=%.2f wt=%.2f flags=0x%x",
					(unsigned)i, animName, (unsigned)s.m_animationIndex, curFrame, (double)s.m_weight, (unsigned)s.m_flags);

				emitLine(dbgLine);

				// stop if too many lines
				if (line > 12) break;
			}
		}
		else
		{
			// no anim SM found
		}
	}
	*/
}

void SoldierNPCBehaviorSM::do_UPDATE(PE::Events::Event *pEvt)
{

	// NPC shooting logic
	SoldierNPC* pSol = getFirstParentByTypePtr<SoldierNPC>();
	if(pSol && pSol->m_needsToShoot && m_state == IDLE)
	{
		// Find the target SoldierNPC
		ClientGameObjectManagerAddon* pGOM = (ClientGameObjectManagerAddon*)(m_pContext->get<CharacterControlContext>()->getGameObjectManagerAddon());
		SoldierNPC* pTarget = nullptr;
		for (PrimitiveTypes::UInt32 i = 0; i < pGOM->m_components.m_size; ++i)
		{
			Component* pC = pGOM->m_components[i].getObject<Component>();
			SoldierNPC* pSNPC = dynamic_cast<SoldierNPC*>(pC);
			if (pSNPC && pSNPC->m_needsToShoot == 5)
			{
				pTarget = pSNPC;
				break;
			}
		}
		if (pTarget)
		{
			// Get scene nodes
			PE::Handle hMySN = pSol->getFirstComponentHandle<PE::Components::SceneNode>();
			PE::Handle hTargetSN = pTarget->getFirstComponentHandle<PE::Components::SceneNode>();
			Vector3 targetPos = hTargetSN.getObject<PE::Components::SceneNode>()->m_base.getPos();

			// Turn towards target
			hMySN.getObject<PE::Components::SceneNode>()->m_base.turnTo(targetPos);

			// Play shooting animation
			//SoldierNPCAnimationSM* pAnimSM = pSol->getFirstComponent<SoldierNPCAnimationSM>();
			/*if (pAnimSM)
			{
				pAnimSM->setAnimation(0, SoldierNPCAnimationSM::STAND_SHOOT, 0, 0, 1, 1, PE::LOOPING);
			}*/
		}
	}


	if (m_state == WAITING_FOR_WAYPOINT)
	{
		if (m_havePatrolWayPoint)
		{
			ClientGameObjectManagerAddon *pGameObjectManagerAddon = (ClientGameObjectManagerAddon *)(m_pContext->get<CharacterControlContext>()->getGameObjectManagerAddon());
			if (pGameObjectManagerAddon)
			{
				// search for waypoint object
				WayPoint *pWP = pGameObjectManagerAddon->getWayPoint(m_curPatrolWayPoint);
				if (pWP)
				{
					m_state = PATROLLING_WAYPOINTS;
					PE::Handle h("SoldierNPCMovementSM_Event_MOVE_TO", sizeof(SoldierNPCMovementSM_Event_MOVE_TO));
					Events::SoldierNPCMovementSM_Event_MOVE_TO *pEvt = new(h) SoldierNPCMovementSM_Event_MOVE_TO(pWP->m_base.getPos());

					m_hMovementSM.getObject<Component>()->handleEvent(pEvt);
					// release memory now that event is processed
					h.release();
				}
			}
		}
		else
		{
			// should not happen, but in any case, set state to idle
			m_state = IDLE;

			PE::Handle h("SoldierNPCMovementSM_Event_STOP", sizeof(SoldierNPCMovementSM_Event_STOP));
			SoldierNPCMovementSM_Event_STOP *pEvt = new(h) SoldierNPCMovementSM_Event_STOP();

			m_hMovementSM.getObject<Component>()->handleEvent(pEvt);
			// release memory now that event is processed
			h.release();
		}
	}
}


}}

