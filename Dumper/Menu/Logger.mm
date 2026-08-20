//
//  Logger.mm
//  Dumper
//
//  Created by Euclid Jan Guillermo on 12/8/25.
//

#include "Logger.h"
#include "Console.h" // Your Console class
#include <cstdarg>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <mutex>
#include <fcntl.h>
#include <unistd.h>
#include <signal.h>
#include <dlfcn.h>
#include <execinfo.h>
#include <mach-o/dyld.h>

#import <Foundation/Foundation.h>

static std::mutex g_LogMutex;

static std::string DumpLogPath()
{
    const char* Home = getenv("HOME");
    if (!Home) Home = "/tmp";
    return std::string(Home) + "/Documents/Dumper.log";
}

// Mirrors every log line into Documents/Dumper.log so post-crash output can be
// pulled from the Files app (or the crash handler's backtrace lands alongside).
static void AppendToDumpLog(const char* Line)
{
    if (!Line) return;

    std::lock_guard<std::mutex> Lock(g_LogMutex);

    FILE* F = fopen(DumpLogPath().c_str(), "a");
    if (!F) return;

    fprintf(F, "%s\n", Line);
    fflush(F);
    fclose(F);
}

// Helper to format string and send to console
static void LogToConsole(int type, const char* fmt, va_list args) {
    char buffer[2048];
    vsnprintf(buffer, sizeof(buffer), fmt, args);
    std::string msg(buffer);

    AppendToDumpLog(msg.c_str());

    @autoreleasepool {
        NSLog(@"%@", [NSString stringWithUTF8String:msg.c_str()]);
    }

    // Assuming Console follows Singleton pattern or global instance
    // Type 0 = Info/Default, 1 = Error, 2 = Success/Special
    if (type == 1) {
        Console::Get().logError(msg);
    } else if (type == 2) {
        Console::Get().logInfo(msg); // Reusing logInfo for "Success" style if available, or just log
    } else {
        Console::Get().log(msg);
    }
}

static void CrashHandler(int Sig, siginfo_t* Info, void* /*Ctx*/)
{
    const int Fd = ::open(DumpLogPath().c_str(), O_CREAT | O_WRONLY | O_APPEND, 0644);
    if (Fd >= 0)
    {
        const intptr_t Slide = _dyld_get_image_vmaddr_slide(0);
        dprintf(Fd, "\n===== DUMPER CRASH (signal %d) slide=0x%lX =====\n", Sig, (unsigned long)Slide);
        if (Info && (Sig == SIGSEGV || Sig == SIGBUS))
            dprintf(Fd, "Faulting address: 0x%p\n", Info->si_addr);

        void* Frames[64];
        const int N = ::backtrace(Frames, 64);
        backtrace_symbols_fd(Frames, N, Fd);

        dprintf(Fd, "===== END CRASH =====\n\n");
        ::close(Fd);
    }

    signal(Sig, SIG_DFL);
    ::raise(Sig);
}

void InstallCrashLogger()
{
    struct sigaction Sa;
    memset(&Sa, 0, sizeof(Sa));
    Sa.sa_sigaction = CrashHandler;
    Sa.sa_flags = SA_SIGINFO | SA_RESETHAND;

    sigaction(SIGSEGV, &Sa, nullptr);
    sigaction(SIGBUS, &Sa, nullptr);
    sigaction(SIGABRT, &Sa, nullptr);
    sigaction(SIGILL, &Sa, nullptr);
    sigaction(SIGFPE, &Sa, nullptr);
}

void LogInfo(const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);
    LogToConsole(0, fmt, args);
    va_end(args);
}

void LogError(const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);
    LogToConsole(1, fmt, args);
    va_end(args);
}

void LogSuccess(const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);
    LogToConsole(2, fmt, args);
    va_end(args);
}
