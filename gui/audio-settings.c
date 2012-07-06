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

#include "instance.h"
#include "audio-settings.h"
#include "gui.h"
#include "petri-foo.h"
#include "jackdriver.h"
#include "driver.h"
#include "sync.h"
#include "dish_file.h"

#include <string.h>
#include <stdlib.h>

static GtkWidget* window;



static void sync_cb(GtkToggleButton* button, gpointer data)
{
    (void)data;

    if (gtk_toggle_button_get_active (button))
        sync_set_method (SYNC_METHOD_JACK);
    else
        sync_set_method (SYNC_METHOD_MIDI);
}


static void restart_cb(GtkButton *button, gpointer data)
{
    (void)button;(void)data;
    driver_restart();
}

static void stop_cb(GtkButton *button, gpointer data)
{
    (void)button;(void)data;
    driver_stop();
}


static void cb_close (GtkWidget* widget, gpointer data)
{
    (void)widget; (void)data;
    debug ("Hiding audio settings window\n");
    gtk_widget_hide (window);
}


void audio_settings_show(void)
{
     debug ("Showing audio settings window\n");
     gtk_widget_show(window);
}


void audio_settings_init (GtkWidget* parent)
{
    GtkWidget* hbox;
    GtkWidget* vbox;
    GtkWidget* tmp;

    debug("Initializing audio settings window\n");

    window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW (window), "Audio Settings");
    gtk_window_set_transient_for(GTK_WINDOW(window), GTK_WINDOW(parent));
    gtk_window_set_modal (GTK_WINDOW (window), TRUE);
    g_signal_connect(GTK_WINDOW(window), "delete-event",
                                G_CALLBACK(cb_close), NULL);

    gtk_container_set_border_width(GTK_CONTAINER(window), GUI_SPACING);

    vbox = gtk_vbox_new(FALSE, GUI_SPACING);
    gtk_container_add(GTK_CONTAINER(window), vbox);
    gtk_widget_show(vbox);

    hbox = gtk_hbox_new(FALSE, GUI_SPACING);
    gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 0);
    gtk_widget_show(hbox);

    tmp = gtk_button_new_with_label("Reconnect");
    gtk_box_pack_start(GTK_BOX(hbox), tmp, FALSE, FALSE, 0);
    g_signal_connect(G_OBJECT(tmp), "clicked",
                                G_CALLBACK(restart_cb), NULL);
    gtk_widget_show(tmp);

    tmp = gtk_button_new_with_label("Disconnect");
    gtk_box_pack_start(GTK_BOX(hbox), tmp, FALSE, FALSE, 0);
    g_signal_connect(G_OBJECT(tmp), "clicked",
                                G_CALLBACK(stop_cb), NULL);
    gtk_widget_show(tmp);

    tmp = gtk_check_button_new_with_label
                            ("Sync to JACK Transport instead of MIDI");
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(tmp),
                    sync_get_method() == SYNC_METHOD_JACK ? TRUE : FALSE);

    gtk_box_pack_start(GTK_BOX(vbox), tmp, FALSE, FALSE, 0);
    g_signal_connect(G_OBJECT(tmp), "toggled",
                                G_CALLBACK(sync_cb), NULL);
    gtk_widget_show(tmp);

    hbox = gtk_hbox_new (FALSE, GUI_SPACING);
    gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 0);
    gtk_widget_show(hbox);

    tmp = gtk_button_new_from_stock(GTK_STOCK_CLOSE);
    gtk_box_pack_end(GTK_BOX(hbox), tmp, FALSE, FALSE, 0);
    g_signal_connect(G_OBJECT(tmp), "clicked", G_CALLBACK (cb_close), NULL);
    gtk_widget_show(tmp);
}
