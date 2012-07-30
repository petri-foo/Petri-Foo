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


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <gtk/gtk.h>

#include "instance.h"
#include "petri-foo.h"
#include "pf_error.h"
#include "phin.h"

#include "audio-settings.h"
#include "bank-ops.h"
#include "channelsection.h"
#include "config.h"
#include "dish_file.h"
#include "driver.h"
#include "global_settings.h"
#include "gui.h"
#include "log_display.h"
#include "mastersection.h"
#include "midisection.h"
#include "mixer.h"
#include "mod_src_gui.h"
#include "msg_log.h"
#include "patchlist.h"
#include "patchsection.h"
#include "patch_set_and_get.h"
#include "patch_util.h"
#include "sample-editor.h"
#include "sample-selector.h"
#include "session.h"
#include "waveform.h"


/* windows */
static GtkWidget* window = 0;
static GtkWidget* patch_section = 0;
static GtkWidget* master_section = 0;
static GtkWidget* midi_section = 0;
static GtkWidget* channel_section = 0;
static GtkWidget* patch_list = 0;


/* main menu */
static GtkWidget* menubar = 0;
static GtkWidget* menu_file = 0;
static GtkWidget* menu_settings = 0;
static GtkWidget* menu_patch = 0;
static GtkWidget* menu_view = 0;
static GtkWidget* menu_help = 0;

/* file menu */
static GtkWidget* menu_file_new = 0;
static GtkWidget* menu_file_open = 0;
static GtkWidget* menu_file_import = 0;
static GtkWidget* menu_file_recent = 0;
static GtkWidget* menu_file_save = 0;
static GtkWidget* menu_file_basic_save_as = 0;
static GtkWidget* menu_file_full_save_as = 0;
static GtkWidget* menu_file_export = 0;

/* settings */
static GtkWidget* menu_settings_auto_preview = 0;

/* view */
static GtkWidget* menu_view_log_display = 0;

/* current patch, makes passing patch id to sample editor easier */
static int cur_patch = -1;


GtkWidget* gui_title_new(const char* msg)
{
    GtkWidget* label;
    char* s;

    label = gtk_label_new(NULL);
    s = g_strdup_printf("<b>%s</b>", msg);
    gtk_label_set_markup(GTK_LABEL(label), s);
    gtk_misc_set_alignment(GTK_MISC(label), 0.0, 0.5);

    g_free(s);
    return label;
}


GtkWidget* gui_hpad_new(int size)
{
    GtkWidget* pad;

    pad = gtk_alignment_new(0, 0, 1, 1);
    gtk_widget_set_size_request(pad, size, 0);

    return pad;
}


GtkWidget* gui_vpad_new(int size)
{
    GtkWidget* pad;

    pad = gtk_alignment_new(0, 0, 1, 1);
    gtk_widget_set_size_request(pad, 0, size);

    return pad;
}


GtkWidget* gui_section_new(const char* name, GtkWidget** box)
{
    GtkWidget* vbox;
    GtkWidget* title;
    GtkWidget* hbox;
    GtkWidget* pad;

    /* vbox */
    vbox = gtk_vbox_new(FALSE, 0);

    /* title */
    title = gui_title_new(name);
    gtk_box_pack_start(GTK_BOX(vbox), title, TRUE, TRUE, 0);
    gtk_widget_show(title);

    /* pad */
    pad = gui_vpad_new(GUI_TITLESPACE);
    gtk_box_pack_start(GTK_BOX(vbox), pad, FALSE, FALSE, 0);
    gtk_widget_show(pad);

    /* hbox */
    hbox = gtk_hbox_new(FALSE, 0);
    gtk_box_pack_start(GTK_BOX(vbox), hbox, TRUE, TRUE, 0);
    gtk_widget_show(hbox);

    /* pad */
    pad = gui_hpad_new(GUI_INDENT);
    gtk_box_pack_start(GTK_BOX(hbox), pad, FALSE, FALSE, 0);
    gtk_widget_show(pad);

    *box = hbox;
    return vbox;
}


