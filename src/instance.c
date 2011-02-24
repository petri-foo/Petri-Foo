#include "instance.h"

#include <stdlib.h>
#include <string.h>


static char *instance_name = 0;

const char* get_instance_name(void)
{
    return (instance_name) ? instance_name : DEFAULT_INSTANCE_NAME;
}


void set_instance_name(const char* str)
{
    size_t len;

    free_instance_name();

    if (!str)
        return;

    len = strlen(str);

    instance_name = malloc(len + 1);
    strcpy(instance_name, str);
}


void free_instance_name(void)
{
    free(instance_name);
    instance_name = 0;
}
