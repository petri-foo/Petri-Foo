#include "bank-ops.h"

#include <stdlib.h>
#include <string.h>

#include "dish_file.h"
#include "patch.h"
#include "patch_util.h"
#include "petri-foo.h"

static char *last_bank = 0;


inline static char* strconcat(const char* str1, const char* str2)
{
    char* str = malloc(strlen(str1) + strlen(str2) + 1);
    strcpy(str, str1);
    strcat(str, str2);
    return str;
}


static char* get_file_filter(void)
{
    char* filter = 0;

    if (!filter)
    {
        const char* ext = dish_file_extension();
        size_t l = strlen(ext) + 1; /* additional char to store * */
        filter = malloc(l + 1);
        filter[0] = '*';
        strcat(filter, ext);
    }

    return filter;
}


static void file_chooser_add_filter(GtkWidget* chooser, const char* name,
                                                 const char* pattern)
{
    GtkFileFilter* filter = gtk_file_filter_new();
    gtk_file_filter_add_pattern(filter, pattern);
    gtk_file_filter_set_name(filter, name);
    gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(chooser), filter);
}


int bank_ops_save_as (GtkWidget* parent_window)
{
    GtkWidget *dialog;
    int val;

    char* filter = strconcat("*", dish_file_extension());
    char* untitled_dish = strconcat("untitled", dish_file_extension());

    dialog = gtk_file_chooser_dialog_new("Save Bank",
                                    GTK_WINDOW(parent_window),
                                    GTK_FILE_CHOOSER_ACTION_SAVE,
                                    GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
                                    GTK_STOCK_SAVE_AS, GTK_RESPONSE_ACCEPT,
                                    NULL);

    gtk_file_chooser_set_do_overwrite_confirmation(
                                    GTK_FILE_CHOOSER(dialog), TRUE);

    gtk_file_chooser_set_current_name(GTK_FILE_CHOOSER(dialog),
                    (last_bank != 0) ? last_bank : untitled_dish);

    file_chooser_add_filter(dialog, "Petri-Foo files", filter);
    file_chooser_add_filter(dialog, "All files", "*");

    if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT)
    {
        char *name = (char *)
            gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog));

        if ((val = dish_file_write(name)) < 0)
        {
            errmsg ("Failed to write file %s\n", name);
            GtkWidget* msg = gtk_message_dialog_new(GTK_WINDOW(dialog),
                                    GTK_DIALOG_MODAL,
                                    GTK_MESSAGE_ERROR,
                                    GTK_BUTTONS_CLOSE,
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
        }
    }
    else
    {
        val = -1;
    }

    gtk_widget_destroy(dialog);

    free(untitled_dish);
    free(filter);

    return val;
}


int bank_ops_save(GtkWidget* parent_window)
{
     int val;

     if (!last_bank)
        val = bank_ops_save_as(parent_window);
     else
        val = dish_file_write(last_bank);

     return val;
}


int bank_ops_open(GtkWidget* parent_window)
{
    GtkWidget* dialog;
    int val;
    char* filter = strconcat("*", dish_file_extension());

    dialog = gtk_file_chooser_dialog_new("Open Bank",
                                          GTK_WINDOW(parent_window),
                                          GTK_FILE_CHOOSER_ACTION_OPEN,
                                          GTK_STOCK_CANCEL,
                                          GTK_RESPONSE_CANCEL,
                                          GTK_STOCK_OPEN,
                                          GTK_RESPONSE_ACCEPT, NULL);
    if (last_bank)
        gtk_file_chooser_select_filename(GTK_FILE_CHOOSER(dialog),
                                                        last_bank);

    file_chooser_add_filter(dialog, "Petri-Foo files", filter);
    file_chooser_add_filter(dialog, "All files", "*");

    if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT)
    {
        char *name = (char *)
            gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog));

        if ((val = dish_file_read(name)) < 0)
        {
            errmsg("Failed to read file %s\n", name);
            GtkWidget* msg = gtk_message_dialog_new(GTK_WINDOW(dialog),
                                    GTK_DIALOG_MODAL,
                                    GTK_MESSAGE_ERROR,
                                    GTK_BUTTONS_CLOSE,
                                    "Failed to read file %s\n.", name);

            g_signal_connect_swapped(G_OBJECT(msg), "response",
                                    G_CALLBACK(gtk_widget_destroy), msg);
            gtk_widget_show (msg);
        }
        else
        {
            debug ("Succesfully read file %s\n", name);
            free(last_bank);
            last_bank = strdup(name);
        }
    }
    else
    {
        val = -1;
    }

    gtk_widget_destroy(dialog);

    free(filter);

    return val;
}


int bank_ops_new(void)
{
    patch_destroy_all();
    free(last_bank);
    last_bank = 0;
    return 0;
}