void gui_attach(GtkTable* table, GtkWidget* widget, guint l, guint r,
                                                    guint t, guint b)
{
    gtk_table_attach(table, widget, l, r, t, b, GTK_FILL | GTK_EXPAND,
                                                GTK_SHRINK,
                                                0, 0);
    gtk_widget_show(widget);
}


GtkWidget*  gui_label_attach(const char* str, GtkTable* table,
                                              guint l, guint r,
                                              guint t, guint b)
{
    GtkWidget* label = gtk_label_new(str);
    gtk_misc_set_alignment(GTK_MISC(label), 1.0, 0.5);
    gui_attach(table, label, l, r, t, b);
    gtk_widget_show(label);
    return label;
}


void gui_pack(GtkBox* box , GtkWidget* widget)
{
    gtk_box_pack_start(box, widget, FALSE, FALSE, 0);
    gtk_widget_show(widget);
}


/* packs (and shows) label into box (returns label) */
GtkWidget*  gui_label_pack(const char* str, GtkBox* box)
{
    GtkWidget* label = gtk_label_new(str);
    gtk_misc_set_alignment(GTK_MISC(label), 1.0, 0.5);
    gui_pack(box, label);
    return label;
}



/* when the 'x' button is clicked on the titlebar */
static gboolean
cb_delete (GtkWidget* widget, GdkEvent* event, gpointer data)
{
    (void)widget;(void)event;(void)data;
    /* we return false, causing the "destroy" event to be
     * released on "widget" */
    return FALSE;
}


static void cb_quit (GtkWidget* widget, gpointer data)
{
    (void)widget;(void)data;
    debug ("Quitting\n");
    gtk_main_quit ( );
}


/* enables ok button if there is input in entry, disables otherwise */
static gboolean
cb_menu_patch_name_verify (GtkWidget * entry, GdkEventKey * event,
                                              GtkDialog * dialog)
{
    (void)event;(void)dialog;
    int val = strlen((char *)gtk_entry_get_text(GTK_ENTRY (entry)));

    if (val)
      gtk_dialog_set_response_sensitive(dialog, GTK_RESPONSE_ACCEPT, TRUE);
    else
      gtk_dialog_set_response_sensitive(dialog, GTK_RESPONSE_ACCEPT, FALSE);

    return FALSE; /* make sure normal event handlers grab signal */
}


void cb_menu_patch_add(GtkWidget* menu_item, gpointer data)
{
    (void)menu_item;(void)data;
    static int patch_no = 1;
    char buf[80];

    GtkWidget *dialog;
    GtkWidget *entry;
    GtkWidget*  content_area; /* formerly: dialog->vbox */

    /* dialog box */
    dialog = gtk_dialog_new_with_buttons("Add Patch",
                            GTK_WINDOW(window),
                            GTK_DIALOG_MODAL |
                            GTK_DIALOG_DESTROY_WITH_PARENT,
                            GTK_STOCK_OK,
                            GTK_RESPONSE_ACCEPT,
                            GTK_STOCK_CANCEL,
                            GTK_RESPONSE_REJECT, NULL);

    gtk_dialog_set_default_response(GTK_DIALOG (dialog),
                            GTK_RESPONSE_ACCEPT);

    /* create entry box */
    entry = gtk_entry_new();
    gtk_entry_set_max_length(GTK_ENTRY(entry), PATCH_MAX_NAME);
    snprintf(buf, 80, "Untitled Patch #%d", patch_no++);
    buf[79] = 0; /* paranoiac critical method */
    gtk_entry_set_text(GTK_ENTRY(entry), buf);
    gtk_entry_set_activates_default(GTK_ENTRY(entry), TRUE);

    g_signal_connect(G_OBJECT(entry), "key-press-event",
                            G_CALLBACK(cb_menu_patch_name_verify),
                            (gpointer) dialog);

    g_signal_connect(G_OBJECT(entry), "key-release-event",
                            G_CALLBACK(cb_menu_patch_name_verify),
                            (gpointer)dialog);

    content_area = gtk_dialog_get_content_area(GTK_DIALOG(dialog));
    gtk_box_pack_start(GTK_BOX(content_area), entry, FALSE, FALSE, 0);
    gtk_widget_show (entry);

    /* guaranteed to have a string in there if response is 'accept'
     * due to sensitivity callback */
    if (gtk_dialog_run (GTK_DIALOG (dialog)) == GTK_RESPONSE_ACCEPT)
    {
        int id = patch_create();
        const char* name = gtk_entry_get_text(GTK_ENTRY(entry));

        if (id < 0)
        {
            msg_log(MSG_ERROR, "Failed to create a new patch (%s).\n",
                                            pf_error_str(pf_error_get()));
            return;
        }

        patch_set_name(id, name);
        patch_set_channel(id, channel_section_get_channel(
                                    CHANNEL_SECTION(channel_section)));
        msg_log(MSG_MESSAGE, "created new patch '%s'\n", name);
        patch_list_update(PATCH_LIST(patch_list), id, PATCH_LIST_PATCH);
    }

    gtk_widget_destroy (dialog);
}


