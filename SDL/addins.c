#include <SDL_loadso.h>
#include <stdio.h>
#include <ctype.h>

#ifdef _WIN32
#include <windows.h>
#endif

#include "addins.h"
#include "utils.h"

// Use SDL for cross-platform dynamic loading
#define load_library(filename) SDL_LoadObject(filename)
#define get_address(handle, name) SDL_LoadFunction(handle, name)
#define unload_library(handle) SDL_UnloadObject(handle)

// Another implementation:
/*
#ifdef _WIN32
#define load_library(filename) LoadLibrary(filename)
#define get_address(handle, name) GetProcAddress(handle, name)
#define close_library(handle) FreeLibrary(handle)
#else
#define load_library(filename) dlopen(filename, RTLD_LAZY)
#define get_address(handle, name) dlsym(handle, name)
#define close_library(handle) dlclose(handle)
#endif
*/

static addin_t *addins[MAX_ADDINS];
static unsigned addins_count = 0;

static addin_import_error_t manifest_import(addin_manifest_t *manifest, const char *filename);
static void addin_unload(addin_t *addin);
static void addin_free(addin_t *addin);

static bool is_windows_build(void);
static bool is_windows_console_build(void);

addin_import_error_t addin_import(const char *filename)
{
    // Imports an add-in given the filename for the .dll/.so file.
    // Does nothing if the add-in has already been imported.
    // Returns true upon failure.

    if (addins_count == MAX_ADDINS)
        return ADDIN_IMPORT_MAX_ADDINS;

    if (!file_exists(filename))
        return ADDIN_IMPORT_LIBRARY_NOT_FOUND;

    for (unsigned i = 0; i < addins_count; ++i)
    {
        if (strcmp(filename, addins[i]->filename) == 0)
        {
            return ADDIN_IMPORT_ALREADY_IMPORTED; 
        }
    }

    addin_t *addin = NULL;
    size_t length = strlen(filename);
    char *manifest_filename = (char *)malloc(length * sizeof(char) + sizeof(".ini"));
    addin_import_error_t error = ADDIN_IMPORT_MANIFEST_NOT_FOUND;
    if (!manifest_filename)
        goto cleanup;
    replace_extension(filename, length, manifest_filename, ".ini");

    addin = (addin_t *)calloc(1, sizeof(addin_t));
    error = ADDIN_IMPORT_MEMORY_ALLOCATION_FAIL;
    if (!addin)
        goto cleanup;
    
    addin->filename = strdup(filename);
    if (!addin->filename)
        goto cleanup;
    
    if ((error = manifest_import(&addin->manifest, manifest_filename)))
        goto cleanup;

    error = ADDIN_IMPORT_LIBRARY_LOAD_FAIL;
    addin->handle = load_library(filename);
    if (!addin->handle)
        goto cleanup;

    addin->start = (addin_start_pointer_t)get_address(addin->handle, "start");
    addin->stop = (addin_stop_pointer_t)get_address(addin->handle, "stop");
    addin_stop_pointer_t addin_init_function = (addin_stop_pointer_t)get_address(addin->handle, "_addin_init");
    if (!addin->start || !addin->stop || !addin_init_function)
        goto cleanup;
    
    bool windows_build = is_windows_build();
    bool windows_console_build = is_windows_console_build();

    bool addin_uses_sameboy_windows_console_build = addin_init_function();

    if (windows_build)
    {
        error = ADDIN_IMPORT_TARGETS_WRONG_SAMEBOY_WINDOWS_BUILD;
        if (windows_console_build && !addin_uses_sameboy_windows_console_build)
        {
            printf("Could not load add-in. Add-in does not target SameBoy debugger build.\n");
            printf("Either open with sameboy.exe or define ADDIN_WINDOWS_CONSOLE_BUILD and link add-in with sameboy_debugger.lib.\n");
            goto cleanup;
        }
        else if (!windows_console_build && addin_uses_sameboy_windows_console_build)
        {
            printf("Could not load add-in. Add-in targets SameBoy debugger build rather than SameBoy build.\n");
            printf("Either open with sameboy_debugger.exe or undefine ADDIN_WINDOWS_CONSOLE_BUILD and link add-in with sameboy.lib.\n");
            goto cleanup;
        }
    }

    addin->active = false;

    addins[addins_count++] = addin;
    
    return ADDIN_IMPORT_OK; // Success

cleanup:
    free(manifest_filename);
    addin_free(addin);
    return error; // Failure
}

