#define __INTLRESOURCES__
#define __FINDER__
#define __AIFF__

#define Byte MacByte
#define RGBColor MacRGBColor
#define BOOL MacBOOL
#import <Foundation/Foundation.h>
#undef Byte
#undef RGBColor
#undef BOOL

#include <cstdlib>
#include <cstring>
#include <atomic>
#include <ctime>
#include <cxxabi.h>
#include <exception>
#include <typeinfo>
#include <fcntl.h>
#include <pthread.h>
#include <unistd.h>
#include <signal.h>
#include <execinfo.h>
#include <mach/mach.h>
#include <mach/mach_vm.h>
#include <sys/ucontext.h>

#include "Utility/CppMacros.h"
#include "WWLib/always.h"
#include <windows.h>

#include "Lib/BaseType.h"
#include "Common/Errors.h"
#include "Common/INI.h"
#include "Common/INIException.h"
#include "MacOSCrashHandler.h"
#include "MacOSCrashTrail.h"

// ── Signal handler (mirrors UnHandledExceptionFilter) ──

static const int kCrashReportFiles = 4;
static const size_t kCrashReportPathSize = 1024;
static char s_crashReportPaths[kCrashReportFiles][kCrashReportPathSize];
static char s_crashReportBuild[256];
static char s_crashReportArgs[2048];
static int s_crashReportFd = -1;
static std::atomic<pthread_t> s_crashThread{nullptr};

static void prepareCrashReportPaths(const char* reportNamePrefix) {
    const char* home = getenv("HOME");
    if (home == nullptr) {
        return;
    }

#if defined(RTS_ZEROHOUR)
    const char* userDataDir = "Command and Conquer Generals Zero Hour Data";
#else
    const char* userDataDir = "Command and Conquer Generals Data";
#endif
    snprintf(s_crashReportPaths[0], kCrashReportPathSize, "%s/%s/%sMacCrash.txt", home, userDataDir, reportNamePrefix);
    snprintf(s_crashReportPaths[1], kCrashReportPathSize, "%s.bak", s_crashReportPaths[0]);
    for (int backup = 2; backup < kCrashReportFiles; ++backup) {
        snprintf(s_crashReportPaths[backup], kCrashReportPathSize, "%s.bak%d", s_crashReportPaths[0], backup);
    }
}

static void prepareCrashReportBuild() {
    NSBundle* bundle = [NSBundle mainBundle];
    NSString* version = [bundle objectForInfoDictionaryKey:@"CFBundleShortVersionString"];
    NSString* build = [bundle objectForInfoDictionaryKey:@"CFBundleVersion"];
    snprintf(s_crashReportBuild, sizeof(s_crashReportBuild), "CRASH BUILD: %s (%s)",
        version != nil ? version.UTF8String : "?", build != nil ? build.UTF8String : "?");

    NSString* arguments = [[[NSProcessInfo processInfo] arguments] componentsJoinedByString:@" "];
    snprintf(s_crashReportArgs, sizeof(s_crashReportArgs), "CRASH ARGS: %s", arguments.UTF8String);
}

static void writeCrashText(int fd, const char* text) {
    if (fd < 0) {
        return;
    }

    write(fd, text, strlen(text));
    write(fd, "\n", 1);
}

static void logCrashLine(const char* line) {
    writeCrashText(STDOUT_FILENO, line);
    writeCrashText(s_crashReportFd, line);
}

static void flushStdoutIfUnlocked() {
    if (ftrylockfile(stdout) != 0) {
        return;
    }

    fflush(stdout);
    funlockfile(stdout);
}

static void openCrashReport() {
    if (s_crashReportPaths[0][0] == '\0') {
        return;
    }

    for (int backup = kCrashReportFiles - 1; backup > 0; --backup) {
        rename(s_crashReportPaths[backup - 1], s_crashReportPaths[backup]);
    }

    s_crashReportFd = open(s_crashReportPaths[0], O_WRONLY | O_CREAT | O_TRUNC, 0644);
}

static void closeCrashReport() {
    if (s_crashReportFd < 0) {
        return;
    }

    close(s_crashReportFd);
    s_crashReportFd = -1;
}

static void logCrashTime() {
    char line[64];
    const time_t now = time(nullptr);
    struct tm utc;
    gmtime_r(&now, &utc);
    snprintf(line, sizeof(line), "CRASH TIME: %04d-%02d-%02d %02d:%02d:%02d UTC",
        utc.tm_year + 1900, utc.tm_mon + 1, utc.tm_mday, utc.tm_hour, utc.tm_min, utc.tm_sec);
    logCrashLine(line);
}

static void logCrashBacktrace() {
    void* callstack[128];
    const int frames = backtrace(callstack, 128);
    logCrashLine("CRASH BACKTRACE:");
    backtrace_symbols_fd(callstack, frames, STDOUT_FILENO);
    if (s_crashReportFd >= 0) {
        backtrace_symbols_fd(callstack, frames, s_crashReportFd);
    }
}

