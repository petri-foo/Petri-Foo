#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <gtk/gtk.h>

#include "instance.h"
#include "petri-foo.h"
#include "patch.h"
#include "mixer.h"
#include "gui.h"
#include "patchsection.h"
#include "mastersection.h"
#include "channelsection.h"
#include "midisection.h"
#include "patchlist.h"
#include "waveform.h"
#include "sample-editor.h"
#include "sample-selector.h"
#include "bank-ops.h"
#include "audio-settings.h"

/* windows */
static GtkWidget* window;
static GtkWidget* patch_section;
static GtkWidget* master_section;
static GtkWidget* midi_section;
static GtkWidget* channel_section;
static GtkWidget* patch_list;

/* main menu */
static GtkWidget* menu_file;
static GtkWidget* menu_file_item;
static GtkWidget* menu_file_new_bank;
static GtkWidget* menu_file_open_bank;
static GtkWidget* menu_file_save_bank;
static GtkWidget* menu_file_save_bank_as;
static GtkWidget* menu_file_quit;
static GtkWidget* menu_file_vsep;
static GtkWidget* menu_settings;
static GtkWidget* menu_settings_item;
static GtkWidget* menu_settings_audio;
static GtkWidget* menu_patch;
static GtkWidget* menu_patch_item;
static GtkWidget* menu_patch_add;
static GtkWidget* menu_patch_duplicate;
static GtkWidget* menu_patch_rename;
static GtkWidget* menu_patch_remove;
static GtkWidget* menu_help;
static GtkWidget* menu_help_item;
static GtkWidget* menu_help_stfu;
static GtkWidget* menu_help_about;


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

/* when the 'x' button is clicked on the titlebar */
static gboolean cb_delete (GtkWidget* widget, GdkEvent* event,
			   gpointer data)
{
    /* we return false, causing the "destroy" event to be released on "widget" */
    return FALSE;
}


static void cb_quit (GtkWidget* widget, gpointer data)
{
    debug ("Quitting\n");
    gtk_main_quit ( );
}


/* enables ok button if there is input in entry, disables otherwise */
static gboolean cb_menu_patch_name_verify (GtkWidget * entry,
					   GdkEventKey * event,
					   GtkDialog * dialog)
{
    int val = strlen ((char *) gtk_entry_get_text (GTK_ENTRY (entry)));

    if (val)
	gtk_dialog_set_response_sensitive (dialog, GTK_RESPONSE_ACCEPT,
					   TRUE);
    else
	gtk_dialog_set_response_sensitive (dialog, GTK_RESPONSE_ACCEPT,
					   FALSE);

    return FALSE;		/* make sure normal event handlers grab signal */
}


static void cb_menu_patch_add (GtkWidget * menu_item, GtkWidget * main_window)
{
    GtkWidget *dialog;
    GtkWidget *entry;
    int val;

    /* dialog box */
    dialog = gtk_dialog_new_with_buttons ("Add Patch",
					  GTK_WINDOW (main_window),
					  GTK_DIALOG_MODAL |
					  GTK_DIALOG_DESTROY_WITH_PARENT,
					  GTK_STOCK_OK,
					  GTK_RESPONSE_ACCEPT,
					  GTK_STOCK_CANCEL,
					  GTK_RESPONSE_REJECT, NULL);
    gtk_dialog_set_default_response (GTK_DIALOG (dialog),
				     GTK_RESPONSE_ACCEPT);

    /* create entry box */
    entry = gtk_entry_new_with_max_length (PATCH_MAX_NAME);
    gtk_entry_set_text (GTK_ENTRY (entry), "Patch Name");
    gtk_entry_set_activates_default (GTK_ENTRY (entry), TRUE);

    /* I wish I had a "changed" event... */
    g_signal_connect (G_OBJECT (entry), "key-press-event",
		      G_CALLBACK (cb_menu_patch_name_verify),
		      (gpointer) dialog);
    g_signal_connect (G_OBJECT (entry), "key-release-event",
		      G_CALLBACK (cb_menu_patch_name_verify),
		      (gpointer) dialog);
    gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dialog)->vbox), entry, FALSE,
			FALSE, 0);
    gtk_widget_show (entry);

    /* guaranteed to have a string in there if response is 'accept'
     * due to sensitivity callback */
    if (gtk_dialog_run (GTK_DIALOG (dialog)) == GTK_RESPONSE_ACCEPT)
    {
	val = patch_create ((char *) gtk_entry_get_text (GTK_ENTRY (entry)));
	if (val < 0)
	{
	    errmsg ("Failed to create a new patch (%s).\n",
		    patch_strerror (val));
	    return;
	}

	patch_set_channel(val, channel_section_get_channel(CHANNEL_SECTION(channel_section)));
	patch_list_update (PATCH_LIST(patch_list), val, PATCH_LIST_PATCH);
    }

    gtk_widget_destroy (dialog);
}


