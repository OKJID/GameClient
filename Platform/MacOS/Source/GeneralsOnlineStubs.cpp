
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
    
}

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
