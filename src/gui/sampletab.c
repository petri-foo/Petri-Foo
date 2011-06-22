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


typedef struct _SampleTabPrivate SampleTabPrivate;

#define SAMPLE_TAB_GET_PRIVATE(obj)     \
    (G_TYPE_INSTANCE_GET_PRIVATE((obj), \
        SAMPLE_TAB_TYPE, SampleTabPrivate))

struct _SampleTabPrivate
{
    int patch;
    GtkWidget* waveform;
    GtkWidget* mode_opt;
    GtkWidget* file_label;
    GtkWidget* file_button;
    GtkWidget* reverse_check;
    GtkWidget* to_end_check;
};


enum { WAVEFORM_WIDTH = 256, WAVEFORM_HEIGHT = 96 };


G_DEFINE_TYPE(SampleTab, sample_tab, GTK_TYPE_VBOX);


static void sample_tab_class_init(SampleTabClass* klass)
{
    GtkObjectClass *object_class = GTK_OBJECT_CLASS(klass);
    sample_tab_parent_class = g_type_class_peek_parent(klass);
    g_type_class_add_private(object_class, sizeof(SampleTabPrivate));
}


static void update_file_button(SampleTabPrivate* p)
{
    const char* name;
    char* base;

    name = patch_get_sample_name(p->patch);

    if (!name)
    {
        gtk_label_set_text(GTK_LABEL(p->file_label), "Load Sample");
    }
    else
    {
        base = g_path_get_basename(name);
        gtk_label_set_text(GTK_LABEL(p->file_label), base);
        g_free(base);
    }
}


static void update_to_end_check(SampleTabPrivate* p)
{
    PatchPlayMode mode = patch_get_play_mode(p->patch);
    int index = basic_combo_get_active(p->mode_opt);

    if (index != -1)
    {
        switch(index)
        {
        case LOOP:
        case PINGPONG:
            gtk_widget_set_sensitive(p->to_end_check, TRUE);
            break;
        default:
            gtk_widget_set_sensitive(p->to_end_check, FALSE);
            break;
        }
    }

    if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(p->to_end_check)))
        mode |= PATCH_PLAY_TO_END;
    else
        mode &= ~PATCH_PLAY_TO_END;

    patch_set_play_mode(p->patch, mode);
}


static gboolean
waveform_cb(GtkWidget* wf, GdkEventButton* event, SampleTabPrivate* p)
{
    (void)wf;
    if (event->button == 1)
    {   /* don't open the sample-editor if there is no sample! */
        if (patch_get_frames(p->patch))
            sample_editor_show(p->patch);
    }
    return FALSE;
}


static void set_mode(SampleTabPrivate* p)
{
    PatchPlayMode mode = 0;

    switch(basic_combo_get_active(p->mode_opt))
    {
    case TRIM:      mode = PATCH_PLAY_TRIM;                         break;
    case LOOP:      mode = PATCH_PLAY_LOOP;                         break;
    case PINGPONG:  mode = PATCH_PLAY_PINGPONG | PATCH_PLAY_LOOP;   break;
    default:        mode = PATCH_PLAY_SINGLESHOT;                   break;
    }

    if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(p->reverse_check)))
        mode |= PATCH_PLAY_REVERSE;
    else
        mode |= PATCH_PLAY_FORWARD;

    if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(p->to_end_check)))
        mode |= PATCH_PLAY_TO_END;
    else
        mode &= ~PATCH_PLAY_TO_END;

    update_to_end_check(p);
    patch_set_play_mode(p->patch, mode);
}


static void to_end_cb(GtkWidget* combo, SampleTabPrivate* p)
{
    (void)combo;
    set_mode(p);
}

static void mode_cb(GtkWidget* combo, SampleTabPrivate* p)
{
    (void)combo;
    set_mode(p);
}


static void reverse_cb(GtkWidget* check, SampleTabPrivate* p)
{
    (void)check;
    set_mode(p);
}


