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
#include <gdk/gdk.h>
#include <gdk/gdkx.h>
#include "phinprivate.h"

void phin_warp_pointer (int xsrc, int ysrc,
                        int xdest, int ydest)
{
    (void)xsrc; (void)ysrc;
/* this function used to use the Xlib function:
    XWarpPointer (GDK_DISPLAY ( ), None, None, 0, 0, 0, 0, x, y);

    where:
    x = xdest - xsrc;
    y = ydest - ysrc;
*/
    gdk_display_warp_pointer (gdk_display_get_default(),
                              gdk_screen_get_default(), xdest, ydest);
}



void set_cairo_rgba_from_gdk(cairo_t* cr, GdkColor* col, double a)
{
    double r = col->red / 65535.0f;
    double g = col->green / 65535.0f;
    double b = col->blue / 65535.0f;

    if (supports_alpha)
        cairo_set_source_rgba(cr, r, g, b, a);
    else
        cairo_set_source_rgb(cr, r, g, b);
}


void gdk_col_to_double(GdkColor* c, double* r, double* g, double* b)
{
    *r = c->red / 65535.0f;
    *g = c->green / 65535.0f;
    *b = c->blue / 65535.0f;
}


/* Only some X servers support alpha channels. Always have a fallback */
gboolean supports_alpha = FALSE;


/* Only some X servers support alpha channels. Always have a fallback */
void phin_screen_changed(GtkWidget *widget, GdkScreen *old_screen,
                                                    gpointer userdata)
{
    (void)old_screen; (void)userdata;
    /* To check if the display supports alpha channels, get the colormap */
    GdkScreen *screen = gtk_widget_get_screen(widget);
    GdkVisual *colormap = gdk_screen_get_rgba_visual(screen);

    if (!colormap)
    {
        debug("Your screen does not support alpha channels!\n");
        colormap = gdk_screen_get_system_visual(screen);
        supports_alpha = FALSE;
    }
    else
    {
        debug("Your screen supports alpha channels!\n");
        supports_alpha = TRUE;
    }

    /* Now we have a colormap appropriate for the screen, use it */
    gtk_widget_set_visual(widget, colormap);
}
