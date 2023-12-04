#pragma once

#include "common.h"
#include "cgameresourceserviceserver.h"
#include "cschemasystem.h"
#include "igameevents.h"
#include "igameeventsystem.h"

namespace interfaces
{
	bool Initialize(ISmmAPI *ismm, char *error, size_t maxlen);

	inline CGameResourceService *pGameResourceServiceServer = nullptr;
	inline CSchemaSystem *pSchemaSystem = nullptr;
	inline IVEngineServer2 *pEngine = nullptr;
	inline ISource2Server *pServer = nullptr;
	inline IGameEventManager2 *pGameEventManager = nullptr;
	inline IGameEventSystem *pGameEventSystem = nullptr;
	inline void *pPhysicsQuery = nullptr;
}