static void cb_menu_patch_duplicate (GtkWidget * menu_item,
				     GtkWidget * main_window)
{
    int val;
    int cp;

    if ((cp = patch_list_get_current_patch(PATCH_LIST(patch_list))) < 0)
    {
	debug ("Duplicate what, jackass?\n");
	return;
    }

    val = patch_duplicate (cp);
    if (val < 0)
    {
	errmsg ("Failed to create a new patch (%s).\n",
		patch_strerror (val));
	return;
    }

    patch_list_update (PATCH_LIST(patch_list), val, PATCH_LIST_PATCH);
}


static void cb_menu_patch_rename (GtkWidget * menu_item,
				  GtkWidget * main_window)
{
    GtkWidget *dialog;
    GtkWidget *entry;
    int val;
    int index;
    int cp;

    if ((cp = patch_list_get_current_patch(PATCH_LIST(patch_list))) < 0)
    {
	debug ("No patches to rename, infidel.\n");
	return;
    }

    /* dialog box */
    dialog = gtk_dialog_new_with_buttons ("Rename Patch",
					  GTK_WINDOW (main_window),
					  GTK_DIALOG_MODAL |
					  GTK_DIALOG_DESTROY_WITH_PARENT,
					  GTK_STOCK_OK,
					  GTK_RESPONSE_ACCEPT,
					  GTK_STOCK_CANCEL,
					  GTK_RESPONSE_REJECT, NULL);
    gtk_dialog_set_default_response (GTK_DIALOG (dialog),
				     GTK_RESPONSE_ACCEPT);

    /* create entry box */
    entry = gtk_entry_new_with_max_length (PATCH_MAX_NAME);
    gtk_entry_set_text (GTK_ENTRY (entry), patch_get_name (cp));
    gtk_entry_set_activates_default (GTK_ENTRY (entry), TRUE);

    /* I wish I had a "changed" event... */
    g_signal_connect (G_OBJECT (entry), "key-press-event",
		      G_CALLBACK (cb_menu_patch_name_verify),
		      (gpointer) dialog);
    g_signal_connect (G_OBJECT (entry), "key-release-event",
		      G_CALLBACK (cb_menu_patch_name_verify),
		      (gpointer) dialog);
    gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dialog)->vbox), entry, FALSE,
			FALSE, 0);
    gtk_widget_show (entry);

    /* guaranteed to have a string in there if response is 'accept'
     * due to sensitivity callback */
    if (gtk_dialog_run (GTK_DIALOG (dialog)) == GTK_RESPONSE_ACCEPT)
    {
	val =
	    patch_set_name (cp,
			    (char *)
			    gtk_entry_get_text (GTK_ENTRY (entry)));
	if (val < 0)
	{
	    errmsg ("Failed to rename patch (%s).\n",
		    patch_strerror (val));
	    return;
	}

	index = patch_list_get_current_index (PATCH_LIST(patch_list));
	patch_list_update (PATCH_LIST(patch_list), index, PATCH_LIST_INDEX);
    }

    gtk_widget_destroy (dialog);
}


