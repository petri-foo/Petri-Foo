#include <gtk/gtk.h>
#include "sampletab.h"
#include "gui.h"
#include "waveform.h"
#include "mixer.h"
#include "sample-selector.h"
#include "sample-editor.h"
#include "patch_set_and_get.h"
#include "basic_combos.h"

enum
{
    SINGLESHOT,
    TRIM,
    LOOP,
    PINGPONG
};


static GtkWidget* parent_window = 0;


static void sample_tab_class_init(SampleTabClass* klass);
static void sample_tab_init(SampleTab* self);

G_DEFINE_TYPE(SampleTab, sample_tab, GTK_TYPE_VBOX)


static void sample_tab_class_init(SampleTabClass* klass)
{
    sample_tab_parent_class = g_type_class_peek_parent(klass);
}


static void update_file_button(SampleTab* self)
{
    char* name;
    char* base;

    name = patch_get_sample_name(self->patch);

    if (*name == '\0')
    {
        gtk_label_set_text(GTK_LABEL(self->file_label), "Load Sample");
    }
    else
    {
        base = g_path_get_basename(name);
        gtk_label_set_text(GTK_LABEL(self->file_label), base);
        g_free(base);
    }
    g_free(name);
}


static void update_to_end_check(SampleTab* self)
{
    PatchPlayMode mode = patch_get_play_mode(self->patch);
    int index = basic_combo_get_active(self->mode_opt);

    if (index != -1)
    {
        switch(index)
        {
        case LOOP:
        case PINGPONG:
            gtk_widget_set_sensitive(self->to_end_check, TRUE);
            break;
        default:
            gtk_widget_set_sensitive(self->to_end_check, FALSE);
            break;
        }
    }

    if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(self->to_end_check)))
        mode |= PATCH_PLAY_TO_END;
    else
        mode &= ~PATCH_PLAY_TO_END;

    patch_set_play_mode(self->patch, mode);
}


static gboolean
waveform_cb(GtkWidget* wf, GdkEventButton* event, SampleTab* self)
{
    (void)wf;
    if (event->button == 1)
    {   /* don't open the sample-editor if there is no sample! */
        if (patch_get_frames (self->patch))
            sample_editor_show(self->patch);
    }
    return FALSE;
}


static void set_mode(SampleTab* self)
{
    PatchPlayMode mode = 0;

    switch(basic_combo_get_active(self->mode_opt))
    {
    case TRIM:      mode = PATCH_PLAY_TRIM;                         break;
    case LOOP:      mode = PATCH_PLAY_LOOP;                         break;
    case PINGPONG:  mode = PATCH_PLAY_PINGPONG | PATCH_PLAY_LOOP;   break;
    default:        mode = PATCH_PLAY_SINGLESHOT;                   break;
    }

    if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(
                                                    self->reverse_check)))
        mode |= PATCH_PLAY_REVERSE;
    else
        mode |= PATCH_PLAY_FORWARD;

    if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(
                                                    self->to_end_check)))
        mode |= PATCH_PLAY_TO_END;
    else
        mode &= ~PATCH_PLAY_TO_END;

    update_to_end_check(self);
    patch_set_play_mode(self->patch, mode);
}


static void to_end_cb(GtkWidget* opt, SampleTab* self)
{
    (void)opt;
    set_mode(self);
}

static void mode_cb(GtkWidget* opt, SampleTab* self)
{
    (void)opt;
    set_mode(self);
}


static void reverse_cb(GtkWidget* button, SampleTab* self)
{
    (void)button;
    set_mode(self);
}


static void file_cb(GtkButton* button, SampleTab* self)
{
    GtkWidget* window = gtk_widget_get_toplevel(GTK_WIDGET(button));

    if (!gtk_widget_is_toplevel(window))
    {
        debug("failed to discover top-level window\n");
    }

    sample_selector_show(self->patch, window);
    update_file_button(self);
    gtk_widget_queue_draw(self->waveform);
}


static void connect(SampleTab* self)
{
    g_signal_connect(G_OBJECT(self->waveform), "button-press-event",
		     G_CALLBACK(waveform_cb), (gpointer) self);
    g_signal_connect(G_OBJECT(self->mode_opt), "changed",
		     G_CALLBACK(mode_cb), (gpointer) self);
    g_signal_connect(G_OBJECT(self->reverse_check), "toggled",
		     G_CALLBACK(reverse_cb), (gpointer) self);
    g_signal_connect(G_OBJECT(self->file_button), "clicked",
		     G_CALLBACK(file_cb), (gpointer) self);
    g_signal_connect(G_OBJECT(self->to_end_check), "toggled",
		     G_CALLBACK(to_end_cb), (gpointer) self);
}


static void block(SampleTab* self)
{
    g_signal_handlers_block_by_func(self->mode_opt, mode_cb, self);
    g_signal_handlers_block_by_func(self->reverse_check, reverse_cb, self);
    g_signal_handlers_block_by_func(self->to_end_check, to_end_cb, self);
}


static void unblock(SampleTab* self)
{
    g_signal_handlers_unblock_by_func(self->mode_opt, mode_cb, self);
    g_signal_handlers_unblock_by_func(self->reverse_check, reverse_cb, self);
    g_signal_handlers_unblock_by_func(self->to_end_check, to_end_cb, self);
}


