#include "PrimeEngine/APIAbstraction/APIAbstractionDefines.h"

#include "PrimeEngine/Lua/LuaEnvironment.h"
#include "PrimeEngine/Scene/SkeletonInstance.h"
#include "PrimeEngine/Scene/MeshInstance.h"
#include "PrimeEngine/Scene/RootSceneNode.h"

#include "SoldierNPC.h"
#include "SoldierNPCAnimationSM.h"
#include "SoldierNPCMovementSM.h"
#include "SoldierNPCBehaviorSM.h"
#include "AnimationLODComponent.h"

#include "PartialAnimation.h"

#include "PhysicsComponent.h"
#include "PhysicsManager.h"


using namespace PE;
using namespace PE::Components;
using namespace CharacterControl::Events;

namespace CharacterControl{
namespace Components {

PE_IMPLEMENT_CLASS1(SoldierNPC, Component);

SoldierNPC::SoldierNPC(PE::GameContext &context, PE::MemoryArena arena, PE::Handle hMyself, Event_CreateSoldierNPC *pEvt) : Component(context, arena, hMyself)
{

	// hierarchy of soldier and replated components and variables (note variables are just variables, they are not passed events to)
	// scene
	// +-components
	//   +-soldier scene node
	//   | +-components
	//   |   +-soldier skin
	//   |     +-components
	//   |       +-soldier animation state machine
	//   |       +-soldier weapon skin scene node
	//   |         +-components
	//   |           +-weapon mesh

	// game objects
	// +-components
	//   +-soldier npc
	//     +-variables
	//     | +-m_hMySN = soldier scene node
	//     | +-m_hMySkin = skin
	//     | +-m_hMyGunSN = soldier weapon skin scene node
	//     | +-m_hMyGunMesh = weapon mesh
	//     +-components
	//       +-soldier scene node (restricted to no events. this is for state machines to be able to locate the scene node)
	//       +-movement state machine
	//       +-behavior state machine

    
	// need to acquire redner context for this code to execute thread-safe
	m_pContext->getGPUScreen()->AcquireRenderContextOwnership(pEvt->m_threadOwnershipMask);
	

	PE::Handle hSN("SCENE_NODE", sizeof(SceneNode));
	SceneNode *pMainSN = new(hSN) SceneNode(*m_pContext, m_arena, hSN);
	pMainSN->addDefaultComponents();



	pMainSN->m_base.setPos(pEvt->m_pos);
	pMainSN->m_base.setU(pEvt->m_u);
	pMainSN->m_base.setV(pEvt->m_v);
	pMainSN->m_base.setN(pEvt->m_n);

	m_needsToShoot = pEvt->m_needsToShoot;

	//// Adding physics component to soldier
	PE::Handle hPhys("PhysicsComponent", sizeof(PhysicsComponent));
	PhysicsComponent* pPhys = new(hPhys) PhysicsComponent(*m_pContext, m_arena, hPhys);
	pPhys->setSphere(pMainSN->m_base.getPos(), 0.5f); // radius
	pPhys->setStatic(false);
	pPhys->m_pOwner = pMainSN;
	PhysicsManager::Instance()->addComponent(pPhys);
	//pMainSN->addComponent(pPhys->getHandle());

	RootSceneNode::Instance()->addComponent(hSN);

	// add the scene node as component of soldier without any handlers. this is just data driven way to locate scnenode for soldier's components
	{
		static int allowedEvts[] = {0};
		addComponent(hSN, &allowedEvts[0]);
	}

	int numskins = 1; // 8
	for (int iSkin = 0; iSkin < numskins; ++iSkin)
	{
		float z = (iSkin / 4) * 1.5f;
		float x = (iSkin % 4) * 1.5f;
		PE::Handle hSN("SCENE_NODE", sizeof(SceneNode));
		SceneNode *pSN = new(hSN) SceneNode(*m_pContext, m_arena, hSN);
		pSN->addDefaultComponents();

		pSN->m_base.setPos(Vector3(x, 0, z));
		
		// rotation scene node to rotate soldier properly, since soldier from Maya is facing wrong direction
		PE::Handle hRotateSN("SCENE_NODE", sizeof(SceneNode));
		SceneNode *pRotateSN = new(hRotateSN) SceneNode(*m_pContext, m_arena, hRotateSN);
		pRotateSN->addDefaultComponents();

		pSN->addComponent(hRotateSN);

		pRotateSN->m_base.turnLeft(3.1415);

		PE::Handle hSoldierAnimSM("SoldierNPCAnimationSM", sizeof(SoldierNPCAnimationSM));
		SoldierNPCAnimationSM *pSoldierAnimSM = new(hSoldierAnimSM) SoldierNPCAnimationSM(*m_pContext, m_arena, hSoldierAnimSM);
		pSoldierAnimSM->addDefaultComponents();

		// Create Animation LOD Component
		PE::Handle hAnimLOD("AnimationLODComponent", sizeof(AnimationLODComponent));
		AnimationLODComponent *pAnimLOD = new(hAnimLOD) AnimationLODComponent(*m_pContext, m_arena, hAnimLOD, hSN, hSoldierAnimSM);
		pAnimLOD->addDefaultComponents();
		addComponent(hAnimLOD);

		pSoldierAnimSM->m_debugAnimIdOffset = 0;// rand() % 3;

		PE::Handle hSkeletonInstance("SkeletonInstance", sizeof(SkeletonInstance));
		SkeletonInstance *pSkelInst = new(hSkeletonInstance) SkeletonInstance(*m_pContext, m_arena, hSkeletonInstance, 
			hSoldierAnimSM);
		pSkelInst->addDefaultComponents();

		pSkelInst->initFromFiles("soldier_Soldier_Skeleton.skela", "Soldier", pEvt->m_threadOwnershipMask);

		pSkelInst->setAnimSet("soldier_Soldier_Skeleton.animseta", "Soldier");

		//--------------------------------------------------------------------------------------------------------------------------------------------------

		// creating and attaching partialanimationSM to skeleton instance
		PE::Handle hPartialAnimSM("PartialAnimationSM", sizeof(CharacterControl::Components::PartialAnimationSM));
		CharacterControl::Components::PartialAnimationSM* pPartialSM = new(hPartialAnimSM) CharacterControl::Components::PartialAnimationSM(*m_pContext, m_arena, hPartialAnimSM);
		pPartialSM->m_animSetIndex = 0; // same animset index used for soldier
		pPartialSM->m_animIndex = SoldierNPCAnimationSM::STAND_SHOOT; // partial animation to play while idle
		pPartialSM->addDefaultComponents();

		// attach partial anim SM to the skeleton instance so it can find DefaultAnimationSM directly
		pSkelInst->addComponent(hPartialAnimSM);
		addComponent(hPartialAnimSM);

		// pass requested animation type from spawn event into PartialAnimationSM so it can be used to select mode
		pPartialSM->m_animationType = pEvt->m_animationType;

		// debug: log the assigned animation type
		{
			char dbg[128];
			sprintf(dbg, "SoldierNPC: assigned PartialAnimationSM::m_animationType = %d\n", pPartialSM->m_animationType);
			OutputDebugStringA(dbg);
		}

		//--------------------------------------------------------------------------------------------------------------------------------------------------

		PE::Handle hMeshInstance("MeshInstance", sizeof(MeshInstance));
		MeshInstance *pMeshInstance = new(hMeshInstance) MeshInstance(*m_pContext, m_arena, hMeshInstance);
		pMeshInstance->addDefaultComponents();
		
		pMeshInstance->initFromFile(pEvt->m_meshFilename, pEvt->m_package, pEvt->m_threadOwnershipMask);
		
		pSkelInst->addComponent(hMeshInstance);

		// add skin to scene node
		pRotateSN->addComponent(hSkeletonInstance);

		#if !APIABSTRACTION_D3D11
		{
			PE::Handle hMyGunMesh = PE::Handle("MeshInstance", sizeof(MeshInstance));
			MeshInstance *pGunMeshInstance = new(hMyGunMesh) MeshInstance(*m_pContext, m_arena, hMyGunMesh);

			pGunMeshInstance->addDefaultComponents();
			pGunMeshInstance->initFromFile(pEvt->m_gunMeshName, pEvt->m_gunMeshPackage, pEvt->m_threadOwnershipMask);

			// create a scene node for gun attached to a joint

			PE::Handle hMyGunSN = PE::Handle("SCENE_NODE", sizeof(JointSceneNode));
			JointSceneNode *pGunSN = new(hMyGunSN) JointSceneNode(*m_pContext, m_arena, hMyGunSN, 38);
			pGunSN->addDefaultComponents();

			// add gun to joint
			pGunSN->addComponent(hMyGunMesh);

			// add gun scene node to the skin
			pSkelInst->addComponent(hMyGunSN);
		}
		#endif
				
		pMainSN->addComponent(hSN);
	}

	m_pContext->getGPUScreen()->ReleaseRenderContextOwnership(pEvt->m_threadOwnershipMask);
	
#if 1
	// add movement state machine to soldier npc
    PE::Handle hSoldierMovementSM("SoldierNPCMovementSM", sizeof(SoldierNPCMovementSM));
	SoldierNPCMovementSM *pSoldierMovementSM = new(hSoldierMovementSM) SoldierNPCMovementSM(*m_pContext, m_arena, hSoldierMovementSM);
	pSoldierMovementSM->addDefaultComponents();

	// add it to soldier NPC
	addComponent(hSoldierMovementSM);

	// add behavior state machine ot soldier npc
    PE::Handle hSoldierBheaviorSM("SoldierNPCBehaviorSM", sizeof(SoldierNPCBehaviorSM));
	SoldierNPCBehaviorSM *pSoldierBehaviorSM = new(hSoldierBheaviorSM) SoldierNPCBehaviorSM(*m_pContext, m_arena, hSoldierBheaviorSM, hSoldierMovementSM);
	pSoldierBehaviorSM->addDefaultComponents();

	// add it to soldier NPC
	addComponent(hSoldierBheaviorSM);

	StringOps::writeToString(pEvt->m_patrolWayPoint, pSoldierBehaviorSM->m_curPatrolWayPoint, 32);
	pSoldierBehaviorSM->m_havePatrolWayPoint = StringOps::length(pSoldierBehaviorSM->m_curPatrolWayPoint) > 0;

	// start the soldier
	pSoldierBehaviorSM->start();
#endif
}

void SoldierNPC::addDefaultComponents()
{
	Component::addDefaultComponents();

	// custom methods of this component
}



}; // namespace Components
}; // namespace CharacterControl