static addin_import_error_t manifest_import(addin_manifest_t *manifest, const char *filename)
{
    // Parses an addin's INI manifest file

    manifest->display_name = NULL;
    manifest->author = NULL;
    manifest->version = NULL;
    manifest->auto_start = false;

    if (!file_exists(filename))
        return ADDIN_IMPORT_MANIFEST_NOT_FOUND;

    void *parse_dest[] = {&manifest->display_name, &manifest->author, &manifest->version, &manifest->auto_start};
    const char *keys[] = {"display_name", "author", "version", "auto_start"};
    const ini_value_type_t types[] = {INI_VALUE_TYPE_STRING, INI_VALUE_TYPE_STRING, INI_VALUE_TYPE_STRING, INI_VALUE_TYPE_BOOL};
    const int size = sizeof(keys) / sizeof(char *);

    bool result = ini_read_keys(parse_dest, filename, keys, types, size);

    printf("\nManifest contents:\ndisplay_name=%s;\nauthor=%s;\nversion=%s;\nauto_start=%i;\n\n", manifest->display_name, manifest->author, manifest->version, manifest->auto_start);

    return result ? ADDIN_IMPORT_MANIFEST_PARSE_FAIL : ADDIN_IMPORT_OK;
}

void addin_start(addin_t *addin)
{
    start_args_t args = 0;

    args |= START_ARGS_MANUAL;
    //args |= is_windows_console_build() ? START_ARGS_WINDOWS_CONSOLE_BUILD : 0;

    addin_start_ext(addin, args);
}

void addin_start_ext(addin_t *addin, start_args_t args)
{
    if (!addin || addin->active)
        return;

    addin->start(args);
    addin->active = true;
}

void addin_stop(addin_t *addin)
{
    if (addin == NULL || !addin->active)
        return;

    addin->stop();
    //close_library(addin->handle);
    //addin->handle = NULL;
    addin->active = false;
}

addin_t *get_addin(unsigned index)
{
    return index < addins_count ? addins[index] : NULL;
}

unsigned get_addins_count(void)
{
    return addins_count;
}

void addins_close_all(void)
{
    printf("Closing all add-ins.\n");
    for (int i = 0; i < addins_count; ++i)
    {
        if (addins[i])
        {
            addin_unload(addins[i]);
            addin_free(addins[i]);
        }
    }
    addins_count = 0;
}

static void addin_unload(addin_t *addin)
{
    if (!addin || !addin->handle)
        return;
    unload_library(addin->handle);
}

static void addin_free(addin_t *addin)
{
    free(addin->filename);
    free(addin->manifest.display_name);
    free(addin->manifest.author);
    free(addin->manifest.version);
    free(addin);
}

static bool is_windows_build(void)
{
#ifdef _WIN32
    return true;
#else
    return false;
#endif
}

static bool is_windows_console_build(void)
{
    bool result = false;

#ifdef _WIN32
    HANDLE std_out = GetStdHandle(STD_OUTPUT_HANDLE);
    CONSOLE_SCREEN_BUFFER_INFO console_screen_buffer_info;

    if (GetConsoleScreenBufferInfo(std_out, &console_screen_buffer_info))
        result = true;
#endif
    return result;
}


// Additional API methods available to add-ins:

extern GB_gameboy_t gb;
GB_ADDIN_API GB_gameboy_t *GB_EXT_get_GB(void)
{
    return &gb;
}

GB_ADDIN_API const char *GB_EXT_get_version(void)
{
#define str(x) #x
#define xstr(x) str(x)
    return (const char *)xstr(VERSION);
}

GB_ADDIN_API addin_manifest_t *GB_EXT_get_manifest(void)
{
    // Once threading has been implemented for add-ins, I might be able 
    //  to determine which add-in called this function without using any 
    //  sort of add-in indentifier passed as an argument. Then I can return 
    //  the correct manifest struct. 

    // For now, just return this for testing purposes:
    return &addins[0]->manifest;
}


////////// EVENT HANDLING ///////////

GB_ADDIN_API int GB_EXT_event_fullscreen_subscribe(const char *handler)
{
    // TODO: Need to figure out which add-in called this method
    // For now, use addins[0] for testing.
    if (!handler)
    {
        addins[0]->event_handlers.fullscreen = NULL;
        return 0;
    }
    
    addins[0]->event_handlers.fullscreen = (handler_fullscreen_t)get_address(addins[0]->handle, handler);
    return !addins[0]->event_handlers.fullscreen; // Return false if successful
}

void addins_event_fullscreen_invoke(bool isFullscreen)
{
    printf("Checking if need to invoke fullscreen event.\n");
    for (int i = 0; i < addins_count; ++i)
    {
        if (addins[i]->event_handlers.fullscreen)
        {
            printf("Invoking fullscreen event.\n");
            addins[i]->event_handlers.fullscreen(isFullscreen);
        }
            
    }
}
