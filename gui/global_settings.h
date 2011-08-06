/*  Petri-Foo is a fork of the Specimen audio sampler.

    Copyright 2011 Brendan Jones

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


#ifndef SETTINGS_H
#define SETTINGS_H


/*  global settings
    
 */

typedef struct global_settings_def
{
    char*       filename;
    char*       last_sample_dir;
    char*       last_bank_dir;

} global_settings;


void                settings_init(void);
int                 settings_read(const char* path);
int                 settings_write(void);
global_settings*    settings_get(void);
void                settings_free();


#endif
