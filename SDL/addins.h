#ifndef addins_h
#define addins_h

#include <Core/gb.h>
#include <string.h>
#include <stdbool.h>

// Old implementation:
/*
#ifdef _WIN32
#include <windows.h>
#include <libloaderapi.h>
#define handle_t HMODULE
#else
#include <dlfcn.h>
#define handle_t void *
#endif
*/

#define MAX_ADDINS 7

//typedef enum PLATFORM {} PLATFORM; // TODO 

typedef enum {
    ADDIN_IMPORT_OK=0, 
    ADDIN_IMPORT_ALREADY_IMPORTED,
    ADDIN_IMPORT_MAX_ADDINS,
    ADDIN_IMPORT_TARGETS_WRONG_SAMEBOY_WINDOWS_BUILD,
    ADDIN_IMPORT_LIBRARY_NOT_FOUND,
    ADDIN_IMPORT_MANIFEST_NOT_FOUND,
    ADDIN_IMPORT_LIBRARY_LOAD_FAIL,
    ADDIN_IMPORT_MANIFEST_PARSE_FAIL,
    ADDIN_IMPORT_MEMORY_ALLOCATION_FAIL
} addin_import_error_t;

typedef int (*addin_start_pointer_t)(start_args_t);
typedef int (*addin_stop_pointer_t)(void);
typedef int (*addin_init_pointer_t)(void);

typedef struct addin_event_handlers_s
{
    handler_fullscreen_t fullscreen;
    // More event handlers to go here
} addin_event_handlers_t;

typedef struct addin_s
{
    void *handle;
    char *filename; // .dll/.so full file path/name including extension
    addin_manifest_t manifest;
    bool active;

    addin_start_pointer_t start;
    addin_stop_pointer_t stop;

    addin_event_handlers_t event_handlers;
} addin_t;

addin_t *get_addin(unsigned index);
unsigned get_addins_count(void);
//char *get_addin_name(unsigned index);

addin_import_error_t addin_import(const char *filename);
// void addin_remove(unsigned index); // TODO 

void addin_start(addin_t *addin);
void addin_start_ext(addin_t *addin, start_args_t args);
void addin_stop(addin_t *addin);

void addins_close_all(void);

void import_addins_from_addins_folder(void);

void addins_event_fullscreen_invoke(bool isFullscreen);



#endif /* addins_h */
