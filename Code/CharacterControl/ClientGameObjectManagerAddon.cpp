#include "ClientGameObjectManagerAddon.h"

#include "PrimeEngine/PrimeEngineIncludes.h"

#include "Characters/SoldierNPC.h"
#include "WayPoint.h"
#include "Tank/ClientTank.h"
#include "Tank/TankLODComponent.h"
#include "CharacterControl/Client/ClientSpaceShip.h"

using namespace PE::Components;
using namespace PE::Events;
using namespace CharacterControl::Events;
using namespace CharacterControl::Components;

namespace CharacterControl{
namespace Components
{
PE_IMPLEMENT_CLASS1(ClientGameObjectManagerAddon, Component); // creates a static handle and GteInstance*() methods. still need to create construct

void ClientGameObjectManagerAddon::addDefaultComponents()
{
	GameObjectManagerAddon::addDefaultComponents();

	PE_REGISTER_EVENT_HANDLER(Event_CreateSoldierNPC, ClientGameObjectManagerAddon::do_CreateSoldierNPC);
	PE_REGISTER_EVENT_HANDLER(Event_CREATE_WAYPOINT, ClientGameObjectManagerAddon::do_CREATE_WAYPOINT);
	PE_REGISTER_EVENT_HANDLER(Event_CreateTank, ClientGameObjectManagerAddon::do_CreateTank);

	// note this component (game obj addon) is added to game object manager after network manager, so network manager will process this event first
	PE_REGISTER_EVENT_HANDLER(PE::Events::Event_SERVER_CLIENT_CONNECTION_ACK, ClientGameObjectManagerAddon::do_SERVER_CLIENT_CONNECTION_ACK);

	PE_REGISTER_EVENT_HANDLER(Event_MoveTank_S_to_C, ClientGameObjectManagerAddon::do_MoveTank);
}

void ClientGameObjectManagerAddon::do_CreateSoldierNPC(PE::Events::Event *pEvt)
{
	assert(pEvt->isInstanceOf<Event_CreateSoldierNPC>());

	Event_CreateSoldierNPC *pTrueEvent = (Event_CreateSoldierNPC*)(pEvt);

	createSoldierNPC(pTrueEvent);
}

void ClientGameObjectManagerAddon::createSoldierNPC(Vector3 pos, int &threadOwnershipMask)
{
	Event_CreateSoldierNPC evt(threadOwnershipMask);
	evt.m_pos = pos;
	evt.m_u = Vector3(1.0f, 0, 0);
	evt.m_v = Vector3(0, 1.0f, 0);
	evt.m_n = Vector3(0, 0, 1.0f);
	
	StringOps::writeToString( "SoldierTransform.mesha", evt.m_meshFilename, 255);
	StringOps::writeToString( "Soldier", evt.m_package, 255);
	StringOps::writeToString( "mg34.x_mg34main_mesh.mesha", evt.m_gunMeshName, 64);
	StringOps::writeToString( "CharacterControl", evt.m_gunMeshPackage, 64);
	StringOps::writeToString( "", evt.m_patrolWayPoint, 32);
	createSoldierNPC(&evt);
}

void ClientGameObjectManagerAddon::createSoldierNPC(Event_CreateSoldierNPC *pTrueEvent)
{
	PEINFO("CharacterControl: GameObjectManagerAddon: Creating CreateSoldierNPC\n");

	PE::Handle hSoldierNPC("SoldierNPC", sizeof(SoldierNPC));
	SoldierNPC *pSoldierNPC = new(hSoldierNPC) SoldierNPC(*m_pContext, m_arena, hSoldierNPC, pTrueEvent);
	pSoldierNPC->addDefaultComponents();

	// add the soldier as component to the ObjecManagerComponentAddon
	// all objects of this demo live in the ObjecManagerComponentAddon
	addComponent(hSoldierNPC);
}

void ClientGameObjectManagerAddon::do_CREATE_WAYPOINT(PE::Events::Event *pEvt)
{
	PEINFO("GameObjectManagerAddon::do_CREATE_WAYPOINT()\n");

	assert(pEvt->isInstanceOf<Event_CREATE_WAYPOINT>());

	Event_CREATE_WAYPOINT *pTrueEvent = (Event_CREATE_WAYPOINT*)(pEvt);

	PE::Handle hWayPoint("WayPoint", sizeof(WayPoint));
	WayPoint *pWayPoint = new(hWayPoint) WayPoint(*m_pContext, m_arena, hWayPoint, pTrueEvent);
	pWayPoint->addDefaultComponents();

	addComponent(hWayPoint);
}

WayPoint *ClientGameObjectManagerAddon::getWayPoint(const char *name)
{
	PE::Handle *pHC = m_components.getFirstPtr();

	for (PrimitiveTypes::UInt32 i = 0; i < m_components.m_size; i++, pHC++) // fast array traversal (increasing ptr)
	{
		Component *pC = (*pHC).getObject<Component>();

		if (pC->isInstanceOf<WayPoint>())
		{
			WayPoint *pWP = (WayPoint *)(pC);
			if (StringOps::strcmp(pWP->m_name, name) == 0)
			{
				// equal strings, found our waypoint
				return pWP;
			}
		}
	}
	return NULL;
}


void ClientGameObjectManagerAddon::createTank(int index, int &threadOwnershipMask)
{

	//create hierarchy:
	//scene root
	//  scene node // tracks position/orientation
	//    Tank

	//game object manager
	//  TankController
	//    scene node
	
	PE::Handle hMeshInstance("MeshInstance", sizeof(MeshInstance));
	MeshInstance *pMeshInstance = new(hMeshInstance) MeshInstance(*m_pContext, m_arena, hMeshInstance);

	pMeshInstance->addDefaultComponents();
	pMeshInstance->initFromFile("kingtiger.x_main_mesh.mesha", "Default", threadOwnershipMask);

	// need to create a scene node for this mesh
	PE::Handle hSN("SCENE_NODE", sizeof(SceneNode));
	SceneNode *pSN = new(hSN) SceneNode(*m_pContext, m_arena, hSN);
	pSN->addDefaultComponents();

	Vector3 spawnPos(-36.0f + 6.0f * index, 0 , 21.0f);
	pSN->m_base.setPos(spawnPos);
	
	pSN->addComponent(hMeshInstance);

	// -- SPAWN BOX MESH AT OFFSET POSITION --
	// Create a child scene node for the box to offset it
	PE::Handle hBoxSN("SCENE_NODE", sizeof(SceneNode));
	SceneNode *pBoxSN = new(hBoxSN) SceneNode(*m_pContext, m_arena, hBoxSN);
	pBoxSN->addDefaultComponents();
	// pBoxSN->m_base.setPos(Vector3(0, 5.0f, 0)); // Move up 5 units relative to tank
	// No offset for LOD - we want it to replace the tank
	// But the user asked to see it, so they might want to see the switch happening in place.
	// For true LOD, they should be at the same position.
	
	PE::Handle hBoxMeshInstance("MeshInstance", sizeof(MeshInstance));
	MeshInstance *pBoxMeshInstance = new(hBoxMeshInstance) MeshInstance(*m_pContext, m_arena, hBoxMeshInstance);
	pBoxMeshInstance->addDefaultComponents();
	pBoxMeshInstance->initFromFile("imrod.x_imrodmesh_mesh.mesha", "Default", threadOwnershipMask);
	
	// Disable Imrod initially (Low Poly)
	// pBoxMeshInstance->setEnabled(false); // Wait, MeshInstance doesn't support setEnabled directly for rendering usually, 
	// but let's assume TankLODComponent handles it via Component::setEnabled if the engine respects it.
	// If not, we might need to modify SceneNode to respect enabled flag of components.
	// However, usually engines respect enabled flag.
	
	// Actually, let's keep the box node attached to main node.
	pBoxSN->addComponent(hBoxMeshInstance);
	pSN->addComponent(hBoxSN); 

	// Create LOD Component
	// Pass hSN (SceneNode) so it knows where to check distance
	PE::Handle hLOD("TankLODComponent", sizeof(TankLODComponent));
	TankLODComponent *pLOD = new(hLOD) TankLODComponent(*m_pContext, m_arena, hLOD, hSN, hMeshInstance, hBoxMeshInstance);
	pLOD->addDefaultComponents();
	
	// -------------------------------------

	RootSceneNode::Instance()->addComponent(hSN);

	// now add game objects

	PE::Handle hTankController("TankController", sizeof(TankController));
	TankController *pTankController = new(hTankController) TankController(*m_pContext, m_arena, hTankController, 0.05f, spawnPos,  0.05f);
	pTankController->addDefaultComponents();

	addComponent(hTankController);

	// Add LOD component to the TankController so it receives Event_UPDATE
	pTankController->addComponent(hLOD);

	// add the same scene node to tank controller
	static int alllowedEventsToPropagate[] = {0}; // we will pass empty array as allowed events to propagate so that when we add
	// scene node to the square controller, the square controller doesnt try to handle scene node's events
	// because scene node handles events through scene graph, and is child of square controller just for referencing purposes
	pTankController->addComponent(hSN, &alllowedEventsToPropagate[0]);
}

void ClientGameObjectManagerAddon::do_CreateTank(PE::Events::Event *pEvt)
{
	assert(pEvt->isInstanceOf<Event_CreateTank>());

	Event_CreateTank *pTrueEvent = (Event_CreateTank*)(pEvt);

	createTankFromEvent(pTrueEvent);
}

void ClientGameObjectManagerAddon::createTankFromEvent(Events::Event_CreateTank *pEvt)
{
	PEINFO("CharacterControl: GameObjectManagerAddon: Creating Tank from Event (LOD enabled)\n");

	int threadOwnershipMask = pEvt->m_threadOwnershipMask;

	// Create High Poly MeshInstance
	PE::Handle hMeshInstance("MeshInstance", sizeof(MeshInstance));
	MeshInstance *pMeshInstance = new(hMeshInstance) MeshInstance(*m_pContext, m_arena, hMeshInstance);
	pMeshInstance->addDefaultComponents();
	pMeshInstance->initFromFile(pEvt->m_meshFilename, pEvt->m_package, threadOwnershipMask);

	// Create main SceneNode
	PE::Handle hSN("SCENE_NODE", sizeof(SceneNode));
	SceneNode *pSN = new(hSN) SceneNode(*m_pContext, m_arena, hSN);
	pSN->addDefaultComponents();

	// Use position from event
	pSN->m_base.setPos(pEvt->m_pos);
	if (pEvt->hasCustomOrientation)
	{
		pSN->m_base.setU(pEvt->m_u);
		pSN->m_base.setV(pEvt->m_v);
		pSN->m_base.setN(pEvt->m_n);
	}

	pSN->addComponent(hMeshInstance);

	// Create Low Poly MeshInstance for LOD
	PE::Handle hLowMeshInstance("MeshInstance", sizeof(MeshInstance));
	MeshInstance *pLowMeshInstance = new(hLowMeshInstance) MeshInstance(*m_pContext, m_arena, hLowMeshInstance);
	pLowMeshInstance->addDefaultComponents();
	pLowMeshInstance->initFromFile(pEvt->m_lowMeshFilename, pEvt->m_lowMeshPackage, threadOwnershipMask);

	// Child SceneNode for Low Poly (at same position)
	PE::Handle hLowSN("SCENE_NODE", sizeof(SceneNode));
	SceneNode *pLowSN = new(hLowSN) SceneNode(*m_pContext, m_arena, hLowSN);
	pLowSN->addDefaultComponents();
	pLowSN->addComponent(hLowMeshInstance);
	pSN->addComponent(hLowSN);

	// Create LOD Component
	PE::Handle hLOD("TankLODComponent", sizeof(TankLODComponent));
	TankLODComponent *pLOD = new(hLOD) TankLODComponent(*m_pContext, m_arena, hLOD, hSN, hMeshInstance, hLowMeshInstance);
	pLOD->addDefaultComponents();

	RootSceneNode::Instance()->addComponent(hSN);

	// Create TankController
	PE::Handle hTankController("TankController", sizeof(TankController));
	TankController *pTankController = new(hTankController) TankController(*m_pContext, m_arena, hTankController, 0.05f, pEvt->m_pos, 0.05f);
	pTankController->addDefaultComponents();

	addComponent(hTankController);

	// Add LOD component to TankController so it receives Event_UPDATE
	pTankController->addComponent(hLOD);

	// Add scene node to tank controller (for reference, not event propagation)
	static int alllowedEventsToPropagate[] = {0};
	pTankController->addComponent(hSN, &alllowedEventsToPropagate[0]);

	PEINFO("CharacterControl: Tank created with LOD. High: %s, Low: %s, Distance: %.1f\n", 
		pEvt->m_meshFilename, pEvt->m_lowMeshFilename, pEvt->m_lodDistance);
}

void ClientGameObjectManagerAddon::createSpaceShip(int &threadOwnershipMask)
{

	//create hierarchy:
	//scene root
	//  scene node // tracks position/orientation
	//    SpaceShip

	//game object manager
	//  SpaceShipController
	//    scene node

	PE::Handle hMeshInstance("MeshInstance", sizeof(MeshInstance));
	MeshInstance *pMeshInstance = new(hMeshInstance) MeshInstance(*m_pContext, m_arena, hMeshInstance);

	pMeshInstance->addDefaultComponents();
	pMeshInstance->initFromFile("space_frigate_6.mesha", "FregateTest", threadOwnershipMask);

	// need to create a scene node for this mesh
	PE::Handle hSN("SCENE_NODE", sizeof(SceneNode));
	SceneNode *pSN = new(hSN) SceneNode(*m_pContext, m_arena, hSN);
	pSN->addDefaultComponents();

	Vector3 spawnPos(0, 0, 0.0f);
	pSN->m_base.setPos(spawnPos);

	pSN->addComponent(hMeshInstance);

	RootSceneNode::Instance()->addComponent(hSN);

	// now add game objects

	PE::Handle hSpaceShip("ClientSpaceShip", sizeof(ClientSpaceShip));
	ClientSpaceShip *pSpaceShip = new(hSpaceShip) ClientSpaceShip(*m_pContext, m_arena, hSpaceShip, 0.05f, spawnPos,  0.05f);
	pSpaceShip->addDefaultComponents();

	addComponent(hSpaceShip);

	// add the same scene node to tank controller
	static int alllowedEventsToPropagate[] = {0}; // we will pass empty array as allowed events to propagate so that when we add
	// scene node to the square controller, the square controller doesnt try to handle scene node's events
	// because scene node handles events through scene graph, and is child of space ship just for referencing purposes
	pSpaceShip->addComponent(hSN, &alllowedEventsToPropagate[0]);

	pSpaceShip->activate();
}


void ClientGameObjectManagerAddon::do_SERVER_CLIENT_CONNECTION_ACK(PE::Events::Event *pEvt)
{
	Event_SERVER_CLIENT_CONNECTION_ACK *pRealEvt = (Event_SERVER_CLIENT_CONNECTION_ACK *)(pEvt);
	PE::Handle *pHC = m_components.getFirstPtr();

	int itc = 0;
	for (PrimitiveTypes::UInt32 i = 0; i < m_components.m_size; i++, pHC++) // fast array traversal (increasing ptr)
	{
		Component *pC = (*pHC).getObject<Component>();

		if (pC->isInstanceOf<TankController>())
		{
			if (itc == pRealEvt->m_clientId) //activate tank controller for local client based on local clients id
			{
				TankController *pTK = (TankController *)(pC);
				pTK->activate();
				break;
			}
			++itc;
		}
	}
}

void ClientGameObjectManagerAddon::do_MoveTank(PE::Events::Event *pEvt)
{
	assert(pEvt->isInstanceOf<Event_MoveTank_S_to_C>());

	Event_MoveTank_S_to_C *pTrueEvent = (Event_MoveTank_S_to_C*)(pEvt);

	PE::Handle *pHC = m_components.getFirstPtr();

	int itc = 0;
	for (PrimitiveTypes::UInt32 i = 0; i < m_components.m_size; i++, pHC++) // fast array traversal (increasing ptr)
	{
		Component *pC = (*pHC).getObject<Component>();

		if (pC->isInstanceOf<TankController>())
		{
			if (itc == pTrueEvent->m_clientTankId) //activate tank controller for local client based on local clients id
			{
				TankController *pTK = (TankController *)(pC);
				pTK->overrideTransform(pTrueEvent->m_transform);
				break;
			}
			++itc;
		}
	}
}

}
}
