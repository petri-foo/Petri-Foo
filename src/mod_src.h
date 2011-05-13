#ifndef MOD_SRC_H
#define MOD_SRC_H

#include <stdbool.h>


#include "names.h"


/* construct/destruct */
void        mod_src_create(void);
void        mod_src_destroy(void);

/* get a list of modulation sources satisfying mod_src_bitmask */
id_name*    mod_src_get(int mod_src_bitmask);

/* free memory previously allocated by mod_src_get */
void        mod_src_free(id_name*);

/* get id of named mod src as long as it satisfies mod_src_bitmask */
int         mod_src_id(const char*, int mod_src_bitmask);

const char* mod_src_name(int id);

bool        mod_src_is_global(int id);
bool        mod_src_maybe_eg(const char*);
bool        mod_src_maybe_lfo(const char*);

#endif
