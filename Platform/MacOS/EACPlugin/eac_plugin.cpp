// EAC Plugin for macOS — EOS Anti-Cheat Client wrapper
// Credentials loaded from environment variables or .eac_credentials (gitignored)

#include <cstdio>
#include <cstdint>
#include <cstring>
#include <string>
#include <atomic>
#include <fstream>

#include "eos_sdk.h"
#include "eos_init.h"
#include "eos_platform_prereqs.h"
#include "eos_connect.h"
#include "eos_connect_types.h"
#include "eos_anticheatclient.h"
#include "eos_anticheatclient_types.h"
#include "eos_anticheatcommon_types.h"
#include "eos_types.h"

static std::string g_ProductId;
static std::string g_SandboxId;
static std::string g_DeploymentId;
static int g_AnticheatIdentifier = 1;

static bool LoadCredentialsFromFile(const char* path)
{
    std::ifstream file(path);
    if (!file.is_open())
    {
        return false;
    }

    std::string line;
    while (std::getline(file, line))
    {
        if (line.empty() || line[0] == '#')
        {
            continue;
        }

        auto eq = line.find('=');
        if (eq == std::string::npos)
        {
            continue;
        }

        std::string key = line.substr(0, eq);
        std::string val = line.substr(eq + 1);

        if (key == "PRODUCT_ID")          g_ProductId = val;
        else if (key == "SANDBOX_ID")     g_SandboxId = val;
        else if (key == "DEPLOYMENT_ID")  g_DeploymentId = val;
        else if (key == "ANTICHEAT_ID")   g_AnticheatIdentifier = std::atoi(val.c_str());
    }

    return !g_ProductId.empty();
}

static void LoadCredentials()
{
    const char* envProduct    = getenv("EAC_PRODUCT_ID");
    const char* envSandbox    = getenv("EAC_SANDBOX_ID");
    const char* envDeployment = getenv("EAC_DEPLOYMENT_ID");
    const char* envAcId       = getenv("EAC_ANTICHEAT_ID");

    if (envProduct && envSandbox && envDeployment)
    {
        g_ProductId    = envProduct;
        g_SandboxId    = envSandbox;
        g_DeploymentId = envDeployment;
        if (envAcId) g_AnticheatIdentifier = std::atoi(envAcId);
        return;
    }

    if (LoadCredentialsFromFile(".eac_credentials"))
    {
        return;
    }

    LoadCredentialsFromFile("plugins/easyanticheat/.eac_credentials");
}

static EOS_HPlatform g_PlatformHandle = nullptr;
static EOS_HAntiCheatClient g_ACClientHandle = nullptr;
static EOS_HConnect g_ConnectHandle = nullptr;
static EOS_ProductUserId g_LocalProductUserId = nullptr;

static std::atomic<bool> g_bInitialized{false};
static std::atomic<bool> g_bLoggedIn{false};
static std::atomic<bool> g_bSessionStarted{false};

static char g_MiddlewareAuthToken[4096] = {};

typedef void (*LoggingFunc)(const char*);
typedef void (*LoginCallbackFunc)(bool bSuccess);
typedef void (*ACPlayerActionCallbackFunc)(uint32_t, const char*, int32_t, int32_t);
typedef void (*ACIntegrityViolationCallbackFunc)(const char*, int);
typedef void (*SendMessageViaTransportCallbackFunc)(uint32_t, const void*, uint32_t);

static LoggingFunc g_LogFunc = nullptr;
static ACPlayerActionCallbackFunc g_ActionCallback = nullptr;
static ACIntegrityViolationCallbackFunc g_IntegrityCallback = nullptr;
static SendMessageViaTransportCallbackFunc g_SendMessageCallback = nullptr;
static LoginCallbackFunc g_PendingLoginCallback = nullptr;

static EOS_NotificationId g_PeerMessageNotifId = EOS_INVALID_NOTIFICATIONID;
static EOS_NotificationId g_PeerActionNotifId = EOS_INVALID_NOTIFICATIONID;

static void PluginLog(const char* fmt, ...)
{
    char buf[2048];
    va_list args;
    va_start(args, fmt);
    vsnprintf(buf, sizeof(buf), fmt, args);
    va_end(args);

    if (g_LogFunc)
    {
        g_LogFunc(buf);
    }
    else
    {
        printf("[EAC_PLUGIN] %s\n", buf);
        fflush(stdout);
    }
}

