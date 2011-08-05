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


#include "bank-ops.h"

#include <stdlib.h>
#include <string.h>

#include "dish_file.h"
#include "patch.h"
#include "patch_util.h"
#include "petri-foo.h"
#include "gui.h"


static char *last_bank = 0;

static const char* untitled_bank = "Untitled bank";
char* bankname = 0;


const char* bank_ops_bank(void)
{
    return (bankname) ? bankname : untitled_bank;
}


static void set_bankname(const char* name)
{
    if (bankname)
        free(bankname);

    bankname = (name) ? strdup(name) : NULL;
    gui_set_window_title(bankname);
}


inline static char* strconcat(const char* str1, const char* str2)
{
    char* str = malloc(strlen(str1) + strlen(str2) + 1);
    strcpy(str, str1);
    strcat(str, str2);
    return str;
}

/* unused... reason/purpose ???
static char* get_file_filter(void)
{
    char* filter = 0;

    if (!filter)
    {
        const char* ext = dish_file_extension();
        size_t l = strlen(ext) + 1;
        filter = malloc(l + 1);
        filter[0] = '*';
        strcat(filter, ext);
    }

    return filter;
}
*/

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

            gtk_recent_manager_add_item (recent_manager, 
                g_filename_to_uri(name, NULL, NULL));
            debug ("Succesfully wrote file %s\n", name);
            free(last_bank);
            last_bank = strdup(name);
            set_bankname(name);
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
        char* name = (char*) gtk_file_chooser_get_filename(
                   GTK_FILE_CHOOSER(dialog));
        
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
            gtk_recent_manager_add_item (recent_manager, 
                 g_filename_to_uri(name, NULL, NULL));
            free(last_bank);
            last_bank = strdup(name);
            set_bankname(name);
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
    last_bank = NULL;
    set_bankname(NULL);
    return 0;
}

int bank_ops_open_recent(GtkWidget* parent_window, char* filename)
{
    int val; 

    if ((val = dish_file_read(filename)) < 0)
    {
         errmsg("Failed to read file %s\n", filename);
         GtkWidget* msg = gtk_message_dialog_new(GTK_WINDOW(parent_window),
                                   GTK_DIALOG_MODAL,
                                   GTK_MESSAGE_ERROR,
                                   GTK_BUTTONS_CLOSE,
                                   "Failed to read file %s\n.", filename);

         g_signal_connect_swapped(G_OBJECT(msg), "response",
                                   G_CALLBACK(gtk_widget_destroy), msg);
         gtk_widget_show (msg);
    }
    else
    {
        debug ("Succesfully read file %s\n", filename);
        gtk_recent_manager_add_item (recent_manager, 
             g_filename_to_uri(filename, NULL, NULL));
        free(last_bank);
        last_bank = strdup(filename);
        set_bankname(filename);
    }

    return val;
}

