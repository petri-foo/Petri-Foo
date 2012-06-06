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
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <gtk/gtk.h>

#include "basic_combos.h"
#include "gui.h"
#include "petri-foo.h"
#include "pf_error.h"
#include "mixer.h"
#include "msg_log.h"
#include "patch_set_and_get.h"
#include "patch_util.h"
#include "names.h"
#include "sample-selector.h"
#include "sample.h"
#include "global_settings.h"


#include <sndfile.h>

static int patch;

static Sample* last_sample = 0;

typedef struct _raw_box
{
    /* the parent file chooser dialog */
    GtkWidget* dialog;

    /* the raw data box */
    GtkWidget* box;

    /* box contains: */
    GtkWidget* toggle_box;
    GtkWidget* check;
    GtkWidget* auto_preview;
    GtkWidget* resample_checkbox;
    GtkWidget* table;

    /* table contains: */
    GtkWidget* samplerate;
    GtkAdjustment* sr_adj;
    GtkWidget* format;
    GtkWidget* mono;
    GtkWidget* stereo;
    GtkWidget* file_endian;
    GtkWidget* little_endian;
    GtkWidget* big_endian;

    int channels;   /* 1 or 2 */
    int endian;     /* 0 == file, 1 == little, 2 == big */

} raw_box;


static int get_format(raw_box* rb)
{
    int format = basic_combo_get_active(rb->format);

    switch(rb->endian)
    {
    case 2: format |= SF_ENDIAN_BIG;    break;
    case 1: format |= SF_ENDIAN_LITTLE; break;
    case 0:
    default:format |= SF_ENDIAN_FILE;   break;
    }

    return format;
}


static void cb_load(raw_box* rb)
{
    GtkWidget *msg;
    int err;
    char *name = (char *)
            gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(rb->dialog));
    global_settings* settings = settings_get();

    if (!name)
        goto fail;

    if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(rb->check)))
    {
        int samplerate = gtk_spin_button_get_value_as_int(
                                    GTK_SPIN_BUTTON(rb->samplerate));

        if (patch_sample_load(patch, name,    samplerate,
                                                rb->channels,
                                                get_format(rb)) < 0)
        {
            err = pf_error_get();
            goto fail;
        }
    }
    else
    {   /* don't repeat load sample */
        const Sample* s = patch_sample_data(patch);

        if (s->filename && strcmp(name, s->filename) == 0)
            return;

        if (patch_sample_load(patch, name, 0, 0, 0))
        {
            err = pf_error_get();
            goto fail;
        }
    }

    if (name)
    {
        char* dirname = g_path_get_dirname(name);

        if (dirname)
        {
            if (settings->last_sample_dir)
                free(settings->last_sample_dir);

            settings->last_sample_dir = strdup(dirname);
            free(dirname);
        }
    }

    return;

fail:
    if (!name)
    {   /* I don't really think this is possible, but hey. */
        errmsg("no file selected\n");
        msg = gtk_message_dialog_new(GTK_WINDOW(rb->dialog),
                                     GTK_DIALOG_DESTROY_WITH_PARENT,
                                     GTK_MESSAGE_ERROR, GTK_BUTTONS_CLOSE,
                                     "No file selected");
    }
    else
    {
        msg_log(MSG_TYPE_ERROR, /*  Unlike MSG_ERROR, MSG_TYPE_ERROR does
                                    not set the log notification flag */
                "Failed to load sample %s for patch %d (%s)\n",
                name, patch, pf_error_str(err));

        /* do our own notification here: */
        msg = gtk_message_dialog_new(GTK_WINDOW(rb->dialog),
                                     GTK_DIALOG_DESTROY_WITH_PARENT,
                                     GTK_MESSAGE_ERROR, GTK_BUTTONS_CLOSE,
                                     "Failed to load sample %s", name);
    }

    gtk_dialog_run (GTK_DIALOG(msg));
    gtk_widget_destroy(msg);
}


static void cb_preview(raw_box* rb)
{
    char *name = gtk_file_chooser_get_filename(
                                GTK_FILE_CHOOSER(rb->dialog));
    if (!name)
        return;

    if (strchr(name, '.') == NULL)
    {   /*  FIXME: silly method of distiguishing files/folders.
            see: man 3 stat
         */
        return;
    }

    if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(rb->check)))
    {
        int samplerate = gtk_spin_button_get_value_as_int(
                                    GTK_SPIN_BUTTON(rb->samplerate));

        mixer_preview(name, samplerate,  rb->channels, get_format(rb),1);
    }
    else
    {
        int resamp = gtk_toggle_button_get_active(
                            GTK_TOGGLE_BUTTON(rb->resample_checkbox));
        mixer_preview(name, 0, 0, 0, resamp);
    }

    return;
}


