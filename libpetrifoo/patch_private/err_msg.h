/*  Petri-Foo is a fork of the Specimen audio sampler.

    Copyright 2012 James W. Morris

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

#ifndef PRIVATE_ERR_MSG
#define PRIVATE_ERR_MSG

#include <stdio.h>

#define errmsg(...)                         \
{                                           \
    fprintf(stderr, "%40s:%5d  %-35s: ",    \
            __FILE__ + SRC_DIR_STRLEN + 1,  \
            __LINE__, __FUNCTION__);        \
    fprintf(stderr, __VA_ARGS__);           \
}

#endif
