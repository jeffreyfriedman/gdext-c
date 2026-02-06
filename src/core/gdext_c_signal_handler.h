// gdext_c_signal_handler.h - Signal handlers for crash diagnosis

#ifndef GDEXT_C_SIGNAL_HANDLER_H
#define GDEXT_C_SIGNAL_HANDLER_H

#ifdef __cplusplus
extern "C" {
#endif

// Initialize signal handlers (call once at library init)
void gdext_c_signal_handler_init(void);

// Set context for crash reporting (call before risky operations)
void gdext_c_signal_set_context(const char *operation, const char *method, void *object);

#ifdef __cplusplus
}
#endif

#endif // GDEXT_C_SIGNAL_HANDLER_H
