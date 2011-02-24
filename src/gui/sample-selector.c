#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <gtk/gtk.h>
#include "gui.h"
#include "petri-foo.h"
#include "mixer.h"
#include "patch_set_and_get.h"


static int patch;
static char *last_file = 0;


static void cb_load(GtkWidget* dialog)
{
    GtkWidget *msg;
     int val;
     char *curname = patch_get_sample_name (patch);
     char *name = (char *)
            gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog));

     /* don't load the sample if it's already loaded */
    if (strcmp(name, curname) == 0)
    {
        free (curname);
        return;
    }

    if ((val = patch_sample_load(patch, name)) < 0)
    {
        errmsg ("Failed to load sample %s for patch %d (%s)\n", name,
                                            patch, patch_strerror (val));

        msg = gtk_message_dialog_new(GTK_WINDOW(dialog),
                                    GTK_DIALOG_DESTROY_WITH_PARENT,
                                    GTK_MESSAGE_ERROR, GTK_BUTTONS_CLOSE,
                                    "Failed to load sample %s", name);

        gtk_dialog_run (GTK_DIALOG (msg));
        gtk_widget_destroy (msg);
    }
    else
    {
        debug ("Successfully loaded sample %s\n", name);
    }

    free (curname);
}


static void cb_preview(GtkWidget* dialog)
{
    char *name = (char *)
        gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog));
    mixer_preview (name);
    return;
}


static void cancel(void)
{
    char *cur_file;

    if (!last_file)
        return;

    if (strcmp(last_file, "\0") == 0)
    {   /* FIXME: I don't get the logic of this... */
        patch_sample_unload(patch);
    }
    else
    {   /* set patch to old sample if another one is currently loaded */
        cur_file = patch_get_sample_name (patch);

        if (strcmp (last_file, cur_file) != 0)
            patch_sample_load(patch, last_file);

        free(cur_file);
    }

    return;
}


int sample_selector_show(int id, GtkWidget* parent_window)
{
    GtkWidget* dialog;
    GtkWidget* load;
    GtkWidget* preview;

    patch = id;

    dialog = gtk_file_chooser_dialog_new("Load Sample",
                                          GTK_WINDOW(parent_window),
                                          GTK_FILE_CHOOSER_ACTION_OPEN,
                                          GTK_STOCK_CANCEL,
                                          GTK_RESPONSE_CANCEL,
                                          GTK_STOCK_OPEN,
                                          GTK_RESPONSE_ACCEPT, NULL);

    free(last_file);
    last_file = patch_get_sample_name(patch);
    gtk_file_chooser_set_filename(GTK_FILE_CHOOSER(dialog), last_file);

    g_signal_connect_swapped(G_OBJECT(dialog), "file-activated",
                                G_CALLBACK(cb_load), dialog);

   /* FIXME: g_signal_connect(G_OBJECT(GTK_DIALOG(dialog)->action_area->cancel_button),
                                "clicked", G_CALLBACK(cb_cancel), dialog);
*/

    load = gtk_dialog_add_button(GTK_DIALOG(dialog), "Load", 0);
    g_signal_connect_swapped(G_OBJECT(load), "clicked",
                            G_CALLBACK(cb_load), G_OBJECT(dialog));

    preview = gtk_dialog_add_button(GTK_DIALOG(dialog), "Pre_view", 0);
    g_signal_connect_swapped(G_OBJECT(preview), "clicked",
                            G_CALLBACK(cb_preview), G_OBJECT(dialog));
    while (1)
    {
        GtkResponseType response = gtk_dialog_run(GTK_DIALOG(dialog));

        if (response != 0)
        {
            gtk_widget_hide(dialog);
            break;
        }
    }

    free(last_file);
    last_file = 0;

     return 0;
}
