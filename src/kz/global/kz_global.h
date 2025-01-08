#pragma once

#include <vendor/ixwebsocket/ixwebsocket/IXWebSocket.h>

#include "utils/json.h"

#include "kz/kz.h"
#include "kz/global/api/maps.h"

class KZGlobalService : public KZBaseService
{
	using KZBaseService::KZBaseService;

public:
	static void Init();
	static void Cleanup();

	/**
	 * Returns wheter we are currently connected to the API.
	 */
	static bool IsConnected()
	{
		// clang-format off

		return (KZGlobalService::apiSocket != nullptr)
			&& (KZGlobalService::apiSocket->getReadyState() == ix::ReadyState::Open)
			&& (KZGlobalService::handshakeCompleted);

		// clang-format on
	}

	static void OnActivateServer();
	void OnPlayerAuthorized();
	void OnClientDisconnect();

	/**
	 * Submits a new record to the API.
	 *
	 * Returns whether the record was submitted. This could be false if the filter is not ranked.
	 */
	bool SubmitRecord(u32 localId, const char *courseName, const char *modeName, f64 time, u32 teleports, const char *metadata);

private:
	template<typename... Args>
	using Callback = std::function<void(Args...)>;

	/**
	 * URL to make HTTP requests to.
	 *
	 * Read from the configuration file.
	 */
	static std::string apiUrl;

	/**
	 * Access Key used to authenticate the WebSocket connection.
	 *
	 * Read from the configuration file.
	 */
	static std::string apiKey;

	/**
	 * WebSocket connected to the API.
	 */
	static ix::WebSocket *apiSocket;

	/**
	 * Interval at which we need to send ping messages over the WebSocket.
	 *
	 * Set by the API during the handshake.
	 */
	static f64 heartbeatInterval;

	/**
	 * Whether we have completed the handshake already.
	 */
	static bool handshakeCompleted;

	/**
	 * The ID to use for the next WebSocket message.
	 */
	static u64 nextMessageId;

	/**
	 * The API's view of the current map.
	 */
	static std::optional<KZ::API::Map> currentMap;

	/**
	 * Callbacks to execute when the API sends messages with specific IDs.
	 */
	static std::unordered_map<u64, Callback<const Json &>> callbacks;

	/**
	 * Called bx IXWebSocket whenever we receive a message.
	 */
	static void OnWebSocketMessage(const ix::WebSocketMessagePtr &message);

	/**
	 * Called right after establishing the WebSocket connection.
	 */
	static void InitiateWebSocketHandshake(const ix::WebSocketMessagePtr &message);

	/**
	 * Called right after establishing the WebSocket connection.
	 */
	static void CompleteWebSocketHandshake(const ix::WebSocketMessagePtr &message);

	/**
	 * Sends a ping message every `heartbeatInterval` seconds.
	 */
	static void HeartbeatThread();

	/**
	 * Sends a WebSocket message.
	 */
	template<typename T>
	static void SendMessage(const char *event, const T &data);

	/**
	 * Sends a WebSocket message.
	 */
	template<typename T, typename CallbackFunc>
	static void SendMessage(const char *event, const T &data, CallbackFunc callback);
};
