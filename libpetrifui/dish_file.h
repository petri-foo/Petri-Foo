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


#ifndef DISH_FILE_H
#define DISH_FILE_H

#include <stdbool.h>

/*  recommended file extension for petri-foo data files:
    (includes dot)
 */
const char* dish_file_extension(void);


/*  dish_file_state
        dish_file_state stores path data about the most recently
        saved or opened dish file.

    the _state_init and _state_cleanup functions are only concerned
    with the data structure used for storing state information.
*/

void    dish_file_state_init(void);
void    dish_file_state_cleanup(void);

/*  wipe path data */
void    dish_file_state_reset(void);


int     dish_file_state_set_by_path(const char* file_path, bool full_save);

/*  dish_file_has_state must be used to determine if calling
    functions relying on state paths existing is safe or not.
 */

bool        dish_file_has_state(void);

/*  the following are unsafe to call if has_path is false */
bool        dish_file_state_is_full(void);
bool        dish_file_state_is_basic(void);
const char* dish_file_state_parent_dir(void);
const char* dish_file_state_bank_dir(void);
const char* dish_file_state_bank_name(void);
const char* dish_file_state_path(void);


/*  dish_file_read reads a dish file and sets internal state accordingly
    to whether the bank is a full-save type or basic-save
 */
int     dish_file_read(const char* path);

/*  dish_file_write_basic saves a dish file without folders or symlinks
    to samples.
 */
int     dish_file_write_basic(const char* path);

/*  dish_file_write_full saves to /parent/name/name.petri-foo
    and additionally creates symlinks to samples within dirs created
    from the hash of the original sample paths. it also modifies
    interal state so that dish_file_write does the right thing.
 */
int     dish_file_write_full(const char* parent, const char* name);


/*  dish_file_import reads a dish file incorporating its contents
    into existing data. it does not affect dish_file_state.
 */
int     dish_file_import(const char* path);
int     dish_file_export(const char* path);


/*  dish_file_write saves again to the last opened/saved dish file */
int     dish_file_write(void);


#endif
