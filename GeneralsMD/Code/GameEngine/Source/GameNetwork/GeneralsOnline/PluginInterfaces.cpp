#include "GameNetwork/GeneralsOnline/PluginInterfaces.h"
#include "../NGMP_include.h"
#include "../NetworkMesh.h"
#include "../OnlineServices_Init.h"
#include "../OnlineServices_Auth.h"

#define AC_PLUGIN_LOAD_FUNCTION(funcName) \
    AnticheatPlugInterface::Functions.fn##funcName = (FuncDef##funcName)GetProcAddress(g_hACPluginModule, #funcName); \
    if (!AnticheatPlugInterface::Functions.fn##funcName) \
    { \
        NetworkLog(ELogVerbosity::LOG_RELEASE, "Failed to find " #funcName " function", MB_OK); \
        FreeLibrary(g_hACPluginModule); \
        return; \
    }

bool AnticheatPlugInterface::IsExternalProcessRunning()
{
    if (IsPluginLoaded())
    {
        return Functions.fnIsExternalProcessRunning();
    }

    return false;
}

int AnticheatPlugInterface::GetAnticheatIdentifier()
{
    if (IsPluginLoaded())
    {
        return Functions.fnGetAnticheatIdentifier();
    }

    return 0;
}

void AnticheatPlugInterface::LoadPlugin(const char* szPluginName)
{
    NetworkLog(ELogVerbosity::LOG_RELEASE, "[AC] Attempting to load plugin from %s", szPluginName);

    m_bPluginLoadFailed = false;
    g_hACPluginModule = LoadLibraryA(szPluginName);

    if (!g_hACPluginModule)
    {
        g_hACPluginModule = nullptr;
        m_bPluginLoadFailed = true;

        DWORD err = GetLastError();
        NetworkLog(ELogVerbosity::LOG_RELEASE, "[AC] Failed to load %s (%u)", szPluginName, err);
    }
    else
    {
        // set logger 
        AC_PLUGIN_LOAD_FUNCTION(SetLoggingFunction);

        Functions.fnSetLoggingFunction([](const char* szMsg)
            {
                //MessageBoxA(nullptr, szMsg, szMsg, MB_OK);
                NetworkLog(ELogVerbosity::LOG_RELEASE, szMsg);
            });

        // Initialize AC
        AC_PLUGIN_LOAD_FUNCTION(Initialize);

        int result = Functions.fnInitialize();
        NetworkLog(ELogVerbosity::LOG_RELEASE, "Initialize result = %d", result);

        // check loaded
        AC_PLUGIN_LOAD_FUNCTION(IsExternalProcessRunning);

        AC_PLUGIN_LOAD_FUNCTION(GetAnticheatIdentifier);

#if _DEBUG
        SetWindowText(ApplicationHWnd, Functions.fnIsExternalProcessRunning() ? "SECURED" : "INSECURE");
#endif

        // set action required callback
        AC_PLUGIN_LOAD_FUNCTION(SetACActionRequiredCallback);

        Functions.fnSetACActionRequiredCallback([](uint32_t userID, const char* szReason, EAnticheatActionType actionType, EAnticheatActionReason actionReason)
            {
                NGMP_OnlineServices_AuthInterface* pAuthInterface = NGMP_OnlineServicesManager::GetInterface<NGMP_OnlineServices_AuthInterface>();

                NetworkLog(ELogVerbosity::LOG_RELEASE, "[AC] Action required.");

                if (pAuthInterface == nullptr)
                {
                    // no auth interface? bail out
                    NetworkLog(ELogVerbosity::LOG_RELEASE, "[AC] Leaving lobby, lobby isn't secure, no auth interface.");
                    g_bPendingExitLobby = true;
                }
                else
                {
                    // If it's us, leave, if its someone else, d/c them
                    if (pAuthInterface->GetUserID() == userID)
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
                }
            });

        // set transport callback
        AC_PLUGIN_LOAD_FUNCTION(SetSendMessageViaTransportCallback);
        Functions.fnSetSendMessageViaTransportCallback([](uint32_t goUserID, const void* pData, uint32_t dataLen)
            {
                NetworkMesh* pMesh = NGMP_OnlineServicesManager::GetNetworkMesh();
                if (pMesh != nullptr)
                {
                    pMesh->SendACPacket(goUserID, pData, dataLen);
                }
            });

        // AC network message arrived callback
        AC_PLUGIN_LOAD_FUNCTION(ACMessageArrivedViaTransport);

        // Login func
        AC_PLUGIN_LOAD_FUNCTION(Login);
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

        // TODO_AC: Deregister player
    }
}

bool AnticheatPlugInterface::g_bPendingExitLobby = false;

void AnticheatPlugInterface::AC_NetworkMessageArrived(uint32_t goUserID, void* pData, uint32_t dataLen)
{
    // TODO: Cache all of these getprocaddresses
    if (IsPluginLoaded() && Functions.fnACMessageArrivedViaTransport != nullptr)
    {
        NetworkLog(ELogVerbosity::LOG_RELEASE, "[AC] fnOnMessageArrivedViaTransport");
        Functions.fnACMessageArrivedViaTransport(goUserID, pData, dataLen);
    }
}


void AnticheatPlugInterface::Authenticate()
{
    if (IsPluginLoaded() && Functions.fnLogin != nullptr && Functions.fnIsLoggedIn)
    {
        NGMP_OnlineServices_AuthInterface* pAuthInterface = NGMP_OnlineServicesManager::GetInterface<NGMP_OnlineServices_AuthInterface>();
        if (pAuthInterface == nullptr)
        {
            return;
        }

        Functions.fnLogin(pAuthInterface->GetAuthToken().c_str(),
            [](bool bSuccess)
            {
                if (!bSuccess)
                {
                    // TODO_AC: Handle this, its a fatal error
                    return;
                }

                if (Functions.fnIsLoggedIn())
                {
                    char buf[4196];
                    if (Functions.fnGetMiddlewareAuthToken(buf, sizeof(buf)))
                    {
                        NetworkLog(ELogVerbosity::LOG_RELEASE, "[AC] Got MW token: %s", buf);

                        // Now we can begin login
                        NGMP_OnlineServices_AuthInterface* pAuthInterface = NGMP_OnlineServicesManager::GetInterface<NGMP_OnlineServices_AuthInterface>();
                        if (pAuthInterface == nullptr)
                        {
                            return;
                        }

                        pAuthInterface->SendMiddlewareToken(std::string(buf));
                    }
                }
                else
                {
                    // TODO_AC: Handle this, its a fatal error
                }

                
            });
    }
}

bool g_bSessionStarted = false;

void AnticheatPlugInterface::BeginSession()
{
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
}

void AnticheatPlugInterface::EndSession()
{
    if (IsPluginLoaded() && Functions.fnEndSession != nullptr)
    {
        Functions.fnEndSession();
        g_bSessionStarted = false;
    }
}

AnticheatPlugInterface::AnticheatPluginFunctionPtrs AnticheatPlugInterface::Functions;

HMODULE AnticheatPlugInterface::g_hACPluginModule = nullptr;
bool AnticheatPlugInterface::m_bPluginLoadFailed = false;

bool AnticheatPlugInterface::RegisterPlayer(std::string mwUserID, uint32_t goUserID)
{
    if (!g_bSessionStarted) // TODO_AC: This is hacky, it's because on lobby join, the server can send AC_REGISTER_PLAYER before we join the lobby, so we didnt actually start the session yet. We should buffer these messages until session start or something instead of relying on this hacky global
    {
        AnticheatPlugInterface::BeginSession();
    }

    if (IsPluginLoaded() && Functions.fnRegisterPlayer != nullptr)
    {
        NetworkLog(ELogVerbosity::LOG_RELEASE, "RegisterPlayer: %s to %" PRIu64, mwUserID.c_str(), goUserID);

        bool bReg = Functions.fnRegisterPlayer(mwUserID.c_str(), goUserID);
        NetworkLog(ELogVerbosity::LOG_RELEASE, "RegisterPlayerFunc result: %d", bReg);
        return bReg;
    }

    return false;
}


bool AnticheatPlugInterface::DeregisterPlayer(std::string mwUserID, uint32_t goUserID)
{
    if (IsPluginLoaded() && Functions.fnDeregisterPlayer != nullptr)
    {
        NetworkLog(ELogVerbosity::LOG_RELEASE, "DeregisterPlayer: %s to %" PRIu64, mwUserID.c_str(), goUserID);

        bool bReg = Functions.fnDeregisterPlayer(mwUserID.c_str(), goUserID);
        NetworkLog(ELogVerbosity::LOG_RELEASE, "DeregisterPlayerFunc result: %d", bReg);
        return bReg;
    }

    return false;
}

void AnticheatPlugInterface::Tick()
{
    if (IsPluginLoaded() && Functions.fnTick != nullptr)
    {
        Functions.fnTick();
    }
}

void AnticheatPlugInterface::UnloadPlugin()
{
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
        FreeLibrary(g_hACPluginModule);
        g_hACPluginModule = nullptr;
        NetworkLog(ELogVerbosity::LOG_RELEASE, "[AC] Unloaded plugin");
    }
}
