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


#ifndef __SPECIMEN_H__
#define __SPECIMEN_H__

#include "config.h"
#include <stdio.h>
#include <signal.h>

#ifndef DEBUG
# define DEBUG 0
#endif

enum
{
    FUBAR = -69
};

#define DEFAULT_AMPLITUDE 0.7 /* default amplitude stuff is set to, from 0 to 1 */

#ifndef PIXMAPSDIR
# define PIXMAPSDIR INSTALLDIR"/petri-foo/pixmaps/"
#endif

#define errmsg(...) {fprintf(stderr, "%20s:%5d\t%30s", __FILE__, __LINE__, __FUNCTION__); fprintf(stderr, ": "); fprintf(stderr, __VA_ARGS__);}

#if DEBUG
# define debug(...) errmsg(__VA_ARGS__)
#else
# define debug(...)
#endif

typedef sig_atomic_t Atomic;

#endif /* __SPECIMEN_H__ */
