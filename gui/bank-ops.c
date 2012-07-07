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

#include <assert.h>
#include <stdlib.h>
#include <string.h>

#include "dish_file.h"
#include "global_settings.h"
#include "gui.h"
#include "patch.h"
#include "patch_util.h"
#include "petri-foo.h"
#include "msg_log.h"
#include "session.h"


static char *last_bank = 0;

static const char* untitled_bank = "Untitled bank";
char* bankname = 0;


const char* bank_ops_bank(void)
{
    if (session_get_type() == SESSION_TYPE_NONE)
        return (bankname) ? bankname : untitled_bank;

    return session_get_bank();
}


static void set_bankname(const char* name)
{
    if (bankname)
        free(bankname);

    bankname = (name) ? strdup(name) : NULL;
}


static void file_chooser_add_filter(GtkWidget* chooser, const char* name,
                                                 const char* pattern)
{
    GtkFileFilter* filter = gtk_file_filter_new();
    gtk_file_filter_add_pattern(filter, pattern);
    gtk_file_filter_set_name(filter, name);
    gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(chooser), filter);
}


static int save_as(GtkWidget* parent_window, gboolean not_export)
{
    GtkWidget *dialog;
    int val;
    const char* title;
    char* filter = strconcat("*", dish_file_extension());
    char* untitled_dish = strconcat("untitled", dish_file_extension());
    
    global_settings* settings = settings_get();

    if (not_export)
        title = "Save bank as";
    else
        title = "Export bank from session as";

    dialog = gtk_file_chooser_dialog_new(title,
                                    GTK_WINDOW(parent_window),
                                    GTK_FILE_CHOOSER_ACTION_SAVE,
                                    GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
                                    GTK_STOCK_SAVE_AS, GTK_RESPONSE_ACCEPT,
                                    NULL);

    gtk_file_chooser_set_do_overwrite_confirmation(
                                    GTK_FILE_CHOOSER(dialog), TRUE);

    if (last_bank == 0)
        gtk_file_chooser_set_current_folder(GTK_FILE_CHOOSER(dialog),
                                            settings->last_bank_dir);
    else
        gtk_file_chooser_set_current_folder(GTK_FILE_CHOOSER(dialog),
                                            g_path_get_dirname(last_bank));

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
            msg_log(MSG_ERROR, "Failed to write file %s\n", name);
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
            if (recent_manager)
                gtk_recent_manager_add_item (recent_manager, 
                    g_filename_to_uri(name, NULL, NULL));

            msg_log(MSG_MESSAGE, "Successfully wrote file %s\n", name);

            if (not_export)
            {
                free(last_bank);
                last_bank = strdup(name);
            }

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


static int open(GtkWidget* parent_window, gboolean not_import)
{
    GtkWidget* dialog;
    int val;
    const char* title;
    char* filter = strconcat("*", dish_file_extension());
    global_settings* settings = settings_get();

    if (not_import)
        title = "Open bank";
    else
        title = "Import bank";

    dialog = gtk_file_chooser_dialog_new( title,
                                          GTK_WINDOW(parent_window),
                                          GTK_FILE_CHOOSER_ACTION_OPEN,
                                          GTK_STOCK_CANCEL,
                                          GTK_RESPONSE_CANCEL,
                                          GTK_STOCK_OPEN,
                                          GTK_RESPONSE_ACCEPT, NULL);
    if (last_bank)
        gtk_file_chooser_select_filename(GTK_FILE_CHOOSER(dialog),
                                                        last_bank);
    else
        gtk_file_chooser_set_current_folder(GTK_FILE_CHOOSER(dialog),
                                           settings->last_bank_dir);

    file_chooser_add_filter(dialog, "Petri-Foo files", filter);
    file_chooser_add_filter(dialog, "All files", "*");

    if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT)
    {
        char* name = (char*) gtk_file_chooser_get_filename(
                                        GTK_FILE_CHOOSER(dialog));

        msg_log(MSG_MESSAGE, "Loading bank %s\n", name);
        msg_log_reset_notification_state();
        val = dish_file_read(name);

        if (val < 0)
        {
            msg_log(MSG_ERROR, "Failed to read bank %s\n", name);
            GtkWidget* msg = gtk_message_dialog_new(GTK_WINDOW(dialog),
                                    GTK_DIALOG_MODAL,
                                    GTK_MESSAGE_ERROR,
                                    GTK_BUTTONS_CLOSE,
                                    "Failed to read bank %s\n.", name);

            g_signal_connect_swapped(G_OBJECT(msg), "response",
                                    G_CALLBACK(gtk_widget_destroy), msg);
            gtk_widget_show (msg);
        }
        else
        {
            if (msg_log_get_notification_state())
            {
                msg_log(MSG_WARNING, "Bank %s read with errors\n", name);
            }
            else
                msg_log(MSG_MESSAGE, "Successfully read bank %s\n", name);

            if (recent_manager)
                gtk_recent_manager_add_item(recent_manager,
                                    g_filename_to_uri(name, NULL, NULL));
            free(last_bank);
            last_bank = strdup(name);

            if (settings->last_bank_dir)
                free(settings->last_bank_dir);

            settings->last_bank_dir = g_path_get_dirname(name);
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


int bank_ops_open(GtkWidget* parent_window)
{
    assert(session_get_type() == SESSION_TYPE_NONE);
    patch_destroy_all();
    return open(parent_window, TRUE);
}


int bank_ops_import(GtkWidget* parent_window)
{
    return open(parent_window, FALSE);
}


int bank_ops_open_recent(GtkWidget* parent_window, char* filename)
{
    int val;
    assert(session_get_type() == SESSION_TYPE_NONE);

    msg_log(MSG_MESSAGE, "Loading bank %s\n", filename);
    msg_log_reset_notification_state();

    patch_destroy_all();

    val = dish_file_read(filename);

    if (val < 0)
    {
         msg_log(MSG_ERROR, "Failed to read bank %s\n", filename);
         GtkWidget* msg = gtk_message_dialog_new(GTK_WINDOW(parent_window),
                                   GTK_DIALOG_MODAL,
                                   GTK_MESSAGE_ERROR,
                                   GTK_BUTTONS_CLOSE,
                                   "Failed to read bank %s\n.", filename);

         g_signal_connect_swapped(G_OBJECT(msg), "response",
                                   G_CALLBACK(gtk_widget_destroy), msg);
         gtk_widget_show (msg);
    }
    else
    {
        if (msg_log_get_notification_state())
        {
            msg_log(MSG_WARNING, "Bank %s read with errors\n", filename);
        }
        else
            msg_log(MSG_MESSAGE, "Successfully read bank %s\n", filename);

        gtk_recent_manager_add_item (recent_manager, 
             g_filename_to_uri(filename, NULL, NULL));
        free(last_bank);
        last_bank = strdup(filename);
        set_bankname(filename);
    }

    return val;
}


int bank_ops_save_as(GtkWidget* parent_window)
{
    assert(session_get_type() == SESSION_TYPE_NONE);
    return save_as(parent_window, TRUE);
}


int bank_ops_save(GtkWidget* parent_window)
{
    if (session_get_type() == SESSION_TYPE_NONE)
    {
        return (last_bank != 0)
                    ? dish_file_write(last_bank)
                    : save_as(parent_window, TRUE);
    }

    return dish_file_write(session_get_bank());
}


int bank_ops_export(GtkWidget* parent_window)
{
    return save_as(parent_window, FALSE);
}


void bank_ops_force_name(const char* name)
{
    set_bankname(name);
    last_bank = strdup(name);
}
