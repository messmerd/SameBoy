
#include <stdlib.h>
#include <stdio.h>
#include <Core/addin_api.h>

ADDIN_INIT

ADDIN_API void fullscreen_event_handler(bool isFullscreen)
{
    if (isFullscreen)
        printf("Plugin1: Toggled fullscreen on.\n");
    else
        printf("Plugin1: Toggled fullscreen off.\n");
}

ADDIN_API int start(start_args_t args)
{
    printf("Plugin1: Starting...\n");

    const char *version = GB_EXT_get_version();
    printf("Plugin1: SameBoy version=%s\n", version);

    // Subscribe to fullscreen toggle events:
    bool error = GB_EXT_event_fullscreen_subscribe("fullscreen_event_handler");
    if (error)
        printf("Plugin1: Failed to subscribe to fullscreen event.\n");

    GB_gameboy_t *gb = GB_EXT_get_GB();

    if (!gb)
        printf("Plugin1: Error: gb is null.\n");

    if (GB_is_inited(gb))
    {
        printf("Plugin1: GB is inited.\n");
        bool is_sgb = GB_is_sgb(gb);
        if (is_sgb)
            printf("Plugin1: Model is a Super Game Boy.\n");
        else
            printf("Plugin1: Not a Super Game Boy.\n");
    }
    
    printf("Plugin1: Done starting.\n");
    return 0;
}

ADDIN_API int stop()
{
    printf("Plugin1: Stopping.\n");

    return 0;
}

