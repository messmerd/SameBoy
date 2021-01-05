#include <SDL_loadso.h>
#include <stdio.h>
#include <stddef.h>
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

#define get_handler(h) offsetof(addin_event_handlers_t, h)

extern GB_gameboy_t gb;

static addin_t *addins[MAX_ADDINS];
static unsigned addins_count = 0;

static addin_import_error_t manifest_import(addin_manifest_t *manifest, const char *filename);
static unsigned addin_create_id(void);
static void addin_unload(addin_t *addin);
static void addin_free(addin_t *addin);

static bool addin_event_subscribe(unsigned addin_id, size_t handler_relative_address, char *handler_name, bool subscribe);
static void addin_event_invoke(size_t handler_relative_address, char *thread_name, void *arg);
static void addin_event_unsubscribe_all(addin_t *addin);

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

    addin->start = (SDL_ThreadFunction)get_address(addin->handle, "_addin_start");
    addin->stop = (SDL_ThreadFunction)get_address(addin->handle, "_addin_stop");
    SDL_ThreadFunction addin_init_function = (SDL_ThreadFunction)get_address(addin->handle, "_addin_init");
    if (!addin->start || !addin->stop || !addin_init_function)
        goto cleanup;
    
    bool windows_build = is_windows_build();
    bool windows_console_build = is_windows_console_build();

    addin->id = addin_create_id();
    bool addin_uses_sameboy_windows_console_build = addin_init_function((void *)&addin->id);

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

static void shorten_string(char **string, const unsigned max_size)
{
    if (*string && strlen(*string) > max_size)
    {
        char *new_string = (char *)malloc((max_size + 1) * sizeof(char));
        if (!new_string)
            return;
        strncpy(new_string, *string, max_size);
        new_string[max_size] = '\0';
        free(*string);
        *string = new_string;
    }
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
    const int size = sizeof(keys) / sizeof(const char *);

    bool result = ini_read_keys(parse_dest, filename, keys, types, size);

    // Shorten the display name to 20 characters or less if needed
    shorten_string(&manifest->display_name, 20);

    // Shorten the author string to 20 characters or less if needed
    shorten_string(&manifest->author, 20);
    
    // Shorten the version string to 20 characters or less if needed
    shorten_string(&manifest->version, 20);

    printf("\nManifest contents:\ndisplay_name=%s;\nauthor=%s;\nversion=%s;\nauto_start=%i;\n\n", manifest->display_name, manifest->author, manifest->version, manifest->auto_start);

    return result ? ADDIN_IMPORT_MANIFEST_PARSE_FAIL : ADDIN_IMPORT_OK;
}

static unsigned addin_create_id(void)
{
    unsigned id;
    while (true)
    {
        id = rand() % (UINT_MAX - 1) + 1;
        for (int i = 0; i < addins_count; ++i)
        {
            if (addins[i] && addins[i]->id == id)
                continue;
        }
        break;
    }
    return id;
}

void addin_start(addin_t *addin)
{
    addin_start_args_t args = 0;

    args |= START_ARGS_MANUAL;
    //args |= is_windows_console_build() ? START_ARGS_WINDOWS_CONSOLE_BUILD : 0;

    addin_start_ext(addin, args);
}

void addin_start_ext(addin_t *addin, addin_start_args_t args)
{
    if (!addin || addin->active)
        return;

    SDL_Thread *thread = SDL_CreateThread(addin->start, "_addin_start", (void *)&args);
    //printf("addin_start_ext...Thread ID:%lu; Thread Get ID:%lu;\n", SDL_ThreadID(), SDL_GetThreadID(thread));
    SDL_DetachThread(thread);

    addin->active = true;
}

void addin_stop(addin_t *addin)
{
    if (addin == NULL || !addin->active)
        return;

    SDL_Thread *thread = SDL_CreateThread(addin->stop, "_addin_stop", NULL);
    SDL_DetachThread(thread);

    addin_event_unsubscribe_all(addin);

    //close_library(addin->handle);
    //addin->handle = NULL;
    addin->active = false;
}

