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


/*  recommended file extension for petri-foo data files:
    (includes dot)
 */

const char* dish_file_extension(void);

void    dish_file_state_init(void);

/*  dish_file_state_reset resets internal implementation state. it should
    not be called unless all patches have been wiped and petri-foo
    is not running under session management such as NSM.

    * it does not clear existing patch data *
 */
void    dish_file_state_reset(void);

void    dish_file_state_cleanup(void);

/*  dish_file_read reads a dish file and sets internal state accordingly
    to whether the bank is a full-save type or quick-save
 */
int     dish_file_read(const char* path);

/*  dish_file_write_quick saves a dish file without folders or symlinks
    to samples.
 */
int     dish_file_write_quick(const char* path);

/*  dish_file_write_full saves to /parent/name/name.petri-foo
    and additionally creates symlinks to samples within dirs created
    from the hash of the original sample paths. it also modifies
    interal state so that dish_file_write does the right thing.
 */
int     dish_file_write_full(const char* parent, const char* name);

/*  dish_file_write saves again to the last opened/saved dish file */
int     dish_file_write(void);

#endif
