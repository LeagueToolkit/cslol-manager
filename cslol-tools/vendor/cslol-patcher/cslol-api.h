#ifndef CSLOL_API_H
#define CSLOL_API_H

#ifdef __cplusplus
extern "C" {
#endif  // __cplusplus

#include <stdbool.h>
#include <uchar.h>

#ifdef CSLOL_IMPL
#    ifdef _MSC_VER
#        define CSLOL_API __declspec(dllexport)
#    else
#        define CSLOL_API
#    endif
#else
#    ifdef _MSC_VER
#        define CSLOL_API __declspec(dllimport)
#    else
#        define CSLOL_API extern
#    endif
#endif

typedef enum cslol_hook_flags {
    CSLOL_HOOK_DISALBE_NONE = 0u,
    CSLOL_HOOK_DISABLE_VERIFY = 1u,
    CSLOL_HOOK_DISABLE_FILE = 2u,
    CSLOL_HOOK_DISABLE_ALL = (unsigned)(-1),
} cslol_hook_flags;

typedef enum cslol_log_level {
    CSLOL_LOG_ERROR = 0,
    CSLOL_LOG_INFO = 0x10,
    CSLOL_LOG_DEBUG = 0x20,
    CSLOL_LOG_ALL = 0x1000,
} cslol_log_level;

// Msg proc used for injection.
CSLOL_API intptr_t cslol_msg_hookproc(int code, uintptr_t wParam, intptr_t lParam);

// Initialize IPC, returns error if any.
CSLOL_API const char* cslol_init();

// Sets prefix folder, returns error if any.
CSLOL_API const char* cslol_set_config(const char16_t* prefix);

// Sets flags, return error if any.
CSLOL_API const char* cslol_set_flags(cslol_hook_flags flags);

// Set log level, return error if any.
CSLOL_API const char* cslol_set_log_level(cslol_log_level level);

// Pull log message if any.
CSLOL_API const char* cslol_log_pull();

// Find thread id of running lol instance.
CSLOL_API unsigned cslol_find();

// Hook, return error if any.
CSLOL_API const char* cslol_hook(unsigned tid, unsigned post_iters, unsigned event_iters);

#ifdef __cplusplus
}
#endif  // __cplusplus

#endif  // CSLOL_API_H
