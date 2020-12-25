#include <SDL_loadso.h>
#include <stdio.h>
#include <ctype.h>

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

// TODO: Need to free add-ins when SameBoy closes

static addin_t *addins[MAX_ADDINS];
static unsigned addins_count = 0;

static addin_import_error_t manifest_import(addin_manifest_t *manifest, const char *filename);
static void addin_unload(addin_t *addin);
static void addin_free(addin_t *addin);

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
    if (!addin->start || !addin->stop)
        goto cleanup;
    
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
    // Parses an addin's .ini manifest file

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
    addin_start_ext(addin, START_ARGS_MANUAL);
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

/*
GB_ADDIN_API addin_manifest_t GB_EXT_get_manifest(void)
{
    // Once threading has been implemented for add-ins, I might be able 
    //  to determine which add-in called this function without using any 
    //  sort of add-in indentifier passed as an argument. Then I can return 
    //  the correct manifest struct. 
}
*/

GB_ADDIN_API int GB_EXT_api_test_function(int number)
{
    return 100 + number;
}
