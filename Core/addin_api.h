// Include this header in your C/C++ project to gain access to add-in API functions

#ifndef addin_api_h
#define addin_api_h

#define ADDIN_API_VERSION "0.1"

#ifdef _WIN32
#    ifdef ADDINS_SERVER
#        define SB_ADDIN_FUNC __declspec(dllimport)
#        define SB_ADDIN_API __declspec(dllexport)
#    else
#        ifdef __cplusplus
#            define SB_ADDIN_FUNC extern "C" __declspec(dllexport)
#        else
#            define SB_ADDIN_FUNC __declspec(dllexport)
#        endif
#        define SB_ADDIN_API __declspec(dllimport)
#    endif
#else
#    ifdef __cplusplus
#        define SB_ADDIN_FUNC extern "C"
#    else
#        define SB_ADDIN_FUNC
#    endif
#    define SB_ADDIN_API
#endif

#include <Core/gb.h>

#ifndef __printflike
/* Missing from Linux headers. */
#define __printflike(fmtarg, firstvararg) \
__attribute__((__format__ (__printf__, fmtarg, firstvararg)))
#endif

#ifdef __cplusplus
extern "C" { 
#endif

#ifdef ADDIN_WINDOWS_CONSOLE_BUILD
#define _ADDIN_WINDOWS_CONSOLE_BUILD 1
#else
#define _ADDIN_WINDOWS_CONSOLE_BUILD 0
#endif

//////////////////////
// Enums / Structs  //
//////////////////////

typedef unsigned int _addin_id_t;

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
} addin_start_args_t;

typedef struct addin_log_event_args_s {
    const char *string;
    GB_log_attributes attributes;
} addin_log_event_args_t;

//////////////////////
// Wrapper          //
//////////////////////

#define SB_ADDIN_FUNC_CALL __cdecl

/* ADDIN_INIT must be placed at the top of add-in code */
#define ADDIN_INIT \
    SB_ADDIN_FUNC _addin_id_t _addin_id; \
    SB_ADDIN_FUNC int SB_ADDIN_FUNC_CALL _addin_init(void *args) {_addin_id = *(_addin_id_t *)args; return _ADDIN_WINDOWS_CONSOLE_BUILD; } \
    int start(addin_start_args_t args); \
    int stop(void); \
    SB_ADDIN_FUNC int SB_ADDIN_FUNC_CALL _addin_start(void *args) {return start(*(addin_start_args_t *)args); } \
    SB_ADDIN_FUNC int SB_ADDIN_FUNC_CALL _addin_stop(void *args) {return stop(); }

/* Use this macro with API functions that require an add-in id */
#define M(args) _addin_id, args

/* Use these macros when subscribing to events */
#define SUBSCRIBE _addin_id, 1
#define UNSUBSCRIBE _addin_id, 0
#define RANGE_UNSUBSCRIBE _addin_id, 2

//////////////////////
// Event Handlers   //
//////////////////////

/*
typedef void (*SBAPI_vblank_callback_t)(void);
typedef void (*SBAPI_log_callback_t)(const char *string, GB_log_attributes attributes);
typedef char *(*SBAPI_input_callback_t)(void);
typedef uint32_t (*SBAPI_rgb_encode_callback_t)(uint8_t r, uint8_t g, uint8_t b);
typedef void (*SBAPI_infrared_callback_t)(bool on);
typedef void (*SBAPI_rumble_callback_t)(double rumble_amplitude);
typedef void (*SBAPI_serial_transfer_bit_start_callback_t)(bool bit_to_send);
typedef bool (*SBAPI_serial_transfer_bit_end_callback_t)(void);
typedef void (*SBAPI_update_input_hint_callback_t)(void);
typedef void (*SBAPI_joyp_write_callback_t)(uint8_t value);
typedef void (*SBAPI_icd_pixel_callback_t)(uint8_t row);
typedef void (*SBAPI_icd_hreset_callback_t)(void);
typedef void (*SBAPI_icd_vreset_callback_t)(void);
typedef void (*SBAPI_boot_rom_load_callback_t)(GB_boot_rom_t type);

// Additional:
typedef void (*SBAPI_step_callback_t)(void);
typedef void (*SBAPI_fullscreen_callback_t)(bool is_fullscreen);
typedef void (*SBAPI_pause_callback_t)(bool is_paused);
*/

