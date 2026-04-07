#pragma once


enum class EAnticheatActionType : int32_t
{
    NONE = 0,
    KICK = 1
};

enum class EAnticheatActionReason : int32_t
{
    Unknown = 0,
    InternalError = 1,
    InvalidMessage = 2,
    AuthFailure = 3,
    ACNotRunning = 4,
    HeartbeatTimedOut = 5,
    ClientViolation = 6,
    BackendViolation = 7,
    TempCooldown = 8,
    TempBanned = 9,
    PermaBanned = 10
};


static class AnticheatPlugInterface
{
public:
    static void AC_NetworkMessageArrived(uint32_t goUserID, void* pData, uint32_t dataLen);

    static bool IsPluginLoaded()
    {
        return g_hACPluginModule != nullptr;
    }

    static void LoadPlugin(const char* szPluginName);
    static void Authenticate();
    static void UnloadPlugin();
    static void Tick();

    static bool RegisterPlayer(std::string mwUserID, uint32_t goUserID);

    static void BeginSession();
    static void EndSession();

    // Callbacks from plugin
    typedef void (*LoginCallback)(bool bSuccess);
    typedef void (*LoggingFunc)(const char*);
    typedef void (*FuncDefACPlayerActionRequiredCallbackFunc)(uint32_t, const char*, EAnticheatActionType, EAnticheatActionReason);
    typedef void (*FuncDefSetACActionRequiredCallback)(FuncDefACPlayerActionRequiredCallbackFunc);
    typedef void (*SendMessageViaTransportCallbackFunc)(uint32_t, const void*, uint32_t);

    // Func defs
    typedef void (*FuncDefSetLoggingFunction)(LoggingFunc);
    typedef int (*FuncDefInitialize)(void);
    typedef bool (*FuncDefIsLoaded)(void);
    
    typedef void (*FuncDefSetSendMessageViaTransportCallback)(SendMessageViaTransportCallbackFunc);
    typedef void (*FuncDefACMessageArrigedViaTransport)(uint32_t, void*, uint32_t);
    typedef void (*FuncDefLogin)(const char* szGameToken, LoginCallback cb);
    typedef bool (*FuncDefGetMiddlewareAuthToken)(char* buffer, size_t bufferSize);
    typedef bool (*FuncDefIsLoggedIn)(void);
    typedef void (*FuncDefBeginSession)(void);
    typedef void (*FuncDefEndSession)(void);
    typedef bool (*FuncDefRegisterPlayer)(const char* szMiddlewareUserID, uint32_t goUserID);
    typedef void (*FuncDefTick)(void);

    struct AnticheatPluginFunctionPtrs
    {
        FuncDefSetLoggingFunction fnSetLoggingFunction = nullptr;
        FuncDefInitialize fnInitialize = nullptr;
        FuncDefIsLoaded fnIsLoaded = nullptr;
        FuncDefSetACActionRequiredCallback fnSetACActionRequiredCallback = nullptr;
        FuncDefSetSendMessageViaTransportCallback fnSetSendMessageViaTransportCallback = nullptr;
        FuncDefACMessageArrigedViaTransport fnACMessageArrigedViaTransport = nullptr;
        FuncDefLogin fnLogin = nullptr;
        FuncDefGetMiddlewareAuthToken fnGetMiddlewareAuthToken = nullptr;
        FuncDefIsLoggedIn fnIsLoggedIn = nullptr;
        FuncDefBeginSession fnBeginSession = nullptr;
        FuncDefEndSession fnEndSession = nullptr;
        FuncDefRegisterPlayer fnRegisterPlayer = nullptr;
        FuncDefTick fnTick = nullptr;
    };
    static AnticheatPluginFunctionPtrs Functions;

    // Module
    static HMODULE g_hACPluginModule;
};

extern HWND ApplicationHWnd;
