// required for ws library
#ifdef _WIN32
#pragma comment(lib, "Ws2_32.Lib")
#pragma comment(lib, "Crypt32.Lib")
#endif

#include <chrono>
#include <thread>

#include "common.h"
#include "kz/kz.h"
#include "kz/mode/kz_mode.h"
#include "kz/timer/kz_timer.h"
#include "kz/option/kz_option.h"
#include "kz/global/kz_global.h"
#include "kz/global/api/handshake.h"
#include "kz/global/api/events.h"
#include "kz/global/api/maps.h"

static_global bool initialized = false;

std::string KZGlobalService::apiUrl = "";
std::string KZGlobalService::apiKey = "";
ix::WebSocket *KZGlobalService::apiSocket = nullptr;
f64 KZGlobalService::heartbeatInterval = -1;
bool KZGlobalService::handshakeCompleted = false;
u64 KZGlobalService::nextMessageId = 1;
std::optional<KZ::API::Map> KZGlobalService::currentMap = std::nullopt;
std::unordered_map<u64, KZGlobalService::Callback<const Json &>> KZGlobalService::callbacks = {};

void KZGlobalService::Init()
{
	if (initialized)
	{
		return;
	}

	META_CONPRINTF("[KZ::Global] Initializing GlobalService...\n");

	KZGlobalService::apiUrl = KZOptionService::GetOptionStr("apiUrl", "https://api.cs2kz.org");
	KZGlobalService::apiKey = KZOptionService::GetOptionStr("apiKey", "");

	if (KZGlobalService::apiUrl.size() < 4 || KZGlobalService::apiUrl.substr(0, 4) != "http")
	{
		META_CONPRINTF("[KZ::Global] Invalid API url. GlobalService will be disabled.\n");
		KZGlobalService::apiUrl = "";
		return;
	}

	if (KZGlobalService::apiKey.empty())
	{
		META_CONPRINTF("[KZ::Global] No API key found! WebSocket connection will be disabled.\n");
		return;
	}

	std::string websocketUrl = KZGlobalService::apiUrl.replace(0, 4, "ws") + "/auth/cs2";
	ix::WebSocketHttpHeaders websocketHeaders;
	websocketHeaders["Authorization"] = std::string("Bearer ") + KZGlobalService::apiKey;

#ifdef _WIN32
	ix::initNetSystem();
#endif

	KZGlobalService::apiSocket = new ix::WebSocket();
	KZGlobalService::apiSocket->setUrl(websocketUrl);
	KZGlobalService::apiSocket->setExtraHeaders(websocketHeaders);
	KZGlobalService::apiSocket->setOnMessageCallback(KZGlobalService::OnWebSocketMessage);

	META_CONPRINTF("[KZ::Global] Establishing WebSocket connection... (url = '%s')\n", websocketUrl.c_str());
	KZGlobalService::apiSocket->start();

	initialized = true;
}

void KZGlobalService::Cleanup()
{
	META_CONPRINTF("[KZ::Global] Cleaning up...\n");

	if (KZGlobalService::apiSocket)
	{
		META_CONPRINTF("[KZ::Global] Closing WebSocket...\n");

		KZGlobalService::apiSocket->stop();
		delete KZGlobalService::apiSocket;
		KZGlobalService::apiSocket = nullptr;
	}

#ifdef _WIN32
	ix::uninitNetSystem();
#endif
}

