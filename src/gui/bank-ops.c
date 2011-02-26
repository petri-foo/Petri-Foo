#include "bank-ops.h"

#include <stdlib.h>
#include <string.h>

#include "beef.h"
#include "patch.h"
#include "petri-foo.h"

static char *last_bank = 0;


static void save_bank_as_verify(GtkWidget* dialog)
{
    GtkWidget *msg;
    int val;
    char *name = (char *)
            gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog));

    if ((val = beef_write (name)) < 0)
    {
        errmsg ("Failed to write file %s\n", name);
        msg = gtk_message_dialog_new(GTK_WINDOW(dialog), GTK_DIALOG_MODAL,
                                    GTK_MESSAGE_ERROR, GTK_BUTTONS_CLOSE,
                                    "Failed to write file %s\n.", name);
        g_signal_connect_swapped(G_OBJECT(msg), "response",
                                    G_CALLBACK(gtk_widget_destroy), msg);
        gtk_widget_show (msg);
    }
    else
    {
        debug ("Succesfully wrote file %s\n", name);
        free(last_bank);
        last_bank = strdup(name);
        gtk_dialog_response(GTK_DIALOG(dialog), GTK_RESPONSE_ACCEPT);
     }
}


static void open_bank_verify(GtkWidget * dialog)
{
    GtkWidget *msg;
    int val;
    char *name = (char *)
            gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog));

    if ((val = beef_read (name)) < 0)
    {
        errmsg ("Failed to open file %s\n", name);
        msg = gtk_message_dialog_new(GTK_WINDOW(dialog), GTK_DIALOG_MODAL,
                                    GTK_MESSAGE_ERROR, GTK_BUTTONS_CLOSE,
                                    "Failed to open file %s.", name);
        g_signal_connect_swapped(G_OBJECT(msg), "response",
                                    G_CALLBACK (gtk_widget_destroy), msg);
        gtk_widget_show (msg);
    }
    else
    {
        debug ("Succesfully opened file %s\n", name);
        free(last_bank);
        last_bank = strdup(name);
        gtk_dialog_response(GTK_DIALOG(dialog), GTK_RESPONSE_ACCEPT);
     }
}


int bank_ops_save_as (GtkWidget* parent_window)
{
    GtkWidget *dialog;
    int val;

    dialog = gtk_file_chooser_dialog_new("Save Bank",
                                    GTK_WINDOW(parent_window),
                                    GTK_FILE_CHOOSER_ACTION_SAVE,
                                    GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
                                    GTK_STOCK_OPEN, GTK_RESPONSE_ACCEPT,
                                    NULL);

    gtk_file_chooser_set_do_overwrite_confirmation(
                                    GTK_FILE_CHOOSER(dialog), TRUE);

    gtk_file_chooser_set_current_name(GTK_FILE_CHOOSER(dialog),
                    (last_bank != 0) ? last_bank : "Untitled bank");

    g_signal_connect_swapped(G_OBJECT(dialog),
                             "file-activated",
                             G_CALLBACK(save_bank_as_verify),
                             dialog);

    if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT)
        val = 0;
    else
        val = -1;

     gtk_widget_destroy(dialog);

     return val;
}


int bank_ops_save(GtkWidget* parent_window)
{
     int val;

     if (!last_bank)
        val = bank_ops_save_as(parent_window);
     else
        val = beef_write(last_bank);

     return val;
}


int bank_ops_open(GtkWidget* parent_window)
{
    GtkWidget* dialog;
    int val;

    dialog = gtk_file_chooser_dialog_new("Open Bank",
                                          GTK_WINDOW(parent_window),
                                          GTK_FILE_CHOOSER_ACTION_OPEN,
                                          GTK_STOCK_CANCEL,
                                          GTK_RESPONSE_CANCEL,
                                          GTK_STOCK_OPEN,
                                          GTK_RESPONSE_ACCEPT, NULL);

    gtk_file_chooser_set_filename(GTK_FILE_CHOOSER(dialog), last_bank);

    g_signal_connect_swapped(G_OBJECT(dialog),
                             "file-activated",
                             G_CALLBACK(open_bank_verify),
                             dialog);

    if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT)
        val = 0;
    else
        val = -1;

    gtk_widget_destroy(dialog);

    return val;
}


int bank_ops_new(void)
{
    patch_destroy_all();
    free(last_bank);
    last_bank = 0;
    return 0;
}
