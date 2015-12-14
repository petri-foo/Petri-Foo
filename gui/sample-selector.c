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

    gboolean dont_preview;

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
    GtkFileFilter* filter;
    int err;
    char *name = (char *)
            gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(rb->dialog));
    global_settings* settings = settings_get();
    const char* filtername;

    mixer_flush_preview();

    if (!name)
        goto fail;

    if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(rb->check)))
    {
        int samplerate = gtk_spin_button_get_value_as_int(
                                    GTK_SPIN_BUTTON(rb->samplerate));

        if (patch_sample_load(patch, name,    samplerate,
                                                rb->channels,
                                                get_format(rb),
                                                true) < 0)
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

        if (patch_sample_load(patch, name, 0, 0, 0, true))
        {
            err = pf_error_get();
            goto fail;
        }
    }

    msg_log(MSG_MESSAGE, "loaded sample '%s' for patch %d '%s'\n",
                                name, patch, patch_get_name(patch));

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

    /*  propagate certain user-set characteristics of the chooser
     *  into the global settings
     */

    filter = gtk_file_chooser_get_filter(GTK_FILE_CHOOSER(rb->dialog));
    filtername = gtk_file_filter_get_name(filter);

    if (filtername != 0)
    {
        if (settings->sample_file_filter)
            free(settings->sample_file_filter);

        settings->sample_file_filter = strdup(filtername);
    }

    /* the main menu not the chooser sets the global auto-preview on/off
    settings->sample_auto_preview =
        gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(rb->auto_preview));
    */
    return;

fail:
    if (!name)
    {   /* I don't really think this is possible, but hey. */
        msg_log(MSG_ERROR, "no file selected\n");
        msg = gtk_message_dialog_new(GTK_WINDOW(rb->dialog),
                                     GTK_DIALOG_DESTROY_WITH_PARENT,
                                     GTK_MESSAGE_ERROR, GTK_BUTTONS_CLOSE,
                                     "No file selected");
    }
    else
    {
        msg_log(MSG_ERROR,
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
    char *name;

    if (!rb)
        return;

    name = gtk_file_chooser_get_filename(
                                GTK_FILE_CHOOSER(rb->dialog));
    if (!name)
        return;

    if (!is_valid_file(name))
        return;

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
    mixer_flush_preview();

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
                                 last_sample->sndfile_format,
                                 true);
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


static raw_box* raw_box_new(GtkWidget* dialog)
{
    GtkTable* t;
    int y = 0;
    global_settings* settings = settings_get();
    id_name* ids;

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

    rb->auto_preview = gtk_check_button_new_with_label("Auto Preview");
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(rb->auto_preview),
                                        settings->sample_auto_preview);
    gtk_widget_show(rb->auto_preview);
    gtk_box_pack_start(GTK_BOX(rb->toggle_box), rb->auto_preview, FALSE,
                                                                FALSE, 0);

    rb->resample_checkbox = gtk_check_button_new_with_label("Resample");
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(rb->resample_checkbox),
                                                                FALSE);
    gtk_widget_show(rb->resample_checkbox);
    gtk_box_pack_start(GTK_BOX(rb->toggle_box), rb->resample_checkbox,
                                                        FALSE, FALSE, 0);

    gtk_box_pack_start(GTK_BOX(rb->box), rb->toggle_box, TRUE, TRUE, 0);
    gtk_widget_show(rb->toggle_box);

    rb->table = gtk_table_new(5, 2, FALSE);
    gtk_box_pack_start(GTK_BOX(rb->box), rb->table, TRUE, TRUE, 0);

    t = GTK_TABLE(rb->table);

    ids = names_sample_raw_format_get();
    rb->format = basic_combo_id_name_create(ids);
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

    g_signal_connect(GTK_OBJECT(rb->check), "toggled",
                                            G_CALLBACK(raw_toggled_cb), rb);
    free(ids);
    return rb;
}