void KZGlobalService::OnActivateServer()
{
	if (!initialized)
	{
		KZGlobalService::Init();
		return;
	}

	if (!KZGlobalService::IsConnected())
	{
		return;
	}

	bool ok = false;
	CUtlString currentMapName = g_pKZUtils->GetCurrentMapName(&ok);

	if (!ok)
	{
		META_CONPRINTF("[KZ::Global] Failed to get current map name. Cannot send `map-change` event.\n");
		return;
	}

	KZ::API::events::MapChange data(currentMapName.Get());

	auto callback = [currentMapName](const KZ::API::events::MapInfo &mapInfo)
	{
		KZGlobalService::currentMap = mapInfo.map;

		if (!mapInfo.map.has_value())
		{
			META_CONPRINTF("[KZ::Global] %s is not global.\n", currentMapName.Get());
			return;
		}

		for (const KZ::API::Map::Course &course : mapInfo.map->courses)
		{
			KZ::course::UpdateCourseGlobalID(course.name.c_str(), course.id);
		}

		KZ::API::events::WantWorldRecords data(mapInfo.map->id);

		auto callback = [](const KZ::API::events::WorldRecords &records)
		{
			for (const KZ::API::Record &record : records.records)
			{
				const KZCourse *course = KZ::course::GetCourseByGlobalCourseID(record.course.id);

				if (course == nullptr)
				{
					META_CONPRINTF("[KZ::Global] Could not find current course?\n");
					continue;
				}

				PluginId modeID = KZ::mode::GetModeInfo(record.mode).id;

				KZTimerService::InsertRecordToCache(record.time, course, modeID, record.nubPoints != 0, true);
			}
		};

		KZGlobalService::SendMessage("want-world-records", data, callback);
	};

	KZGlobalService::SendMessage("map-change", data, callback);
}

void KZGlobalService::OnPlayerActive()
{
	if (!this->player->IsConnected() || !this->player->IsAuthenticated())
	{
		return;
	}

	KZ::API::events::PlayerJoin data = {
		.steamId = this->player->GetSteamId64(),
		.name = this->player->GetName(),
		.ipAddress = this->player->GetIpAddress(),
	};

	auto callback = [player = this->player](const KZ::API::events::PlayerJoinAck &ack)
	{
		const std::string preferences = ack.preferences.ToString();

		META_CONPRINTF("[KZ::Global] %s is %sbanned. preferences:\n```\n%s\n```\n", player->GetName(), ack.isBanned ? "" : "not ",
					   preferences.c_str());

		player->optionService->InitializeGlobalPrefs(preferences);
	};

	KZGlobalService::SendMessage("player-join", data, callback);

	if (KZGlobalService::currentMap.has_value())
	{
		KZ::API::events::WantPlayerRecords data = {
			.mapId = KZGlobalService::currentMap->id,
			.playerId = this->player->GetSteamId64(),
		};

		auto callback = [player = this->player](const KZ::API::events::PlayerRecords &pbs)
		{
			for (const KZ::API::Record &record : pbs.records)
			{
				const KZCourse *course = KZ::course::GetCourseByGlobalCourseID(record.course.id);

				if (course == nullptr)
				{
					META_CONPRINTF("[KZ::Global] Could not find current course?\n");
					continue;
				}

				PluginId modeID = KZ::mode::GetModeInfo(record.mode).id;

				player->timerService->InsertPBToCache(record.time, course, modeID, record.nubPoints != 0, true, "",
													  record.nubPoints != 0 ? record.nubPoints : record.proPoints);
			}
		};

		KZGlobalService::SendMessage("want-player-records", data, callback);
	}
}

void KZGlobalService::OnClientDisconnect()
{
	if (!this->player->IsConnected() || !this->player->IsAuthenticated())
	{
		return;
	}

	CUtlString getPrefsError;
	CUtlString getPrefsResult;
	this->player->optionService->GetPreferencesAsJSON(&getPrefsError, &getPrefsResult);

	if (!getPrefsError.IsEmpty())
	{
		META_CONPRINTF("[KZ::Global] Failed to get preferences: %s\n", getPrefsError.Get());
		META_CONPRINTF("[KZ::Global] Not sending `player-leave` event.\n");
		return;
	}

	KZ::API::events::PlayerLeave data = {
		.steamId = this->player->GetSteamId64(),
		.name = this->player->GetName(),
		.preferences = Json(getPrefsResult.Get()),
	};

	KZGlobalService::SendMessage("player-leave", data);
}

