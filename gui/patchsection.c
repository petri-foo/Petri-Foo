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
#include <gdk/gdkkeysyms.h>
#include <phat/phat.h>
#include <stdlib.h>
#include "patchsection.h"
#include "petri-foo.h"
#include "gui.h"
#include "sampletab.h"
#include "voicetab.h"
#include "envelopetab.h"
#include "lfotab.h"
#include "mixer.h"
#include "paramtab.h"
#include "patch_set_and_get.h"


static const char* deftitle = "<b>Empty Bank</b>";


typedef struct _PatchSectionPrivate PatchSectionPrivate;

#define PATCH_SECTION_GET_PRIVATE(obj)  \
    (G_TYPE_INSTANCE_GET_PRIVATE((obj), \
        PATCH_SECTION_TYPE, PatchSectionPrivate))

struct _PatchSectionPrivate
{
    int patch;
    guint refresh;
    GtkWidget* title;

    GtkWidget* notebook;
    GtkWidget* sample_tab;

    GtkWidget* amp_tab;
    GtkWidget* pitch_tab;
    GtkWidget* filter_tab;

    GtkWidget* voice_tab;

    GtkWidget* env_tab;
    GtkWidget* lfo_tab;
};

G_DEFINE_TYPE(PatchSection, patch_section, GTK_TYPE_VBOX);


static void patch_section_class_init(PatchSectionClass* klass)
{
    GtkObjectClass *object_class = GTK_OBJECT_CLASS(klass);
    patch_section_parent_class = g_type_class_peek_parent(klass);
    g_type_class_add_private(object_class, sizeof(PatchSectionPrivate));
}


void add_page(const char* str, GtkNotebook* book, GtkWidget* widget)
{
    GtkWidget* label = gtk_label_new(str);
    gtk_notebook_append_page(book, widget, label);
    gtk_widget_show(label);
    gtk_widget_show(widget);
}


static void patch_section_init(PatchSection* self)
{
    PatchSectionPrivate* p = PATCH_SECTION_GET_PRIVATE(self);
    GtkBox* box = GTK_BOX(self);

    p->patch = -1;

    /* title */
    p->title = gui_title_new("");
    gui_pack(box, p->title);

    gui_pack(box, gui_hpad_new(GUI_TITLESPACE));
    gui_pack(box, gui_hpad_new(GUI_SPACING * 2));

    /* notebook */
    p->notebook = gtk_notebook_new();
    gui_pack(box, p->notebook);

    /* sample page */
    p->sample_tab = sample_tab_new();
    add_page("Sample", GTK_NOTEBOOK(p->notebook), p->sample_tab);

    /* Amp page */
    p->amp_tab = param_tab_new();
    param_tab_set_param(PARAM_TAB(p->amp_tab), PATCH_PARAM_AMPLITUDE);
    add_page("Amp", GTK_NOTEBOOK(p->notebook), p->amp_tab);

    /* pitch page */
    p->pitch_tab = param_tab_new();
    param_tab_set_param(PARAM_TAB(p->pitch_tab), PATCH_PARAM_PITCH);
    add_page("Pitch", GTK_NOTEBOOK(p->notebook), p->pitch_tab);

    /* filter page */
    p->filter_tab = param_tab_new();
    param_tab_set_param(PARAM_TAB(p->filter_tab), PATCH_PARAM_CUTOFF);
    add_page("Filter", GTK_NOTEBOOK(p->notebook), p->filter_tab);

    /* voice page */
    p->voice_tab = voice_tab_new();
    add_page("Voice", GTK_NOTEBOOK(p->notebook), p->voice_tab);

    /* envelope page */
    p->env_tab = envelope_tab_new();
    add_page("EG", GTK_NOTEBOOK(p->notebook), p->env_tab);

    /* lfo page */
    p->lfo_tab = lfo_tab_new();
    add_page("LFO", GTK_NOTEBOOK(p->notebook), p->lfo_tab);
}


GtkWidget* patch_section_new(void)
{
    return (GtkWidget*) g_object_new(PATCH_SECTION_TYPE, NULL);
}


void patch_section_set_patch(PatchSection* self, int patch)
{
    PatchSectionPrivate* p = PATCH_SECTION_GET_PRIVATE(self);
    char* name;
    char* title;

    p->patch = patch;

    if (patch < 0)
    {
        gtk_widget_set_sensitive(p->notebook, FALSE);
        gtk_label_set_markup(GTK_LABEL(p->title), deftitle);
    }
    else
    {
        gtk_widget_set_sensitive(p->notebook, TRUE);
        name = patch_get_name(patch);
        title = g_strdup_printf("<b>%s</b>", name);
        gtk_label_set_markup(GTK_LABEL(p->title), title);
        g_free(title);
        g_free(name);
    }

    sample_tab_set_patch(SAMPLE_TAB(p->sample_tab), patch);
    voice_tab_set_patch(VOICE_TAB(p->voice_tab), patch);

    param_tab_set_patch(PARAM_TAB(p->amp_tab), patch);
    param_tab_set_patch(PARAM_TAB(p->pitch_tab), patch);
    param_tab_set_patch(PARAM_TAB(p->filter_tab), patch);

    envelope_tab_set_patch(ENVELOPE_TAB(p->env_tab), patch);
    lfo_tab_set_patch(LFO_TAB(p->lfo_tab), patch);
}
