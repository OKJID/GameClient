#ifdef __APPLE__
#include <stddef.h>
#include <stdarg.h>
#include <stdint.h>

extern "C" {
    // Sentry Stubs
    typedef struct sentry_options_s sentry_options_t;
    typedef union { unsigned long long _bits; } sentry_value_t;
    typedef enum { SENTRY_LEVEL_DEBUG = -1, SENTRY_LEVEL_INFO = 0, SENTRY_LEVEL_WARNING = 1, SENTRY_LEVEL_ERROR = 2, SENTRY_LEVEL_FATAL = 3 } sentry_level_t;

    sentry_options_t* sentry_options_new() { return nullptr; }
    void sentry_options_set_dsn(sentry_options_t*, const char*) {}
    void sentry_options_set_database_path(sentry_options_t*, const char*) {}
    void sentry_options_set_release(sentry_options_t*, const char*) {}
    void sentry_options_set_environment(sentry_options_t*, const char*) {}
    void sentry_options_set_debug(sentry_options_t*, int) {}
    void sentry_options_set_logger_level(sentry_options_t*, sentry_level_t) {}
    void sentry_options_set_logger(sentry_options_t*, void (*)(sentry_level_t, const char*, va_list, void*)) {}
    void sentry_init(sentry_options_t*) {}
    void sentry_close() {}
    sentry_value_t sentry_value_new_object() { sentry_value_t v = {0}; return v; }
    sentry_value_t sentry_value_new_string(const char*) { sentry_value_t v = {0}; return v; }
    sentry_value_t sentry_value_new_int32(int32_t) { sentry_value_t v = {0}; return v; }
    void sentry_value_set_by_key(sentry_value_t, const char*, sentry_value_t) {}
    void sentry_set_context(const char*, sentry_value_t) {}
    void sentry_set_extra(const char*, sentry_value_t) {}
    void sentry_set_tag(const char*, const char*) {}
    sentry_value_t sentry_value_new_message_event(sentry_level_t, const char*, const char*) { sentry_value_t v = {0}; return v; }
    void sentry_capture_event(sentry_value_t) {}
    
    // SteamNetworkingSockets Stubs
    struct SteamNetworkingIdentity;
    struct SteamNetworkingErrMsg;
    
    bool GameNetworkingSockets_Init(const SteamNetworkingIdentity*, SteamNetworkingErrMsg*) { return true; }
    void GameNetworkingSockets_Kill() {}
    
    void SteamNetworkingIdentity_ToString(const SteamNetworkingIdentity*, char*, size_t) {}
    void* SteamNetworkingSockets_LibV12() { return nullptr; }
    void* SteamNetworkingUtils_LibV4() { return nullptr; }
    
    // cURL stubs
    void curl_easy_cleanup(void*) {}
    int curl_easy_getinfo(void*, int, ...) { return 0; }
    void* curl_easy_init() { return nullptr; }
    int curl_easy_setopt(void*, int, ...) { return 0; }
    const char* curl_easy_strerror(int) { return "curl error"; }
    void curl_global_cleanup() {}
    int curl_global_init(long) { return 0; }
    int curl_multi_add_handle(void*, void*) { return 0; }
    int curl_multi_cleanup(void*) { return 0; }
    void* curl_multi_info_read(void*, int*) { return nullptr; }
    void* curl_multi_init() { return nullptr; }
    int curl_multi_perform(void*, int*) { return 0; }
    int curl_multi_poll(void*, void*, unsigned int, int, int*) { return 0; }
    int curl_multi_remove_handle(void*, void*) { return 0; }
    void* curl_slist_append(void*, const char*) { return nullptr; }
    void curl_slist_free_all(void*) {}
    int curl_ws_recv(void*, void*, size_t, size_t*, const void**) { return 0; }
    int curl_ws_send(void*, const void*, size_t, size_t*, size_t, unsigned int) { return 0; }
}
#endif
#include <string>

bool SetStringInRegistry(std::string path, std::string key, std::string val) { return true; }
bool GetStringFromRegistry(std::string path, std::string key, std::string &val) { return false; }
bool SetUnsignedIntInRegistry(std::string path, std::string key, unsigned int val) { return true; }
bool GetUnsignedIntFromRegistry(std::string path, std::string key, unsigned int &val) { return false; }

void StopAsyncDNSCheck() {}
void StackDumpFromAddresses(void**, unsigned int, void (*)(char const*)) {}
void FillStackAddresses(void**, unsigned int, unsigned int) {}
void OSDisplaySetBusyState(bool, bool) {}
#include "Common/AsciiString.h"
AsciiString g_LastErrorDump;

#include "Common/INI.h"
#include "GameNetwork/WOLBrowser/WebBrowser.h"
const FieldParse WebBrowserURL::m_URLFieldParseTable[] = { {0} };
WebBrowser* TheWebBrowser = nullptr;

#include "WWLib/registry.h"
RegistryClass::RegistryClass(const char*, bool) : Key(0), IsValid(false) {}
RegistryClass::~RegistryClass() {}
int RegistryClass::Get_Int(const char*, int def) { return def; }

#include "WWDownload/ftp.h"
Cftp::Cftp() {}
Cftp::~Cftp() {}

// CDownload vtable
#include "WWDownload/Download.h"

HRESULT CDownload::PumpMessages() { return 0; }
HRESULT CDownload::Abort() { return 0; }
HRESULT CDownload::DownloadFile(LPCSTR server, LPCSTR username, LPCSTR password, LPCSTR file, LPCSTR localfile, LPCSTR regkey, bool tryresume) { return 0; }
HRESULT CDownload::GetLastLocalFile(char *local_file, int maxlen) { return 0; }