bool KZGlobalService::SubmitRecord(u32 localId, const char *courseName, const char *modeName, f64 time, u32 teleports, const char *metadata)
{
	if (!KZGlobalService::currentMap.has_value())
	{
		META_CONPRINTF("[KZ::Global] Cannot submit record on non-global map.\n");
		return false;
	}

	if (!(KZ_STREQI(modeName, "vnl") || KZ_STREQI(modeName, "ckz")))
	{
		META_CONPRINTF("[KZ::Global] Cannot submit record on non-global mode.\n");
		return false;
	}

	const KZ::API::Map::Course *course = nullptr;

	for (const KZ::API::Map::Course &c : KZGlobalService::currentMap->courses)
	{
		if (KZ_STREQ(c.name.c_str(), courseName))
		{
			course = &c;
			break;
		}
	}

	if (course == nullptr)
	{
		META_CONPRINTF("[KZ::Global] Cannot submit record on unknown course.\n");
		return false;
	}

	KZ::API::events::NewRecord data = {
		.playerId = this->player->GetSteamId64(),
		.filterId = KZ_STREQI(modeName, "vnl") ? course->filters.vanilla.id : course->filters.classic.id,
		.styles = {}, // TODO
		.teleports = teleports,
		.time = time,
	};

	auto callback = [time, localId](const KZ::API::events::NewRecordAck &ack)
	{
		META_CONPRINTF("[KZ::Global] Record submitted under ID %d\n", ack.recordId);

		KZ::timer::GlobalRankData data = {
			.recordId = ack.recordId,
			.playerRating = ack.playerRating,
			.time = time,
			.nubData =
				{
					.isFirstRun = ack.nubData.isFirstRun,
					.rank = ack.nubData.rank,
					.points = ack.nubData.points,
					.maxRank = ack.nubData.leaderboardSize,
				},
			.proData =
				{
					.isFirstRun = ack.proData.isFirstRun,
					.rank = ack.proData.rank,
					.points = ack.proData.points,
					.maxRank = ack.proData.leaderboardSize,
				},
		};

		KZ::timer::UpdateGlobalRankData(localId, data);
	};

	KZGlobalService::SendMessage("new-record", data, callback);

	return true;
}

void KZGlobalService::OnWebSocketMessage(const ix::WebSocketMessagePtr &message)
{
	switch (message->type)
	{
		case ix::WebSocketMessageType::Open:
		{
			META_CONPRINTF("[KZ::Global] WebSocket connection established.\n");
			InitiateWebSocketHandshake(message);
			break;
		}

		case ix::WebSocketMessageType::Close:
		{
			META_CONPRINTF("[KZ::Global] WebSocket connection closed.\n");
			break;
		}

		case ix::WebSocketMessageType::Error:
		{
			META_CONPRINTF("[KZ::Global] WebSocket error: `%s`\n", message->errorInfo.reason.c_str());
			break;
		}

		case ix::WebSocketMessageType::Ping:
		{
			META_CONPRINTF("[KZ::Global] Ping!\n");
			break;
		}

		case ix::WebSocketMessageType::Pong:
		{
			META_CONPRINTF("[KZ::Global] Pong!\n");
			break;
		}

		case ix::WebSocketMessageType::Message:
		{
			META_CONPRINTF("[KZ::Global] Received WebSocket message:\n```\n%s\n```\n", message->str.c_str());

			if (!KZGlobalService::handshakeCompleted)
			{
				CompleteWebSocketHandshake(message);
				break;
			}

			Json payload(message->str);

			if (!payload.IsValid())
			{
				META_CONPRINTF("[KZ::Global] WebSocket message is not valid JSON.\n");
				return;
			}

			u64 messageId = 0;

			if (!payload.Get("id", messageId))
			{
				META_CONPRINTF("[KZ::Global] WebSocket message does not contain a valid ID.\n");
				return;
			}

			auto callback = KZGlobalService::callbacks.find(messageId);

			if (callback != KZGlobalService::callbacks.end())
			{
				callback->second(payload);
				KZGlobalService::callbacks.erase(messageId);
			}

			break;
		}

		case ix::WebSocketMessageType::Fragment:
		{
			// unused
			break;
		}
	}
}

