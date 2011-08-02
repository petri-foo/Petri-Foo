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


#include <gtk/gtk.h>

#include "phin.h"

#include "channelsection.h"
#include "gui.h"
#include "patchlist.h"
#include "midi.h"
#include "patch_set_and_get.h"



G_DEFINE_TYPE(ChannelSection, channel_section, GTK_TYPE_VBOX)


static void channel_section_class_init(ChannelSectionClass* klass)
{
    channel_section_parent_class = g_type_class_peek_parent(klass);
}


static void channel_cb(PhinSliderButton* button, ChannelSection* self)
{
    int channel = (int)phin_slider_button_get_value(button);
    PatchList* list = gui_get_patch_list();

    patch_set_channel(self->patch, channel-1);
    patch_list_update(list, patch_list_get_current_patch(list),
                                                PATCH_LIST_PATCH);
}


static void lower_vel_cb(PhinSliderButton* button, ChannelSection* self)
{
    int lower_vel = (int)phin_slider_button_get_value(button);
    PatchList* list = gui_get_patch_list();

    patch_set_lower_vel(self->patch, lower_vel);
    patch_list_update(list, patch_list_get_current_patch(list),
                                                PATCH_LIST_PATCH);
}


static void upper_vel_cb(PhinSliderButton* button, ChannelSection* self)
{
    int upper_vel = (int)phin_slider_button_get_value(button);
    PatchList* list = gui_get_patch_list();

    patch_set_upper_vel(self->patch, upper_vel);
    patch_list_update(list, patch_list_get_current_patch(list),
                                                PATCH_LIST_PATCH);
}


static void connect(ChannelSection* self)
{
    g_signal_connect(G_OBJECT(self->chan_sb), "value-changed",
                        G_CALLBACK(channel_cb), (gpointer) self);
    g_signal_connect(G_OBJECT(self->lower_vel_sb), "value-changed",
                        G_CALLBACK(lower_vel_cb), (gpointer) self);
    g_signal_connect(G_OBJECT(self->upper_vel_sb), "value-changed",
                        G_CALLBACK(upper_vel_cb), (gpointer) self);
}


static void channel_section_init(ChannelSection* self)
{
    GtkBox* box = GTK_BOX(self);
    GtkWidget* table = gtk_table_new( 3, 2, 1);
    GtkWidget* lbl_chan = gtk_label_new("Channel");
    GtkWidget* lbl_lower = gtk_label_new("Lower Vel.");
    GtkWidget* lbl_upper = gtk_label_new("Upper Vel.");
    
    self->patch = -1;

    gtk_table_attach_defaults(GTK_TABLE(table),lbl_chan,0,1,0,1);
    gtk_widget_show(lbl_chan);
    gtk_table_attach_defaults(GTK_TABLE(table),lbl_lower,0,1,1,2);
    gtk_widget_show(lbl_lower);
    gtk_table_attach_defaults(GTK_TABLE(table),lbl_upper,0,1,2,3);
    gtk_widget_show(lbl_upper);

    /* channel sliderbutton */
    self->chan_sb = phin_slider_button_new_with_range(1, 1, MIDI_CHANS,1,0);
    phin_slider_button_set_threshold(PHIN_SLIDER_BUTTON(self->chan_sb),
                                                        GUI_THRESHOLD);
    gtk_table_attach_defaults(GTK_TABLE(table),self->chan_sb,1,2,0,1);
    gtk_widget_show(self->chan_sb);

    /* lower velocity slide */ 
    self->lower_vel_sb = phin_slider_button_new_with_range(0, 0, 127, 1, 0);
    phin_slider_button_set_threshold(PHIN_SLIDER_BUTTON(self->lower_vel_sb),
                                                        GUI_THRESHOLD);
    gtk_table_attach_defaults(GTK_TABLE(table),self->lower_vel_sb,1,2,1,2);
    gtk_widget_show(self->lower_vel_sb);

    /* upper velocity slider */
    self->upper_vel_sb = phin_slider_button_new_with_range(127, 0, 127, 1, 0);
    phin_slider_button_set_threshold(PHIN_SLIDER_BUTTON(self->upper_vel_sb),
                                                        GUI_THRESHOLD);
    gtk_table_attach_defaults(GTK_TABLE(table),self->upper_vel_sb,1,2,2,3);
    gtk_widget_show(self->upper_vel_sb);
    
    gui_pack(box, table);
    
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
    gtk_widget_set_sensitive(self->lower_vel_sb, val);
    gtk_widget_set_sensitive(self->upper_vel_sb, val);
}


GtkWidget* channel_section_new(void)
{
    return (GtkWidget*) g_object_new(CHANNEL_SECTION_TYPE, NULL);
}


void channel_section_set_patch(ChannelSection* self, int patch)
{
    int channel, lower_vel, upper_vel;

    self->patch = patch;

    if (patch < 0)
        set_sensitive(self, FALSE);
    else
    {
        set_sensitive(self, TRUE);
        channel = patch_get_channel(patch);
        lower_vel = patch_get_lower_vel(patch);
        upper_vel = patch_get_upper_vel(patch);

        block(self);
        phin_slider_button_set_value(PHIN_SLIDER_BUTTON(self->chan_sb),
                                                                channel+1);
        phin_slider_button_set_value(PHIN_SLIDER_BUTTON(self->lower_vel_sb),
                                                                lower_vel);
        phin_slider_button_set_value(PHIN_SLIDER_BUTTON(self->upper_vel_sb),
                                                                upper_vel);
        unblock(self);
    }
}


int channel_section_get_channel(ChannelSection* self)
{
    return (self->patch < 0) ? 0 : patch_get_channel(self->patch);
}