void cb_menu_patch_add_default(GtkWidget* menu_item, gpointer data)
{
    (void)menu_item;(void)data;
    int id = patch_create_default();

    if (id < 0)
    {
        msg_log(MSG_ERROR, "Failed to create a new patch (%s).\n",
                                            pf_error_str(pf_error_get()));
        return;
    }

    patch_set_channel(id,
            channel_section_get_channel(
                        CHANNEL_SECTION(channel_section)));
    msg_log(MSG_MESSAGE, "added Default patch\n");

    patch_list_update(PATCH_LIST(patch_list), id, PATCH_LIST_PATCH);
}


void cb_menu_patch_duplicate(GtkWidget* menu_item, gpointer data)
{
    (void)menu_item;(void)data;
    int id;
    int cp;

    if ((cp = patch_list_get_current_patch(PATCH_LIST(patch_list))) < 0)
    {   /* let's be a little more polite here shall we? */
        msg_log(MSG_WARNING, "No patch to duplicate\n");
        return;
    }

    if ((id = patch_duplicate(cp)) < 0)
    {
        msg_log(MSG_ERROR, "Failed to duplicate patch (%s).\n",
                                            pf_error_str(pf_error_get()));
        return;
    }

    msg_log(MSG_MESSAGE, "patch %d duplicated as %d '%s'\n", cp, id,
                                                    patch_get_name(id));

    patch_list_update(PATCH_LIST(patch_list), id, PATCH_LIST_PATCH);
}