inline static GtkWidget* file_button_new(SampleTab* self)
{
    GtkWidget* button;
    GtkWidget* hbox;
    GtkWidget* vsep;
    GtkWidget* image;

    button = gtk_button_new();
    hbox = gtk_hbox_new(FALSE, 0);
    self->file_label = gtk_label_new("Load File");
    vsep = gtk_vseparator_new();
    image = gtk_image_new_from_file(PIXMAPSDIR "open.png");

    gtk_box_pack_start(GTK_BOX(hbox), self->file_label, TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(hbox), vsep, FALSE, FALSE, GUI_SPACING);
    gtk_box_pack_start(GTK_BOX(hbox), image, FALSE, FALSE, 0);
    gtk_container_add(GTK_CONTAINER(button), hbox);

    gtk_misc_set_alignment(GTK_MISC(self->file_label), 0.0, 0.5);
    
    gtk_widget_show(hbox);
    gtk_widget_show(self->file_label);
    gtk_widget_show(vsep);
    gtk_widget_show(image);

    return button;
}


static void sample_tab_init(SampleTab* self)
{
    const char* mode_str[] = {
        "Single Shot", "Trim", "Loop", "Ping Pong", 0
    };

    GtkBox* box = GTK_BOX(self);
    GtkWidget* section;
    GtkWidget* hbox;
    GtkWidget* vbox;
    GtkWidget* pad;

    self->patch = -1;
    gtk_container_set_border_width(GTK_CONTAINER(self), GUI_BORDERSPACE);

    /* sample section */
    section = gui_section_new("Sample", &hbox);
    gtk_box_pack_start(box, section, FALSE, FALSE, 0);
    gtk_widget_show(section);

    /* vbox */
    vbox = gtk_vbox_new(FALSE, GUI_SPACING);
    gtk_box_pack_start(GTK_BOX(hbox), vbox, TRUE, TRUE, 0);
    gtk_widget_show(vbox);

    /* file button */
    self->file_button = file_button_new(self);
    gtk_box_pack_start(GTK_BOX(vbox), self->file_button, TRUE, TRUE, 0);
    gtk_widget_show(self->file_button); 

    /* waveform preview */
    self->waveform = waveform_new();
    waveform_set_patch(         WAVEFORM(self->waveform), self->patch);
    waveform_set_size(          WAVEFORM(self->waveform), 256, 64);
    waveform_set_interactive(   WAVEFORM(self->waveform), FALSE);
    gtk_box_pack_start(GTK_BOX(vbox), self->waveform, TRUE, TRUE, 0);
    gtk_widget_show(self->waveform);
    sample_editor_set_thumb(self->waveform);

    /* section padding */
    pad = gui_vpad_new(GUI_SECSPACE);
    gtk_box_pack_start(box, pad, FALSE, FALSE, 0);
    gtk_widget_show(pad);
    
    /* playback section */
    section = gui_section_new("Playback", &hbox);
    gtk_box_pack_start(box, section, FALSE, FALSE, 0);
    gtk_widget_show(section);

    /* mode option menu */
    self->mode_opt = basic_combo_create(mode_str);
    gtk_box_pack_start(GTK_BOX(hbox), self->mode_opt, TRUE, TRUE, 0);
    gtk_widget_show(self->mode_opt);

    /* pad */
    pad = gui_hpad_new(GUI_SPACING);
    gtk_box_pack_start(GTK_BOX(hbox), pad, FALSE, FALSE, 0);
    gtk_widget_show(pad);
    
    /* reverse check */
    self->reverse_check = gtk_check_button_new_with_label("Reverse");
    gtk_box_pack_start(GTK_BOX(hbox), self->reverse_check, TRUE, TRUE, 0);
    gtk_widget_show(self->reverse_check);

    /* to end check */
    self->to_end_check = gtk_check_button_new_with_label("To End");
    gtk_box_pack_start(GTK_BOX(hbox), self->to_end_check, TRUE, TRUE, 0);
    gtk_widget_show(self->to_end_check);

    connect(self);
}


GtkWidget* sample_tab_new(void)
{
    return (GtkWidget*) g_object_new(SAMPLE_TAB_TYPE, NULL);
}


void sample_tab_set_patch(SampleTab* self, int patch)
{
    GtkTreeIter iter;

    self->patch = patch;
    waveform_set_patch(WAVEFORM(self->waveform), patch);
    block(self);

    if (patch < 0)
    {
        gtk_label_set_text(GTK_LABEL(self->file_label), "Load Sample");
    }
    else
    {
        int mode;
        update_file_button(self);
        mode = patch_get_play_mode(patch);
        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(self->reverse_check),
                                                mode & PATCH_PLAY_REVERSE);
        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(self->to_end_check),
                                                mode & PATCH_PLAY_TO_END);
        if (mode & PATCH_PLAY_TRIM)
            mode = TRIM;
        else if (mode & PATCH_PLAY_PINGPONG)
            mode = PINGPONG;
        else if (mode & PATCH_PLAY_LOOP)
            mode = LOOP;
        else
            mode = SINGLESHOT;

        if (basic_combo_get_iter_at_index(self->mode_opt, mode, &iter))
        {
            gtk_combo_box_set_active_iter(GTK_COMBO_BOX(self->mode_opt),
                                                                    &iter);
        }
    }

    update_to_end_check(self);
    unblock(self);
}


void sample_tab_set_parent_window(SampleTab* self, GtkWidget* window)
{
    (void)self;
    parent_window = window;
}
