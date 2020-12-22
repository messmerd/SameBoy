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

// TODO: Move this somewhere else?
typedef struct addin_manifest_s
{
    char *display_name;
    char *author;
    char *version;
    bool auto_start;
} addin_manifest_t;

typedef enum {START_ARGS_AUTO=1, START_ARGS_MANUAL=2, START_ARGS_RELOAD=4} start_args_t;

GB_ADDIN_API GB_gameboy_t *SAMEBOY_get_GB(void);

//GB_ADDIN_API addin_manifest_t SAMEBOY_get_manifest(void);

GB_ADDIN_API int SAMEBOY_api_test_function(int number);

GB_ADDIN_API const char *SAMEBOY_get_version(void);

// More stuff will go here

#ifdef __cplusplus
}
#endif

#endif /* addin_api_h */
