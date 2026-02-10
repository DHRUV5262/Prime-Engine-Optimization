#include "PrimeEngine/APIAbstraction/APIAbstractionDefines.h"

#include "PrimeEngine/Lua/LuaEnvironment.h"
#include "PrimeEngine/Lua/EventGlue/EventDataCreators.h"


#include "WayPoint.h"


using namespace PE;
using namespace PE::Components;
using namespace CharacterControl::Events;

namespace CharacterControl{
namespace Events{

PE_IMPLEMENT_CLASS1(Event_CREATE_WAYPOINT, PE::Events::Event);

void Event_CREATE_WAYPOINT::SetLuaFunctions(PE::Components::LuaEnvironment *pLuaEnv, lua_State *luaVM)
{
	static const struct luaL_Reg l_Event_CREATE_WAYPOINT[] = {
		{"Construct", l_Construct},
		{NULL, NULL} // sentinel
	};

	// register the functions in current lua table which is the table for Event_CreateSoldierNPC
	luaL_register(luaVM, 0, l_Event_CREATE_WAYPOINT);
}

int Event_CREATE_WAYPOINT::l_Construct(lua_State* luaVM)
{
    PE::Handle h("EVENT", sizeof(Event_CREATE_WAYPOINT));
	Event_CREATE_WAYPOINT *pEvt = new(h) Event_CREATE_WAYPOINT;

	// get arguments from stack
	int numArgs, numArgsConst;
	numArgs = numArgsConst = 15;

	const char* wayPointName = lua_tostring(luaVM, -numArgs--);
	
	// Read next waypoint names (as a Lua table)
	int nextWaypointsIdx = -numArgs--;
	pEvt->m_numNextWayPoints = 0;

	if (lua_isstring(luaVM, nextWaypointsIdx)) {
		const char* wpNames = lua_tostring(luaVM, nextWaypointsIdx);
		// Split by space
		char buf[96]; // enough for 3 names of 32 chars each
		StringOps::writeToString(wpNames, buf, 96);

		char* token = strtok(buf, " ");
		int idx = 0;
		while (token && idx < 3) {
			StringOps::writeToString(token, pEvt->m_nextWaypointNames[idx], 32);
			pEvt->m_numNextWayPoints++;
			token = strtok(nullptr, " ");
			idx++;
		}
	}
	else if (lua_istable(luaVM, nextWaypointsIdx)) {
		int len = lua_objlen(luaVM, nextWaypointsIdx);
		for (int i = 1; i <= len && i <= 3; ++i) {
			lua_pushinteger(luaVM, i);
			lua_gettable(luaVM, nextWaypointsIdx);
			const char* wpName = lua_tostring(luaVM, -1);
			if (wpName) {
				StringOps::writeToString(wpName, pEvt->m_nextWaypointNames[pEvt->m_numNextWayPoints], 32);
				pEvt->m_numNextWayPoints++;
			}
			lua_pop(luaVM, 1);
		}
	}

	float positionFactor = 1.0f / 100.0f;
	Vector3 pos, u, v, n;
	pos.m_x = (float)lua_tonumber(luaVM, -numArgs--) * positionFactor;
	pos.m_y = (float)lua_tonumber(luaVM, -numArgs--) * positionFactor;
	pos.m_z = (float)lua_tonumber(luaVM, -numArgs--) * positionFactor;

	u.m_x = (float)lua_tonumber(luaVM, -numArgs--); u.m_y = (float)lua_tonumber(luaVM, -numArgs--); u.m_z = (float)lua_tonumber(luaVM, -numArgs--);
	v.m_x = (float)lua_tonumber(luaVM, -numArgs--); v.m_y = (float)lua_tonumber(luaVM, -numArgs--); v.m_z = (float)lua_tonumber(luaVM, -numArgs--);
	n.m_x = (float)lua_tonumber(luaVM, -numArgs--); n.m_y = (float)lua_tonumber(luaVM, -numArgs--); n.m_z = (float)lua_tonumber(luaVM, -numArgs--);

	pEvt->m_peuuid = LuaGlue::readPEUUID(luaVM, -numArgs--);

	

	// set data values before popping memory off stack
	StringOps::writeToString(wayPointName, pEvt->m_name, 32);
	//StringOps::writeToString(nextWayPointName, pEvt->m_nextWaypointName, 32);
	
	lua_pop(luaVM, numArgsConst); //Second arg is a count of how many to pop

	pEvt->m_base.loadIdentity();
	pEvt->m_base.setPos(pos);
	pEvt->m_base.setU(u);
	pEvt->m_base.setV(v);
	pEvt->m_base.setN(n);

	LuaGlue::pushTableBuiltFromHandle(luaVM, h); 

	return 1;
}
};
namespace Components {

PE_IMPLEMENT_CLASS1(WayPoint, Component);

// create waypoint form creation event
WayPoint::WayPoint(PE::GameContext &context, PE::MemoryArena arena, PE::Handle hMyself, const Events::Event_CREATE_WAYPOINT *pEvt)
: Component(context, arena, hMyself)
{
	StringOps::writeToString(pEvt->m_name, m_name, 32);

	m_numNextWayPoints = pEvt->m_numNextWayPoints;
	for(int i = 0; i < m_numNextWayPoints; i++)
		StringOps::writeToString(pEvt->m_nextWaypointNames[i], m_nextWayPointNames[i], 32);


	m_base = pEvt->m_base;
}

void WayPoint::addDefaultComponents()
{
	Component::addDefaultComponents();

	// custom methods of this component
}

}; // namespace Components
}; // namespace CharacterControl
