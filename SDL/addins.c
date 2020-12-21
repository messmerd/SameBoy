#include <stdio.h>
#include <ctype.h>

#include "addins.h"
#include "utils.h"

#ifdef _WIN32
#define load_library(filename) LoadLibrary(filename)
#define get_address(handle, name) GetProcAddress(handle, name)
#define close_library(handle) FreeLibrary(handle)
#else
#define load_library(filename) dlopen(filename, RTLD_LAZY)
#define get_address(handle, name) dlsym(handle, name)
#define close_library(handle) dlclose(handle)
#endif

// TODO: Need to free add-ins when SameBoy closes

static addin_t *addins[MAX_ADDINS];
static unsigned addins_count = 0;

static addin_import_error_t manifest_import(addin_manifest_t *manifest, const char *filename);
static void addin_free(addin_t *addin);
static bool file_exists(const char *filename);

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
    // TODO: Make this into an .ini file parser and put it in utils.c

    manifest->display_name = NULL;
    manifest->author = NULL;
    manifest->version = NULL;
    manifest->auto_start = false;

    FILE *fp = NULL;
    fopen_s(&fp, filename, "r");
    if (!fp)
        return ADDIN_IMPORT_MANIFEST_NOT_FOUND;

    char buffer[100];
    char key[100];
    char value[100];

    while (fgets(buffer, 100, fp))
    {
        if (!isalpha(buffer[0]))
            continue;
        char *start = strchr(buffer, '=');
        if (!start || start == &buffer[0])
            continue;
        char *key_end = start - 1;
        while (isspace(*key_end))
            key_end--;
        strncpy(key, buffer, key_end - &buffer[0] + 1);
        key[key_end - &buffer[0] + 1] = '\0';
        start++;

        while (isspace(*start))
            start++;
        char *end = &buffer[98];
        while (end > start && isspace(*end))
            end--;

        if (end > start)
        {
            strncpy(value, start, end - start);
            value[end - start] = '\0';
        }
        else
            value[0] = '\0';
        
        printf("key=%s;\nvalue=%s;\n", key, value);

        if (strcmp(key, "display_name") == 0)
            manifest->display_name = strdup(value);
        else if (strcmp(key, "author") == 0)
            manifest->author = strdup(value);
        else if (strcmp(key, "version") == 0)
            manifest->version = strdup(value);
        else if (strcmp(key, "auto_start") == 0)
            manifest->auto_start = value[0] == '1' || strcmp(value, "true") || strcmp(value, "TRUE");
    }

    fclose(fp);
    return ADDIN_IMPORT_OK;
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

static void addin_free(addin_t *addin)
{
    free(addin->filename);
    free(addin->manifest.display_name);
    free(addin->manifest.author);
    free(addin->manifest.version);
}

static bool file_exists(const char *filename)
{
    FILE *file = NULL;
    fopen_s(&file, filename, "r");
    if (file)
    {
        fclose(file);
        return true;
    }
    return false;
}

// Additional API methods available to add-ins:

extern GB_gameboy_t gb;
GB_ADDIN_API GB_gameboy_t *SAMEBOY_get_GB(void)
{
    return &gb;
}

/*
GB_ADDIN_API addin_manifest_t SAMEBOY_get_manifest(void)
{
    // Once threading has been implemented for add-ins, I might be able 
    //  to determine which add-in called this function without using any 
    //  sort of add-in indentifier passed as an argument. Then I can return 
    //  the correct manifest struct. 
}
*/

GB_ADDIN_API int SAMEBOY_api_test_function(int number)
{
    return 100 + number;
}
