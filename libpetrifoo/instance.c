/*  Petri-Foo is a fork of the Specimen audio sampler.

    Original Specimen author Pete Bessman
    Copyright 2005 Pete Bessman
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

    This file is a derivative of a Specimen original, modified 2011
*/


#include "instance.h"

#include <stdlib.h>
#include <string.h>

#include "config.h"

static char *instance_name = 0;

const char* get_instance_name(void)
{
    return (instance_name) ? instance_name : PACKAGE;
}


void set_instance_name(const char* str)
{
    size_t len;

    free_instance_name();

    if (!str)
        return;

    len = strlen(str);

    instance_name = malloc(len + 1);
    strcpy(instance_name, str);
}


void free_instance_name(void)
{
    free(instance_name);
    instance_name = 0;
}
