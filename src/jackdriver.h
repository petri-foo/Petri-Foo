#ifndef __JACKDRIVER_H__
#define __JACKDRIVER_H__

#include "config.h"

#include <jack/jack.h>


void            jackdriver_set_uuid(char *uuid);
jack_client_t*  jackdriver_get_client(void);


#endif /* __JACKDRIVER_H__ */