static void file_cb(GtkButton* button, SampleTab* self)
{
    SampleTabPrivate* p = SAMPLE_TAB_GET_PRIVATE(self);
    GtkWidget* window = gtk_widget_get_toplevel(GTK_WIDGET(button));

    if (!gtk_widget_is_toplevel(window))
    {
        debug("failed to discover top-level window\n");
    }

    sample_selector_show(p->patch, window, self);
    update_file_button(p);
    gtk_widget_queue_draw(p->waveform);
    sample_editor_update();
}


static void connect(SampleTab* self)
{
    SampleTabPrivate* p = SAMPLE_TAB_GET_PRIVATE(self);

    g_signal_connect(G_OBJECT(p->waveform), "button-press-event",
                            G_CALLBACK(waveform_cb), (gpointer)p);

    g_signal_connect(G_OBJECT(p->mode_opt), "changed",
                            G_CALLBACK(mode_cb), (gpointer)p);
    g_signal_connect(G_OBJECT(p->reverse_check), "toggled",
                            G_CALLBACK(reverse_cb), (gpointer)p);
    g_signal_connect(G_OBJECT(p->file_button), "clicked",
                            G_CALLBACK(file_cb), (gpointer)self);
    g_signal_connect(G_OBJECT(p->to_end_check), "toggled",
                            G_CALLBACK(to_end_cb), (gpointer)p);

}


static void block(SampleTabPrivate* p)
{
    g_signal_handlers_block_by_func(p->mode_opt,        mode_cb,    p);
    g_signal_handlers_block_by_func(p->reverse_check,   reverse_cb, p);
    g_signal_handlers_block_by_func(p->to_end_check,    to_end_cb,  p);
}


static void unblock(SampleTabPrivate* p)
{
    g_signal_handlers_unblock_by_func(p->mode_opt,      mode_cb,    p);
    g_signal_handlers_unblock_by_func(p->reverse_check, reverse_cb, p);
    g_signal_handlers_unblock_by_func(p->to_end_check,  to_end_cb,  p);
}


static GtkWidget* file_button_new(SampleTabPrivate* p)
{
    GtkWidget* button;
    GtkWidget* hbox;
    GtkWidget* vsep;
    GtkWidget* image;

    button = gtk_button_new();
    hbox = gtk_hbox_new(FALSE, 0);
    p->file_label = gtk_label_new("Load File");
    vsep = gtk_vseparator_new();
    image = gtk_image_new_from_file(PIXMAPSDIR "open.png");

    gtk_box_pack_start(GTK_BOX(hbox), p->file_label, TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(hbox), vsep, FALSE, FALSE, GUI_SPACING);
    gtk_box_pack_start(GTK_BOX(hbox), image, FALSE, FALSE, 0);
    gtk_container_add(GTK_CONTAINER(button), hbox);

    gtk_misc_set_alignment(GTK_MISC(p->file_label), 0.0, 0.5);
    
    gtk_widget_show(hbox);
    gtk_widget_show(p->file_label);
    gtk_widget_show(vsep);
    gtk_widget_show(image);

    return button;
}


