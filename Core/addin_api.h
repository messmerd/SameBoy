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

typedef enum START_ARGS {START_AUTO=1, START_MANUAL=2, START_RELOAD=4} START_ARGS;

GB_ADDIN_API GB_gameboy_t *SAMEBOY_get_GB(void);

// More stuff will go here

#ifdef __cplusplus
}
#endif

#endif /* addin_api_h */
