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

void AnticheatPlugInterface::LoadPlugin(const char* szPluginName)
{
    g_hACPluginModule = LoadLibraryA(szPluginName);

    if (!g_hACPluginModule)
    {
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
        AC_PLUGIN_LOAD_FUNCTION(IsLoaded);

#if _DEBUG
        SetWindowText(ApplicationHWnd, Functions.fnIsLoaded() ? "SECURED" : "INSECURE");
#endif

        // set action required callback
        AC_PLUGIN_LOAD_FUNCTION(SetACActionRequiredCallback);

        Functions.fnSetACActionRequiredCallback([](uint32_t userID, const char* szReason, EAnticheatActionType actionType, EAnticheatActionReason actionReason)
            {
                NetworkLog(ELogVerbosity::LOG_RELEASE, "[AC] Leaving lobby, lobby isn't secure.");
                extern void PopBackToLobby();
                PopBackToLobby();
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

        // TODO_AC: Deregister player
    }
}

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
            });
    }
}

void AnticheatPlugInterface::BeginSession()
{
    if (IsPluginLoaded() && Functions.fnBeginSession != nullptr)
    {
        Functions.fnBeginSession;
    }
}

void AnticheatPlugInterface::EndSession()
{
    if (IsPluginLoaded() && Functions.fnEndSession != nullptr)
    {
        Functions.fnEndSession;
    }
}

AnticheatPlugInterface::AnticheatPluginFunctionPtrs AnticheatPlugInterface::Functions;

HMODULE AnticheatPlugInterface::g_hACPluginModule = nullptr;

bool AnticheatPlugInterface::RegisterPlayer(std::string mwUserID, uint32_t goUserID)
{
    if (IsPluginLoaded() && Functions.fnRegisterPlayer != nullptr)
    {
        NetworkLog(ELogVerbosity::LOG_RELEASE, "RegisterPlayer: %s to %" PRIu64, mwUserID.c_str(), goUserID);

        bool bReg = Functions.fnRegisterPlayer(mwUserID.c_str(), goUserID);
        NetworkLog(ELogVerbosity::LOG_RELEASE, "RegisterPlayerFunc result: %d", bReg);
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
        FreeLibrary(g_hACPluginModule);
        g_hACPluginModule = nullptr;
    }
}
