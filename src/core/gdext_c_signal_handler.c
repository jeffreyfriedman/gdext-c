// gdext_c_signal_handler.c - Signal handlers for crash diagnosis
// Catches SIGABRT, SIGSEGV, etc. and provides detailed crash information

#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <string.h>
#include <execinfo.h>  // For backtrace()
#include <unistd.h>    // For write()

// Track the last operation for crash context
static char last_operation[256] = "Unknown";
static char last_method[128] = "Unknown";
static void* last_object = NULL;

// Signal handler for crash diagnosis
static void crash_signal_handler(int sig, siginfo_t *info, void *context) {
    const char *sig_name = "UNKNOWN";
    
    switch(sig) {
        case SIGSEGV: sig_name = "SIGSEGV (Segmentation Fault)"; break;
        case SIGABRT: sig_name = "SIGABRT (Abort)"; break;
        case SIGBUS: sig_name = "SIGBUS (Bus Error)"; break;
        case SIGILL: sig_name = "SIGILL (Illegal Instruction)"; break;
        case SIGFPE: sig_name = "SIGFPE (Floating Point Exception)"; break;
    }
    
    // Use write() instead of fprintf() as it's async-signal-safe
    const char header[] = "\n\n"
        "╔═══════════════════════════════════════════════════════════════╗\n"
        "║ 🚨 CRASH DETECTED - SIGNAL HANDLER ACTIVATED 🚨              ║\n"
        "╚═══════════════════════════════════════════════════════════════╝\n\n";
    write(STDERR_FILENO, header, sizeof(header) - 1);
    
    // Print signal info
    char sig_buf[256];
    snprintf(sig_buf, sizeof(sig_buf), "Signal: %s (%d)\n", sig_name, sig);
    write(STDERR_FILENO, sig_buf, strlen(sig_buf));
    
    if (info) {
        char addr_buf[256];
        snprintf(addr_buf, sizeof(addr_buf), "Fault Address: %p\n", info->si_addr);
        write(STDERR_FILENO, addr_buf, strlen(addr_buf));
        
        if (sig == SIGSEGV) {
            const char *segv_reason = "Unknown";
            switch(info->si_code) {
                case SEGV_MAPERR: segv_reason = "Address not mapped"; break;
                case SEGV_ACCERR: segv_reason = "Invalid permissions"; break;
            }
            char reason_buf[256];
            snprintf(reason_buf, sizeof(reason_buf), "SIGSEGV Reason: %s\n", segv_reason);
            write(STDERR_FILENO, reason_buf, strlen(reason_buf));
        }
    }
    
    // Print context
    const char context_header[] = "\n🔍 CRASH CONTEXT:\n";
    write(STDERR_FILENO, context_header, sizeof(context_header) - 1);
    
    char context_buf[512];
    snprintf(context_buf, sizeof(context_buf), 
        "   Last Operation: %s\n"
        "   Last Method: %s\n"
        "   Last Object: %p\n\n",
        last_operation, last_method, last_object);
    write(STDERR_FILENO, context_buf, strlen(context_buf));
    
    // Get stack trace (macOS)
    const char bt_header[] = "📚 STACK TRACE:\n";
    write(STDERR_FILENO, bt_header, sizeof(bt_header) - 1);
    
    void *buffer[128];
    int nptrs = backtrace(buffer, 128);
    
    // backtrace_symbols_fd is async-signal-safe
    backtrace_symbols_fd(buffer, nptrs, STDERR_FILENO);
    
    const char footer[] = "\n"
        "╔═══════════════════════════════════════════════════════════════╗\n"
        "║ 🚨 END CRASH REPORT 🚨                                        ║\n"
        "╚═══════════════════════════════════════════════════════════════╝\n\n";
    write(STDERR_FILENO, footer, sizeof(footer) - 1);
    
    // Re-raise signal with default handler to generate core dump
    signal(sig, SIG_DFL);
    raise(sig);
}

// Public API to set operation context
void gdext_c_signal_set_context(const char *operation, const char *method, void *object) {
    if (operation) {
        strncpy(last_operation, operation, sizeof(last_operation) - 1);
        last_operation[sizeof(last_operation) - 1] = '\0';
    }
    if (method) {
        strncpy(last_method, method, sizeof(last_method) - 1);
        last_method[sizeof(last_method) - 1] = '\0';
    }
    last_object = object;
}

// Initialize signal handlers
void gdext_c_signal_handler_init(void) {
    fprintf(stderr, "[gdext-c] 🛡️  Installing crash signal handlers...\n");
    
    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_flags = SA_SIGINFO | SA_RESETHAND;  // Get siginfo, reset to default after first call
    sa.sa_sigaction = crash_signal_handler;
    sigemptyset(&sa.sa_mask);
    
    // Install handlers for common crash signals
    if (sigaction(SIGSEGV, &sa, NULL) == -1) {
        fprintf(stderr, "[gdext-c] ⚠️  Failed to install SIGSEGV handler\n");
    }
    if (sigaction(SIGABRT, &sa, NULL) == -1) {
        fprintf(stderr, "[gdext-c] ⚠️  Failed to install SIGABRT handler\n");
    }
    if (sigaction(SIGBUS, &sa, NULL) == -1) {
        fprintf(stderr, "[gdext-c] ⚠️  Failed to install SIGBUS handler\n");
    }
    if (sigaction(SIGILL, &sa, NULL) == -1) {
        fprintf(stderr, "[gdext-c] ⚠️  Failed to install SIGILL handler\n");
    }
    if (sigaction(SIGFPE, &sa, NULL) == -1) {
        fprintf(stderr, "[gdext-c] ⚠️  Failed to install SIGFPE handler\n");
    }
    
    fprintf(stderr, "[gdext-c] ✅ Signal handlers installed successfully\n");
}