void KZGlobalService::InitiateWebSocketHandshake(const ix::WebSocketMessagePtr &message)
{
	META_CONPRINTF("[KZ::Global] Performing handshake...\n");
	assert(message->type == ix::WebSocketMessageType::Open);

	bool ok = false;
	CUtlString currentMapName = g_pKZUtils->GetCurrentMapName(&ok);

	if (!ok)
	{
		META_CONPRINTF("[KZ::Global] Failed to get current map name. Cannot initiate handshake.\n");
		return;
	}

	KZ::API::handshake::Hello hello(currentMapName.Get());

	for (Player *player : g_pPlayerManager->players)
	{
		if (player->IsAuthenticated())
		{
			hello.AddPlayer(player->GetSteamId64(), player->GetName());
		}
	}

	Json payload {};

	if (!hello.ToJson(payload))
	{
		META_CONPRINTF("[KZ::Global] Failed to encode 'Hello' message.\n");
		return;
	}

	KZGlobalService::apiSocket->send(payload.ToString());

	META_CONPRINTF("[KZ::Global] Sent 'Hello' message.\n");
}

void KZGlobalService::CompleteWebSocketHandshake(const ix::WebSocketMessagePtr &message)
{
	META_CONPRINTF("[KZ::Global] Completing handshake...\n");
	assert(message->type == ix::WebSocketMessageType::Message);

	Json payload(message->str);
	KZ::API::handshake::HelloAck ack;

	if (!ack.FromJson(payload))
	{
		META_CONPRINTF("[KZ::Global] Failed to decode 'HelloAck' message.\n");
		return;
	}

	// x0.85 to reduce risk of timing out if there's high network latency
	KZGlobalService::heartbeatInterval = ack.heartbeatInterval * 0.85;
	KZGlobalService::handshakeCompleted = true;

	std::thread(HeartbeatThread).detach();

	if (ack.map.has_value())
	{
		ack.map.swap(KZGlobalService::currentMap);
	}

	META_CONPRINTF("[KZ::Global] Completed handshake!\n");
}

void KZGlobalService::HeartbeatThread()
{
	if (!KZGlobalService::IsConnected())
	{
		return;
	}

	while (KZGlobalService::heartbeatInterval > 0)
	{
		if (KZGlobalService::IsConnected())
		{
			KZGlobalService::apiSocket->ping("");
			META_CONPRINTF("[KZ::Global] Sent heartbeat. (interval=%.2fs)\n", KZGlobalService::heartbeatInterval);
		}

		std::this_thread::sleep_for(std::chrono::milliseconds((u64)(KZGlobalService::heartbeatInterval * 1000)));
	}
}

template<typename T>
void KZGlobalService::SendMessage(const char *event, const T &data)
{
	u64 messageId = KZGlobalService::nextMessageId++;

	Json payload {};
	payload.Set("id", messageId);
	payload.Set("event", event);
	payload.Set("data", data);

	KZGlobalService::apiSocket->send(payload.ToString());
}

template<typename T, typename Callback>
void KZGlobalService::SendMessage(const char *event, const T &data, Callback callback)
{
	u64 messageId = KZGlobalService::nextMessageId++;

	Json payload {};
	payload.Set("id", messageId);
	payload.Set("event", event);
	payload.Set("data", data);

	KZGlobalService::callbacks[messageId] = [messageId, callback](Json responseJson)
	{
		if (!responseJson.IsValid())
		{
			META_CONPRINTF("[KZ::Global] WebSocket message is not valid JSON.\n");
			return;
		}

		typename std::remove_const<typename std::remove_reference<typename decltype(std::function(callback))::argument_type>::type>::type
			responseData {};

		if (!responseJson.Get("data", responseData))
		{
			META_CONPRINTF("[KZ::Global] WebSocket message does not contain a valid `data` field.\n");
			return;
		}

		META_CONPRINTF("[KZ::Global] Calling callback (%d).\n", messageId);
		callback(responseData);
	};

	KZGlobalService::apiSocket->send(payload.ToString());
}
