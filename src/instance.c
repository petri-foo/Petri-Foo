#include "instance.h"

char *get_instance_name (void)
{
	return (instance_name) ? instance_name : DEFAULT_INSTANCE_NAME;
}
