#include "kz_db.h"
#include "queries/maps.h"
#include "vendor/sql_mm/src/public/sql_mm.h"

/*
	Inserts the map information into the database.
	Retrieves the MapID of the map and stores it in a global variable.
*/

using namespace KZ::Database;

static_global bool mapSetUp = false;
i32 KZDatabaseService::currentMapID {};

bool KZDatabaseService::IsMapSetUp()
{
	return mapSetUp;
}

void KZDatabaseService::SetupMap()
{
	mapSetUp = false;
	if (!KZDatabaseService::IsReady())
	{
		META_CONPRINTF("KZDatabaseService::SetupMap() called too early!\n");
		return;
	}

	Transaction txn;
	char query[1024];
	auto mapName = g_pKZUtils->GetServerGlobals()->mapname.ToCStr();
	auto databaseType = KZDatabaseService::GetDatabaseType();
	switch (databaseType)
	{
		case DatabaseType_SQLite:
		{
			V_snprintf(query, sizeof(query), sqlite_maps_insert, mapName);
			txn.queries.push_back(query);
			V_snprintf(query, sizeof(query), sqlite_maps_update, mapName);
			txn.queries.push_back(query);
			break;
		}
		case DatabaseType_MySQL:
		{
			V_snprintf(query, sizeof(query), mysql_maps_upsert, mapName);
			txn.queries.push_back(query);
			break;
		}
		default:
		{
			// This shouldn't happen.
			query[0] = 0;
			break;
		}
	}

	V_snprintf(query, sizeof(query), sql_maps_findid, mapName, mapName);
	txn.queries.push_back(query);
	// clang-format off
	KZDatabaseService::GetDatabaseConnection()->ExecuteTransaction(
		txn, 
		[databaseType](std::vector<ISQLQuery *> queries) 
		{
			switch (databaseType)
			{
				case DatabaseType_SQLite:
				{
					auto resultSet = queries[2]->GetResultSet();
					if (resultSet->FetchRow())
					{
						KZDatabaseService::currentMapID = resultSet->GetInt(0);
					}
					break;
				}
				case DatabaseType_MySQL:
				{
					auto resultSet = queries[1]->GetResultSet();
					if (resultSet->FetchRow())
					{
						KZDatabaseService::currentMapID = resultSet->GetInt(0);
					}
					break;
				}
				default:
				{
					// This shouldn't happen.
					break;
				}
			}
			META_CONPRINTF("[KZDB] Map setup successful, current map ID: %i\n", KZDatabaseService::currentMapID);
			CALL_FORWARD(eventListeners, OnMapSetup);
			KZDatabaseService::SetupCourses();
		},
		OnGenericTxnFailure);
	// clang-format on
}