// EOS Callbacks
static void EOS_CALL OnConnectLoginComplete(const EOS_Connect_LoginCallbackInfo* Data)
{
    if (Data->ResultCode == EOS_EResult::EOS_Success)
    {
        g_LocalProductUserId = Data->LocalUserId;
        g_bLoggedIn = true;

        EOS_Connect_CopyIdTokenOptions copyOpts = {};
        copyOpts.ApiVersion = EOS_CONNECT_COPYIDTOKEN_API_LATEST;
        copyOpts.LocalUserId = g_LocalProductUserId;

        EOS_Connect_IdToken* idToken = nullptr;
        EOS_EResult result = EOS_Connect_CopyIdToken(g_ConnectHandle, &copyOpts, &idToken);
        if (result == EOS_EResult::EOS_Success && idToken && idToken->JsonWebToken)
        {
            strncpy(g_MiddlewareAuthToken, idToken->JsonWebToken, sizeof(g_MiddlewareAuthToken) - 1);
            EOS_Connect_IdToken_Release(idToken);
            PluginLog("[EAC] CopyIdToken OK, token len=%zu", strlen(g_MiddlewareAuthToken));
        }
        else
        {
            PluginLog("[EAC] CopyIdToken FAILED: %d", (int)result);
        }

        PluginLog("[EAC] Login SUCCESS");
    }
    else
    {
        PluginLog("[EAC] Login FAILED: %d", (int)Data->ResultCode);
        g_bLoggedIn = false;
    }

    if (g_PendingLoginCallback)
    {
        g_PendingLoginCallback(g_bLoggedIn);
        g_PendingLoginCallback = nullptr;
    }
}

static void EOS_CALL OnPeerMessageToPeer(const EOS_AntiCheatCommon_OnMessageToClientCallbackInfo* Data)
{
    if (!g_SendMessageCallback)
    {
        return;
    }

    uint32_t goUserId = 0;
    if (Data->ClientHandle)
    {
        goUserId = (uint32_t)(uintptr_t)Data->ClientHandle;
    }

    g_SendMessageCallback(goUserId, Data->MessageData, Data->MessageDataSizeBytes);
}

static void EOS_CALL OnPeerActionRequired(const EOS_AntiCheatCommon_OnClientActionRequiredCallbackInfo* Data)
{
    if (!g_ActionCallback)
    {
        return;
    }

    uint32_t goUserId = 0;
    if (Data->ClientHandle)
    {
        goUserId = (uint32_t)(uintptr_t)Data->ClientHandle;
    }

    const char* reason = Data->ActionReasonDetailsString ? Data->ActionReasonDetailsString : "Unknown";
    g_ActionCallback(goUserId, reason, (int32_t)Data->ClientAction, (int32_t)Data->ActionReasonCode);
}

// Exported functions — must match PluginInterfaces.h typedefs exactly

