#include "GameNetwork/GeneralsOnline/PluginInterfaces.h"
#include "../NGMP_include.h"
#include "../NetworkMesh.h"
#include "../OnlineServices_Init.h"
#include "../OnlineServices_Auth.h"

#ifdef __APPLE__
#include <dlfcn.h>
#include "Common/System/NativeFileSystem.h"
#endif

bool AnticheatPlugInterface::g_bPendingExitLobby = false;

#if defined(GENERALS_ONLINE_USE_PLUGINS_INTERFACE)

#ifdef __APPLE__
#define AC_PLUGIN_LOAD_FUNCTION(funcName) \
    AnticheatPlugInterface::Functions.fn##funcName = (FuncDef##funcName)dlsym(g_hACPluginModule, #funcName); \
    if (!AnticheatPlugInterface::Functions.fn##funcName) \
    { \
        NetworkLog(ELogVerbosity::LOG_RELEASE, "Failed to find " #funcName " function"); \
        dlclose(g_hACPluginModule); \
        g_hACPluginModule = nullptr; \
        return; \
    }
#else
#define AC_PLUGIN_LOAD_FUNCTION(funcName) \
    AnticheatPlugInterface::Functions.fn##funcName = (FuncDef##funcName)GetProcAddress(g_hACPluginModule, #funcName); \
    if (!AnticheatPlugInterface::Functions.fn##funcName) \
    { \
        NetworkLog(ELogVerbosity::LOG_RELEASE, "Failed to find " #funcName " function"); \
        FreeLibrary(g_hACPluginModule); \
        g_hACPluginModule = nullptr; \
        return; \
    }
#endif

bool AnticheatPlugInterface::IsExternalProcessRunning()
{
    if (IsPluginLoaded() && Functions.fnIsExternalProcessRunning != nullptr)
    {
        return Functions.fnIsExternalProcessRunning();
    }

    return false;
}

int AnticheatPlugInterface::GetAnticheatIdentifier()
{
    if (IsPluginLoaded() && Functions.fnGetAnticheatIdentifier != nullptr)
    {
        return Functions.fnGetAnticheatIdentifier();
    }

    return 0;
}

int AnticheatPlugInterface::GetConnectionLatencyForUser(std::string mwUserID, uint32_t goUserID)
{
#if defined(AC_ENABLED)
    if (IsPluginLoaded() && Functions.fnGetConnectionLatencyForUser != nullptr)
    {
        return Functions.fnGetConnectionLatencyForUser(mwUserID.c_str(), goUserID);
    }
#endif

    return 0;
}

void AnticheatPlugInterface::LoadPlugin(const char* szPluginName)
{
    if (g_hACPluginModule != nullptr || IsPluginLoaded())
    {
        return;
    }

    if (szPluginName == nullptr)
    {
        NetworkLog(ELogVerbosity::LOG_RELEASE, "[AC] ERROR: Plugin name is null");
        m_bPluginLoadFailed = true;
        return;
    }

    NetworkLog(ELogVerbosity::LOG_RELEASE, "[AC] Attempting to load plugin from %s", szPluginName);

#if defined(_DEBUG)
    szPluginName = "F:\\gen\\ACPlugin_EAC\\build\\Debug\\easyanticheat.dll";
#endif

    m_bPluginLoadFailed = false;
#ifdef __APPLE__
    std::string safePath = NativeFileSystem::get_safe_path(szPluginName);
    g_hACPluginModule = dlopen(safePath.c_str(), RTLD_LAZY);

    if (!g_hACPluginModule)
    {
        m_bPluginLoadFailed = true;
        const char* err = dlerror();
        NetworkLog(ELogVerbosity::LOG_RELEASE, "[AC] Failed to load %s (%s)", szPluginName, err ? err : "unknown error");
    }
#else
    g_hACPluginModule = LoadLibraryA(szPluginName);

    if (!g_hACPluginModule)
    {
        g_hACPluginModule = nullptr;
        m_bPluginLoadFailed = true;

        DWORD err = GetLastError();
        NetworkLog(ELogVerbosity::LOG_RELEASE, "[AC] Failed to load %s (%u)", szPluginName, err);
    }
#endif
    else
    {
#if defined(AC_ENABLED)
        // set logger 
        AC_PLUGIN_LOAD_FUNCTION(SetLoggingFunction);

        Functions.fnSetLoggingFunction([](const char* szMsg)
            {
                //MessageBoxA(nullptr, szMsg, szMsg, MB_OK);
                NetworkLog(ELogVerbosity::LOG_RELEASE, szMsg);
            });

        // Initialize AC
        AC_PLUGIN_LOAD_FUNCTION(Initialize);

        int result = Functions.fnInitialize([](const char* szMiddlewareID, uint64_t goUserID, EConnectionState newState) // on connection state changed callback
            {
                NetworkMesh* pMesh = NGMP_OnlineServicesManager::GetNetworkMesh();
                if (pMesh != nullptr)
                {
                    std::map<int64_t, PlayerConnection>& connections = pMesh->GetAllConnections();
                    for (auto& kvPair : connections)
                    {
                        if (kvPair.first == goUserID)
                        {
                            kvPair.second.UpdateState(newState, pMesh);
                        }
                    }
                }
            });
        NetworkLog(ELogVerbosity::LOG_RELEASE, "Initialize result = %d", result);

        // check loaded
        AC_PLUGIN_LOAD_FUNCTION(IsExternalProcessRunning);

        AC_PLUGIN_LOAD_FUNCTION(GetAnticheatIdentifier);

#if _DEBUG && !defined(__APPLE__)
        if (ApplicationHWnd != nullptr)
        {
            SetWindowText(ApplicationHWnd, Functions.fnIsExternalProcessRunning() ? "SECURED" : "INSECURE");
        }
#endif

        // integrity callback
        AC_PLUGIN_LOAD_FUNCTION(SetACIntegrityViolationOccurredCallback);

        Functions.fnSetACIntegrityViolationOccurredCallback([](const char* szReason, int violationType)
            {
                if (szReason == nullptr)
                {
                    szReason = "(null reason)";
                }

                NetworkLog(ELogVerbosity::LOG_RELEASE, "[AC] Leaving lobby, local AC integrity violation occured (%d): %s.", violationType, szReason);
                g_bPendingExitLobby = true;
            });

        // set action required callback
        AC_PLUGIN_LOAD_FUNCTION(SetACActionRequiredCallback);

        Functions.fnSetACActionRequiredCallback([](uint32_t userID, const char* szReason, EAnticheatActionType actionType, EAnticheatActionReason actionReason)
            {
                if (szReason == nullptr)
                {
                    szReason = "(null reason)";
                }

                NGMP_OnlineServices_AuthInterface* pAuthInterface = NGMP_OnlineServicesManager::GetInterface<NGMP_OnlineServices_AuthInterface>();

                NetworkLog(ELogVerbosity::LOG_RELEASE, "[AC] Action required: %s", szReason);

                if (pAuthInterface == nullptr)
                {
                    // no auth interface? bail out
                    NetworkLog(ELogVerbosity::LOG_RELEASE, "[AC] Leaving lobby, lobby isn't secure, no auth interface.");
                    g_bPendingExitLobby = true;
                    return;
                }

                // If it's us, leave, if its someone else, d/c them
                uint32_t localUserID = pAuthInterface->GetUserID();
                if (localUserID == userID)
                {
                    NetworkLog(ELogVerbosity::LOG_RELEASE, "[AC] Leaving lobby, lobby isn't secure, action was requested against local user.");
                    g_bPendingExitLobby = true;
                }
                else
                {
                    NetworkLog(ELogVerbosity::LOG_RELEASE, "[AC] Disconnecting remote user, lobby isn't secure, action was requested against remote user %u.", userID);

                    NetworkMesh* pMesh = NGMP_OnlineServicesManager::GetNetworkMesh();
                    if (pMesh != nullptr)
                    {
                        pMesh->DisconnectUser(userID);
                        NetworkLog(ELogVerbosity::LOG_RELEASE, "[AC] Disconnected: %u.", userID);
                    }
                    else // no mesh, just back out
                    {
                        NetworkLog(ELogVerbosity::LOG_RELEASE, "[AC] Leaving lobby, lobby isn't secure, actionable player was remote, but no mesh exists to take action.");
                        g_bPendingExitLobby = true;
                    }
                }
            });

        // set transport callback
        AC_PLUGIN_LOAD_FUNCTION(SetSendMessageViaTransportCallback);
        Functions.fnSetSendMessageViaTransportCallback([](uint32_t goUserID, const void* pData, uint32_t dataLen)
            {
                if (pData == nullptr || dataLen == 0)
                {
                    NetworkLog(ELogVerbosity::LOG_RELEASE, "[AC] ERROR: SendMessageViaTransport received null/empty data");
                    return;
                }

                // prefer websocket if we have it, otherwise fall back to p2p mesh
                bool bFallbackToP2P = false;

                if (AnticheatPlugInterface::DoesACPluginProvideSecureGameTransport())
                {
                    bFallbackToP2P = true;
                }
                else
                {
                    std::shared_ptr<WebSocket>  pWS = NGMP_OnlineServicesManager::GetWebSocket();
                    if (pWS != nullptr)
                    {
                        if (pWS->IsConnected())
                        {
                            if (dataLen > 0)
                            {
                                std::vector<uint8_t> vecPayload((uint8_t*)pData, (uint8_t*)pData + dataLen);
                                pWS->SendData_ACMessage(goUserID, vecPayload);
                            }
                            else
                            {
                                bFallbackToP2P = true;
                            }
                        }
                        else
                        {
                            bFallbackToP2P = true;
                        }
                    }
                    else
                    {
                        bFallbackToP2P = true;
                    }
                }

                if (bFallbackToP2P)
                {
                    NetworkLog(ELogVerbosity::LOG_RELEASE, "[AC] AC Packets - WebSocket unavailable, falling back to P2P");
                    NetworkMesh* pMesh = NGMP_OnlineServicesManager::GetNetworkMesh();
                    if (pMesh != nullptr)
                    {
                        pMesh->SendACPacket(goUserID, pData, dataLen);
                    }
                    else
                    {
                        NetworkLog(ELogVerbosity::LOG_RELEASE, "[AC] ERROR: Cannot send AC packet - NetworkMesh is null");
                    }
                }
            });

        // AC network message arrived callback
        AC_PLUGIN_LOAD_FUNCTION(ACMessageArrivedViaTransport);

        // transport funcs
        AC_PLUGIN_LOAD_FUNCTION(DoesACPluginProvideSecureGameTransport);
        AC_PLUGIN_LOAD_FUNCTION(StartSignalling);
        AC_PLUGIN_LOAD_FUNCTION(SendPacket);
        AC_PLUGIN_LOAD_FUNCTION(GetNextRecvPacketSize);
        AC_PLUGIN_LOAD_FUNCTION(RecvPacket);
        AC_PLUGIN_LOAD_FUNCTION(GetConnectionLatencyForUser);

        AC_PLUGIN_LOAD_FUNCTION(DisconnectPlayer);
        AC_PLUGIN_LOAD_FUNCTION(DisconnectAll);

        // Login funcs
        AC_PLUGIN_LOAD_FUNCTION(Login);
        AC_PLUGIN_LOAD_FUNCTION(RefreshToken);
        AC_PLUGIN_LOAD_FUNCTION(IsLoggedIn);
        AC_PLUGIN_LOAD_FUNCTION(GetMiddlewareAuthToken);

        // Begin and end session funcs
        AC_PLUGIN_LOAD_FUNCTION(BeginSession);
        AC_PLUGIN_LOAD_FUNCTION(EndSession);

        // register player funcs
        AC_PLUGIN_LOAD_FUNCTION(RegisterPlayer);
        AC_PLUGIN_LOAD_FUNCTION(DeregisterPlayer);

        AC_PLUGIN_LOAD_FUNCTION(Tick);
        AC_PLUGIN_LOAD_FUNCTION(Shutdown);
#else
    // Initialize AC
    AC_PLUGIN_LOAD_FUNCTION(Initialize);

    int result = Functions.fnInitialize();
    NetworkLog(ELogVerbosity::LOG_RELEASE, "Initialize result = %d", result);

       AC_PLUGIN_LOAD_FUNCTION(IsExternalProcessRunning);
        AC_PLUGIN_LOAD_FUNCTION(GetAnticheatIdentifier);
#endif
    }
}

void AnticheatPlugInterface::AC_NetworkMessageArrived(uint32_t goUserID, void* pData, uint32_t dataLen)
{
#if defined(AC_ENABLED)
    if (pData == nullptr || dataLen == 0)
    {
        NetworkLog(ELogVerbosity::LOG_RELEASE, "[AC] ERROR: AC_NetworkMessageArrived received null/empty data");
        return;
    }

    // TODO: Cache all of these getprocaddresses
    if (IsPluginLoaded() && Functions.fnACMessageArrivedViaTransport != nullptr)
    {
        NetworkLog(ELogVerbosity::LOG_RELEASE, "[AC] fnOnMessageArrivedViaTransport");
        Functions.fnACMessageArrivedViaTransport(goUserID, pData, dataLen);
    }
#endif
}


void AnticheatPlugInterface::Authenticate()
{
#if defined(AC_ENABLED)
    if (IsPluginLoaded() && Functions.fnLogin != nullptr && Functions.fnIsLoggedIn != nullptr)
    {
        NGMP_OnlineServices_AuthInterface* pAuthInterface = NGMP_OnlineServicesManager::GetInterface<NGMP_OnlineServices_AuthInterface>();
        if (pAuthInterface == nullptr)
        {
            return;
        }

        std::string authToken = pAuthInterface->GetAuthToken();
        Functions.fnLogin(authToken.c_str(),
            [](bool bSuccess)
            {
                if (!bSuccess)
                {
                    // TODO_AC: Handle this, its a fatal error
                    return;
                }

#ifdef __APPLE__
                m_tokenCreationTime = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
#else
                m_tokenCreationTime = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::utc_clock::now().time_since_epoch()).count();
#endif

                if (Functions.fnIsLoggedIn != nullptr && Functions.fnIsLoggedIn())
                {
                    char buf[4196];
                    if (Functions.fnGetMiddlewareAuthToken != nullptr && Functions.fnGetMiddlewareAuthToken(buf, sizeof(buf)))
                    {
                        NetworkLog(ELogVerbosity::LOG_RELEASE, "[AC] Got MW token: %s", buf);

                        // Now we can begin login
                        NGMP_OnlineServices_AuthInterface* pAuthInterface = NGMP_OnlineServicesManager::GetInterface<NGMP_OnlineServices_AuthInterface>();
                        if (pAuthInterface != nullptr)
                        {
                            pAuthInterface->SendMiddlewareToken(std::string(buf));
                        }
                        else
                        {
                            NetworkLog(ELogVerbosity::LOG_RELEASE, "[AC] ERROR: Auth interface became null during login callback");
                        }
                    }
                }
                else
                {
                    // TODO_AC: Handle this, its a fatal error
                }

                
            });
    }
#endif
}

bool g_bSessionStarted = false;

void AnticheatPlugInterface::BeginSession()
{
#if defined(AC_ENABLED)
    NetworkLog(ELogVerbosity::LOG_RELEASE, "[AC] BeginSession() called");
    NetworkLog(ELogVerbosity::LOG_RELEASE, "[AC] IsPluginLoaded=%d, fnBeginSession=%p", IsPluginLoaded(), Functions.fnBeginSession);
    
    if (IsPluginLoaded() && Functions.fnBeginSession != nullptr)
    {
        NetworkLog(ELogVerbosity::LOG_RELEASE, "[AC] Calling plugin fnBeginSession()");
        Functions.fnBeginSession();
        g_bSessionStarted = true;
        NetworkLog(ELogVerbosity::LOG_RELEASE, "[AC] Plugin fnBeginSession() completed");
    }
    else
    {
        NetworkLog(ELogVerbosity::LOG_RELEASE, "[AC] ERROR: Cannot call fnBeginSession - plugin not loaded or function pointer is null");
    }
#endif
}

void AnticheatPlugInterface::EndSession()
{
#if defined(AC_ENABLED)
    if (IsPluginLoaded() && Functions.fnEndSession != nullptr)
    {
        Functions.fnEndSession();
        g_bSessionStarted = false;
    }
#endif
}

bool AnticheatPlugInterface::DoesACPluginProvideSecureGameTransport()
{
#if defined(AC_ENABLED)
    if (IsPluginLoaded() && Functions.fnDoesACPluginProvideSecureGameTransport != nullptr)
    {
        return Functions.fnDoesACPluginProvideSecureGameTransport();
    }
#endif

    return false;
}

void AnticheatPlugInterface::SendPacket(const char* szMiddlewareUserID, uint64_t targetGoUserID, void* pData, int numBytes, ENetworkChannels channel, EPacketReliability reliability)
{
#if defined(AC_ENABLED)
    if (IsPluginLoaded() && Functions.fnSendPacket != nullptr)
    {
        Functions.fnSendPacket(szMiddlewareUserID, targetGoUserID, pData, numBytes, channel, reliability);
    }
#endif
}

void AnticheatPlugInterface::StartSignalling(const char* szMiddlewareUserID, uint64_t goUserID)
{
#if defined(AC_ENABLED)
    if (IsPluginLoaded() && Functions.fnStartSignalling != nullptr)
    {
        Functions.fnStartSignalling(szMiddlewareUserID, goUserID);
    }
#endif
}

int AnticheatPlugInterface::GetNextRecvPacketSize(uint8_t channelToReceiveOn)
{
#if defined(AC_ENABLED)
    if (IsPluginLoaded() && Functions.fnGetNextRecvPacketSize != nullptr)
    {
        return Functions.fnGetNextRecvPacketSize(channelToReceiveOn);
    }
#endif

    return 0;
}

bool AnticheatPlugInterface::RecvPacket(uint8_t** pOutData, uint8_t channelToReceiveOn)
{
#if defined(AC_ENABLED)
    if (IsPluginLoaded() && Functions.fnRecvPacket != nullptr)
    {
        return Functions.fnRecvPacket(pOutData, channelToReceiveOn);
    }
#endif

    return false;
}

void AnticheatPlugInterface::DisconnectPlayer(const char* szMiddlewareUserID, uint64_t goUserID)
{
#if defined(AC_ENABLED)
    if (IsPluginLoaded() && Functions.fnDisconnectPlayer != nullptr)
    {
        Functions.fnDisconnectPlayer(szMiddlewareUserID, goUserID);
    }
#endif
}

void AnticheatPlugInterface::DisconnectAll()
{
#if defined(AC_ENABLED)
    if (IsPluginLoaded() && Functions.fnDisconnectAll != nullptr)
    {
        Functions.fnDisconnectAll();
    }
#endif
}

AnticheatPlugInterface::AnticheatPluginFunctionPtrs AnticheatPlugInterface::Functions;

#ifdef __APPLE__
void* AnticheatPlugInterface::g_hACPluginModule = nullptr;
#else
HMODULE AnticheatPlugInterface::g_hACPluginModule = nullptr;
#endif
bool AnticheatPlugInterface::m_bPluginLoadFailed = false;

int64_t AnticheatPlugInterface::m_tokenCreationTime = -1;

bool AnticheatPlugInterface::RegisterPlayer(std::string mwUserID, uint32_t goUserID)
{
#if defined(AC_ENABLED)
    if (!g_bSessionStarted) // TODO_AC: This is hacky, it's because on lobby join, the server can send AC_REGISTER_PLAYER before we join the lobby, so we didnt actually start the session yet. We should buffer these messages until session start or something instead of relying on this hacky global
    {
        AnticheatPlugInterface::BeginSession();
    }

    if (IsPluginLoaded() && Functions.fnRegisterPlayer != nullptr)
    {
        NetworkLog(ELogVerbosity::LOG_RELEASE, "RegisterPlayer: %s to %" PRIu32, mwUserID.c_str(), goUserID);

        bool bReg = Functions.fnRegisterPlayer(mwUserID.c_str(), goUserID);
        NetworkLog(ELogVerbosity::LOG_RELEASE, "RegisterPlayerFunc result: %d", bReg);
        return bReg;
    }

    return false;
#else
    return true;
#endif
}


bool AnticheatPlugInterface::DeregisterPlayer(std::string mwUserID, uint32_t goUserID)
{
#if defined(AC_ENABLED)
    if (IsPluginLoaded() && Functions.fnDeregisterPlayer != nullptr)
    {
        NetworkLog(ELogVerbosity::LOG_RELEASE, "DeregisterPlayer: %s to %" PRIu32, mwUserID.c_str(), goUserID);

        bool bReg = Functions.fnDeregisterPlayer(mwUserID.c_str(), goUserID);
        NetworkLog(ELogVerbosity::LOG_RELEASE, "DeregisterPlayerFunc result: %d", bReg);
        return bReg;
    }

    return false;
#else
    return true;
#endif
}

void AnticheatPlugInterface::Tick()
{
#if defined(AC_ENABLED)
    if (IsPluginLoaded() && Functions.fnTick != nullptr)
    {
        Functions.fnTick();

        // Do we need to refresh our token?
        if (Functions.fnIsLoggedIn != nullptr && Functions.fnIsLoggedIn())
        {
#ifdef __APPLE__
            int64_t now = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
#else
            int64_t now = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::utc_clock::now().time_since_epoch()).count();
#endif
            if (m_tokenCreationTime != -1 && now - m_tokenCreationTime >= 45 * 60 * 1000) // refresh every 45m, tokens last 60m, giving us a 15m buffer to refresh and retry if something goes wrong
            {
                NetworkLog(ELogVerbosity::LOG_RELEASE, "[AC] Token is about to expire, refreshing...");
                RefreshToken();
            }
        }
    }
#endif
}

void AnticheatPlugInterface::RefreshToken()
{
#if defined(AC_ENABLED)
    if (IsPluginLoaded() && Functions.fnRefreshToken != nullptr && Functions.fnIsLoggedIn != nullptr)
    {
        NetworkLog(ELogVerbosity::LOG_RELEASE, "[AC] Refreshing token");
        NGMP_OnlineServices_AuthInterface* pAuthInterface = NGMP_OnlineServicesManager::GetInterface<NGMP_OnlineServices_AuthInterface>();
        if (pAuthInterface == nullptr)
        {
            return;
        }

#ifdef __APPLE__
        m_tokenCreationTime = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
#else
        m_tokenCreationTime = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::utc_clock::now().time_since_epoch()).count();
#endif

        std::string authToken = pAuthInterface->GetAuthToken();
        Functions.fnRefreshToken(authToken.c_str(),
            [](bool bSuccess)
            {
                NetworkLog(ELogVerbosity::LOG_RELEASE, "[AC] Refreshed token: %d", bSuccess);
                if (!bSuccess)
                {
                    NetworkLog(ELogVerbosity::LOG_RELEASE, "[AC] ERROR: Token refresh failed");
                    // TODO_AC: Handle this, its a fatal error
                    return;
                }
            });
    }
#endif
}

void AnticheatPlugInterface::UnloadPlugin()
{
#if defined(AC_ENABLED)
    if (IsPluginLoaded())
    {
        NetworkLog(ELogVerbosity::LOG_RELEASE, "[AC] Starting Shutdown");
        if (Functions.fnShutdown != nullptr)
        {
            NetworkLog(ELogVerbosity::LOG_RELEASE, "[AC] Shutdown in progress");
            Functions.fnShutdown();
        }
        NetworkLog(ELogVerbosity::LOG_RELEASE, "[AC] Shutdown Complete");

        NetworkLog(ELogVerbosity::LOG_RELEASE, "[AC] Unloading plugin");
#ifdef __APPLE__
        dlclose(g_hACPluginModule);
#else
        FreeLibrary(g_hACPluginModule);
#endif
        g_hACPluginModule = nullptr;
        NetworkLog(ELogVerbosity::LOG_RELEASE, "[AC] Unloaded plugin");
    }
#endif
}

#endif