static void logFaultMemoryRegion(uintptr_t address) {
    char line[256];
    mach_vm_address_t regionStart = address;
    mach_vm_size_t regionSize = 0;
    vm_region_basic_info_data_64_t info;
    mach_msg_type_number_t infoCount = VM_REGION_BASIC_INFO_COUNT_64;
    mach_port_t objectName = MACH_PORT_NULL;
    const kern_return_t result = mach_vm_region(mach_task_self(), &regionStart, &regionSize, VM_REGION_BASIC_INFO_64,
        (vm_region_info_t)&info, &infoCount, &objectName);

    if (result != KERN_SUCCESS) {
        snprintf(line, sizeof(line), "CRASH FAULT REGION: none above 0x%lx (kern %d)", (unsigned long)address, result);
        logCrashLine(line);
        return;
    }

    if (regionStart > address) {
        snprintf(line, sizeof(line), "CRASH FAULT REGION: unmapped, next region 0x%llx",
            (unsigned long long)regionStart);
        logCrashLine(line);
        return;
    }

    snprintf(line, sizeof(line), "CRASH FAULT REGION: 0x%llx-0x%llx protection %d max %d",
        (unsigned long long)regionStart, (unsigned long long)(regionStart + regionSize), info.protection, info.max_protection);
    logCrashLine(line);
}

static void logFaultContext(const siginfo_t* info, void* context) {
    char line[256];
    const uintptr_t faultAddress = (uintptr_t)info->si_addr;
    snprintf(line, sizeof(line), "CRASH FAULT ADDRESS: 0x%lx code %d", (unsigned long)faultAddress, info->si_code);
    logCrashLine(line);

#if defined(__arm64__)
    const ucontext_t* userContext = (const ucontext_t*)context;
    const __darwin_arm_thread_state64& state = userContext->uc_mcontext->__ss;
    snprintf(line, sizeof(line), "CRASH REGISTERS: pc 0x%llx lr 0x%llx sp 0x%llx fp 0x%llx x0 0x%llx x19 0x%llx x20 0x%llx x21 0x%llx",
        (unsigned long long)__darwin_arm_thread_state64_get_pc(state), (unsigned long long)__darwin_arm_thread_state64_get_lr(state),
        (unsigned long long)__darwin_arm_thread_state64_get_sp(state), (unsigned long long)__darwin_arm_thread_state64_get_fp(state),
        (unsigned long long)state.__x[0], (unsigned long long)state.__x[19], (unsigned long long)state.__x[20],
        (unsigned long long)state.__x[21]);
    logCrashLine(line);
#endif

    logFaultMemoryRegion(faultAddress);
}

static void waitForCrashReportOrExit() {
    pthread_t reportingThread = nullptr;
    if (s_crashThread.compare_exchange_strong(reportingThread, pthread_self())) {
        return;
    }

    if (pthread_equal(reportingThread, pthread_self())) {
        _exit(1);
    }

    for (;;) {
        pause();
    }
}

static void macosSignalHandler(int sig, siginfo_t* info, void* context) {
    waitForCrashReportOrExit();
    flushStdoutIfUnlocked();
    openCrashReport();
    logCrashTime();

    char line[64];
    snprintf(line, sizeof(line), "FATAL: Caught signal %d", sig);
    logCrashLine(line);

    MacOSCrashTrail::report(logCrashLine);
    logFaultContext(info, context);
    logCrashBacktrace();
    logCrashLine(s_crashReportBuild);
    logCrashLine(s_crashReportArgs);
    closeCrashReport();

    signal(sig, SIG_DFL);
    raise(sig);
}

static void installCrashHandler(int sig) {
    struct sigaction action = {};
    action.sa_sigaction = macosSignalHandler;
    action.sa_flags = SA_SIGINFO;
    sigemptyset(&action.sa_mask);
    sigaction(sig, &action, nullptr);
}

static std::terminate_handler s_previousTerminateHandler = nullptr;

static void recordUnknownExceptionType() {
    const std::type_info* type = abi::__cxa_current_exception_type();
    if (type == nullptr) {
        MacOSCrashTrail::setExceptionReason("unknown exception");
        return;
    }

    int status = 0;
    char* demangled = abi::__cxa_demangle(type->name(), nullptr, nullptr, &status);
    MacOSCrashTrail::setExceptionReason("%s", demangled != nullptr ? demangled : type->name());
    free(demangled);
}

static void recordTerminatingException() {
    std::exception_ptr exception = std::current_exception();
    if (!exception) {
        return;
    }

    try {
        @try {
            std::rethrow_exception(exception);
        } @catch (NSException* nsException) {
            MacOSCrashTrail::setExceptionReason("%s: %s", nsException.name.UTF8String, nsException.reason.UTF8String ?: "");
        }
    } catch (const INIException& iniException) {
        MacOSCrashTrail::setExceptionReason("INIException: %s", iniException.mFailureMessage ?: "no message");
    } catch (const std::exception& stdException) {
        MacOSCrashTrail::setExceptionReason("%s: %s", typeid(stdException).name(), stdException.what());
    } catch (ErrorCode errorCode) {
        MacOSCrashTrail::setExceptionReason("ErrorCode 0x%08X", (unsigned int)errorCode);
    } catch (decltype(INI_UNKNOWN_TOKEN) iniError) {
        MacOSCrashTrail::setExceptionReason("INI error 0x%08X", (unsigned int)iniError);
    } catch (...) {
        recordUnknownExceptionType();
    }
}

static void macosTerminateHandler() {
    recordTerminatingException();
    if (s_previousTerminateHandler != nullptr) {
        s_previousTerminateHandler();
    }
    abort();
}

void MacOS_InstallCrashHandlers(const char* reportNamePrefix) {
    MacOSCrashTrail::start();
    prepareCrashReportPaths(reportNamePrefix);
    prepareCrashReportBuild();
    installCrashHandler(SIGSEGV);
    installCrashHandler(SIGBUS);
    installCrashHandler(SIGABRT);
    s_previousTerminateHandler = std::set_terminate(macosTerminateHandler);
}
