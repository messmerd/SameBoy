#ifndef addins_h
#define addins_h

#ifdef _WIN32
#include <windows.h>
#include <libloaderapi.h>
#define handle_t HMODULE
#else
#include <dlfcn.h>
#define handle_t void*
#endif

#include <string.h>
#include <stdbool.h>

//#define ADDINS_SERVER
//#include <Core/addin_api.h>
#include <Core/gb.h>

typedef int (__stdcall *addin_start_pointer_t)(START_ARGS);
typedef int (__stdcall *addin_stop_pointer_t)(void);

unsigned get_addins_count(void);
char* get_addin_name(unsigned index);

typedef struct addin_s
{
    handle_t handle;
    char* name;
    char* path;
    char* author;
    bool active;
    bool auto_start;

    addin_start_pointer_t start;
    addin_stop_pointer_t stop;
} addin_t;

addin_t* addins;

addin_t* addin_load(const char* fname);
bool addin_reload(addin_t* addin);

void addin_start(addin_t* addin);
void addin_start_ext(addin_t* addin, START_ARGS args);
void addin_stop(addin_t* addin);

void addin_free(addin_t* addin, bool freeEverything);

void populate_addin_list(void);



#endif /* addins_h */