extern "C" {

void SetLoggingFunction(LoggingFunc func)
{
    g_LogFunc = func;
    PluginLog("[EAC] Logging function set");
}

int Initialize(void)
{
    PluginLog("[EAC] Initialize() called");

    LoadCredentials();
    PluginLog("[EAC] Credentials: product='%s' sandbox='%s' deploy='%s' ac_id=%d",
        g_ProductId.c_str(), g_SandboxId.c_str(), g_DeploymentId.c_str(), g_AnticheatIdentifier);

    if (g_ProductId.empty() || g_SandboxId.empty() || g_DeploymentId.empty())
    {
        PluginLog("[EAC] WARNING: Credentials not configured. Set EAC_PRODUCT_ID/EAC_SANDBOX_ID/EAC_DEPLOYMENT_ID env vars or create .eac_credentials file");
        g_bInitialized = true;
        return -3;
    }

    EOS_InitializeOptions initOpts = {};
    initOpts.ApiVersion = EOS_INITIALIZE_API_LATEST;
    initOpts.ProductName = "GeneralsOnline";
    initOpts.ProductVersion = "1.0";

    EOS_EResult result = EOS_Initialize(&initOpts);
    if (result != EOS_EResult::EOS_Success && result != EOS_EResult::EOS_AlreadyConfigured)
    {
        PluginLog("[EAC] EOS_Initialize FAILED: %d", (int)result);
        return -1;
    }

    EOS_Platform_Options platformOpts = {};
    platformOpts.ApiVersion = EOS_PLATFORM_OPTIONS_API_LATEST;
    platformOpts.ProductId = g_ProductId.c_str();
    platformOpts.SandboxId = g_SandboxId.c_str();
    platformOpts.DeploymentId = g_DeploymentId.c_str();
    platformOpts.bIsServer = EOS_FALSE;
    platformOpts.Flags = EOS_PF_DISABLE_OVERLAY;

    g_PlatformHandle = EOS_Platform_Create(&platformOpts);
    if (!g_PlatformHandle)
    {
        PluginLog("[EAC] EOS_Platform_Create FAILED (credentials likely invalid)");
        g_bInitialized = true;
        return -2;
    }

    g_ConnectHandle = EOS_Platform_GetConnectInterface(g_PlatformHandle);
    g_ACClientHandle = EOS_Platform_GetAntiCheatClientInterface(g_PlatformHandle);

    if (!g_ACClientHandle)
    {
        PluginLog("[EAC] WARNING: AntiCheatClient interface is NULL (EAC might not be enabled for this product)");
    }

    g_bInitialized = true;
    PluginLog("[EAC] Initialize() OK. Platform=%p AC=%p Connect=%p", g_PlatformHandle, g_ACClientHandle, g_ConnectHandle);
    return 0;
}

bool IsExternalProcessRunning(void)
{
    // On macOS, the EAC bootstrapper (start_protected_game) is optional for now
    // Return true so the game doesn't block on this check
    return true;
}

int GetAnticheatIdentifier(void)
{
    return g_AnticheatIdentifier;
}

void SetACActionRequiredCallback(ACPlayerActionCallbackFunc func)
{
    g_ActionCallback = func;
    PluginLog("[EAC] SetACActionRequiredCallback set");
}

void SetACIntegrityViolationOccurredCallback(ACIntegrityViolationCallbackFunc func)
{
    g_IntegrityCallback = func;
    PluginLog("[EAC] SetACIntegrityViolationOccurredCallback set");
}

void SetSendMessageViaTransportCallback(SendMessageViaTransportCallbackFunc func)
{
    g_SendMessageCallback = func;
    PluginLog("[EAC] SetSendMessageViaTransportCallback set");
}

void ACMessageArrivedViaTransport(uint32_t goUserID, void* pData, uint32_t dataLen)
{
    if (!g_ACClientHandle || !g_bSessionStarted)
    {
        return;
    }

    EOS_AntiCheatClient_ReceiveMessageFromPeerOptions opts = {};
    opts.ApiVersion = EOS_ANTICHEATCLIENT_RECEIVEMESSAGEFROMPEER_API_LATEST;
    opts.PeerHandle = (EOS_AntiCheatCommon_ClientHandle)(uintptr_t)goUserID;
    opts.DataLengthBytes = dataLen;
    opts.Data = pData;

    EOS_AntiCheatClient_ReceiveMessageFromPeer(g_ACClientHandle, &opts);
}

void Login(const char* szGameToken, LoginCallbackFunc cb)
{
    PluginLog("[EAC] Login() called, token len=%zu", strlen(szGameToken));
    g_PendingLoginCallback = cb;

    if (!g_ConnectHandle)
    {
        PluginLog("[EAC] Login FAILED: no connect handle (platform not initialized)");
        if (cb) cb(false);
        g_PendingLoginCallback = nullptr;
        return;
    }

    EOS_Connect_Credentials credentials = {};
    credentials.ApiVersion = EOS_CONNECT_CREDENTIALS_API_LATEST;
    credentials.Token = szGameToken;
    credentials.Type = EOS_EExternalCredentialType::EOS_ECT_DEVICEID_ACCESS_TOKEN;

    EOS_Connect_LoginOptions loginOpts = {};
    loginOpts.ApiVersion = EOS_CONNECT_LOGIN_API_LATEST;
    loginOpts.Credentials = &credentials;

    EOS_Connect_Login(g_ConnectHandle, &loginOpts, nullptr, OnConnectLoginComplete);
}

void RefreshToken(const char* szGameToken, LoginCallbackFunc cb)
{
    PluginLog("[EAC] RefreshToken() called");
    Login(szGameToken, cb);
}

bool IsLoggedIn(void)
{
    return g_bLoggedIn;
}

bool GetMiddlewareAuthToken(char* buffer, size_t bufferSize)
{
    if (g_MiddlewareAuthToken[0] == '\0')
    {
        return false;
    }

    strncpy(buffer, g_MiddlewareAuthToken, bufferSize - 1);
    buffer[bufferSize - 1] = '\0';
    return true;
}

void BeginSession(void)
{
    PluginLog("[EAC] BeginSession() called");

    if (!g_ACClientHandle)
    {
        PluginLog("[EAC] BeginSession SKIPPED: no AC client handle");
        g_bSessionStarted = true;
        return;
    }

    // Register callbacks
    EOS_AntiCheatClient_AddNotifyMessageToPeerOptions msgOpts = {};
    msgOpts.ApiVersion = EOS_ANTICHEATCLIENT_ADDNOTIFYMESSAGETOPEER_API_LATEST;
    g_PeerMessageNotifId = EOS_AntiCheatClient_AddNotifyMessageToPeer(g_ACClientHandle, &msgOpts, nullptr,
        (EOS_AntiCheatClient_OnMessageToPeerCallback)OnPeerMessageToPeer);

    EOS_AntiCheatClient_AddNotifyPeerActionRequiredOptions actionOpts = {};
    actionOpts.ApiVersion = EOS_ANTICHEATCLIENT_ADDNOTIFYPEERACTIONREQUIRED_API_LATEST;
    g_PeerActionNotifId = EOS_AntiCheatClient_AddNotifyPeerActionRequired(g_ACClientHandle, &actionOpts, nullptr,
        (EOS_AntiCheatClient_OnPeerActionRequiredCallback)OnPeerActionRequired);

    EOS_AntiCheatClient_BeginSessionOptions beginOpts = {};
    beginOpts.ApiVersion = EOS_ANTICHEATCLIENT_BEGINSESSION_API_LATEST;
    beginOpts.LocalUserId = g_LocalProductUserId;
    beginOpts.Mode = EOS_EAntiCheatClientMode::EOS_ACCM_PeerToPeer;

    EOS_EResult result = EOS_AntiCheatClient_BeginSession(g_ACClientHandle, &beginOpts);
    PluginLog("[EAC] BeginSession result: %d", (int)result);

    g_bSessionStarted = true;
}

void EndSession(void)
{
    PluginLog("[EAC] EndSession() called");

    if (!g_ACClientHandle || !g_bSessionStarted)
    {
        g_bSessionStarted = false;
        return;
    }

    EOS_AntiCheatClient_EndSessionOptions endOpts = {};
    endOpts.ApiVersion = EOS_ANTICHEATCLIENT_ENDSESSION_API_LATEST;
    EOS_AntiCheatClient_EndSession(g_ACClientHandle, &endOpts);

    if (g_PeerMessageNotifId != EOS_INVALID_NOTIFICATIONID)
    {
        EOS_AntiCheatClient_RemoveNotifyMessageToPeer(g_ACClientHandle, g_PeerMessageNotifId);
        g_PeerMessageNotifId = EOS_INVALID_NOTIFICATIONID;
    }
    if (g_PeerActionNotifId != EOS_INVALID_NOTIFICATIONID)
    {
        EOS_AntiCheatClient_RemoveNotifyPeerActionRequired(g_ACClientHandle, g_PeerActionNotifId);
        g_PeerActionNotifId = EOS_INVALID_NOTIFICATIONID;
    }

    g_bSessionStarted = false;
}

bool RegisterPlayer(const char* szMiddlewareUserID, uint32_t goUserID)
{
    PluginLog("[EAC] RegisterPlayer mwID=%s goID=%u", szMiddlewareUserID, goUserID);

    if (!g_ACClientHandle || !g_bSessionStarted)
    {
        PluginLog("[EAC] RegisterPlayer SKIPPED: not ready");
        return false;
    }

    EOS_AntiCheatClient_RegisterPeerOptions opts = {};
    opts.ApiVersion = EOS_ANTICHEATCLIENT_REGISTERPEER_API_LATEST;
    opts.PeerHandle = (EOS_AntiCheatCommon_ClientHandle)(uintptr_t)goUserID;
    opts.PeerProductUserId = EOS_ProductUserId_FromString(szMiddlewareUserID);
    opts.ClientType = EOS_EAntiCheatCommonClientType::EOS_ACCCT_ProtectedClient;
    opts.ClientPlatform = EOS_EAntiCheatCommonClientPlatform::EOS_ACCCP_Unknown;

    EOS_EResult result = EOS_AntiCheatClient_RegisterPeer(g_ACClientHandle, &opts);
    PluginLog("[EAC] RegisterPlayer result: %d", (int)result);
    return result == EOS_EResult::EOS_Success;
}

bool DeregisterPlayer(const char* szMiddlewareUserID, uint32_t goUserID)
{
    PluginLog("[EAC] DeregisterPlayer mwID=%s goID=%u", szMiddlewareUserID, goUserID);

    if (!g_ACClientHandle || !g_bSessionStarted)
    {
        return false;
    }

    EOS_AntiCheatClient_UnregisterPeerOptions opts = {};
    opts.ApiVersion = EOS_ANTICHEATCLIENT_UNREGISTERPEER_API_LATEST;
    opts.PeerHandle = (EOS_AntiCheatCommon_ClientHandle)(uintptr_t)goUserID;

    EOS_EResult result = EOS_AntiCheatClient_UnregisterPeer(g_ACClientHandle, &opts);
    PluginLog("[EAC] DeregisterPlayer result: %d", (int)result);
    return result == EOS_EResult::EOS_Success;
}

void Tick(void)
{
    if (g_PlatformHandle)
    {
        EOS_Platform_Tick(g_PlatformHandle);
    }
}

void Shutdown(void)
{
    PluginLog("[EAC] Shutdown() called");

    if (g_bSessionStarted)
    {
        EndSession();
    }

    if (g_PlatformHandle)
    {
        EOS_Platform_Release(g_PlatformHandle);
        g_PlatformHandle = nullptr;
    }

    EOS_Shutdown();
    g_bInitialized = false;
    g_bLoggedIn = false;
    PluginLog("[EAC] Shutdown complete");
}

} // extern "C"
