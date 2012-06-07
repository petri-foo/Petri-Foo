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


#include "patch_event.h"

#include "patch_private/patch_event_data.h"



int patch_event_set(int patch_id, EvType evtype, ...)
{
    return 0;
}


BaseEvent* patch_event_get(int patch_id, EvType evtype, ...)
{
    return 0;
}



/* assertions on these to make sure correct type used */
bool base_event_get_bool(BaseEvent* ev)
{
    return false;
}


float base_event_get_float(BaseEvent* ev)
{
    return 0;
}


int base_event_get_int(BaseEvent* ev)
{
    return 0;
}



