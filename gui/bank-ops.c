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
#include "file_ops.h"
#include "global_settings.h"
#include "gui.h"
#include "patch.h"
#include "patch_util.h"
#include "petri-foo.h"
#include "msg_log.h"
#include "session.h"


static const char* untitled_bank = "Untitled bank";


static void file_chooser_add_filter(GtkWidget* chooser, const char* name,
                                                 const char* pattern)
{
    GtkFileFilter* filter = gtk_file_filter_new();
    gtk_file_filter_add_pattern(filter, pattern);
    gtk_file_filter_set_name(filter, name);
    gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(chooser), filter);
}


static int basic_save_as(GtkWidget* parent_window, gboolean not_export)
{
    GtkWidget *dialog;
    int val;
    const char* title;
    char* filter = strconcat("*", dish_file_extension());
    char* untitled_dish = strconcat("untitled", dish_file_extension());

    global_settings* settings = settings_get();

    if (not_export)
        title = "Basic Save bank as";
    else
        title = "Basic Export bank from session as";

    dialog = gtk_file_chooser_dialog_new(title,
                                    GTK_WINDOW(parent_window),
                                    GTK_FILE_CHOOSER_ACTION_SAVE,
                                    GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
                                    GTK_STOCK_SAVE_AS, GTK_RESPONSE_ACCEPT,
                                    NULL);

    gtk_file_chooser_set_do_overwrite_confirmation(
                                    GTK_FILE_CHOOSER(dialog), TRUE);

    if (!dish_file_has_state())
    {
        gtk_file_chooser_set_current_folder(GTK_FILE_CHOOSER(dialog),
                                            settings->last_bank_dir);
        gtk_file_chooser_set_current_name(GTK_FILE_CHOOSER(dialog),
                                            untitled_dish);
    }
    else
    {
        const char* tmp = 0;
        char* fn;

        if (dish_file_state_is_full())
            tmp = dish_file_state_parent_dir();
        else
            tmp = dish_file_state_bank_dir();

        debug("tmp:     '%s'\n", tmp);
        debug("parent:  '%s'\n", dish_file_state_parent_dir());
        debug("bank:    '%s'\n", dish_file_state_bank_dir());

        gtk_file_chooser_set_current_folder(GTK_FILE_CHOOSER(dialog), tmp);
        fn = file_ops_join_ext( dish_file_state_bank_name(),
                                dish_file_extension());

        gtk_file_chooser_set_current_name(GTK_FILE_CHOOSER(dialog), fn);
        free(fn);
    }

    file_chooser_add_filter(dialog, "Petri-Foo files", filter);
    file_chooser_add_filter(dialog, "All files", "*");

    if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT)
    {
        char *name = (char *)
            gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog));

        if ((val = dish_file_write_basic(name)) < 0)
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
            if (recent_manager && not_export)
                gtk_recent_manager_add_item (recent_manager, 
                    g_filename_to_uri(name, NULL, NULL));

            msg_log(MSG_MESSAGE, "Successfully wrote file %s\n", name);
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


void name_buf_txt_ins_cb(GtkEntryBuffer* buf,   guint pos, gchar* chars,
                                                guint count,
                                                gpointer user_data)
{
}


