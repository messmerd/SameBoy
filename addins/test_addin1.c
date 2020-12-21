
#include <stdlib.h>
#include <stdio.h>
#include <Core/addin_api.h>

ADDIN_API int start(start_args_t args)
{
    printf("Plugin1: Starting...\n");
    
    int number = SAMEBOY_api_test_function(23);
    printf("Plugin1: number=%i\n", number);

    GB_gameboy_t* gb = SAMEBOY_get_GB();

    if (!gb)
        printf("Plugin1: Error: gb is null.\n");

    bool is_sgb = GB_is_sgb(gb);
    if (is_sgb)
        printf("Plugin1: Model is a Super Game Boy.\n");
    else
        printf("Plugin1: Not a Super Game Boy.\n");
    
    printf("Plugin1: Done starting.\n");
    return 0;
}

ADDIN_API int stop()
{
    printf("Plugin1: Stopping.\n");

    return 0;
}
