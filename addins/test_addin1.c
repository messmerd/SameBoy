
#include <stdlib.h>
#include <stdio.h>
#include <Core/addin_api.h>

//extern GB_gameboy_t gb;

ADDIN_API int start(START_ARGS args)
{
    printf("Plugin1: Starting...\n");
    GB_gameboy_t* gb = SAMEBOY_get_GB();

    bool is_sgb = GB_is_sgb(gb);
    if (is_sgb)
        printf("Plugin1: Model is a Super Game Boy.\n");
    else
        printf("Plugin1: Not a Super Game Boy.\n");

    return 0;
}

ADDIN_API int stop()
{
    printf("Plugin1: Stopping.\n");

    return 0;
}
