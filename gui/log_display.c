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


#include <gtk/gtk.h>

#include "log_display.h"
#include "gui.h"
#include "msg_log.h"
#include "petri-foo.h"

#include <string.h>
#include <stdlib.h>


static GtkWidget*   window;
static GtkWidget*   textview;



static gboolean cb_close(GtkWidget* widget)
{
    gtk_widget_hide(widget);
    cb_menu_view_log_display_showing(false);
    return TRUE;
}


static void msg_log_callback(const char* msg, int msg_base_type)
{
    GtkTextBuffer*  buffer;
    GtkTextIter     iter;
    GtkTextMark*    mark;
    const char* tag = 0;

    switch(msg_base_type)
    {
    case MSG___TYPE_DEBUG:      tag = "debug";      break;
    case MSG___TYPE_WARNING:    tag = "warning";    break;
    case MSG___TYPE_ERROR:      tag = "error";      break;
    case MSG___TYPE_CRITICAL:   tag = "critical";   break;
    default:                    tag = "message";    break;
    }

    buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(textview));
    gtk_text_buffer_get_end_iter(buffer, &iter);

    gtk_text_buffer_insert_with_tags_by_name(buffer, &iter, msg, -1,
                                                        tag, NULL);

    mark = gtk_text_buffer_get_mark (buffer, "scroll");
    gtk_text_buffer_move_mark (buffer, mark, &iter);
    gtk_text_view_scroll_mark_onscreen(GTK_TEXT_VIEW(textview), mark);
}


void log_display_show(void)
{
     gtk_widget_show(window);
}


void log_display_hide(void)
{
     gtk_widget_hide(window);
}


void log_display_init(GtkWidget* parent)
{
    (void)parent; /* why is this still here? */
    GtkWidget* swindow;
    GtkWidget* vbox;

    GtkTextBuffer*  buffer;
    GtkTextIter iter;

    debug("Initializing audio settings window\n");

    window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW (window), "Petri-Foo Message Log");
    gtk_window_set_default_size(GTK_WINDOW(window), 700, 160);
    gtk_window_set_modal (GTK_WINDOW (window), FALSE);

    g_signal_connect(window, "delete-event", 
                        G_CALLBACK(cb_close), NULL);

    gtk_container_set_border_width(GTK_CONTAINER(window), GUI_SPACING);

    vbox = gtk_vbox_new(FALSE, GUI_SPACING);
    gtk_container_add(GTK_CONTAINER(window), vbox);
    gtk_widget_show(vbox);

    swindow = gtk_scrolled_window_new(NULL, NULL);
    gtk_box_pack_start(GTK_BOX(vbox), swindow, TRUE, TRUE, 0);
    gtk_widget_show(swindow);

    textview = gtk_text_view_new();
    gtk_container_add(GTK_CONTAINER(swindow), textview);
    gtk_widget_show(textview);

    buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(textview));

    /*  didn't need this scroll mark until colour tags were added. */
    gtk_text_buffer_get_end_iter(buffer, &iter);
    gtk_text_buffer_create_mark(buffer, "scroll", &iter, TRUE);

    gtk_text_buffer_create_tag(buffer,  "debug",
                                        "foreground", "blue", NULL);

    gtk_text_buffer_create_tag(buffer,  "message", NULL);

    gtk_text_buffer_create_tag(buffer,  "warning",
                                        "foreground", "purple", NULL);

    gtk_text_buffer_create_tag(buffer,  "error",
                                        "foreground", "red", NULL);

    gtk_text_buffer_create_tag(buffer,  "critical",
                                        "foreground", "red", NULL);


    /* Tell msg log to use our callback which updates the text view */
    msg_log_set_message_cb(msg_log_callback);
}