static void cb_menu_patch_remove (GtkWidget * menu_item, gpointer data)
{
    int val;
    int cp;
    int index;

    if ((cp = patch_list_get_current_patch(PATCH_LIST(patch_list))) < 0)
    {
	debug ("No patches to remove, you dolt.\n");
	return;
    }

    index = patch_list_get_current_index (PATCH_LIST(patch_list));
    if ((val = patch_destroy (cp)) < 0)
    {
	errmsg ("Error removing patch %d (%s).\n", cp,
		patch_strerror (val));
	return;
    }

    if (index == 0)
    {
	patch_list_update (PATCH_LIST(patch_list), index, PATCH_LIST_INDEX);
    }
    else
    {
	patch_list_update (PATCH_LIST(patch_list), index - 1, PATCH_LIST_INDEX);
    }
}


static void cb_menu_file_new_bank (GtkWidget * widget, gpointer data)
{
    if (bank_ops_new ( ) == 0)
    {
	patch_list_update (PATCH_LIST(patch_list), 0, PATCH_LIST_INDEX);
    }
}


static void cb_menu_file_open_bank (GtkWidget * widget, gpointer data)
{
    if (bank_ops_open ( ) == 0)
    {
	patch_list_update (PATCH_LIST(patch_list), 0, PATCH_LIST_INDEX);
    }
}


static void cb_menu_file_save_bank (GtkWidget * widget, gpointer data)
{
    bank_ops_save ( );
}


static void cb_menu_file_save_bank_as (GtkWidget * widget, gpointer data)
{
    bank_ops_save_as ( );
}


static void cb_menu_settings_audio (GtkWidget * widget, gpointer data)
{
    audio_settings_show (window);
}


static void cb_menu_help_stfu (GtkWidget* widget, gpointer data)
{
    mixer_flush();
}


static void cb_menu_help_about (GtkWidget* widget, gpointer data)
{
    GdkPixbuf* logo;
    const char* authors[] = { "Pete Bessman (original author)",
                             "See the AUTHORS file for others", 0 };

    /* should this be freed later on? */
    logo = gdk_pixbuf_new_from_file(PIXMAPSDIR "petri-foo.png", NULL);

    gtk_show_about_dialog(
        GTK_WINDOW(data),
        "name", "Petri-Foo",
        "logo", logo,
        "authors", authors,
        "version", VERSION,
        "copyright",    "(C) 2004-2005 Pete Bessman\n"
                        "(C) 2006-2007 others\n"
                        "(C) 2011 James Morris\n",
        NULL);
}


static void cb_patch_list_changed(PatchList* list, gpointer data)
{
    int patch = patch_list_get_current_patch(list);
    
    patch_section_set_patch(PATCH_SECTION(patch_section), patch);
    midi_section_set_patch(MIDI_SECTION(midi_section), patch);
    channel_section_set_patch(CHANNEL_SECTION(channel_section), patch);
}