static void sample_tab_init(SampleTab* self)
{
    SampleTabPrivate* p = SAMPLE_TAB_GET_PRIVATE(self);
    const char* mode_str[] = {
        "Single Shot", "Trim", "Loop", "Ping Pong", 0
    };

    GtkBox* box = GTK_BOX(self);
    GtkWidget* section;
    GtkWidget* hbox;
    GtkWidget* vbox;
    GtkWidget* pad;

    gtk_container_set_border_width(GTK_CONTAINER(self), GUI_BORDERSPACE);

    p->patch = -1;

    /* sample section */
    section = gui_section_new("Sample", &hbox);
    gtk_box_pack_start(box, section, FALSE, FALSE, 0);
    gtk_widget_show(section);

    /* vbox */
    vbox = gtk_vbox_new(FALSE, GUI_SPACING);
    gtk_box_pack_start(GTK_BOX(hbox), vbox, TRUE, TRUE, 0);
    gtk_widget_show(vbox);

    /* file button */
    p->file_button = file_button_new(p);
    gtk_box_pack_start(GTK_BOX(vbox), p->file_button, TRUE, TRUE, 0);
    gtk_widget_show(p->file_button); 

    /* waveform preview */
    p->waveform = waveform_new();
    waveform_set_patch(         WAVEFORM(p->waveform), p->patch);

    waveform_set_size(          WAVEFORM(p->waveform), WAVEFORM_WIDTH,
                                                       WAVEFORM_HEIGHT);

    waveform_set_interactive(   WAVEFORM(p->waveform), FALSE);
    gtk_box_pack_start(GTK_BOX(vbox),    p->waveform,  TRUE, TRUE, 0);
    gtk_widget_show(p->waveform);
    sample_editor_set_thumb(p->waveform);

    /* section padding */
    pad = gui_vpad_new(GUI_SECSPACE);
    gtk_box_pack_start(box, pad, FALSE, FALSE, 0);
    gtk_widget_show(pad);
    
    /* playback section */
    section = gui_section_new("Playback", &hbox);
    gtk_box_pack_start(box, section, FALSE, FALSE, 0);
    gtk_widget_show(section);

    /* mode option menu */
    p->mode_opt = basic_combo_create(mode_str);
    gtk_box_pack_start(GTK_BOX(hbox), p->mode_opt, TRUE, TRUE, 0);
    gtk_widget_show(p->mode_opt);

    /* pad */
    pad = gui_hpad_new(GUI_SPACING);
    gtk_box_pack_start(GTK_BOX(hbox), pad, FALSE, FALSE, 0);
    gtk_widget_show(pad);
    
    /* reverse check */
    p->reverse_check = gtk_check_button_new_with_label("Reverse");
    gtk_box_pack_start(GTK_BOX(hbox), p->reverse_check, TRUE, TRUE, 0);
    gtk_widget_show(p->reverse_check);

    /* to end check */
    p->to_end_check = gtk_check_button_new_with_label("To End");
    gtk_box_pack_start(GTK_BOX(hbox), p->to_end_check, TRUE, TRUE, 0);
    gtk_widget_show(p->to_end_check);

    connect(self);
}


GtkWidget* sample_tab_new(void)
{
    return (GtkWidget*) g_object_new(SAMPLE_TAB_TYPE, NULL);
}


void sample_tab_set_patch(SampleTab* self, int patch)
{
    SampleTabPrivate* p = SAMPLE_TAB_GET_PRIVATE(self);
    GtkTreeIter iter;

    p->patch = patch;
    waveform_set_patch(WAVEFORM(p->waveform), patch);
    block(p);

    if (patch < 0)
    {
        gtk_label_set_text(GTK_LABEL(p->file_label), "Load Sample");
    }
    else
    {
        int mode;
        update_file_button(p);
        mode = patch_get_play_mode(patch);
        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(p->reverse_check),
                                                mode & PATCH_PLAY_REVERSE);
        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(p->to_end_check),
                                                mode & PATCH_PLAY_TO_END);
        if (mode & PATCH_PLAY_TRIM)
            mode = TRIM;
        else if (mode & PATCH_PLAY_PINGPONG)
            mode = PINGPONG;
        else if (mode & PATCH_PLAY_LOOP)
            mode = LOOP;
        else
            mode = SINGLESHOT;

        if (basic_combo_get_iter_at_index(p->mode_opt, mode, &iter))
        {
            gtk_combo_box_set_active_iter(GTK_COMBO_BOX(p->mode_opt),
                                                                    &iter);
        }
    }

    update_to_end_check(p);
    unblock(p);
}


void sample_tab_update_waveforms(SampleTab* self)
{
    SampleTabPrivate* p = SAMPLE_TAB_GET_PRIVATE(self);
    gtk_widget_queue_draw(p->waveform);
    sample_editor_update();
}
