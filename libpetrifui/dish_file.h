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

char*   dish_file_name(const char* path, const char* basename);

/*  dish_file_read reads a dish file and sets internal state accordingly
    to whether the bank is a full-save type or quick-save
 */
int     dish_file_read(const char* path);

/*  dish_file_write saves a dish file. depending on the internal state
    it might write a quick-save dish file or full-save dir+file+symlinks
 */
int     dish_file_write(const char* path);

/*  dish_file_write_full saves the dish file within dir 'bank_dir'
    and additionally creates symlinks to samples within dirs created
    from the hash of the original sample paths. it also modifies
    interal state so that dish_file_write does the right thing.
 */
int     dish_file_write_full(const char* bank_dir);


/*  dish_file_new resets internal implementation state. it should
    not be called unless all patches have been wiped and petri-foo
    is not running under session management such as NSM.

    * it does not clear existing patch data *
 */
void    dish_file_new(void);

#endif
