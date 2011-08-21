/*  Phin is a fork of the PHAT Audio Toolkit.
    Phin is part of Petri-Foo. Petri-Foo is a fork of Specimen.

    Original author Pete Bessman
    Copyright 2005 Pete Bessman
    Copyright 2011 James W. Morris

    This file is part of Phin.

    Phin is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License version 2 as
    published by the Free Software Foundation.

    Phin is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with Phin.  If not, see <http://www.gnu.org/licenses/>.

    This file is a derivative of a PHAT original, modified 2011
*/
#ifndef __PHIN_PRIVATE_H__
#define __PHIN_PRIVATE_H__

#include <gtk/gtk.h>
#include <stdio.h>
#include "config.h"


#ifndef DEBUG
#define DEBUG 0
#endif


/*  no mechanism to set debug for petri-foo but not phin, so force no debug
    here:
 */

#ifdef DEBUG
#undef DEBUG
#define DEBUG 0
#endif


#define debug(...) if (DEBUG) fprintf (stderr, __VA_ARGS__)


extern gboolean supports_alpha;

void phin_screen_changed(GtkWidget *widget, GdkScreen *old_screen,
                                                    gpointer userdata);

void phin_warp_pointer (int xsrc, int ysrc, int xdest, int ydest);

void set_cairo_rgba_from_gdk(cairo_t* cr, GdkColor* col, double a);
void gdk_col_to_double(GdkColor* c, double* r, double* g, double* b);


#endif /* __PHIN_PRIVATE_H__ */
