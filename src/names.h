#ifndef NAMES_H
#define NAMES_H

#include <stdbool.h>


void            names_create(void);
void            names_destroy(void);

const char**    names_lfo_shapes_get(void);
int             names_lfo_shapes_id_from_str(const char*);

char**          names_mod_srcs_get(void);
int             names_mod_srcs_id_from_str(const char*);

const char**    names_egs_get(void);
bool            names_egs_maybe_eg_id(const char*);
int             names_egs_id_from_str(const char*);

const char**    names_lfos_get(void);
bool            names_lfos_maybe_lfo_id(const char*);
int             names_lfos_id_from_str(const char*);

const char**    names_params_get(void);
int             names_params_id_from_str(const char*);


#endif
