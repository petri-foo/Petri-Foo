#include <gtk/gtk.h>
#include <phat/phat.h>
#include "channelsection.h"
#include "gui.h"
#include "patchlist.h"
#include "midi.h"
#include "patch_set_and_get.h"



G_DEFINE_TYPE(ChannelSection, channel_section, GTK_TYPE_VBOX)

static void channel_section_destroy (GtkObject * object);


static void channel_section_class_init(ChannelSectionClass* klass)
{
    GtkObjectClass *object_class = GTK_OBJECT_CLASS(klass);

    object_class->destroy = channel_section_destroy;
    channel_section_parent_class = g_type_class_peek_parent(klass);
}


static void channel_section_destroy(GtkObject* object)
{
    GtkObjectClass* klass = GTK_OBJECT_CLASS(channel_section_parent_class);

    if (klass->destroy)
        klass->destroy(object);
}


static void channel_cb(PhatSliderButton* button, ChannelSection* self)
{
    int channel = (int)phat_slider_button_get_value(button);
    PatchList* list = gui_get_patch_list();

    patch_set_channel(self->patch, channel-1);
    patch_list_update(list, patch_list_get_current_patch(list),
                                                PATCH_LIST_PATCH);
}


static void connect(ChannelSection* self)
{
    g_signal_connect(G_OBJECT(self->chan_sb), "value-changed",
                        G_CALLBACK(channel_cb), (gpointer) self);
}


static void channel_section_init(ChannelSection* self)
{
    GtkBox* box = GTK_BOX(self);
    GtkWidget* hbox;
    GtkWidget* label;
    GtkWidget* pad;

    self->patch = -1;

    /* hbox */
    hbox = gtk_hbox_new(FALSE, 0);
    gtk_box_pack_start(box, hbox, FALSE, FALSE, 0);
    gtk_widget_show(hbox);
    
    /* channel label */
    label = gtk_label_new("Channel:");
    gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 0);
    gtk_widget_show(label);

    /* hpad */
    pad = gui_hpad_new(GUI_TEXTSPACE);
    gtk_box_pack_start(GTK_BOX(hbox), pad, FALSE, FALSE, 0);
    gtk_widget_show(pad);

    /* channel sliderbutton */
    self->chan_sb = phat_slider_button_new_with_range(1, 1, MIDI_CHANS,1,0);
    phat_slider_button_set_threshold(PHAT_SLIDER_BUTTON(self->chan_sb),
                                                        GUI_THRESHOLD);
    gtk_box_pack_start(GTK_BOX(hbox), self->chan_sb, TRUE, TRUE, 0);
    gtk_widget_show(self->chan_sb);

    /* done */
    connect(self);
}


static void block(ChannelSection* self)
{
    g_signal_handlers_block_by_func(self->chan_sb, channel_cb, self);
}


static void unblock(ChannelSection* self)
{
    g_signal_handlers_unblock_by_func(self->chan_sb, channel_cb, self);
}


static void set_sensitive(ChannelSection* self, gboolean val)
{
    gtk_widget_set_sensitive(self->chan_sb, val);
}


GtkWidget* channel_section_new(void)
{
    return (GtkWidget*) g_object_new(CHANNEL_SECTION_TYPE, NULL);
}


void channel_section_set_patch(ChannelSection* self, int patch)
{
    int channel;

    self->patch = patch;

    if (patch < 0)
    {
	set_sensitive(self, FALSE);
    }
    else
    {
	set_sensitive(self, TRUE);

	channel = patch_get_channel(patch);

	block(self);

	phat_slider_button_set_value(PHAT_SLIDER_BUTTON(self->chan_sb), channel+1);
	
	unblock(self);
    }
}


int channel_section_get_channel(ChannelSection* self)
{
    if (self->patch < 0)
	return 0;
    
    return patch_get_channel(self->patch);
}
