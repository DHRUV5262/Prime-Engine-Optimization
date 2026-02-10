#include "TankLODComponent.h"
#include "PrimeEngine/Events/StandardEvents.h"
#include "PrimeEngine/Scene/CameraManager.h"
#include "PrimeEngine/Scene/SceneNode.h"
#include "PrimeEngine/GameObjectModel/Camera.h"
#include "PrimeEngine/Scene/CameraSceneNode.h"
#include "PrimeEngine/Lua/LuaEnvironment.h"
#include "PrimeEngine/Scene/MeshInstance.h"

namespace CharacterControl {
namespace Components {

PE_IMPLEMENT_CLASS1(TankLODComponent, PE::Components::Component);

TankLODComponent::TankLODComponent(PE::GameContext &context, PE::MemoryArena arena, PE::Handle hMyself, 
	PE::Handle hSceneNode, PE::Handle hHighPoly, PE::Handle hLowPoly)
	: PE::Components::Component(context, arena, hMyself), m_hSceneNode(hSceneNode), m_hHighPoly(hHighPoly), m_hLowPoly(hLowPoly), m_isHighPoly(true)
{
}

void TankLODComponent::addDefaultComponents()
{
	Component::addDefaultComponents();
	PE_REGISTER_EVENT_HANDLER(PE::Events::Event_UPDATE, TankLODComponent::do_UPDATE);
}

void TankLODComponent::do_UPDATE(PE::Events::Event *pEvt)
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

	// Get My Position (from stored SceneNode)
	if (!m_hSceneNode.isValid()) return;

	PE::Components::SceneNode *pSN = m_hSceneNode.getObject<PE::Components::SceneNode>();
	Vector3 myPos = pSN->m_worldTransform.getPos();

	// Calculate distance squared
	float distSq = (camPos - myPos).lengthSqr();
	
	// Threshold squared (e.g., 20 units)
	float thresholdSq = 20.0f * 20.0f; // 20 units distance

	bool shouldBeHigh = distSq < thresholdSq;

	// Update logic to switch meshes
	// Optimize: Only update if state changes
	if (shouldBeHigh != m_isHighPoly) 
	{
		// FIX: Use separate variables and check for null before using
		// Cast to MeshInstance directly since we know they are MeshInstances
		PE::Components::MeshInstance *pHigh = m_hHighPoly.isValid() ? m_hHighPoly.getObject<PE::Components::MeshInstance>() : NULL;
		PE::Components::MeshInstance *pLow = m_hLowPoly.isValid() ? m_hLowPoly.getObject<PE::Components::MeshInstance>() : NULL;

		PEINFO("LOD SWITCH: DistSq: %f, Threshold: %f. Switching to %s\n", distSq, thresholdSq, shouldBeHigh ? "High Poly" : "Low Poly");

		if (shouldBeHigh)
		{
			if(pHigh) {
				pHigh->setEnabled(true);
				pHigh->m_culledOut = false;
			}
			if(pLow) {
				pLow->setEnabled(false);
				pLow->m_culledOut = true; // Manually cull
			}
		}
		else
		{
			if(pHigh) {
				pHigh->setEnabled(false);
				pHigh->m_culledOut = true; // Manually cull
			}
			if(pLow) {
				pLow->setEnabled(true);
				pLow->m_culledOut = false;
			}
		}
		m_isHighPoly = shouldBeHigh;
	}
	else
	{
		// Initialization check: ensure correct state on startup or if somehow state became inconsistent
		// This logic runs only when state variable matches what we expect, but we want to verify actual mesh state
		PE::Components::MeshInstance *pHigh = m_hHighPoly.isValid() ? m_hHighPoly.getObject<PE::Components::MeshInstance>() : NULL;
		PE::Components::MeshInstance *pLow = m_hLowPoly.isValid() ? m_hLowPoly.getObject<PE::Components::MeshInstance>() : NULL;
		
		if (m_isHighPoly)
		{
			// If we think we are High Poly, ensure Low is culled
			if (pLow && !pLow->m_culledOut) 
			{
				pLow->setEnabled(false);
				pLow->m_culledOut = true;
			}
			if (pHigh && pHigh->m_culledOut)
			{
				pHigh->setEnabled(true);
				pHigh->m_culledOut = false;
			}
		}
		else
		{
			// If we think we are Low Poly, ensure High is culled
			if (pHigh && !pHigh->m_culledOut)
			{
				pHigh->setEnabled(false);
				pHigh->m_culledOut = true;
			}
			if (pLow && pLow->m_culledOut)
			{
				pLow->setEnabled(true);
				pLow->m_culledOut = false;
			}
		}
	}
}

}; // namespace Components
}; // namespace CharacterControl