void cb_menu_patch_rename(GtkWidget* menu_item, gpointer data)
{
    (void)menu_item;(void)data;

    GtkWidget *dialog;
    GtkWidget *entry;
    GtkWidget*  content_area; /* formerly: dialog->vbox */
    int val;
    int index;
    int cp;

    if ((cp = patch_list_get_current_patch(PATCH_LIST(patch_list))) < 0)
    {
        debug ("No patches to rename, infidel.\n");
        return;
    }

    /* dialog box */
    dialog = gtk_dialog_new_with_buttons("Rename Patch",
                            GTK_WINDOW(window),
                            GTK_DIALOG_MODAL |
                            GTK_DIALOG_DESTROY_WITH_PARENT,
                            GTK_STOCK_OK,
                            GTK_RESPONSE_ACCEPT,
                            GTK_STOCK_CANCEL,
                            GTK_RESPONSE_REJECT, NULL);

    gtk_dialog_set_default_response(GTK_DIALOG(dialog),
                                    GTK_RESPONSE_ACCEPT);

    /* create entry box */
    entry = gtk_entry_new();
    gtk_entry_set_max_length(GTK_ENTRY(entry), PATCH_MAX_NAME);
    gtk_entry_set_text(GTK_ENTRY(entry), patch_get_name (cp));
    gtk_entry_set_activates_default(GTK_ENTRY(entry), TRUE);

    g_signal_connect(G_OBJECT (entry), "key-press-event",
                        G_CALLBACK(cb_menu_patch_name_verify),
                        (gpointer)dialog);

    g_signal_connect(G_OBJECT(entry), "key-release-event",
                        G_CALLBACK(cb_menu_patch_name_verify),
                        (gpointer) dialog);

    content_area = gtk_dialog_get_content_area(GTK_DIALOG(dialog));
    gtk_box_pack_start(GTK_BOX(content_area), entry, FALSE, FALSE, 0);
    gtk_widget_show (entry);

    /* guaranteed to have a string in there if response is 'accept'
     * due to sensitivity callback */
    if (gtk_dialog_run (GTK_DIALOG (dialog)) == GTK_RESPONSE_ACCEPT)
    {
        char* oname = patch_get_name(cp);
        const char* name = gtk_entry_get_text(GTK_ENTRY(entry));
        val = patch_set_name(cp, name);

        if (val < 0)
        {
            msg_log(MSG_ERROR, "Failed to rename patch (%s).\n",
                                            pf_error_str(pf_error_get()));
            return;
        }

        msg_log(MSG_MESSAGE, "patch '%s' renamed as '%s'\n", oname, name);
        index = patch_list_get_current_index (PATCH_LIST(patch_list));
        patch_list_update (PATCH_LIST(patch_list), index, PATCH_LIST_INDEX);
    }

    gtk_widget_destroy (dialog);
}


void cb_menu_patch_remove(GtkWidget* menu_item, gpointer data)
{
    (void)menu_item;(void)data;

    int cp;
    int index;
    const char* name;

    if ((cp = patch_list_get_current_patch(PATCH_LIST(patch_list))) < 0)
    {
        debug ("No patches to remove\n");
        return;
    }

    name = patch_get_name(cp);
    index = patch_list_get_current_index (PATCH_LIST(patch_list));
    patch_destroy(cp);

    msg_log(MSG_MESSAGE, "destroyed patch '%s'\n", name);

    if (index == 0)
        patch_list_update(PATCH_LIST(patch_list), index,
                                                    PATCH_LIST_INDEX);
    else
        patch_list_update(PATCH_LIST(patch_list), index - 1,
                                                    PATCH_LIST_INDEX);
}


static void cb_menu_file_new_bank(GtkWidget * widget, gpointer data)
{
    (void)widget;(void)data;
    if (bank_ops_new() == 0)
        gui_refresh();
}

static void cb_menu_file_open_bank(GtkWidget * widget, gpointer data)
{
    (void)widget;(void)data;
    if (bank_ops_open(window) == 0)
        gui_refresh();
}

static void cb_menu_file_import(GtkWidget * widget, gpointer data)
{
    (void)widget;(void)data;
    if (bank_ops_import(window) == 0)
        gui_refresh();
}

static void cb_menu_file_save(GtkWidget * widget, gpointer data)
{
    (void)widget;(void)data;
    bank_ops_save(window);
}

static void cb_menu_file_basic_save_as(GtkWidget * widget, gpointer data)
{
    (void)widget;(void)data;
    bank_ops_basic_save_as(window);
}

static void cb_menu_file_full_save_as(GtkWidget * widget, gpointer data)
{
    (void)widget;(void)data;
    bank_ops_full_save_as(window);
}

static void cb_menu_file_export(GtkWidget * widget, gpointer data)
{
    (void)widget;(void)data;
    bank_ops_export(window);
}


static void cb_menu_settings_audio (GtkWidget * widget, gpointer data)
{
    (void)widget;(void)data;
    audio_settings_show();
}