#define SBAPI_DECLARE_VBLANK_HANDLER(handler) void handler(void); \
    SB_ADDIN_FUNC int SB_ADDIN_FUNC_CALL _vblank_handler(void *args) {handler(); return 0; }
#define SBAPI_DECLARE_LOG_HANDLER(handler) void handler(addin_log_event_args_t args); \
    SB_ADDIN_FUNC int SB_ADDIN_FUNC_CALL _log_handler(void *_args) {handler(*(addin_log_event_args_t *)_args); return 0; }

#define SBAPI_DECLARE_STEP_HANDLER(handler) void handler(void); \
    SB_ADDIN_FUNC int SB_ADDIN_FUNC_CALL _step_handler(void *args) {handler(); return 0; }
#define SBAPI_DECLARE_FULLSCREEN_HANDLER(handler) void handler(bool is_fullscreen); \
    SB_ADDIN_FUNC int SB_ADDIN_FUNC_CALL _fullscreen_handler(void *args) {handler(*(bool *)args); return 0; }
#define SBAPI_DECLARE_MENU_HANDLER(handler) void handler(bool menu_open); \
    SB_ADDIN_FUNC int SB_ADDIN_FUNC_CALL _menu_handler(void *args) {handler(*(bool *)args); return 0; }


////////// EVENT HANDLING - SUBSCRIBE ///////////

// Wrappers for SameBoy Core events:

SB_ADDIN_API bool SBAPI_event_vblank_subscribe(unsigned addin_id, bool subscribe);
SB_ADDIN_API bool SBAPI_event_log_subscribe(unsigned addin_id, bool subscribe);
SB_ADDIN_API bool SBAPI_event_input_subscribe(unsigned addin_id, bool subscribe);
SB_ADDIN_API bool SBAPI_event_async_input_subscribe(unsigned addin_id, bool subscribe);
SB_ADDIN_API bool SBAPI_event_rgb_encode_subscribe(unsigned addin_id, bool subscribe);
SB_ADDIN_API bool SBAPI_event_infrared_subscribe(unsigned addin_id, bool subscribe);
SB_ADDIN_API bool SBAPI_event_rumble_subscribe(unsigned addin_id, bool subscribe);
SB_ADDIN_API bool SBAPI_event_update_input_hint_subscribe(unsigned addin_id, bool subscribe);

/* Called when a new boot ROM is needed. The handler should call GB_load_boot_rom or GB_load_boot_rom_from_buffer */
SB_ADDIN_API bool SBAPI_event_boot_rom_load_subscribe(unsigned addin_id, bool subscribe);

/* These APIs are used when using internal clock */
SB_ADDIN_API bool SBAPI_event_serial_transfer_bit_start_subscribe(unsigned addin_id, bool subscribe);
SB_ADDIN_API bool SBAPI_event_serial_transfer_bit_end_subscribe(unsigned addin_id, bool subscribe);

/* For integration with SFC/SNES emulators */
SB_ADDIN_API bool SBAPI_event_joyp_write_subscribe(unsigned addin_id, bool subscribe);
SB_ADDIN_API bool SBAPI_event_icd_pixel_subscribe(unsigned addin_id, bool subscribe);
SB_ADDIN_API bool SBAPI_event_icd_hreset_subscribe(unsigned addin_id, bool subscribe);
SB_ADDIN_API bool SBAPI_event_icd_vreset_subscribe(unsigned addin_id, bool subscribe);


// Additional events for SameBoy:

SB_ADDIN_API bool SBAPI_event_step_subscribe(unsigned addin_id, bool subscribe);
SB_ADDIN_API bool SBAPI_event_fullscreen_subscribe(unsigned addin_id, bool subscribe);
SB_ADDIN_API bool SBAPI_event_menu_subscribe(unsigned addin_id, bool subscribe);

//////////////////////
// API Functions    //
//////////////////////

