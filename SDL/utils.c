#include <SDL.h>
#include <ctype.h>
#include <stdio.h>
#include <string.h>
#include "utils.h"

#define INI_BUFFER_SIZE 200

const char *resource_folder(void)
{
#ifdef DATA_DIR
    return DATA_DIR;
#else
    static const char *ret = NULL;
    if (!ret) {
        ret = SDL_GetBasePath();
        if (!ret) {
            ret = "./";
        }
    }
    return ret;
#endif
}

char *resource_path(const char *filename)
{
    static char path[1024];
    snprintf(path, sizeof(path), "%s%s", resource_folder(), filename);
    return path;
}

void replace_extension(const char *src, size_t length, char *dest, const char *ext)
{
    memcpy(dest, src, length);
    dest[length] = 0;

    /* Remove extension */
    for (size_t i = length; i--;) {
        if (dest[i] == '/') break;
        if (dest[i] == '.') {
            dest[i] = 0;
            break;
        }
    }

    /* Add new extension */
    strcat(dest, ext);
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

bool ini_read_keys(void *dest[], const char *filename, const char *keys[], const ini_value_type_t types[], const size_t size)
{
    if (!dest || !keys || !types || size == 0)
        return true; // Invalid arguments

    FILE *fp = NULL;
    fopen_s(&fp, filename, "r");
    if (!fp)
        return true;
    
    char buffer[INI_BUFFER_SIZE] = {'\0'};
    char key[INI_BUFFER_SIZE] = {'\0'};
    char value[INI_BUFFER_SIZE] = {'\0'};

    while (fgets(buffer, INI_BUFFER_SIZE, fp))
    {
        // A key must start with a letter. 
        // Whitespace preceeding key is not allowed.
        if (!isalpha(buffer[0]))
        {
            memset(buffer, '\0', INI_BUFFER_SIZE);
            continue;
        }
        
        // Locate the '=' that divides key from value
        char *value_start = strchr(buffer, '=');
        if (!value_start || value_start == &buffer[0])
        {
            memset(buffer, '\0', INI_BUFFER_SIZE);
            continue;
        }
        
        // Trim whitespace b/w key and '='
        char *key_end = value_start - 1;
        while (isspace(*key_end) && key_end > &buffer[0])
            key_end--;
        
        // Get key
        strncpy(key, buffer, key_end - &buffer[0] + 1);
        key[key_end - &buffer[0] + 1] = '\0';

        // This can significantly increase the performance of ini_read_key() for 
        //  large INI files at little cost to this function:
        if (size == 1 && strcmp(keys[0], key) != 0)
        {
            memset(buffer, '\0', INI_BUFFER_SIZE);
            continue;
        }

        value_start++;

        // Get value, trimming whitespace off the end
        if (*value_start == '\n' || *value_start == '\0')
            value[0] = '\0';
        else
        {
            char *end = &buffer[INI_BUFFER_SIZE - 1];
            while (end >= value_start && (*end == '\0' || isspace(*end)))
                end--;
            
            strncpy(value, value_start, end - value_start + 1);
            value[end - value_start + 1] = '\0';
        }

        // Store results
        for (size_t i = 0; i < size; ++i)
        {
            if (strcmp(keys[i], key) != 0)
                continue;
            
            switch (types[i])
            {
                case INI_VALUE_TYPE_STRING:
                    /* 
                    Evil pointer trickery so that uninitialized C strings 
                        can be passed to this function alongside other data types. 
                        If you don't call this function correctly, it may cause 
                        a seg fault.
                    */
                    void **ptr = dest[i];
                    *ptr = strdup(value);
                    break;
                case INI_VALUE_TYPE_DOUBLE:
                    *(double *)dest[i] = atof(value);
                    break;
                case INI_VALUE_TYPE_LONG_LONG:
                    *(long long *)dest[i] = atoll(value);
                    break;
                case INI_VALUE_TYPE_LONG:
                    *(long *)dest[i] = atol(value);
                    break;
                case INI_VALUE_TYPE_INT:
                    *(int *)dest[i] = atoi(value);
                    break;
                case INI_VALUE_TYPE_BOOL:
                    // Trim whitespace off of beginning for bools, and make lowercase
                    value_start = &value[0];
                    while (*value_start && isspace(*value_start))
                        value_start++;
                    for (char *value_pointer = value_start; *value_pointer; value_pointer++)
                        *value_pointer = tolower(*value_pointer);
                    *(bool *)dest[i] = value_start[0] == '1' || strcmp(value_start, "true") == 0;
                    break;
                default:
                    return true;
            }
        }
        
        memset(buffer, '\0', INI_BUFFER_SIZE);
    }

    fclose(fp);
    return false; // Success
}

bool ini_read_key(void *dest, const char *filename, const char key[], const ini_value_type_t type)
{
    // This function is just an easier-to-use version of ini_read_keys for when you only have one key to read.
    return ini_read_keys((void *[]){dest}, filename, (const char *[]){key}, (const ini_value_type_t []){type}, 1);
}