addin_t *get_addin(unsigned index)
{
    return index < addins_count ? addins[index] : NULL;
}

addin_t *get_addin_from_id(unsigned id)
{
    for (int i = 0; i < addins_count; ++i)
    {
        if (addins[i] && addins[i]->id == id)
            return addins[i];
    }
    return NULL;
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

static bool addin_event_subscribe(unsigned addin_id, size_t handler_relative_address, char *handler_name, bool subscribe)
{
    addin_t *addin = get_addin_from_id(addin_id);
    if (!addin)
        return true;

    //printf("Event subscribe...Add-in name:%s;\n", addin->manifest.display_name);

    SDL_ThreadFunction *func = (SDL_ThreadFunction *)((void *)(&addin->event_handlers) + handler_relative_address);
    if (subscribe)
    {
        *func = (SDL_ThreadFunction)get_address(addin->handle, handler_name);
        return *func == NULL;
    }
    else
        *func = NULL;
    return false;
}

static void addin_event_invoke(size_t handler_relative_address, char *thread_name, void *arg)
{
    for (int i = 0; i < addins_count; ++i)
    {
        SDL_ThreadFunction func = *(SDL_ThreadFunction *)((void *)(&addins[i]->event_handlers) + handler_relative_address);
        if (func)
        {
            //printf("Invoking %s event.\n", thread_name);
            //printf("struct base=%p;\nrelatv addr=%X;\nfunc addr  =%p;\n\n", (void *)&(addins[i]->event_handlers), handler_relative_address, (void *)(SDL_ThreadFunction *)((void *)&(addins[i]->event_handlers) + handler_relative_address));
            //printf("func   =%p;\ncorrect=%p;\n\n", (void *)func, (void *)addins[i]->event_handlers.pause);
            SDL_Thread *thread = SDL_CreateThread(func, thread_name, arg);
            SDL_DetachThread(thread);
        }
    }
}

static void addin_event_unsubscribe_all(addin_t *addin)
{
    if (addin)
        memset(&addin->event_handlers, 0, sizeof(addin->event_handlers));
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

//////////////////////
// API Functions    //
//////////////////////

// Wrappers for SameBoy Core API functions:

void SBAPI_init(GB_model_t model) {GB_init(&gb, model); }
bool SBAPI_is_inited(void) {return GB_is_inited(&gb); }
bool SBAPI_is_cgb(void) {return GB_is_cgb(&gb); }
bool SBAPI_is_sgb(void) {return GB_is_sgb(&gb); }
bool SBAPI_is_hle_sgb(void) {return GB_is_hle_sgb(&gb); }
GB_model_t SBAPI_get_model(void) {return GB_get_model(&gb); }
void SBAPI_free(void) {GB_free(&gb); }
void SBAPI_reset(void) {GB_reset(&gb); }
void SBAPI_switch_model_and_reset(GB_model_t model) {GB_switch_model_and_reset(&gb, model); }

uint8_t SBAPI_run(void) {return GB_run(&gb); }
uint64_t SBAPI_run_frame(void) {return GB_run_frame(&gb); }

void *SBAPI_get_direct_access(GB_direct_access_t access, size_t *size, uint16_t *bank) {return GB_get_direct_access(&gb, access, size, bank); }

void *SBAPI_get_user_data(void) {return GB_get_user_data(&gb); }
void SBAPI_set_user_data(void *data) {GB_set_user_data(&gb, data); }

int SBAPI_load_boot_rom(const char *path) {return GB_load_boot_rom(&gb, path); }
void SBAPI_load_boot_rom_from_buffer(const unsigned char *buffer, size_t size) {GB_load_boot_rom_from_buffer(&gb, buffer, size); }
int SBAPI_load_rom(const char *path) {return GB_load_rom(&gb, path); }
void SBAPI_load_rom_from_buffer(const uint8_t *buffer, size_t size) {GB_load_rom_from_buffer(&gb, buffer, size); }
int SBAPI_load_isx(const char *path) {return GB_load_isx(&gb, path); }

int SBAPI_save_battery_size(void) {return GB_save_battery_size(&gb); }
int SBAPI_save_battery_to_buffer(uint8_t *buffer, size_t size) {return GB_save_battery_to_buffer(&gb, buffer, size); }
int SBAPI_save_battery(const char *path) {return GB_save_battery(&gb, path); }

void SBAPI_load_battery_from_buffer(const uint8_t *buffer, size_t size) {GB_load_battery_from_buffer(&gb, buffer, size); }
void SBAPI_load_battery(const char *path) {GB_load_battery(&gb, path); }

void SBAPI_set_turbo_mode(bool on, bool no_frame_skip) {GB_set_turbo_mode(&gb, on, no_frame_skip); }
void SBAPI_set_rendering_disabled(bool disabled) {GB_set_rendering_disabled(&gb, disabled); }

void SBAPI_log(const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    GB_log(&gb, fmt, args); 
    va_end(args);
}
void SBAPI_attributed_log(GB_log_attributes attributes, const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    GB_attributed_log(&gb, attributes, fmt, args); 
    va_end(args);
}

void SBAPI_set_pixels_output(uint32_t *output) {GB_set_pixels_output(&gb, output); }
void SBAPI_set_border_mode(GB_border_mode_t border_mode) {GB_set_border_mode(&gb, border_mode); }

void SBAPI_set_infrared_input(bool state) {GB_set_infrared_input(&gb, state); }

void SBAPI_set_palette(const GB_palette_t *palette) {GB_set_palette(&gb, palette); }

/* These APIs are used when using external clock */
bool SBAPI_serial_get_data_bit(void) {return GB_serial_get_data_bit(&gb); }
void SBAPI_serial_set_data_bit(bool data) {GB_serial_set_data_bit(&gb, data); }

void SBAPI_disconnect_serial(void) {GB_disconnect_serial(&gb); }

/* For cartridges with an alarm clock */
unsigned SBAPI_time_to_alarm(void) {return GB_time_to_alarm(&gb); } // 0 if no alarm

uint32_t SBAPI_get_clock_rate(void) {return GB_get_clock_rate(&gb); }
void SBAPI_set_clock_multiplier(double multiplier) {GB_set_clock_multiplier(&gb, multiplier); }

unsigned SBAPI_get_screen_width(void) {return GB_get_screen_width(&gb); }
unsigned SBAPI_get_screen_height(void) {return GB_get_screen_height(&gb); }
double SBAPI_get_usual_frame_rate(void) {return GB_get_usual_frame_rate(&gb); }
unsigned SBAPI_get_player_count(void) {return GB_get_player_count(&gb); }

// Additional API functions available to add-ins:


GB_gameboy_t *SBAPI_get_GB(void)
{
    return &gb;
}

const char *SBAPI_get_version(void)
{
#define str(x) #x
#define xstr(x) str(x)
    return (const char *)xstr(VERSION);
}

addin_manifest_t *SBAPI_get_manifest(void)
{
    // Once threading has been implemented for add-ins, I might be able 
    //  to determine which add-in called this function without using any 
    //  sort of add-in indentifier passed as an argument. Then I can return 
    //  the correct manifest struct. 

    // For now, just return this for testing purposes:
    return &addins[0]->manifest;
}


////////// EVENT HANDLING - SUBSCRIBE ///////////

// Wrappers for SameBoy Core events:

bool SBAPI_event_vblank_subscribe(unsigned addin_id, bool subscribe)
{
    return addin_event_subscribe(addin_id, get_handler(vblank), "_vblank_handler", subscribe);
}

bool SBAPI_event_log_subscribe(unsigned addin_id, bool subscribe)
{
    return addin_event_subscribe(addin_id, get_handler(log), "_log_handler", subscribe);
}

bool SBAPI_event_input_subscribe(unsigned addin_id, bool subscribe)
{
    return addin_event_subscribe(addin_id, get_handler(fullscreen), "_input_handler", subscribe);
}

bool SBAPI_event_async_input_subscribe(unsigned addin_id, bool subscribe)
{
    return addin_event_subscribe(addin_id, get_handler(async_input), "_async_input_handler", subscribe);
}

bool SBAPI_event_rgb_encode_subscribe(unsigned addin_id, bool subscribe)
{
    return addin_event_subscribe(addin_id, get_handler(rgb_encode), "_rgb_encode_handler", subscribe);
}

bool SBAPI_event_infrared_subscribe(unsigned addin_id, bool subscribe)
{
    return addin_event_subscribe(addin_id, get_handler(infrared), "_infrared_handler", subscribe);
}

bool SBAPI_event_rumble_subscribe(unsigned addin_id, bool subscribe)
{
    return addin_event_subscribe(addin_id, get_handler(rumble), "_rumble_handler", subscribe);
}

bool SBAPI_event_update_input_hint_subscribe(unsigned addin_id, bool subscribe)
{
    return addin_event_subscribe(addin_id, get_handler(update_input_hint), "_update_input_hint_handler", subscribe);
}

bool SBAPI_event_boot_rom_load_subscribe(unsigned addin_id, bool subscribe)
{
    return addin_event_subscribe(addin_id, get_handler(boot_rom_load), "_boot_rom_load_handler", subscribe);
}

bool SBAPI_event_serial_transfer_bit_start_subscribe(unsigned addin_id, bool subscribe)
{
    return addin_event_subscribe(addin_id, get_handler(serial_transfer_bit_start), "_serial_transfer_bit_start_handler", subscribe);
}

bool SBAPI_event_serial_transfer_bit_end_subscribe(unsigned addin_id, bool subscribe)
{
    return addin_event_subscribe(addin_id, get_handler(serial_transfer_bit_end), "_serial_transfer_bit_end_handler", subscribe);
}

bool SBAPI_event_joyp_write_subscribe(unsigned addin_id, bool subscribe)
{
    return addin_event_subscribe(addin_id, get_handler(joyp_write), "_joyp_write_handler", subscribe);
}

bool SBAPI_event_icd_pixel_subscribe(unsigned addin_id, bool subscribe)
{
    return addin_event_subscribe(addin_id, get_handler(icd_pixel), "_icd_pixel_handler", subscribe);
}

bool SBAPI_event_icd_hreset_subscribe(unsigned addin_id, bool subscribe)
{
    return addin_event_subscribe(addin_id, get_handler(icd_hreset), "_icd_hreset_handler", subscribe);
}

bool SBAPI_event_icd_vreset_subscribe(unsigned addin_id, bool subscribe)
{
    return addin_event_subscribe(addin_id, get_handler(icd_vreset), "_icd_vreset_handler", subscribe);
}

// Additional events for SameBoy:

bool SBAPI_event_step_subscribe(unsigned addin_id, bool subscribe)
{
    return addin_event_subscribe(addin_id, get_handler(step), "_step_handler", subscribe); 
}

bool SBAPI_event_fullscreen_subscribe(unsigned addin_id, bool subscribe)
{
    return addin_event_subscribe(addin_id, get_handler(fullscreen), "_fullscreen_handler", subscribe);
}

bool SBAPI_event_pause_subscribe(unsigned addin_id, bool subscribe)
{
    return addin_event_subscribe(addin_id, get_handler(pause), "_pause_handler", subscribe);
}


////////// EVENT HANDLING - INVOKE ///////////

// Wrappers for SameBoy Core events:


// Additional events for SameBoy:

void addins_event_step_invoke(void)
{
    addin_event_invoke(get_handler(step), "_step_handler", NULL);
}

void addins_event_fullscreen_invoke(bool is_fullscreen)
{
    addin_event_invoke(get_handler(fullscreen), "_fullscreen_handler", (void *)&is_fullscreen);
}

void addins_event_pause_invoke(bool is_paused)
{
    addin_event_invoke(get_handler(pause), "_pause_handler", (void *)&is_paused);
}
