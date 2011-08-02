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

#include "voicetab.h"
#include "gui.h"
#include "patch_set_and_get.h"
#include "bool_section.h"
#include "float_section.h"


typedef struct _VoiceTabPrivate VoiceTabPrivate;

#define VOICE_TAB_GET_PRIVATE(obj)      \
    (G_TYPE_INSTANCE_GET_PRIVATE((obj), \
        VOICE_TAB_TYPE, VoiceTabPrivate))

struct _VoiceTabPrivate
{
    int patch;
    guint refresh;
    GtkWidget* cut_sb;
    GtkWidget* cutby_sb;

    GtkWidget* mono_check;
    GtkWidget* legato_sect;

    GtkWidget* porta_sect;
    GtkWidget* time_sect;
};


G_DEFINE_TYPE(VoiceTab, voice_tab, GTK_TYPE_VBOX);


static void voice_tab_class_init(VoiceTabClass* klass)
{
    GtkObjectClass *object_class = GTK_OBJECT_CLASS(klass);
    voice_tab_parent_class = g_type_class_peek_parent(klass);
    g_type_class_add_private(object_class, sizeof(VoiceTabPrivate));
}


static void cut_cb(PhinSliderButton* button, VoiceTabPrivate* p)
{
    int val = phin_slider_button_get_value(button);
    patch_set_cut(p->patch, val);
}


static void cutby_cb(PhinSliderButton* button, VoiceTabPrivate* p)
{
    int val = phin_slider_button_get_value(button);
    patch_set_cut_by(p->patch, val);
}


static void porta_cb(BoolSection* b, VoiceTabPrivate* p)
{
    if (bool_section_get_active(b))
        gtk_widget_set_sensitive(p->time_sect, TRUE);
    else
        gtk_widget_set_sensitive(p->time_sect, FALSE);
}


static void mono_cb(GtkToggleButton* button, VoiceTabPrivate* p)
{
    patch_set_monophonic(p->patch, gtk_toggle_button_get_active(button));
    gtk_widget_set_sensitive(p->legato_sect,
                            gtk_toggle_button_get_active(button));
}


static void connect(VoiceTabPrivate* p)
{
    g_signal_connect(G_OBJECT(p->cut_sb), "value-changed",
                        G_CALLBACK(cut_cb), (gpointer)p);
    g_signal_connect(G_OBJECT(p->cutby_sb), "value-changed",
                        G_CALLBACK(cutby_cb), (gpointer)p);

    g_signal_connect(G_OBJECT(p->mono_check), "toggled",
                        G_CALLBACK(mono_cb), (gpointer)p);

    g_signal_connect(G_OBJECT(p->porta_sect), "toggled",
                        G_CALLBACK(porta_cb), (gpointer)p);
}


static void block(VoiceTabPrivate* p)
{
    g_signal_handlers_block_by_func(p->cut_sb,      cut_cb,     p);
    g_signal_handlers_block_by_func(p->cutby_sb,    cutby_cb,   p);
    g_signal_handlers_block_by_func(p->mono_check,  mono_cb,    p);
    g_signal_handlers_block_by_func(p->porta_sect,  porta_cb,   p);
}


static void unblock(VoiceTabPrivate* p)
{
    g_signal_handlers_unblock_by_func(p->cut_sb,        cut_cb,     p);
    g_signal_handlers_unblock_by_func(p->cutby_sb,      cutby_cb,   p);
    g_signal_handlers_unblock_by_func(p->mono_check,    mono_cb,    p);
    g_signal_handlers_unblock_by_func(p->porta_sect,    porta_cb,   p);
}


