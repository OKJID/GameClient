
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
