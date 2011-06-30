/*  Petri-Foo is a fork of the Specimen audio sampler.

    Copyright 2011 James W. Morris

    This file is part of Petri-Foo.

    Petri-Foo is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License version 2 as
    published by the Free Software Foundation.

    Petri-Foo is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with Petri-Foo.  If not, see <http://www.gnu.org/licenses/>.
*/


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