static int full_save_as(GtkWidget* parent_window, gboolean not_export)
{
    GtkWidget* dialog = 0;
    GtkWidget* action_area = 0;
    GtkWidget* content_area = 0;
    GtkWidget* alignment = 0;
    GtkWidget* vbox = 0;
    GtkWidget* table = 0;
    GtkTable* t = 0;
    GtkWidget* name_entry = 0;
    GtkWidget* folder_button = 0;
    GtkWidget* w = 0;

    enum { TABLE_WIDTH = 4, TABLE_HEIGHT = 2 };

    int a1 = 0, a2 = 1;
    int b1 = 1, b2 = TABLE_WIDTH;
    int y = 0;

    const char* title = 0;
    char* folder = 0;

    global_settings* settings = settings_get();

    if (not_export)
        title = "Full Save bank as";
    else
        title = "Full Export bank from session as";

    dialog = gtk_dialog_new_with_buttons(title, GTK_WINDOW(parent_window),
                        GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
                        GTK_STOCK_OK,       GTK_RESPONSE_ACCEPT,
                        GTK_STOCK_CANCEL,   GTK_RESPONSE_REJECT,    NULL);

    content_area = gtk_dialog_get_content_area(GTK_DIALOG(dialog));
    alignment = gtk_alignment_new(0, 0, 1, 1);
    gtk_alignment_set_padding(GTK_ALIGNMENT(alignment), GUI_BORDERSPACE,
                                                        GUI_BORDERSPACE,
                                                        GUI_BORDERSPACE,
                                                        GUI_BORDERSPACE);
    gtk_container_add(GTK_CONTAINER(content_area), alignment);
    vbox = gtk_vbox_new(FALSE, GUI_SPACING);
    gtk_container_add(GTK_CONTAINER(alignment), vbox);

    table = gtk_table_new(TABLE_HEIGHT, TABLE_WIDTH, TRUE);
    gui_pack(GTK_BOX(vbox), table);
    t = GTK_TABLE(table);
    gtk_table_set_col_spacing(t, 0, GUI_TEXTSPACE);
    gui_label_attach("Name:", t, a1, a2, y, y + 1);
    name_entry = gtk_entry_new();
    gui_attach(t, name_entry, b1, b2, y, y + 1);

    g_signal_connect(G_OBJECT(gtk_entry_get_buffer(GTK_ENTRY(name_entry))),
        "inserted-text", G_CALLBACK(name_buf_txt_ins_cb), NULL);

    ++y;
    gui_label_attach("Create Folder in:", t, a1, a2, y, y + 1);
    folder_button = gtk_file_chooser_button_new("Select folder",
                                GTK_FILE_CHOOSER_ACTION_SELECT_FOLDER);
    gui_attach(t, folder_button, b1, b2, y, y + 1);

    gtk_widget_show_all(dialog);

    while(1)
    {
        if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT)
        {
            char* uri = 0;
            const char* name = 0;

            uri = gtk_file_chooser_get_uri(GTK_FILE_CHOOSER(folder_button));

            if (uri && strncmp(uri, "file://", 7) == 0)
            {
                folder = strdup(uri + 7);
                free(uri);
            }
            else
                folder = uri;

            name = gtk_entry_get_text(GTK_ENTRY(name_entry));
            debug("folder:'%s'\tname (name_entry):'%s'\n", folder,name);

            if (folder && name)
            {
                dish_file_write_full(folder, name);

                if (recent_manager)
                    gtk_recent_manager_add_item(recent_manager,
                                    g_filename_to_uri(name, NULL, NULL));
                break;
            }
        }
        else
            break;
    }

    gtk_widget_destroy(dialog);

    return 0;
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

    if (dish_file_has_state())
    {
        const char* tmp = 0;
        tmp = dish_file_state_is_full() ? dish_file_state_parent_dir()
                                        : dish_file_state_bank_dir();
        gtk_file_chooser_select_filename(GTK_FILE_CHOOSER(dialog), tmp);
    }
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

        val = (not_import)  ? dish_file_read(name)
                            : dish_file_import(name);
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

            if (recent_manager && not_import)
                gtk_recent_manager_add_item(recent_manager,
                                    g_filename_to_uri(name, NULL, NULL));

            if (settings->last_bank_dir)
                free(settings->last_bank_dir);

            settings->last_bank_dir = g_path_get_dirname(name);
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
    return 0;
}


int bank_ops_open(GtkWidget* parent_window)
{
    assert(session_get_type() == SESSION_TYPE_NONE);
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

        if (recent_manager)
            gtk_recent_manager_add_item (recent_manager, 
                            g_filename_to_uri(filename, NULL, NULL));
    }

    return val;
}


int bank_ops_basic_save_as(GtkWidget* parent_window)
{
    assert(session_get_type() == SESSION_TYPE_NONE);
    return basic_save_as(parent_window, TRUE);
}

int bank_ops_full_save_as(GtkWidget* parent_window)
{
    assert(session_get_type() == SESSION_TYPE_NONE);
    return full_save_as(parent_window, TRUE);
}


int bank_ops_save(GtkWidget* parent_window)
{
    if (dish_file_has_state())
        return dish_file_write();

    return bank_ops_basic_save_as(parent_window);
}


int bank_ops_export(GtkWidget* parent_window)
{
    return basic_save_as(parent_window, FALSE);
}

