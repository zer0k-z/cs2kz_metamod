#pragma once

#include "common.h"
#include "utils/json.h"

namespace KZ::API
{
	class PlayerInfo
	{
	public:
		PlayerInfo() = default;

		PlayerInfo(u64 id, std::string name) : id(std::to_string(id)), name(name) {}

		PlayerInfo(std::string id, std::string name) : id(id), name(name) {}

		bool ToJson(Json &json) const
		{
			json.Set("id", this->id);
			json.Set("name", this->name);
			return true;
		}

		bool FromJson(const Json &json)
		{
			if (!json.Get("id", this->id))
			{
				return false;
			}

			if (!json.Get("name", this->name))
			{
				return false;
			}

			return true;
		}

	private:
		std::string id {};
		std::string name {};
	};
} // namespace KZ::API