static void cb_menu_settings_auto_preview(GtkWidget* widget, gpointer data)
{
    (void)widget;(void)data;
    global_settings* settings = settings_get();
    settings->sample_auto_preview = gtk_check_menu_item_get_active(
                        GTK_CHECK_MENU_ITEM(menu_settings_auto_preview));
}


void cb_menu_view_log_display_showing(gboolean active)
{
    gtk_check_menu_item_set_active(
                    GTK_CHECK_MENU_ITEM(menu_view_log_display), active);
}


static void cb_menu_view_log_display(GtkWidget* widget, gpointer data)
{
    (void)widget; (void)data;
    if (gtk_check_menu_item_get_active(
                GTK_CHECK_MENU_ITEM(menu_view_log_display)))
    {
        log_display_show();
    }
    else
        log_display_hide();
}






static void cb_menu_help_stfu (GtkWidget* widget, gpointer data)
{
    (void)widget;(void)data;
    mixer_flush();
}


static void cb_menu_help_about (GtkWidget* widget, gpointer data)
{
    (void)widget;
    GdkPixbuf* logo = 0;
    const char* authors[] = {   "Pete Bessman - original Specimen author",
                                "James Morris - Petri-Foo creator",
                                "See the AUTHORS file for others", 0 };

/*  should this be freed later on?  */
    logo = gdk_pixbuf_new_from_file(PIXMAPS_DIR "/petri-foo.png", NULL);
    gtk_show_about_dialog(
        GTK_WINDOW(data),
        "name", "Petri-Foo",
        "logo", logo,
        "authors", authors,
        "version", VERSION,
        "website", "http://petri-foo.sourceforge.net/",
        "copyright",    "(C) 2004-2005 Pete Bessman\n"
                        "(C) 2006-2007 others\n"
                        "(C) 2011-2012 James Morris\n",
        "comments",
        #if HAVE_LIBLO
        "Built with Non-Session-Manager support\n"
        #else
        "Built without Non-Session-Manager support\n"
        #endif
        #if HAVE_JACK_SESSION_H
        "Built with JACK Session Support",
        #else
        "Built without JACK Session Support",
        #endif
        NULL);
}


static void cb_patch_list_changed(PatchList* list, gpointer data)
{
    (void)data;
    cur_patch = patch_list_get_current_patch(list);

    if (cur_patch < 0)
        sample_editor_hide();
    else if (sample_editor_get_visible())
        sample_editor_show(cur_patch);

    patch_section_set_patch(PATCH_SECTION(patch_section), cur_patch);
    midi_section_set_patch(MIDI_SECTION(midi_section), cur_patch);
    channel_section_set_patch(CHANNEL_SECTION(channel_section), cur_patch);
}


static void cb_recent_chooser_item_activated (GtkRecentChooser *chooser
               , gpointer *data)
{
    (void)data;
    gchar *uri;
    gchar *filename;
    GtkRecentInfo *recentInfo;
    debug("recent-menu item-activated");
    uri = gtk_recent_chooser_get_current_uri (chooser);
    filename = g_filename_from_uri(uri, NULL, NULL);
    recentInfo = gtk_recent_chooser_get_current_item(chooser);
    gtk_recent_manager_add_item(recent_manager, uri);
    gtk_recent_info_unref(recentInfo);

    if (bank_ops_open_recent(window, (char*) filename) == 0)
    {
        gui_refresh();
    }

    g_free(uri);
    g_free(filename);
}


