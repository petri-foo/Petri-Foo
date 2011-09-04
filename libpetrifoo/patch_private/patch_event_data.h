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


#ifndef PATCH_EVENT_DATA_H
#define PATCH_EVENT_DATA_H


#include <stdbool.h>
#include <stdint.h>


#define EV_MAX_IDS 4
/* id[0] = patch_id*/

struct _PatchEvent
{
    int id[EV_MAX_IDS];
    

}/* typedef'd in patch_event.h */;


typedef struct _EventSetVar
{
    uint16_t type;
    uint16_t size;
    int patch_id;

    int var_id;

    union {
        bool    b;
        int     i;
        float   f;
        char*   data;
    };

} EventSetVar;


typedef struct _EventSetParam
{
    uint16_t type;
    uint16_t size;
    int patch_id;

    int     param;
    float   value;

} EventSetParam;


typedef struct _EventSetParMod
{
    uint16_t type;
    uint16_t size;
    int patch_id;

    int     param;
    int     slot;
    float   value;

} EventSetParMod;

#endif
