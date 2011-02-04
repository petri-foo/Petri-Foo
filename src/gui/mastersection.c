#include <gtk/gtk.h>
#include <phat/phat.h>
#include "mastersection.h"
#include "petri-foo.h"
#include "gui.h"
#include "mixer.h"

static GtkVBoxClass* parent_class;

static void master_section_class_init(MasterSectionClass* klass);
static void master_section_init(MasterSection* self);


GType master_section_get_type(void)
{
    static GType type = 0;

    if (!type)
    {
	static const GTypeInfo info =
	    {
		sizeof (MasterSectionClass),
		NULL,
		NULL,
		(GClassInitFunc) master_section_class_init,
		NULL,
		NULL,
		sizeof (MasterSection),
		0,
		(GInstanceInitFunc) master_section_init,
	    };

	type = g_type_register_static(GTK_TYPE_VBOX, "MasterSection", &info, 0);
    }

    return type;
}


static void master_section_class_init(MasterSectionClass* klass)
{
    parent_class = g_type_class_peek_parent(klass);
}


static void volume_changed_cb(PhatFanSlider* slider, gpointer data)
{
    float val;

    val = phat_fan_slider_get_value(slider);
    mixer_set_volume(val);
}


static void master_section_init(MasterSection* self)
{
    GtkBox* box = GTK_BOX(self);
    GtkWidget* hbox;
    GtkWidget* label;
    GtkWidget* pad;

    gtk_box_set_spacing(box, GUI_SPACING);
    
    /* volume */
    label = gtk_label_new(NULL);
    self->volume_fan = phat_hfan_slider_new_with_range(DEFAULT_VOLUME, 0.0, 1.0, 0.1);
    hbox = gtk_hbox_new(FALSE, GUI_TEXTSPACE);

    gtk_label_set_markup(GTK_LABEL(label), "<b>Master</b>");
    gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(hbox), self->volume_fan, TRUE, TRUE, 0);
    gtk_box_pack_start(box, hbox, FALSE, FALSE, 0);

    gtk_widget_show(label);
    gtk_widget_show(self->volume_fan);
    gtk_widget_show(hbox);

    /* pad */
    pad = gui_vpad_new(GUI_SPACING/2);
    gtk_box_pack_start(box, pad, FALSE, FALSE, 0);
    gtk_widget_show(pad);
    
    /* signal */
    g_signal_connect(self->volume_fan, "value-changed",
		     G_CALLBACK(volume_changed_cb), NULL);
}


GtkWidget* master_section_new(void)
{
    return (GtkWidget*) g_object_new(MASTER_SECTION_TYPE, NULL);
}


void master_section_update(MasterSection* self)
{
    float volume;

    volume = mixer_get_volume();

    g_signal_handlers_block_by_func(self->volume_fan, volume_changed_cb, NULL);
    phat_fan_slider_set_value(PHAT_FAN_SLIDER(self->volume_fan), volume);
    g_signal_handlers_unblock_by_func(self->volume_fan, volume_changed_cb, NULL);
}
    
