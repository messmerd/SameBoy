
#include <stdlib.h>
#include <stdio.h>
#include <Core/addin_api.h>

ADDIN_INIT

SBAPI_DECLARE_FULLSCREEN_HANDLER(fullscreen_handler)
SBAPI_DECLARE_MENU_HANDLER(menu_handler)

int start(addin_start_args_t args)
{
    printf("Add-in 1: Starting...\n");
    
    const char *version = SBAPI_get_version();
    printf("Add-in 1: SameBoy version=%s\n", version);

    // Subscribe to fullscreen toggle events:
    bool error = SBAPI_event_fullscreen_subscribe(SUBSCRIBE);
    if (error)
        printf("Add-in 1: Failed to subscribe to fullscreen event.\n");

    error = SBAPI_event_pause_subscribe(SUBSCRIBE);
    if (error)
        printf("Add-in 1: Failed to subscribe to pause event.\n");

    GB_gameboy_t *gb = SBAPI_get_GB();

    if (!gb)
        printf("Add-in 1: Error: gb is null.\n");

    if (SBAPI_is_inited())
    {
        printf("Add-in 1: GB is inited.\n");
        bool is_sgb = SBAPI_is_sgb();
        if (is_sgb)
            printf("Add-in 1: Model is a Super Game Boy.\n");
        else
            printf("Add-in 1: Not a Super Game Boy.\n");
    }
    
    printf("Add-in 1: Done starting.\n\n");
    return 0;
}

int stop()
{
    printf("Add-in 1: Done stopping.\n\n");

    // Clean up memory and other housekeeping here
    // Events are automatically unsubscribed when this function is called

    return 0;
}

void fullscreen_handler(bool is_fullscreen)
{
    if (is_fullscreen)
        printf("Add-in 1: Toggled fullscreen on.\n");
    else
        printf("Add-in 1: Toggled fullscreen off.\n");
}

void menu_handler(bool is_paused)
{
    if (is_paused)
        printf("Add-in 1: Opened menu.\n");
    else
        printf("Add-in 1: Closed menu.\n");
}
