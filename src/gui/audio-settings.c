#include <gtk/gtk.h>

#include "audio-settings.h"
#include "gui.h"
#include "petri-foo.h"
#include "jackdriver.h"
#include "driver.h"
#include "sync.h"


#include <string.h>


static GtkWidget* window;


#ifdef HAVE_JACK_SESSION
static int gui_session_cb(void *data)
{
    jack_session_event_t *ev = (jack_session_event_t *)data;
    char filename[256];
    char command[256];

    snprintf(filename,  sizeof(filename), "%sbank.beef", ev->session_dir);
    snprintf(command,   sizeof(command),
                        "petri-foo -U %s ${SESSION_DIR}myfile.state",
                        ev->client_uuid                                 );

    /* FIXME: implement!
    your_save_function( filename );
     */

    ev->command_line = strdup(command);
    jack_session_reply(jackdriver_get_client(), ev);

    if (ev->type == JackSessionSaveAndQuit)
         gtk_main_quit();

    jack_session_event_free(ev);

    return 0;
}

void audio_settings_session_cb(jack_session_event_t *event, void *arg )
{
    g_idle_add(gui_session_cb, event);
}
#endif


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