static void cb_cancel(void)
{
    if (!last_sample->filename)
    {
        patch_sample_unload(patch);
        return;
    }

    if (strcmp(patch_get_sample_name(patch), last_sample->filename) != 0)
    {
        msg_log(MSG_MESSAGE, "Restoring sample:%s\n",
                                last_sample->filename);

        patch_sample_load(patch, last_sample->filename,
                                 last_sample->raw_samplerate,
                                 last_sample->raw_channels,
                                 last_sample->sndfile_format);
    }

    return;
}


static void raw_toggles_toggled_cb(GtkToggleButton* tog, gpointer data)
{
    raw_box* rb = data;

    if (gtk_toggle_button_get_active(tog))
    {
        if (tog == GTK_TOGGLE_BUTTON(rb->stereo))
            rb->channels = 2;
        else if (tog == GTK_TOGGLE_BUTTON(rb->mono))
            rb->channels = 1;
        else if (tog == GTK_TOGGLE_BUTTON(rb->big_endian))
            rb->endian = 2;
        else if (tog == GTK_TOGGLE_BUTTON(rb->little_endian))
            rb->endian = 1;
        else if (tog == GTK_TOGGLE_BUTTON(rb->file_endian))
            rb->endian = 0;
    }
}


static void raw_toggled_cb(GtkToggleButton* raw_toggle, gpointer data)
{
    raw_box* rb = data;
    if (gtk_toggle_button_get_active(raw_toggle))
        gtk_widget_show_all(rb->table);
    else
        gtk_widget_hide(rb->table);
}


static void selection_changed_cb(GtkFileChooser* dialog, gpointer data)
{
    (void)dialog;

    raw_box* rb = (raw_box*)data;

    if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(rb->auto_preview)))
        cb_preview(rb);
}


static raw_box* raw_box_new(GtkWidget* dialog)
{
    GtkTable* t;
    int y = 0;

    raw_box* rb = malloc(sizeof(*rb));

    if (!rb)
    {
        debug("failed to alloc raw data entry box\n");
        return 0;
    }

    rb->dialog = dialog;
    rb->channels = 1;
    rb->endian = 0;

    rb->box = gtk_vbox_new(FALSE, 0);
    rb->toggle_box = gtk_hbox_new(FALSE,10);

    rb->check = gtk_check_button_new_with_label("Raw/Headerless");
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(rb->check), FALSE);
    gtk_widget_show(rb->check);
    gtk_box_pack_start(GTK_BOX(rb->toggle_box), rb->check, FALSE, FALSE, 0);
    g_signal_connect(GTK_OBJECT(rb->check), "toggled",
                                            G_CALLBACK(raw_toggled_cb), rb);

    rb->auto_preview = gtk_check_button_new_with_label("Auto Preview");
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(rb->auto_preview),
                                                                FALSE);
    gtk_widget_show(rb->auto_preview);
    gtk_box_pack_start(GTK_BOX(rb->toggle_box), rb->auto_preview, FALSE,
                                                                FALSE, 0);

    rb->resample_checkbox = gtk_check_button_new_with_label("Resample");
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(rb->resample_checkbox),
                                                                FALSE);
    gtk_widget_show(rb->resample_checkbox);
    gtk_box_pack_start(GTK_BOX(rb->toggle_box), rb->resample_checkbox, FALSE,
                                                                FALSE, 0);


    g_signal_connect(dialog, "selection-changed",
                                    G_CALLBACK(selection_changed_cb), rb);

    gtk_box_pack_start(GTK_BOX(rb->box), rb->toggle_box, TRUE, TRUE, 0);
    gtk_widget_show(rb->toggle_box);

    rb->table = gtk_table_new(5, 2, FALSE);
    gtk_box_pack_start(GTK_BOX(rb->box), rb->table, TRUE, TRUE, 0);

    t = GTK_TABLE(rb->table);

    rb->format = basic_combo_id_name_create(names_sample_raw_format_get());
    gui_attach(t, rb->format, 0, 2, y, y + 1);

    gui_label_attach("Sample rate:", t, 2, 4, y, y + 1);

    rb->sr_adj = (GtkAdjustment*)
        gtk_adjustment_new(44100, 8000, 192000, 100, 1000, 0);
    rb->samplerate = gtk_spin_button_new(rb->sr_adj, 1.0, 0);
    gui_attach(t, rb->samplerate, 3, 4, y, y + 1);

    ++y;

    rb->mono = gtk_radio_button_new_with_label(NULL,"Mono");
    rb->stereo = gtk_radio_button_new_with_label_from_widget(
                                            GTK_RADIO_BUTTON(rb->mono),
                                                            "Stereo");
    gui_attach(t, rb->mono, 0, 1, y, y + 1);
    gui_attach(t, rb->stereo, 1, 2, y, y + 1);

    rb->file_endian = gtk_radio_button_new_with_label(NULL, "File Endian");
    rb->little_endian = gtk_radio_button_new_with_label_from_widget(
                                    GTK_RADIO_BUTTON(rb->file_endian),
                                                        "Little Endian");
    rb->big_endian = gtk_radio_button_new_with_label_from_widget(
                                    GTK_RADIO_BUTTON(rb->little_endian),
                                                        "Big Endian");
    gui_attach(t, rb->file_endian, 2, 3, y, y + 1);
    gui_attach(t, rb->little_endian, 3, 4, y, y + 1);
    gui_attach(t, rb->big_endian, 4, 5, y, y + 1);

    g_signal_connect(GTK_OBJECT(rb->mono),          "toggled",
                                G_CALLBACK(raw_toggles_toggled_cb), rb);
    g_signal_connect(GTK_OBJECT(rb->stereo),        "toggled",
                                G_CALLBACK(raw_toggles_toggled_cb), rb);
    g_signal_connect(GTK_OBJECT(rb->file_endian),   "toggled",
                                G_CALLBACK(raw_toggles_toggled_cb), rb);
    g_signal_connect(GTK_OBJECT(rb->little_endian), "toggled",
                                G_CALLBACK(raw_toggles_toggled_cb), rb);
    g_signal_connect(GTK_OBJECT(rb->big_endian),    "toggled",
                                G_CALLBACK(raw_toggles_toggled_cb), rb);

    /* table should be hidden by default */
    gtk_widget_hide(rb->table);

    return rb;
}