int gui_init(void)
{
    GtkWidget* window_vbox;
    GtkWidget* master_hbox;
    GtkWidget* vbox;
    global_settings* settings = settings_get();

    debug ("Initializing GUI\n");

    /* main window */
    window = gtk_window_new (GTK_WINDOW_TOPLEVEL);

    g_signal_connect(G_OBJECT(window), "delete-event",
                            G_CALLBACK (cb_delete), NULL);

    g_signal_connect(G_OBJECT(window), "destroy",
                            G_CALLBACK(cb_quit), NULL);

    /* setup the window's main vbox */
    window_vbox = gtk_vbox_new (FALSE, 0);
    gtk_container_add (GTK_CONTAINER (window), window_vbox);

    /* the menubar */
    menubar = gtk_menu_bar_new();
    gtk_box_pack_start(GTK_BOX(window_vbox), menubar, FALSE, FALSE, 0);

    menu_file = gui_menu_add(menubar, "File", NULL, NULL);

    menu_file_new = gui_menu_add(menu_file, "New",
            G_CALLBACK(cb_menu_file_new_bank),      window);
    menu_file_open = gui_menu_add(menu_file, "Open...",
            G_CALLBACK(cb_menu_file_open_bank),     window);
    menu_file_import = gui_menu_add(menu_file, "Import...",
            G_CALLBACK(cb_menu_file_import),        window);

    gtk_widget_set_tooltip_text(menu_file_new,
                                "New bank");
    gtk_widget_set_tooltip_text(menu_file_open,
                                "Open a saved bank");
    gtk_widget_set_tooltip_text(menu_file_import,
                                "Import a saved bank into current bank");

    menu_file_recent = gtk_menu_item_new_with_label("Open Recent");
    gtk_menu_shell_append(GTK_MENU_SHELL(menu_file), menu_file_recent);

    /* seperator */
    gui_menu_add(menu_file, NULL, NULL, NULL);

    menu_file_save = gui_menu_add(menu_file, "Save",
            G_CALLBACK(cb_menu_file_save),          window);

    menu_file_basic_save_as =
        gui_menu_add(menu_file, "Save As...",
            G_CALLBACK(cb_menu_file_basic_save_as),  window);

    menu_file_full_save_as =
        gui_menu_add(menu_file, "Full Save As...",
            G_CALLBACK(cb_menu_file_full_save_as),  window);

    menu_file_export = gui_menu_add(menu_file,"Export from session...",
            G_CALLBACK(cb_menu_file_export),       window);
    gtk_widget_set_sensitive(menu_file_export, FALSE);

    gtk_widget_set_tooltip_text(menu_file_save,
                                "Save bank");
    gtk_widget_set_tooltip_text(menu_file_basic_save_as,
                                "Basic Save bank as");
    gtk_widget_set_tooltip_text(menu_file_full_save_as,
                    "Full bank save, link samples, ready for archiving");

    /* seperator */
    gui_menu_add(menu_file, NULL, NULL, NULL);
    gui_menu_add(menu_file, "Quit", G_CALLBACK(cb_quit), NULL);

    /* patch menu */
    menu_patch = gui_menu_add(menubar, "Patch", NULL, NULL);
    gui_menu_add(menu_patch, "Add...",
            G_CALLBACK(cb_menu_patch_add),          window);
    gui_menu_add(menu_patch, "Add Default",
            G_CALLBACK(cb_menu_patch_add_default),  window);
    gui_menu_add(menu_patch, "Duplicate",
            G_CALLBACK(cb_menu_patch_duplicate),    window);
    gui_menu_add(menu_patch, "Rename...",
            G_CALLBACK(cb_menu_patch_rename),       window);
    gui_menu_add(menu_patch, "Remove",
            G_CALLBACK(cb_menu_patch_remove),       NULL);

    /* settings menu */
    menu_settings = gui_menu_add(menubar, "Settings", NULL, NULL);
    gui_menu_add(menu_settings, "Audio...",
            G_CALLBACK(cb_menu_settings_audio),     NULL);

    /* auto-preview */
    menu_settings_auto_preview =
        gtk_check_menu_item_new_with_label("Sample auto-preview");
    gtk_check_menu_item_set_active(
        GTK_CHECK_MENU_ITEM(menu_settings_auto_preview),
            settings->sample_auto_preview);

    g_signal_connect(GTK_OBJECT(menu_settings_auto_preview), "toggled",
        G_CALLBACK(cb_menu_settings_auto_preview), NULL);

    gtk_menu_shell_append(GTK_MENU_SHELL(menu_settings),
                                        menu_settings_auto_preview);
    gtk_widget_show(menu_settings_auto_preview);

    /* view menu */
    menu_view = gui_menu_add(menubar, "View", NULL, NULL);
    menu_view_log_display =
        gui_menu_check_add(menu_view, "Message Log", FALSE,
                G_CALLBACK(cb_menu_view_log_display), window);

    /* help menu */
    menu_help = gui_menu_add(menubar, "Help", NULL, NULL);
    gui_menu_add(menu_help, "All Sound Off!",
            G_CALLBACK(cb_menu_help_stfu),          NULL);
    gui_menu_add(menu_help, "About...",
            G_CALLBACK(cb_menu_help_about),         window);

    if (!session_is_nsm())
        gui_recent_files_load();

    gtk_widget_show_all(menubar);

    /* setup the main window's master hbox, and left and right boxes */
    master_hbox = gtk_hbox_new(FALSE, GUI_SECSPACE);
    gtk_container_set_border_width(GTK_CONTAINER(master_hbox),
                                        GUI_BORDERSPACE);
    gtk_box_pack_start(GTK_BOX(window_vbox), master_hbox, TRUE, TRUE, 0);

    /* left vbox */
    vbox = gtk_vbox_new(FALSE, GUI_SPACING);
    gtk_box_pack_start(GTK_BOX(master_hbox), vbox, FALSE, FALSE, 0);
    gtk_widget_show(vbox);
    
    /* master section */
    master_section = master_section_new();
    gtk_box_pack_start(GTK_BOX(vbox), master_section, FALSE, FALSE, 0);
    gtk_widget_show(master_section);

    /* patch list */
    patch_list = patch_list_new();
    gtk_box_pack_start(GTK_BOX(vbox), patch_list, TRUE, TRUE, 0);
    g_signal_connect(G_OBJECT(patch_list), "changed",
                                    G_CALLBACK(cb_patch_list_changed),
                                    NULL);
    gtk_widget_show(patch_list);

    /* channel section */
    channel_section = channel_section_new();
    gtk_box_pack_start(GTK_BOX(vbox), channel_section, FALSE, FALSE, 0);
    gtk_widget_show(channel_section);

    /* right vbox */
    vbox = gtk_vbox_new(FALSE, GUI_SPACING);
    gtk_box_pack_start(GTK_BOX(master_hbox), vbox, TRUE, TRUE, 0);
    gtk_widget_show(vbox);

    /* patch section */
    patch_section = patch_section_new();
    gtk_box_pack_start(GTK_BOX(vbox), patch_section, TRUE, TRUE, 0);
    gtk_widget_show(patch_section);

    /* midi section */
    midi_section = midi_section_new();
    gtk_box_pack_start(GTK_BOX(vbox), midi_section, FALSE, FALSE, 0);
    gtk_widget_show(midi_section);

    /* done */
    gtk_widget_show (master_hbox);
    gtk_widget_show (window_vbox);
    gtk_widget_show (window);

    /* intialize children */
    sample_editor_init(window);
    audio_settings_init(window);
    log_display_init(window);

    /* priming updates */

    if (session_is_active())
        gui_set_session_mode();

    gui_refresh();

    return 0;
}


