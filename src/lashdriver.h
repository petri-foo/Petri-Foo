#ifndef __LASHDRIVER_H__
#define __LASHDRIVER_H__

#include "config.h"

#ifdef HAVE_LASH

#include <lash/lash.h>

void lashdriver_init          (lash_args_t *lash_args);
void lashdriver_start         (void);
void lashdriver_set_jack_name (char *name);
void lashdriver_set_alsa_id   (int id);
void lashdriver_stop          (void);

#endif /* HAVE_LASH */

#endif /* __LASHDRIVER_H__ */
