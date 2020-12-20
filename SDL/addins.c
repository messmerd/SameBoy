#include <stdio.h>

#include "addins.h"

#ifdef _WIN32
#define load_library(filename) LoadLibrary(filename)
#define get_address(handle, name) GetProcAddress(handle, name)
#define close_library(handle) FreeLibrary(handle)
#else
#define load_library(filename) dlopen(filename, RTLD_LAZY)
#define get_address(handle, name) dlsym(handle, name)
#define close_library(handle) dlclose(handle)
#endif

addin_t *addins;

bool file_exists(const char *filename);
static void populate_addin(addin_t *addin, const char *filename);

addin_t *addin_load(const char *filename)
{
    if (filename == NULL || !file_exists(filename))
        return NULL;

    addin_t* addin = (addin_t*)malloc(sizeof(addin_t));

    addin->handle = load_library(filename);

    populate_addin(addin, filename);

    return addin;
}



void addin_stop(addin_t *addin)
{
    if (addin == NULL || !addin->active)
        return;

    addin->stop();
    close_library(addin->handle);
    addin->handle = NULL;
    addin->active = false;
}


static void populate_addin(addin_t *addin, const char *filename)
{
    // Assume addin is not NULL

    if (addin->handle != NULL)
    {
        addin->start = (addin_start_pointer_t)get_address(addin->handle, "start");
        addin->stop = (addin_stop_pointer_t)get_address(addin->handle, "stop");

        if (!addin->start || !addin->stop)
        {
            free(addin);
            addin = NULL;
            return;
        }
        
        addin->name = _strdup("Placeholder name");
        if (!filename)
            addin->path = _strdup(filename);
        addin->author = _strdup("Placeholder author");
        
        addin->active = false;
        addin->auto_start = false;
    }
    else
    {
        free(addin);
        addin = NULL;
    }
}

bool file_exists(const char *filename)
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


extern GB_gameboy_t gb;
GB_ADDIN_API GB_gameboy_t *SAMEBOY_get_GB(void)
{
    return &gb;
}
