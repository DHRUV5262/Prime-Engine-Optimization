#include "PrimeEngine/PrimeEngineIncludes.h"
#include "PrimeEngine/Lua/LuaEnvironment.h"
#include "Events.h"

using namespace PE;
namespace CharacterControl {
namespace Events{

PE_IMPLEMENT_CLASS1(Event_CreateSoldierNPC, PE::Events::Event_CREATE_SKELETON);

void Event_CreateSoldierNPC::SetLuaFunctions(PE::Components::LuaEnvironment *pLuaEnv, lua_State *luaVM)
{
	static const struct luaL_Reg l_Event_CreateSoldierNPC[] = {
		{"Construct", l_Construct},
		{NULL, NULL} // sentinel
	};

	// register the functions in current lua table which is the table for Event_CreateSoldierNPC
	luaL_register(luaVM, 0, l_Event_CreateSoldierNPC);
}

int Event_CreateSoldierNPC::l_Construct(lua_State* luaVM)
{
    PE::Handle h("EVENT", sizeof(Event_CreateSoldierNPC));
	
	// get arguments from stack
	int numArgs, numArgsConst;
	numArgs = numArgsConst = 21;

	PE::GameContext *pContext = (PE::GameContext*)(lua_touserdata(luaVM, -numArgs--));

	// this function should only be called frm game thread, so we can use game thread thread owenrship mask
	Event_CreateSoldierNPC *pEvt = new(h) Event_CreateSoldierNPC(pContext->m_gameThreadThreadOwnershipMask);

	const char* name = lua_tostring(luaVM, -numArgs--);
	const char* package = lua_tostring(luaVM, -numArgs--);

	const char* gunMeshName = lua_tostring(luaVM, -numArgs--);
	const char* gunMeshPackage = lua_tostring(luaVM, -numArgs--);

	int needsToShoot = (int)lua_tonumber(luaVM, -numArgs--);
	pEvt->m_needsToShoot = needsToShoot;

	int animationType = (int)lua_tonumber(luaVM, -numArgs--);
	// store animationType on event so code that constructs soldier can pass it to components
	pEvt->m_animationType = animationType;

	// debug: log animationType read from Lua
	{
		char dbg[128];
		sprintf(dbg, "Event_CreateSoldierNPC::l_Construct - animationType = %d\n", animationType);
		OutputDebugStringA(dbg);
	}

	float positionFactor = 1.0f / 100.0f;

	Vector3 playerPos, u, v, n;
	playerPos.m_x = (float)lua_tonumber(luaVM, -numArgs--) * positionFactor;
	playerPos.m_y = (float)lua_tonumber(luaVM, -numArgs--) * positionFactor;
	playerPos.m_z = (float)lua_tonumber(luaVM, -numArgs--) * positionFactor;

	u.m_x = (float)lua_tonumber(luaVM, -numArgs--); u.m_y = (float)lua_tonumber(luaVM, -numArgs--); u.m_z = (float)lua_tonumber(luaVM, -numArgs--);
	v.m_x = (float)lua_tonumber(luaVM, -numArgs--); v.m_y = (float)lua_tonumber(luaVM, -numArgs--); v.m_z = (float)lua_tonumber(luaVM, -numArgs--);
	n.m_x = (float)lua_tonumber(luaVM, -numArgs--); n.m_y = (float)lua_tonumber(luaVM, -numArgs--); n.m_z = (float)lua_tonumber(luaVM, -numArgs--);

	pEvt->m_peuuid = LuaGlue::readPEUUID(luaVM, -numArgs--);


	const char* wayPointName = NULL;

	if (!lua_isnil(luaVM, -numArgs))
	{
		// have patrol waypoint name
		wayPointName = lua_tostring(luaVM, -numArgs--);
	}
	else
		// ignore
		numArgs--;

	// set data values before popping memory off stack
	StringOps::writeToString(name, pEvt->m_meshFilename, 255);
	StringOps::writeToString(package, pEvt->m_package, 255);

	StringOps::writeToString(gunMeshName, pEvt->m_gunMeshName, 64);
	StringOps::writeToString(gunMeshPackage, pEvt->m_gunMeshPackage, 64);
	StringOps::writeToString(wayPointName, pEvt->m_patrolWayPoint, 32);

	lua_pop(luaVM, numArgsConst); //Second arg is a count of how many to pop

	pEvt->hasCustomOrientation = true;
	pEvt->m_pos = playerPos;
	pEvt->m_u = u;
	pEvt->m_v = v;
	pEvt->m_n = n;

	LuaGlue::pushTableBuiltFromHandle(luaVM, h); 

	return 1;
}

PE_IMPLEMENT_CLASS1(Event_MoveTank_C_to_S, Event);

Event_MoveTank_C_to_S::Event_MoveTank_C_to_S(PE::GameContext &context)
: Networkable(context, this)
{

}

void *Event_MoveTank_C_to_S::FactoryConstruct(PE::GameContext& context, PE::MemoryArena arena)
{
	Event_MoveTank_C_to_S *pEvt = new (arena) Event_MoveTank_C_to_S(context);
	return pEvt;
}

int Event_MoveTank_C_to_S::packCreationData(char *pDataStream)
{
	return PE::Components::StreamManager::WriteMatrix4x4(m_transform, pDataStream);
}

int Event_MoveTank_C_to_S::constructFromStream(char *pDataStream)
{
	int read = 0;
	read += PE::Components::StreamManager::ReadMatrix4x4(&pDataStream[read], m_transform);
	return read;
}


PE_IMPLEMENT_CLASS1(Event_MoveTank_S_to_C, Event_MoveTank_C_to_S);

Event_MoveTank_S_to_C::Event_MoveTank_S_to_C(PE::GameContext &context)
: Event_MoveTank_C_to_S(context)
{

}

void *Event_MoveTank_S_to_C::FactoryConstruct(PE::GameContext& context, PE::MemoryArena arena)
{
	Event_MoveTank_S_to_C *pEvt = new (arena) Event_MoveTank_S_to_C(context);
	return pEvt;
}

int Event_MoveTank_S_to_C::packCreationData(char *pDataStream)
{
	int size = 0;
	size += Event_MoveTank_C_to_S::packCreationData(&pDataStream[size]);
	size += PE::Components::StreamManager::WriteInt32(m_clientTankId, &pDataStream[size]);
	return size;
}

int Event_MoveTank_S_to_C::constructFromStream(char *pDataStream)
{
	int read = 0;
	read += Event_MoveTank_C_to_S::constructFromStream(&pDataStream[read]);
	read += PE::Components::StreamManager::ReadInt32(&pDataStream[read], m_clientTankId);
	return read;
}

PE_IMPLEMENT_CLASS1(Event_Tank_Throttle, Event);

PE_IMPLEMENT_CLASS1(Event_Tank_Turn, Event);

PE_IMPLEMENT_CLASS1(Event_CreateTank, PE::Events::Event_CREATE_MESH);

void Event_CreateTank::SetLuaFunctions(PE::Components::LuaEnvironment *pLuaEnv, lua_State *luaVM)
{
	static const struct luaL_Reg l_Event_CreateTank[] = {
		{"Construct", l_Construct},
		{NULL, NULL} // sentinel
	};

	// register the functions in current lua table which is the table for Event_CreateTank
	luaL_register(luaVM, 0, l_Event_CreateTank);
}

int Event_CreateTank::l_Construct(lua_State* luaVM)
{
	PE::Handle h("EVENT", sizeof(Event_CreateTank));
	
	// get arguments from stack
	// Args: context(1), meshName(1), meshPackage(1), lowMeshName(1), lowMeshPackage(1), lodDistance(1), pos(3), u(3), v(3), n(3), peuuid(1) = 19 total
	int numArgs, numArgsConst;
	numArgs = numArgsConst = 19;

	PE::GameContext *pContext = (PE::GameContext*)(lua_touserdata(luaVM, -numArgs--));

	// this function should only be called from game thread
	Event_CreateTank *pEvt = new(h) Event_CreateTank(pContext->m_gameThreadThreadOwnershipMask);

	const char* meshName = lua_tostring(luaVM, -numArgs--);
	const char* meshPackage = lua_tostring(luaVM, -numArgs--);

	const char* lowMeshName = lua_tostring(luaVM, -numArgs--);
	const char* lowMeshPackage = lua_tostring(luaVM, -numArgs--);

	float lodDistance = (float)lua_tonumber(luaVM, -numArgs--);

	float positionFactor = 1.0f / 100.0f;

	Vector3 pos, u, v, n;
	pos.m_x = (float)lua_tonumber(luaVM, -numArgs--) * positionFactor;
	pos.m_y = (float)lua_tonumber(luaVM, -numArgs--) * positionFactor;
	pos.m_z = (float)lua_tonumber(luaVM, -numArgs--) * positionFactor;

	u.m_x = (float)lua_tonumber(luaVM, -numArgs--); u.m_y = (float)lua_tonumber(luaVM, -numArgs--); u.m_z = (float)lua_tonumber(luaVM, -numArgs--);
	v.m_x = (float)lua_tonumber(luaVM, -numArgs--); v.m_y = (float)lua_tonumber(luaVM, -numArgs--); v.m_z = (float)lua_tonumber(luaVM, -numArgs--);
	n.m_x = (float)lua_tonumber(luaVM, -numArgs--); n.m_y = (float)lua_tonumber(luaVM, -numArgs--); n.m_z = (float)lua_tonumber(luaVM, -numArgs--);

	// peuuid might be nil
	if (!lua_isnil(luaVM, -numArgs))
		pEvt->m_peuuid = LuaGlue::readPEUUID(luaVM, -numArgs--);
	else
		numArgs--;

	// Store data on event
	StringOps::writeToString(meshName, pEvt->m_meshFilename, 255);
	StringOps::writeToString(meshPackage, pEvt->m_package, 255);
	StringOps::writeToString(lowMeshName, pEvt->m_lowMeshFilename, 255);
	StringOps::writeToString(lowMeshPackage, pEvt->m_lowMeshPackage, 255);
	pEvt->m_lodDistance = lodDistance;

	pEvt->hasCustomOrientation = true;
	pEvt->m_pos = pos;
	pEvt->m_u = u;
	pEvt->m_v = v;
	pEvt->m_n = n;

	lua_pop(luaVM, numArgsConst);

	LuaGlue::pushTableBuiltFromHandle(luaVM, h); 

	return 1;
}

};
};
