#ifndef _CHARACTER_CONTROL_ANIMATION_LOD_COMPONENT_H_
#define _CHARACTER_CONTROL_ANIMATION_LOD_COMPONENT_H_

#include "PrimeEngine/Events/Component.h"

namespace CharacterControl {
namespace Components {

struct AnimationLODComponent : public PE::Components::Component
{
	PE_DECLARE_CLASS(AnimationLODComponent);

	AnimationLODComponent(PE::GameContext &context, PE::MemoryArena arena, PE::Handle hMyself, 
		PE::Handle hSceneNode, PE::Handle hAnimSM);

	virtual ~AnimationLODComponent() {}

	virtual void addDefaultComponents();

	PE_DECLARE_IMPLEMENT_EVENT_HANDLER_WRAPPER(do_UPDATE);
	virtual void do_UPDATE(PE::Events::Event *pEvt);

	PE::Handle m_hSceneNode;
	PE::Handle m_hAnimSM;
	int m_prevFramesToSkip;
};

}; // namespace Components
}; // namespace CharacterControl

#endif

