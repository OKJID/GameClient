#pragma once

#include "NGMP_include.h"
#include <ws2ipdef.h>

// TODO_IPC
#if !defined(USE_IPC_TRANSPORT_LAYER)
#include "ValveNetworkingSockets/steam/steamnetworkingsockets.h"
#endif

class NetRoom_ChatMessagePacket;

enum class EConnectionState
{
	NOT_CONNECTED,
	CONNECTING_DIRECT,
	FINDING_ROUTE,
	CONNECTED_DIRECT,
	CONNECTION_FAILED,
	CONNECTION_DISCONNECTED
};

// TODO_IPC
#if !defined(USE_IPC_TRANSPORT_LAYER)
// trivial signalling client interface
class ISignalingClient
{
public:
	virtual ISteamNetworkingConnectionSignaling* CreateSignalingForConnection(const SteamNetworkingIdentity& identityPeer, SteamNetworkingErrMsg& errMsg) = 0;

	virtual void Poll() = 0;

	/// Disconnect from the server and close down our polling thread.
	virtual void Release() = 0;
};
#endif

class NetworkMesh;
class PlayerConnection
{
public:
	PlayerConnection()
	{
		
	}

	// TODO_IPC
#if !defined(USE_IPC_TRANSPORT_LAYER)
	PlayerConnection(int64_t userID, HSteamNetConnection hSteamConnection);
#else
	PlayerConnection(int64_t userID);
#endif

	EConnectionState GetState() const { return m_State; }

	int SendGamePacket(void* pBuffer, uint32_t totalDataSize);

	void UpdateLatencyHistogram();

	bool IsIPV4();
	bool IsDirect()
	{
		std::string strConnectionType = GetConnectionType();
		return strConnectionType.find("Relayed") == std::string::npos;
	}

	// TODO_IPC
#if !defined(USE_IPC_TRANSPORT_LAYER)
	int Recv(SteamNetworkingMessage_t** pMsg);
#else
	int Recv();
#endif

	int GetHighestHistoricalLatency()
	{
		int highestLatency = 0;
		for (int latencyHistory : m_vecLatencyHistory)
		{
			if (latencyHistory > highestLatency)
			{
				highestLatency = latencyHistory;
			}
		}

		return highestLatency;
	}

	std::vector<int> m_vecLatencyHistory;
	std::string GetStats();

	std::string GetConnectionType();

	void UpdateState(EConnectionState newState, NetworkMesh* pOwningMesh);
	void SetDisconnected(bool bWasError, NetworkMesh* pOwningMesh, bool bIsRetrying);
	
	int64_t m_userID = -1;

	EConnectionState m_State = EConnectionState::NOT_CONNECTED;
	
	int64_t pingSent = -1;

	int m_SignallingAttempts = 0;
	
	int GetLatency();
	float GetConnectionQuality();

	// TODO_IPC
#if !defined(USE_IPC_TRANSPORT_LAYER)
	HSteamNetConnection m_hSteamConnection = k_HSteamNetConnection_Invalid;
#endif
};

struct LobbyMemberEntry;

struct QueuedGamePacket
{
	CBitStream* m_bs = nullptr;
	int64_t m_userID = -1;
};

class NetworkMesh
{
public:
	NetworkMesh();

	~NetworkMesh()
	{
		// TODO_IPC
#if !defined(USE_IPC_TRANSPORT_LAYER)
		if (m_pSignaling != nullptr)
		{
			delete m_pSignaling;
			m_pSignaling = nullptr;
		}
#endif
	}

	void Flush();

	void RegisterConnectivity(int64_t userID);
	void UpdateConnectivity(PlayerConnection* connection);

	std::function<void(int64_t, std::wstring, PlayerConnection*)> m_cbOnConnected = nullptr;
	void RegisterForConnectionEvents(std::function<void(int64_t, std::wstring, PlayerConnection*)> cb)
	{
		m_cbOnConnected = cb;
	}

	void DeregisterForConnectionEvents()
	{
		m_cbOnConnected = nullptr;
	}

	int getMaximumLatency()
	{
		int highestLatency = 0;

		for (auto& kvPair : m_mapConnections)
		{
			PlayerConnection& conn = kvPair.second;
			if (conn.GetLatency() > highestLatency)
			{
				highestLatency = conn.GetLatency();
			}
		}

		return highestLatency;
	}

	Real getMaximumHistoricalLatency()
	{
		int highestLatency = 0;

		for (auto& kvPair : m_mapConnections)
		{
			PlayerConnection& conn = kvPair.second;
			if (conn.GetHighestHistoricalLatency() > highestLatency)
			{
				highestLatency = conn.GetHighestHistoricalLatency();
			}
		}

		return Real(highestLatency);
	}


	std::queue<QueuedGamePacket> m_queueQueuedGamePackets;

	bool HasGamePacket();
	QueuedGamePacket RecvGamePacket();
	int SendGamePacket(void* pBuffer, uint32_t totalDataSize, int64_t userID);

	// TODO_IPC
#if !defined(USE_IPC_TRANSPORT_LAYER)
	void StartConnectionSignalling(int64_t remoteUserID, uint16_t preferredPort);
#endif
	void DisconnectUser(int64_t remoteUserID);
	void Disconnect();

	void Tick();

	// TODO_IPC
#if !defined(USE_IPC_TRANSPORT_LAYER)
	HSteamListenSocket GetListenSocketHandle() const { return m_hListenSock; }
#endif

	std::map<int64_t, PlayerConnection>& GetAllConnections()
	{
		return m_mapConnections;
	}

	PlayerConnection* GetConnectionForUser(int64_t user_id)
	{
		if (m_mapConnections.contains(user_id))
		{
			return &m_mapConnections[user_id];
		}

		return nullptr;
	}


private:
	std::map<int64_t, PlayerConnection> m_mapConnections;

	// TODO_IPC
#if !defined(USE_IPC_TRANSPORT_LAYER)
	ISignalingClient* m_pSignaling = nullptr;
	HSteamListenSocket m_hListenSock = k_HSteamListenSocket_Invalid;

	std::string m_strTurnUsername;
	std::string m_strTurnToken;
	std::string m_strTurnUsernameString;
	std::string m_strTurnTokenString;
#endif
};