static void voice_tab_init(VoiceTab* self)
{
    VoiceTabPrivate* p = VOICE_TAB_GET_PRIVATE(self);
    GtkBox* box = GTK_BOX(self);
    GtkWidget* table;
    GtkTable* t;

    int a1 = 0, a2 = 1;
    int b1 = 1, b2 = 2;
    int c1 = 2, c2 = 3;

    int y = 0;

    p->patch = -1;
    p->refresh = -1;

    gtk_container_set_border_width(GTK_CONTAINER(self), GUI_BORDERSPACE);

    /* table */
    table = gtk_table_new(8, 3, FALSE);
    t = (GtkTable*) table;
    gui_pack(box, table);

    /* title */
    gui_attach(t, gui_title_new("Voice"), a1, c2, y, y + 1);
    ++y;

    /* title padding  */
    gui_attach(t, gui_vpad_new(GUI_TITLESPACE), a1, c2, y, y + 1);
    ++y;

    /* cut sliderbutton */
    p->cut_sb = phin_slider_button_new_with_range(0, 0, 99, 1, 0);
    phin_slider_button_set_format(PHIN_SLIDER_BUTTON(p->cut_sb), 0,
                                                    "Cut:", NULL);
    phin_slider_button_set_threshold(PHIN_SLIDER_BUTTON(p->cut_sb),
                                                    GUI_THRESHOLD);
    gui_attach(t, p->cut_sb, a1, a2, y, y + 1);


    /* cutby sliderbutton */
    p->cutby_sb = phin_slider_button_new_with_range(0, 0, 99, 1, 0);
    phin_slider_button_set_format(PHIN_SLIDER_BUTTON(p->cutby_sb), 0,
                                                    "Cut by:", NULL);
    phin_slider_button_set_threshold(PHIN_SLIDER_BUTTON(p->cutby_sb),
                                                    GUI_THRESHOLD);
    gui_attach(t, p->cutby_sb, b1, b2, y, y + 1);
    ++y;

    /* portamento control */
    p->porta_sect = bool_section_new();
    bool_section_set_bool(  BOOL_SECTION(p->porta_sect),
                            PATCH_BOOL_PORTAMENTO);
    gui_attach(t, p->porta_sect, a1, c2, y, y + 1);
    ++y;

    p->time_sect = float_section_new();
    float_section_set_float(FLOAT_SECTION(p->time_sect),
                            PATCH_FLOAT_PORTAMENTO_TIME);
    gui_attach(t, p->time_sect, a1, c2, y, y + 1);
    ++y;

    /* mono check button */
    p->mono_check = gtk_check_button_new_with_label("Monophonic");
    gui_attach(t, p->mono_check, a1, a2, y, y + 1);
    ++y;

    /* legato control */
    p->legato_sect = bool_section_new();
    bool_section_set_bool(  BOOL_SECTION(p->legato_sect),
                            PATCH_BOOL_LEGATO);
    gui_attach(t, p->legato_sect, a1, c2, y, y + 1);
    ++y;

    /* done! */
    connect(p);
}


GtkWidget* voice_tab_new(void)
{
    return (GtkWidget*) g_object_new(VOICE_TAB_TYPE, NULL);
}


void voice_tab_set_patch(VoiceTab* self, int patch)
{
    VoiceTabPrivate* p = VOICE_TAB_GET_PRIVATE(self);
    int cut, cutby;
    gboolean porta, mono;

    p->patch = patch;

    if (patch < 0)
        return;

    cut = patch_get_cut(patch);
    cutby = patch_get_cut_by(patch);
    porta = patch_get_portamento(patch);
    mono = patch_get_monophonic(patch);

    block(p);

    phin_slider_button_set_value(PHIN_SLIDER_BUTTON(p->cut_sb), cut);
    phin_slider_button_set_value(PHIN_SLIDER_BUTTON(p->cutby_sb), cutby);

    bool_section_set_patch(BOOL_SECTION(p->porta_sect), patch);

    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(p->mono_check), mono);

    bool_section_set_patch(BOOL_SECTION(p->legato_sect), patch);
    float_section_set_patch(FLOAT_SECTION(p->time_sect), patch);

    gtk_widget_set_sensitive(p->legato_sect, mono);
    gtk_widget_set_sensitive(p->time_sect, porta);

    unblock(p);
}