int sample_selector_show(int id, GtkWidget* parent_window,
                                 SampleTab* sampletab)
{
    GtkWidget* dialog;
    raw_box* rawbox;
    global_settings* settings = settings_get();

    enum {
        RESPONSE_LOAD = 1,
        RESPONSE_PREVIEW = 2
    };

    last_sample = sample_new();
    patch = id;
    dialog = gtk_file_chooser_dialog_new("Load Sample",
                                          GTK_WINDOW(parent_window),
                                          GTK_FILE_CHOOSER_ACTION_OPEN,
                                          GTK_STOCK_CANCEL,
                                          GTK_RESPONSE_CANCEL,
                                          GTK_STOCK_OPEN,
                                          GTK_RESPONSE_ACCEPT, NULL);
    if ((rawbox = raw_box_new(dialog)))
    {
        gtk_file_chooser_set_extra_widget(GTK_FILE_CHOOSER(dialog),
                                                            rawbox->box);
    }

    sample_shallow_copy(last_sample, patch_sample_data(patch));

    if (last_sample->filename &&
        strcmp(last_sample->filename, "Default") != 0)
    {
        gtk_file_chooser_set_filename(GTK_FILE_CHOOSER(dialog),
                                last_sample->filename);
        gtk_file_chooser_set_current_folder(GTK_FILE_CHOOSER(dialog),
                         g_path_get_dirname(last_sample->filename));
    } 
    else {
        if ( settings->last_sample_dir) 
            gtk_file_chooser_set_current_folder(GTK_FILE_CHOOSER(dialog),
                                          settings->last_sample_dir);
    }

    gtk_dialog_add_button(GTK_DIALOG(dialog),
                                "Loa_d", RESPONSE_LOAD);

    gtk_dialog_add_button(GTK_DIALOG(dialog),
                                "Pre_view", RESPONSE_PREVIEW);

again:
    switch(gtk_dialog_run(GTK_DIALOG(dialog)))
    {
    case GTK_RESPONSE_ACCEPT:
        cb_load(rawbox);
        break;

    case RESPONSE_LOAD:
        cb_load(rawbox);
        sample_tab_update_waveforms(sampletab);
        goto again;

    case RESPONSE_PREVIEW:
        cb_preview(rawbox);
        goto again;

    case GTK_RESPONSE_CANCEL:
        cb_cancel();
    default:
        break;
    }

    gtk_widget_destroy(dialog);
    sample_free(last_sample);
    last_sample = 0;

    return 0;
}

