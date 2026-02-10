#ifndef _CHARACTER_CONTROL_TANK_LOD_COMPONENT_H_
#define _CHARACTER_CONTROL_TANK_LOD_COMPONENT_H_

#include "PrimeEngine/Events/Component.h"
#include "PrimeEngine/Math/Vector3.h"

namespace CharacterControl {
namespace Components {

struct TankLODComponent : public PE::Components::Component
{
	PE_DECLARE_CLASS(TankLODComponent);

	TankLODComponent(PE::GameContext &context, PE::MemoryArena arena, PE::Handle hMyself, 
		PE::Handle hSceneNode, PE::Handle hHighPoly, PE::Handle hLowPoly);

	virtual ~TankLODComponent() {}

	virtual void addDefaultComponents();

	PE_DECLARE_IMPLEMENT_EVENT_HANDLER_WRAPPER(do_UPDATE);
	virtual void do_UPDATE(PE::Events::Event *pEvt);

	PE::Handle m_hSceneNode; // Store handle to the SceneNode (for position)
	PE::Handle m_hHighPoly;
	PE::Handle m_hLowPoly;
	PE::Handle m_hAnimSM; // Handle to DefaultAnimationSM
	bool m_isHighPoly;
};

}; // namespace Components
}; // namespace CharacterControl

#endif
