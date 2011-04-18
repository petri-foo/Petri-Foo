#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <gtk/gtk.h>

#include "basic_combos.h"
#include "gui.h"
#include "petri-foo.h"
#include "mixer.h"
#include "patch_set_and_get.h"
#include "patch_util.h"
#include "names.h"
#include "sample-selector.h"
#include "sample.h"


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
    GtkWidget* check;
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

    if (!name)
        goto fail;

    debug("about to load sample...\n");

    if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(rb->check)))
    {
        int samplerate = gtk_spin_button_get_value_as_int(
                                    GTK_SPIN_BUTTON(rb->samplerate));

        err = patch_sample_load(patch, name,    samplerate,
                                                rb->channels,
                                                get_format(rb));
        if (err)
            goto fail;
    }
    else
    {   /* don't repeat load sample */
        if (strcmp(name, patch_get_sample_name(patch)) == 0)
            return;

        if ((err = patch_sample_load(patch, name, 0, 0, 0)))
            goto fail;
    }

    debug ("Successfully loaded sample %s\n", name);
    return;

fail:
    if (!name)
    {
        errmsg("no file selected\n");
        msg = gtk_message_dialog_new(GTK_WINDOW(rb->dialog),
                                     GTK_DIALOG_DESTROY_WITH_PARENT,
                                     GTK_MESSAGE_ERROR, GTK_BUTTONS_CLOSE,
                                     "No file selected");
    }
    else
    {
        errmsg ("Failed to load sample %s for patch %d (%s)\n", name,
                                            patch, patch_strerror(err));
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

    if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(rb->check)))
    {
        int samplerate = gtk_spin_button_get_value_as_int(
                                    GTK_SPIN_BUTTON(rb->samplerate));

        mixer_preview(name, samplerate,  rb->channels, get_format(rb));
    }
    else
    {
        mixer_preview(name, 0, 0, 0);
    }

    return;
}


static void cb_cancel(void)
{
    debug("cancelling sample load...\n");

    if (!last_sample->filename)
    {
        debug("no sample to restore, just unloading...\n");
        patch_sample_unload(patch);
        return;
    }

    if (strcmp(patch_get_sample_name(patch), last_sample->filename) != 0)
    {
        debug("restoring sample:%s\n", last_sample->filename);
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
        {
            debug("setting raw channels to two\n");
            rb->channels = 2;
        }
        else if (tog == GTK_TOGGLE_BUTTON(rb->mono))
        {
            debug("setting raw channels to one\n");
            rb->channels = 1;
        }
        else if (tog == GTK_TOGGLE_BUTTON(rb->big_endian))
        {
            debug("setting raw endianness to big\n");
            rb->endian = 2;
        }
        else if (tog == GTK_TOGGLE_BUTTON(rb->little_endian))
        {
            debug("setting raw endianness to little\n");
            rb->endian = 1;
        }
        else if (tog == GTK_TOGGLE_BUTTON(rb->file_endian))
        {
            debug("setting raw endianness to file\n");
            rb->endian = 0;
        }
        else
        {
            debug("unknown raw data toggle\n");
        }
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
    GtkWidget* tmp;
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

    rb->check = gtk_check_button_new_with_label("Raw/Headerless");
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(rb->check), FALSE);
    gtk_widget_show(rb->check);
    gtk_box_pack_start(GTK_BOX(rb->box), rb->check, TRUE, TRUE, 0);
    g_signal_connect(GTK_OBJECT(rb->check), "toggled",
                                            G_CALLBACK(raw_toggled_cb), rb);
    rb->table = gtk_table_new(5, 2, FALSE);
    gtk_box_pack_start(GTK_BOX(rb->box), rb->table, TRUE, TRUE, 0);

    t = GTK_TABLE(rb->table);

    rb->format = basic_combo_id_name_create(names_sample_raw_get());
    gtk_table_attach_defaults(t, rb->format, 0, 2, y, y + 1);

    tmp = gtk_label_new("Sample rate:");
    gtk_table_attach_defaults(t, tmp, 2, 4, y, y + 1);

    rb->sr_adj = (GtkAdjustment*)
        gtk_adjustment_new(44100, 8000, 192000, 100, 1000, 0);
    rb->samplerate = gtk_spin_button_new(rb->sr_adj, 1.0, 0);
    gtk_table_attach_defaults(t, rb->samplerate, 3, 4, y, y + 1);

    ++y;

    rb->mono = gtk_radio_button_new(NULL);
    tmp = gtk_label_new("Mono");
    gtk_container_add(GTK_CONTAINER(rb->mono), tmp);
    rb->stereo =gtk_radio_button_new_from_widget(
                                GTK_RADIO_BUTTON(rb->mono));
    tmp = gtk_label_new("Stereo");
    gtk_container_add(GTK_CONTAINER(rb->stereo), tmp);

    gtk_table_attach_defaults(t, rb->mono, 0, 1, y, y + 1);
    gtk_table_attach_defaults(t, rb->stereo, 1, 2, y, y + 1);

    rb->file_endian = gtk_radio_button_new(NULL);
    tmp = gtk_label_new("File Endian");
    gtk_container_add(GTK_CONTAINER(rb->file_endian), tmp);

    rb->little_endian = gtk_radio_button_new_from_widget(
                                GTK_RADIO_BUTTON(rb->file_endian));
    tmp = gtk_label_new("Little Endian");
    gtk_container_add(GTK_CONTAINER(rb->little_endian), tmp);

    rb->big_endian = gtk_radio_button_new_from_widget(
                                GTK_RADIO_BUTTON(rb->little_endian));
    tmp = gtk_label_new("Big Endian");
    gtk_container_add(GTK_CONTAINER(rb->big_endian), tmp);

    gtk_table_attach_defaults(t, rb->file_endian, 2, 3, y, y + 1);
    gtk_table_attach_defaults(t, rb->little_endian, 3, 4, y, y + 1);
    gtk_table_attach_defaults(t, rb->big_endian, 4, 5, y, y + 1);

    g_signal_connect(GTK_OBJECT(rb->mono), "toggled",
                                G_CALLBACK(raw_toggles_toggled_cb), rb);
    g_signal_connect(GTK_OBJECT(rb->stereo), "toggled",
                                G_CALLBACK(raw_toggles_toggled_cb), rb);
    g_signal_connect(GTK_OBJECT(rb->file_endian), "toggled",
                                G_CALLBACK(raw_toggles_toggled_cb), rb);
    g_signal_connect(GTK_OBJECT(rb->little_endian), "toggled",
                                G_CALLBACK(raw_toggles_toggled_cb), rb);
    g_signal_connect(GTK_OBJECT(rb->big_endian), "toggled",
                                G_CALLBACK(raw_toggles_toggled_cb), rb);

    /* table should be hidden by default */
    gtk_widget_hide(rb->table);

    return rb;
}


int sample_selector_show(int id, GtkWidget* parent_window,
                                 SampleTab* sampletab)
{
    GtkWidget* dialog;
    GtkWidget* load;
    GtkWidget* preview;
    GtkWidget* tmp;
    GtkWidget* vbox;
    raw_box* rawbox;

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
    }

    load = gtk_dialog_add_button(GTK_DIALOG(dialog),
                                "Load", RESPONSE_LOAD);

    preview = gtk_dialog_add_button(GTK_DIALOG(dialog),
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

