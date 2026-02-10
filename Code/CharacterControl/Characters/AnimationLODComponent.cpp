#include "AnimationLODComponent.h"
#include "PrimeEngine/Events/StandardEvents.h"
#include "PrimeEngine/Scene/CameraManager.h"
#include "PrimeEngine/Scene/SceneNode.h"
#include "PrimeEngine/GameObjectModel/Camera.h"
#include "PrimeEngine/Scene/CameraSceneNode.h"
#include "PrimeEngine/Scene/DefaultAnimationSM.h"
#include "PrimeEngine/Lua/LuaEnvironment.h"

namespace CharacterControl {
namespace Components {

PE_IMPLEMENT_CLASS1(AnimationLODComponent, PE::Components::Component);

AnimationLODComponent::AnimationLODComponent(PE::GameContext &context, PE::MemoryArena arena, PE::Handle hMyself, 
	PE::Handle hSceneNode, PE::Handle hAnimSM)
	: PE::Components::Component(context, arena, hMyself), m_hSceneNode(hSceneNode), m_hAnimSM(hAnimSM), m_prevFramesToSkip(-1)
{
}

void AnimationLODComponent::addDefaultComponents()
{
	Component::addDefaultComponents();
	PE_REGISTER_EVENT_HANDLER(PE::Events::Event_UPDATE, AnimationLODComponent::do_UPDATE);
}

void AnimationLODComponent::do_UPDATE(PE::Events::Event *pEvt)
{
	// Get Camera Position
	PE::Components::CameraManager *pCamMgr = PE::Components::CameraManager::Instance();
	PE::Handle hCam = pCamMgr->getActiveCameraHandle();
	if (!hCam.isValid()) return;

	PE::Components::Camera *pCam = hCam.getObject<PE::Components::Camera>();
	PE::Components::CameraSceneNode *pCamSN = pCam->getCamSceneNode();
	if (!pCamSN) return;

	Matrix4x4 camWorld = pCamSN->m_worldTransform;
	Vector3 camPos = camWorld.getPos();

	// Get My Position
	if (!m_hSceneNode.isValid()) return;
	PE::Components::SceneNode *pSN = m_hSceneNode.getObject<PE::Components::SceneNode>();
	Vector3 myPos = pSN->m_worldTransform.getPos();

	// Calculate distance squared
	float distSq = (camPos - myPos).lengthSqr();

	// Determine frames to skip (These thresholds should be tuned)
	int framesToSkip = 0;
	if (distSq > 1600.0f) // > 40 units
		framesToSkip = 60; // EXTREME TEST: Update once per second (at 60fps)
	else if (distSq > 900.0f) // > 30 units
		framesToSkip = 30; // Update every 30 frames (~2fps)
	else if (distSq > 400.0f) // > 20 units
		framesToSkip = 15; // Update every 15 frames (~4fps)
	else if (distSq > 100.0f) // > 10 units
		framesToSkip = 5; // Update every 5 frames (~12fps)
	
	if (framesToSkip != m_prevFramesToSkip)
	{
		PEINFO("ANIM LOD: Dist: %.2f (Sq: %.2f). Changing framesToSkip from %d to %d\n", sqrt(distSq), distSq, m_prevFramesToSkip, framesToSkip);
		m_prevFramesToSkip = framesToSkip;
	}

	// Apply to AnimSM
	if (m_hAnimSM.isValid())
	{
		PE::Components::DefaultAnimationSM *pAnimSM = m_hAnimSM.getObject<PE::Components::DefaultAnimationSM>();
		pAnimSM->m_framesToSkip = framesToSkip;
	}
}

}; // namespace Components
}; // namespace CharacterControl

