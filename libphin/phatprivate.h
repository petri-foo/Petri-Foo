#ifndef __PHAT_PRIVATE_H__
#define __PHAT_PRIVATE_H__

#include <gtk/gtk.h>
#include <stdio.h>
#include "config.h"

/* we don't particularly want debugging right now */
#ifdef DEBUG
#undef DEBUG
#define DEBUG 0
#endif

#define debug(...) if (DEBUG) fprintf (stderr, __VA_ARGS__)


extern gboolean supports_alpha;

void phat_screen_changed(GtkWidget *widget, GdkScreen *old_screen,
                                                    gpointer userdata);

void phat_warp_pointer (int xsrc, int ysrc, int xdest, int ydest);

void set_cairo_rgba_from_gdk(cairo_t* cr, GdkColor* col, double a);
void gdk_col_to_double(GdkColor* c, double* r, double* g, double* b);


#endif /* __PHAT_PRIVATE_H__ */
