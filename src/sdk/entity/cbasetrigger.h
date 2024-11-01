#pragma once
#include "cbasemodelentity.h"
#include "utlsymbollarge.h"

class CBaseTrigger : public CBaseModelEntity
{
public:
	DECLARE_SCHEMA_CLASS(CBaseTrigger)

	SCHEMA_FIELD(CUtlSymbolLarge, m_iFilterName)
	SCHEMA_FIELD(CHandle<CBaseEntity>, m_hFilter)
};
