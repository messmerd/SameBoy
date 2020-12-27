// Include this header in your C/C++ project to gain access to add-in API functions

#ifndef addin_api_h
#define addin_api_h

#ifdef _WIN32
#    ifdef ADDINS_SERVER
#        define ADDIN_API __declspec(dllimport)
#        define GB_ADDIN_API __declspec(dllexport)
#    else
#        ifdef __cplusplus
#            define ADDIN_API extern "C" __declspec(dllexport)
#        else
#            define ADDIN_API __declspec(dllexport)
#        endif
#        define GB_ADDIN_API __declspec(dllimport)
#    endif
#else
#    ifdef __cplusplus
#        define ADDIN_API extern "C"
#    else
#        define ADDIN_API
#    endif
#    define GB_ADDIN_API
#endif

#include <Core/gb.h>

#ifdef __cplusplus
extern "C" { 
#endif

#ifdef ADDIN_WINDOWS_CONSOLE_BUILD
#define _ADDIN_WINDOWS_CONSOLE_BUILD 1
#else
#define _ADDIN_WINDOWS_CONSOLE_BUILD 0
#endif
#define ADDIN_INIT ADDIN_API int _addin_init(void) { return _ADDIN_WINDOWS_CONSOLE_BUILD; }

////////////////////
// Enums          //
////////////////////

// TODO: Move this somewhere else?
typedef struct addin_manifest_s {
    char *display_name;
    char *author;
    char *version;
    bool auto_start;
} addin_manifest_t;

typedef enum {
    START_ARGS_AUTO=1, 
    START_ARGS_MANUAL=2, 
    START_ARGS_RELOAD=4
} start_args_t;

////////////////////
// Event Handlers //
////////////////////

typedef void (*handler_fullscreen_t)(bool isFullscreen);
GB_ADDIN_API int GB_EXT_event_fullscreen_subscribe(const char *handler);

////////////////////
// Extended API   //
////////////////////

GB_ADDIN_API GB_gameboy_t *GB_EXT_get_GB(void);

GB_ADDIN_API addin_manifest_t *GB_EXT_get_manifest(void);

GB_ADDIN_API const char *GB_EXT_get_version(void);

// More to come


#ifdef __cplusplus
}
#endif

#endif /* addin_api_h */
