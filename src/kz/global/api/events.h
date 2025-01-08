#pragma once

#include "common.h"

#include "kz/global/api/maps.h"
#include "kz/global/api/records.h"

namespace KZ::API::events
{
	struct PlayerJoin
	{
		u64 steamId;
		std::string name;
		std::string ipAddress;

		bool ToJson(Json &json) const
		{
			json.Set("id", this->steamId);
			json.Set("name", this->name);
			json.Set("ip_address", this->ipAddress);

			return true;
		}
	};

	struct PlayerJoinAck
	{
		bool isBanned;
		Json preferences;

		bool FromJson(const Json &json)
		{
			if (!json.Get("is_banned", this->isBanned))
			{
				return false;
			}

			if (!json.Get("preferences", this->preferences))
			{
				return false;
			}

			return true;
		}
	};

	struct PlayerLeave
	{
		u64 steamId;
		std::string name;
		Json preferences;

		bool ToJson(Json &json) const
		{
			json.Set("id", this->steamId);
			json.Set("name", this->name);
			json.Set("preferences", this->preferences);

			return true;
		}
	};

	struct MapChange
	{
		std::string newMap;

		MapChange(const char *mapName) : newMap(mapName) {}

		bool ToJson(Json &json) const
		{
			json.Set("new_map", this->newMap);

			return true;
		}
	};

	struct MapInfo
	{
		std::optional<KZ::API::Map> map;

		bool FromJson(const Json &json)
		{
			if (!json.Get("map", this->map))
			{
				return false;
			}

			return true;
		}
	};

	struct NewRecord
	{
		u64 playerId;
		u16 filterId;
		std::vector<std::string> styles;
		u32 teleports;
		f64 time;

		bool ToJson(Json &json) const
		{
			json.Set("player_id", this->playerId);
			json.Set("filter_id", this->filterId);
			json.Set("styles", this->styles);
			json.Set("teleports", this->teleports);
			json.Set("time", this->time);

			return true;
		}
	};

	struct NewRecordAck
	{
		struct RecordData
		{
			bool isFirstRun {};
			u32 rank {};
			f64 points = -1;
			u64 leaderboardSize {};
		};

		u32 recordId {};
		f64 playerRating {};
		RecordData nubData {};
		RecordData proData {};

		bool FromJson(const Json &json)
		{
			if (!json.Get("record_id", this->recordId))
			{
				return false;
			}

			if (!json.Get("player_rating", this->playerRating))
			{
				return false;
			}

			if (!json.Get("is_first_nub_record", this->nubData.isFirstRun))
			{
				return false;
			}

			if (json.Get("nub_rank", this->nubData.rank))
			{
				if (!json.Get("nub_points", this->nubData.points))
				{
					return false;
				}

				if (!json.Get("nub_leaderboard_size", this->nubData.leaderboardSize))
				{
					return false;
				}
			}

			if (!json.Get("is_first_pro_record", this->proData.isFirstRun))
			{
				return false;
			}

			if (json.Get("pro_rank", this->proData.rank))
			{
				if (!json.Get("pro_points", this->proData.points))
				{
					return false;
				}

				if (!json.Get("pro_leaderboard_size", this->proData.leaderboardSize))
				{
					return false;
				}
			}

			return true;
		}
	};

	struct WantPlayerRecords
	{
		u16 mapId;
		u64 playerId;

		bool ToJson(Json &json) const
		{
			json.Set("map_id", this->mapId);
			json.Set("player_id", this->playerId);

			return true;
		}
	};

	struct PlayerRecords
	{
		std::vector<Record> records {};

		bool FromJson(const Json &json)
		{
			return json.Get("records", this->records);
		}
	};

	struct WantWorldRecords
	{
		u16 mapId;

		WantWorldRecords(u16 mapId) : mapId(mapId) {}

		bool ToJson(Json &json) const
		{
			return json.Set("map_id", this->mapId);
		}
	};

	struct WorldRecords
	{
		std::vector<Record> records {};

		bool FromJson(const Json &json)
		{
			return json.Get("records", this->records);
		}
	};
} // namespace KZ::API::events