SB_ADDIN_API void SBAPI_init(GB_model_t model);
SB_ADDIN_API bool SBAPI_is_inited(void);
SB_ADDIN_API bool SBAPI_is_cgb(void);
SB_ADDIN_API bool SBAPI_is_sgb(void); // Returns true if the model is SGB or SGB2
SB_ADDIN_API bool SBAPI_is_hle_sgb(void); // Returns true if the model is SGB or SGB2 and the SFC/SNES side is HLE'd
SB_ADDIN_API GB_model_t SBAPI_get_model(void);
SB_ADDIN_API void SBAPI_free(void);
SB_ADDIN_API void SBAPI_reset(void);
SB_ADDIN_API void SBAPI_switch_model_and_reset(GB_model_t model);

/* Returns the time passed, in 8MHz ticks. */
SB_ADDIN_API uint8_t SBAPI_run(void);
/* Returns the time passed since the last frame, in nanoseconds */
SB_ADDIN_API uint64_t SBAPI_run_frame(void);

/* Returns a mutable pointer to various hardware memories. If that memory is banked, the current bank
   is returned at *bank, even if only a portion of the memory is banked. */
SB_ADDIN_API void *SBAPI_get_direct_access(GB_direct_access_t access, size_t *size, uint16_t *bank);

SB_ADDIN_API void *SBAPI_get_user_data(void);
SB_ADDIN_API void SBAPI_set_user_data(void *data);

SB_ADDIN_API int SBAPI_load_boot_rom(const char *path);
SB_ADDIN_API void SBAPI_load_boot_rom_from_buffer(const unsigned char *buffer, size_t size);
SB_ADDIN_API int SBAPI_load_rom(const char *path);
SB_ADDIN_API void SBAPI_load_rom_from_buffer(const uint8_t *buffer, size_t size);
SB_ADDIN_API int SBAPI_load_isx(const char *path);

SB_ADDIN_API int SBAPI_save_battery_size(void);
SB_ADDIN_API int SBAPI_save_battery_to_buffer(uint8_t *buffer, size_t size);
SB_ADDIN_API int SBAPI_save_battery(const char *path);

SB_ADDIN_API void SBAPI_load_battery_from_buffer(const uint8_t *buffer, size_t size);
SB_ADDIN_API void SBAPI_load_battery(const char *path);

SB_ADDIN_API void SBAPI_set_turbo_mode(bool on, bool no_frame_skip);
SB_ADDIN_API void SBAPI_set_rendering_disabled(bool disabled);

SB_ADDIN_API void SBAPI_log(const char *fmt, ...) __printflike(1, 2);
SB_ADDIN_API void SBAPI_attributed_log(GB_log_attributes attributes, const char *fmt, ...) __printflike(2, 3);

SB_ADDIN_API void SBAPI_set_pixels_output(uint32_t *output);
SB_ADDIN_API void SBAPI_set_border_mode(GB_border_mode_t border_mode);

SB_ADDIN_API void SBAPI_set_infrared_input(bool state);

SB_ADDIN_API void SBAPI_set_palette(const GB_palette_t *palette);

/* These APIs are used when using external clock */
SB_ADDIN_API bool SBAPI_serial_get_data_bit(void);
SB_ADDIN_API void SBAPI_serial_set_data_bit(bool data);

SB_ADDIN_API void SBAPI_disconnect_serial(void);

/* For cartridges with an alarm clock */
SB_ADDIN_API unsigned SBAPI_time_to_alarm(void); // 0 if no alarm

SB_ADDIN_API uint32_t SBAPI_get_clock_rate(void);
SB_ADDIN_API void SBAPI_set_clock_multiplier(double multiplier);

SB_ADDIN_API unsigned SBAPI_get_screen_width(void);
SB_ADDIN_API unsigned SBAPI_get_screen_height(void);
SB_ADDIN_API double SBAPI_get_usual_frame_rate(void);
SB_ADDIN_API unsigned SBAPI_get_player_count(void);

/////////////////////////



// The following  API functions are not a part of the SameBoy Core 
//  and allow for interaction with SameBoy itself

SB_ADDIN_API GB_gameboy_t *SBAPI_get_GB(void);

SB_ADDIN_API addin_manifest_t *SBAPI_get_manifest(void);

SB_ADDIN_API const char *SBAPI_get_version(void);

// More to come


#ifdef __cplusplus
}
#endif

#endif /* addin_api_h */
