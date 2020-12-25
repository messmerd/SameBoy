#ifndef utils_h
#define utils_h

#include <stddef.h>
#include <stdbool.h>

typedef enum {
    INI_VALUE_TYPE_STRING,
    INI_VALUE_TYPE_DOUBLE,
    INI_VALUE_TYPE_LONG_LONG,
    INI_VALUE_TYPE_LONG,
    INI_VALUE_TYPE_INT,
    INI_VALUE_TYPE_BOOL
} ini_value_type_t;

const char *resource_folder(void);
char *resource_path(const char *filename);
void replace_extension(const char *src, size_t length, char *dest, const char *ext);
bool file_exists(const char *filename);

/* 
    Parses a given INI file, looking for the specified keys, interpreting 
        the values as the specified types, then storing the results at the desired destinations.
    
    dest: Array of pointers to where the parsed values will be stored.
        NOTE: If you are reading a string, use (void *[]){&my_string} where my_string is a 
        C string.
    filename: INI file to open
    keys: Array of key names
    types: Array that specifies the types that INI values should be interpreted as
    size: The number of elements in dest, keys, and size

    Returns false if successful.
    NOTE: All arrays must use the same ordering for keys/values/types
*/ 
bool ini_read_keys(void *dest[], const char *filename, const char *keys[], const ini_value_type_t types[], const size_t size);

bool ini_read_key(void *dest, const char *filename, const char key[], const ini_value_type_t type);

#endif /* utils_h */
