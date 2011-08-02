#include <gdk/gdk.h>
#include <gdk/gdkx.h>
#include "phinprivate.h"

void phin_warp_pointer (int xsrc, int ysrc,
                        int xdest, int ydest)
{
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
    /* To check if the display supports alpha channels, get the colormap */
    GdkScreen *screen = gtk_widget_get_screen(widget);
    GdkColormap *colormap = gdk_screen_get_rgba_colormap(screen);

    if (!colormap)
    {
        debug("Your screen does not support alpha channels!\n");
        colormap = gdk_screen_get_rgb_colormap(screen);
        supports_alpha = FALSE;
    }
    else
    {
        debug("Your screen supports alpha channels!\n");
        supports_alpha = TRUE;
    }

    /* Now we have a colormap appropriate for the screen, use it */
    gtk_widget_set_colormap(widget, colormap);
}