int gui_init(void)
{
    GtkWidget* window_vbox;
    GtkWidget* master_hbox;
    GtkWidget* menubar;
    GtkWidget* vbox;

    debug ("Initializing GUI\n");

    /* main window */
    window = gtk_window_new (GTK_WINDOW_TOPLEVEL);

    if (instance_name)
    {
        size_t len = strlen (instance_name);
        char title[12 + len];

        strncpy (title, "Petri-Foo - ", 11);
        strncpy (&title[11], instance_name, len);
        title[11 + len] = '\0';

        gtk_window_set_title (GTK_WINDOW (window), title);
    }
    else
      gtk_window_set_title (GTK_WINDOW (window), "Petri-Foo");

    g_signal_connect (G_OBJECT (window), "delete-event",
		      G_CALLBACK (cb_delete), NULL);
    g_signal_connect (G_OBJECT (window), "destroy", G_CALLBACK (cb_quit),
		      NULL);

    /* setup the window's main vbox */
    window_vbox = gtk_vbox_new (FALSE, 0);
    gtk_container_add (GTK_CONTAINER (window), window_vbox);

    /* the menubar */
    menubar = gtk_menu_bar_new ( );
    gtk_box_pack_start (GTK_BOX (window_vbox), menubar, FALSE, FALSE, 0);
    gtk_widget_show (menubar);

    /* file menu */
    menu_file = gtk_menu_new ( );
    menu_file_item = gtk_menu_item_new_with_label ("File");
    gtk_menu_item_set_submenu (GTK_MENU_ITEM (menu_file_item), menu_file);
    gtk_menu_bar_append (GTK_MENU_BAR (menubar), menu_file_item);
    gtk_widget_show (menu_file_item);

    menu_file_new_bank = gtk_menu_item_new_with_label ("New Bank");
    gtk_menu_shell_append (GTK_MENU_SHELL (menu_file), menu_file_new_bank);
    g_signal_connect (G_OBJECT (menu_file_new_bank), "activate",
		      G_CALLBACK (cb_menu_file_new_bank), NULL);
    gtk_widget_show (menu_file_new_bank);

    menu_file_open_bank = gtk_menu_item_new_with_label ("Open Bank...");
    gtk_menu_shell_append (GTK_MENU_SHELL (menu_file),
			   menu_file_open_bank);
    g_signal_connect (G_OBJECT (menu_file_open_bank), "activate",
		      G_CALLBACK (cb_menu_file_open_bank), NULL);
    gtk_widget_show (menu_file_open_bank);

    menu_file_save_bank = gtk_menu_item_new_with_label ("Save Bank");
    gtk_menu_shell_append (GTK_MENU_SHELL (menu_file),
			   menu_file_save_bank);
    g_signal_connect (G_OBJECT (menu_file_save_bank), "activate",
		      G_CALLBACK (cb_menu_file_save_bank), NULL);
    gtk_widget_show (menu_file_save_bank);

    menu_file_save_bank_as =
	gtk_menu_item_new_with_label ("Save Bank As...");
    gtk_menu_shell_append (GTK_MENU_SHELL (menu_file),
			   menu_file_save_bank_as);
    g_signal_connect (G_OBJECT (menu_file_save_bank_as), "activate",
		      G_CALLBACK (cb_menu_file_save_bank_as), NULL);
    gtk_widget_show (menu_file_save_bank_as);

    menu_file_vsep = gtk_menu_item_new ( );
    gtk_menu_shell_append (GTK_MENU_SHELL (menu_file), menu_file_vsep);
    gtk_widget_show (menu_file_vsep);

    menu_file_quit = gtk_menu_item_new_with_label ("Quit");
    gtk_menu_shell_append (GTK_MENU_SHELL (menu_file), menu_file_quit);
    g_signal_connect (G_OBJECT (menu_file_quit), "activate",
		      G_CALLBACK (cb_quit), NULL);
    gtk_widget_show (menu_file_quit);

    /* patch menu */
    menu_patch = gtk_menu_new ( );
    menu_patch_item = gtk_menu_item_new_with_label ("Patch");
    gtk_menu_item_set_submenu (GTK_MENU_ITEM (menu_patch_item), menu_patch);
    gtk_menu_bar_append (GTK_MENU_BAR (menubar), menu_patch_item);
    gtk_widget_show (menu_patch_item);

    menu_patch_add = gtk_menu_item_new_with_label ("Add...");
    gtk_menu_shell_append (GTK_MENU_SHELL (menu_patch), menu_patch_add);
    g_signal_connect (G_OBJECT (menu_patch_add), "activate",
		      G_CALLBACK (cb_menu_patch_add), (gpointer) window);
    gtk_widget_show (menu_patch_add);

    menu_patch_duplicate = gtk_menu_item_new_with_label ("Duplicate");
    gtk_menu_shell_append (GTK_MENU_SHELL (menu_patch), menu_patch_duplicate);
    g_signal_connect (G_OBJECT (menu_patch_duplicate), "activate",
		      G_CALLBACK (cb_menu_patch_duplicate), (gpointer) window);
    gtk_widget_show (menu_patch_duplicate);

    menu_patch_rename = gtk_menu_item_new_with_label ("Rename...");
    gtk_menu_shell_append (GTK_MENU_SHELL (menu_patch), menu_patch_rename);
    g_signal_connect (G_OBJECT (menu_patch_rename), "activate",
		      G_CALLBACK (cb_menu_patch_rename), (gpointer) window);
    gtk_widget_show (menu_patch_rename);

    menu_patch_remove = gtk_menu_item_new_with_label ("Remove");
    gtk_menu_shell_append (GTK_MENU_SHELL (menu_patch), menu_patch_remove);
    g_signal_connect (G_OBJECT (menu_patch_remove), "activate",
		      G_CALLBACK (cb_menu_patch_remove), (gpointer) NULL);
    gtk_widget_show (menu_patch_remove);

    /* settings menu */
    menu_settings = gtk_menu_new ( );
    menu_settings_item = gtk_menu_item_new_with_label ("Settings");
    gtk_menu_item_set_submenu (GTK_MENU_ITEM (menu_settings_item),
			       menu_settings);
    gtk_menu_bar_append (GTK_MENU_BAR (menubar), menu_settings_item);
    gtk_widget_show (menu_settings_item);

    menu_settings_audio = gtk_menu_item_new_with_label ("Audio...");
    gtk_menu_shell_append (GTK_MENU_SHELL (menu_settings),
			   menu_settings_audio);
    g_signal_connect (G_OBJECT (menu_settings_audio), "activate",
		      G_CALLBACK (cb_menu_settings_audio), NULL);
    gtk_widget_show (menu_settings_audio);

    /* help menu */
    menu_help = gtk_menu_new ( );
    menu_help_item = gtk_menu_item_new_with_label ("Help");
    gtk_menu_item_set_submenu (GTK_MENU_ITEM (menu_help_item),
			       menu_help);
    gtk_menu_bar_append (GTK_MENU_BAR (menubar), menu_help_item);
    gtk_widget_show (menu_help_item);

    menu_help_stfu = gtk_menu_item_new_with_label ("STFU!");
    gtk_menu_shell_append (GTK_MENU_SHELL (menu_help),
			   menu_help_stfu);
    g_signal_connect (G_OBJECT (menu_help_stfu), "activate",
		      G_CALLBACK (cb_menu_help_stfu), NULL);
    gtk_widget_show (menu_help_stfu);

    menu_help_about = gtk_menu_item_new_with_label ("About...");
    gtk_menu_shell_append (GTK_MENU_SHELL (menu_help),
			   menu_help_about);
    g_signal_connect (G_OBJECT (menu_help_about), "activate",
		      G_CALLBACK (cb_menu_help_about), window);
    gtk_widget_show (menu_help_about);

    /* setup the main window's master hbox, and left and right boxes */
    master_hbox = gtk_hbox_new(FALSE, GUI_SECSPACE);
    gtk_container_set_border_width(GTK_CONTAINER(master_hbox), GUI_BORDERSPACE);
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
		     G_CALLBACK(cb_patch_list_changed), NULL);
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

    /* priming updates */
    master_section_update(MASTER_SECTION(master_section));
    patch_list_update(PATCH_LIST(patch_list), 0, PATCH_LIST_INDEX);

    return 0;
}


void gui_refresh(void)
{
    patch_list_update (PATCH_LIST(patch_list), 0, PATCH_LIST_INDEX);
}


PatchList* gui_get_patch_list(void)
{
    return PATCH_LIST(patch_list);
}
