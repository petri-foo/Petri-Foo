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
#include <phat/phat.h>
#include "mastersection.h"
#include "petri-foo.h"
#include "gui.h"
#include "mixer.h"



G_DEFINE_TYPE(MasterSection, master_section, GTK_TYPE_VBOX);



static void master_section_class_init(MasterSectionClass* klass)
{
    master_section_parent_class = g_type_class_peek_parent(klass);
}


static void amplitude_changed_cb(PhatFanSlider* slider, gpointer data)
{
    (void)data;
    float val;
    val = phat_fan_slider_get_value(slider);
    mixer_set_amplitude(val);
}


static void master_section_init(MasterSection* self)
{
    GtkBox* box = GTK_BOX(self);
    GtkWidget* hbox;
    GtkWidget* label;
    GtkWidget* pad;

    gtk_box_set_spacing(box, GUI_SPACING);
    
    /* amplitude */
    label = gtk_label_new(NULL);
    self->amplitude_fan =
        phat_hfan_slider_new_with_range(DEFAULT_AMPLITUDE, 0.0, 1.0, 0.1);
    hbox = gtk_hbox_new(FALSE, GUI_TEXTSPACE);

    gtk_label_set_markup(GTK_LABEL(label), "<b>Master</b>");
    gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(hbox), self->amplitude_fan, TRUE, TRUE, 0);
    gtk_box_pack_start(box, hbox, FALSE, FALSE, 0);

    gtk_widget_show(label);
    gtk_widget_show(self->amplitude_fan);
    gtk_widget_show(hbox);

    /* pad */
    pad = gui_vpad_new(GUI_SPACING/2);
    gtk_box_pack_start(box, pad, FALSE, FALSE, 0);
    gtk_widget_show(pad);
    
    /* signal */
    g_signal_connect(self->amplitude_fan, "value-changed",
		     G_CALLBACK(amplitude_changed_cb), NULL);
}


GtkWidget* master_section_new(void)
{
    return (GtkWidget*) g_object_new(MASTER_SECTION_TYPE, NULL);
}


void master_section_update(MasterSection* self)
{
    float amplitude;

    amplitude = mixer_get_amplitude();

    g_signal_handlers_block_by_func(self->amplitude_fan, 
                                    (gpointer)amplitude_changed_cb, NULL);

    phat_fan_slider_set_value(PHAT_FAN_SLIDER(self->amplitude_fan), amplitude);
    g_signal_handlers_unblock_by_func(self->amplitude_fan, amplitude_changed_cb, NULL);
}
    