void create_filters(GtkWidget* chooser)
{
    typedef struct snd_fmt_names
    {
        int sf_fmt;
        char* ext;
        char* name;
    } snd_fmt;

    snd_fmt alt_exts[] = {
    /*  libsndfile only provides a single 'most common' extension for
     *  each format it is capable of reading. Here are some additional
     *  extensions for selected formats:
     *  (followed by sndfile provided extension in comments)
     */
        { SF_FORMAT_AIFF,   "aif",    0}, /* aiff */
        { SF_FORMAT_CAF,    "caff",   0}, /* caf */
        { SF_FORMAT_OGG,    "ogg",    0}, /* oga */
        { SF_FORMAT_SD2,    "sd2f",   0}, /* sd2 */
        { SF_FORMAT_NIST,   "sph",    0}, /* wav */
        { 0, 0, 0 }};

    /* stuff for accessing soundfile format information */
    snd_fmt sound_formats[99];
    SF_FORMAT_INFO  sf_fmt;
    int             sf_fmt_count;
    int             n;

    /* stuff for improving format information */
    snd_fmt* fmt;
    snd_fmt* n_fmt;
    snd_fmt* alt;
    char pat[10];

    GtkFileFilter* filter = 0;

    GSList* f_list;
    GSList* f_item;
    global_settings* settings = settings_get();

    /* generate format information */
    sf_command(0, SFC_GET_FORMAT_MAJOR_COUNT, &sf_fmt_count, sizeof(int));

    for (fmt = sound_formats, n = 0; n < sf_fmt_count; ++n, ++fmt)
    {
        sf_fmt.format = n;
        sf_command(0, SFC_GET_FORMAT_MAJOR, &sf_fmt, sizeof(sf_fmt));
        snprintf(pat, 10, "*.%s", sf_fmt.extension);
        pat[9] = '\0';
        fmt->sf_fmt = sf_fmt.format;
        fmt->name = strdup(sf_fmt.name);
        fmt->ext = strdup(pat);

        for (n_fmt = fmt, alt = alt_exts; alt->sf_fmt != 0; ++alt)
        {
            if (alt->sf_fmt == n_fmt->sf_fmt)
            {
                snprintf(pat, 10, "*.%s", alt->ext);
                pat[9] = '\0';
                ++fmt;
                fmt->sf_fmt = sf_fmt.format;
                fmt->name = strdup(n_fmt->name);
                fmt->ext = strdup(pat);
            }
        }
    }

    fmt->name = 0;
    fmt->ext = 0;

    /* Audio files filter (specifically: files loadable by sndfile) */
    filter = gtk_file_filter_new();
    gtk_file_filter_set_name(filter, "All Audio files");
    for (fmt = sound_formats; fmt->name != 0 && fmt->ext != 0; ++fmt)
    {
        #if DEBUG
        printf("format ext:%s \tname:%s\n", fmt->ext, fmt->name);
        #endif
        gtk_file_filter_add_pattern(filter, fmt->ext);
    }
    gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(chooser), filter);

    /* Individual filters for formats loadable by sndfile
     * NOTE: most formats have one file extension, but some have to be
     * arkward and have alternative extensions...
     */

    for (fmt = sound_formats; fmt->name != 0 && fmt->ext != 0; fmt = n_fmt)
    {
        n_fmt = fmt;
        filter = gtk_file_filter_new();
        gtk_file_filter_set_name(filter, fmt->name);

        while (strcmp(fmt->name, n_fmt->name) == 0)
        {
            gtk_file_filter_add_pattern(filter, n_fmt->ext);
            ++n_fmt;
            if (n_fmt->name == 0)
                break;
        }

        gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(chooser), filter);
    }

    /* All files filter */
    filter = gtk_file_filter_new();
    gtk_file_filter_set_name(filter, "All files");
    gtk_file_filter_add_pattern(filter, "*");
    gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(chooser), filter);

    for (fmt = sound_formats; fmt->name != 0 && fmt->ext != 0; ++fmt)
    {
        free(fmt->name);
        free(fmt->ext);
    }

    f_list = gtk_file_chooser_list_filters(GTK_FILE_CHOOSER(chooser));

    f_item = f_list;

    while(f_item)
    {
        filter = GTK_FILE_FILTER(f_item->data);

        if (strcmp(gtk_file_filter_get_name(filter),
                    settings->sample_file_filter) == 0)
        {
            gtk_file_chooser_set_filter(GTK_FILE_CHOOSER(chooser), filter);
            break;
        }

        f_item = f_item->next;
    }

    g_slist_free(f_list);
}


static gboolean timeout_cancel_dont_preview(gpointer data)
{
    raw_box* rb = (raw_box*)data;
    rb->dont_preview = false;
    return false;
}


static void selection_changed_cb(GtkFileChooser* dialog, gpointer data)
{
    (void)dialog;

    raw_box* rb = (raw_box*)data;

    debug("selection changed... dont_preview:%s\n",
                    (rb->dont_preview ? "T" : "F"));

    if (rb->dont_preview)
        return;

    if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(rb->auto_preview)))
        cb_preview(rb);
}


static void folder_changed_cb(GtkFileChooser* dialog, gpointer data)
{
    (void)dialog;

    raw_box* rb = (raw_box*)data;

    debug("folder changed... dont_preview:%s\n",
                    (rb->dont_preview ? "T" : "F"));

    mixer_flush_preview();

    if (!rb->dont_preview)
    {
        rb->dont_preview = true;
        g_timeout_add(250, timeout_cancel_dont_preview, rb);
    }
}


int sample_selector_show(int id, GtkWidget* parent_window,
                                 SampleTab* sampletab)
{
    enum {  RESPONSE_LOAD = 1,  RESPONSE_PREVIEW = 2 };

    GtkWidget* dialog;
    raw_box* rawbox;
    global_settings* settings = settings_get();

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

    create_filters(dialog);

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

    /*  we don't always want to auto-preview when the "selection-changed"
        signal is emitted on a) when the dialog is created, b) when the
        current-folder is changed.
     */
    if (rawbox)
    {
        rawbox->dont_preview = true;
        g_timeout_add(250, timeout_cancel_dont_preview, rawbox);

        g_signal_connect(rawbox->dialog, "current-folder-changed",
                                    G_CALLBACK(folder_changed_cb), rawbox);

        g_signal_connect(rawbox->dialog, "selection-changed",
                                    G_CALLBACK(selection_changed_cb), rawbox);
    }

again:

    assert(rawbox);
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