void gui_refresh(void)
{
    const char* title = dish_file_state_path();
    gui_set_window_title_bank(title ? title : "Untitled");
    master_section_update(MASTER_SECTION(master_section));
    patch_list_update(PATCH_LIST(patch_list), 0, PATCH_LIST_INDEX);
    cb_patch_list_changed(PATCH_LIST(patch_list), NULL);

    if (!session_is_nsm())
        gui_recent_files_load();
}


PatchList* gui_get_patch_list(void)
{
    return PATCH_LIST(patch_list);
}


void gui_set_window_title_bank(const char* title)
{
    static char* banktitle = 0;
    const char* instancename = get_instance_name();
    char buf[8000];

    if (!instancename)
        instancename = PACKAGE;

    if (session_is_nsm())
    {
        gtk_window_set_title(GTK_WINDOW(window), instancename);
        return;
    }

    if (title)
    {
        free(banktitle);
        banktitle = strdup(title);
    }

    snprintf(buf, 7999, "%s - %s", instancename, (banktitle) ? banktitle
                                                           : "Untitled");
    buf[7999] = '\0';

    if (window)
        gtk_window_set_title(GTK_WINDOW(window), buf);
}


GtkWidget* gui_menu_add(GtkWidget* menu, const char* label, GCallback cb,
                                                            gpointer data)
{
    GtkWidget* item = 0;
    GtkWidget* submenu = 0;

    if (label)
    {
        item = gtk_menu_item_new_with_label(label);

        if (cb)
            g_signal_connect(item, "activate", cb, data);
        else
        {
            submenu = gtk_menu_new();
            gtk_menu_item_set_submenu(GTK_MENU_ITEM(item), submenu);
        }
    }
    else
        item = gtk_menu_item_new();

    gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);

    return (submenu) ? submenu : item;
}


GtkWidget*
gui_menu_check_add(GtkWidget* menu, const char* label,  gboolean active,
                                                        GCallback cb,
                                                        gpointer data)
{
    GtkWidget* item = 0;

    item = gtk_check_menu_item_new_with_label(label);
    gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(item), active);
    g_signal_connect(item, "toggled", cb, data);
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);

    return item;
}


void gui_set_session_mode(void)
{
    debug("GUI entering session mode\n");
    gtk_widget_set_sensitive(menu_file_open, FALSE);
    gtk_widget_set_sensitive(menu_file_recent, FALSE);
    gtk_widget_set_sensitive(menu_file_basic_save_as, FALSE);
    gtk_widget_set_sensitive(menu_file_full_save_as, FALSE);
    gtk_menu_item_set_label(GTK_MENU_ITEM(menu_file_save),
                            "Save session bank");
    gtk_widget_set_sensitive(menu_file_export, TRUE);
}


/* recent items menu */
void gui_recent_files_load(void) 
{
    GtkRecentFilter *recent_filter;
    GtkWidget *menu_file_recent_items;
    recent_manager = gtk_recent_manager_get_default();
    recent_filter = gtk_recent_filter_new();
    gtk_recent_filter_add_mime_type(recent_filter,
                                    "application/x-petri-foo");
    menu_file_recent_items = 
            gtk_recent_chooser_menu_new_for_manager(recent_manager);
    gtk_recent_chooser_add_filter(
            GTK_RECENT_CHOOSER(menu_file_recent_items), recent_filter);
    gtk_recent_chooser_set_show_tips(
            GTK_RECENT_CHOOSER(menu_file_recent_items), TRUE);
    gtk_recent_chooser_set_sort_type(
            GTK_RECENT_CHOOSER(menu_file_recent_items),
            GTK_RECENT_SORT_MRU);
    gtk_recent_chooser_set_limit(
            GTK_RECENT_CHOOSER(menu_file_recent_items), 10);
    gtk_recent_chooser_set_local_only(
            GTK_RECENT_CHOOSER(menu_file_recent_items), FALSE);
    gtk_recent_chooser_menu_set_show_numbers(
            GTK_RECENT_CHOOSER_MENU(menu_file_recent_items), TRUE);
    g_signal_connect(GTK_OBJECT(menu_file_recent_items),
                    "item-activated",
                     G_CALLBACK(cb_recent_chooser_item_activated), window);
    gtk_menu_item_set_submenu(GTK_MENU_ITEM(menu_file_recent), 
            menu_file_recent_items);

}
