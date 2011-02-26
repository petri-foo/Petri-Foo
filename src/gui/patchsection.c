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


static void patch_section_init(PatchSection* self)
{
    PatchSectionPrivate* p = PATCH_SECTION_GET_PRIVATE(self);
    GtkBox* box = GTK_BOX(self);
    GtkWidget* label;
    GtkWidget* table;
    GtkWidget* pad;

    p->patch = -1;
    
    /* table */
    table = gtk_table_new(4, 8, FALSE);
    gtk_box_pack_start(box, table, TRUE, TRUE, 0);
    gtk_widget_show(table);

    /* title */
    p->title = gtk_label_new(NULL);
    gtk_misc_set_alignment(GTK_MISC(p->title), 0.0, 0.0);
    gtk_label_set_markup(GTK_LABEL(p->title), deftitle);
    gtk_table_attach(GTK_TABLE(table), p->title, 0, 8, 0, 1, 
                                                GTK_FILL, 0, 0, 0);
    gtk_widget_show(p->title);

    /* indentation */
    pad = gui_hpad_new(GUI_INDENT);
    gtk_table_attach(GTK_TABLE(table), pad, 0, 1, 0, 1, 0, 0, 0, 0);
    gtk_widget_show(pad);

    /* column 3 padding */
    pad = gui_hpad_new(GUI_TEXTSPACE);
    gtk_table_attach(GTK_TABLE(table), pad, 2, 3, 0, 1, 0, 0, 0, 0);
    gtk_widget_show(pad);
    
    /* column 5 padding */
    pad = gui_hpad_new(GUI_SECSPACE);
    gtk_table_attach(GTK_TABLE(table), pad, 4, 5, 1, 2, 0, 0, 0, 0);
    gtk_widget_show(pad);

    /* column 7 padding */
    pad = gui_hpad_new(GUI_TEXTSPACE);
    gtk_table_attach(GTK_TABLE(table), pad, 6, 7, 1, 2, 0, 0, 0, 0);
    gtk_widget_show(pad);

    /* row 1 spacing */
    gtk_table_set_row_spacing(GTK_TABLE(table), 0, GUI_TITLESPACE);
    
    /* row 3 spacing */
    gtk_table_set_row_spacing(GTK_TABLE(table), 2, GUI_SPACING*2);

    /* notebook */
    p->notebook = gtk_notebook_new();
    gtk_table_attach_defaults(GTK_TABLE(table), p->notebook, 0, 8, 3, 4);
    gtk_widget_show(p->notebook);

    /* sample page */
    p->sample_tab = sample_tab_new();
    label = gtk_label_new("Sample");
    gtk_notebook_append_page(GTK_NOTEBOOK(p->notebook),
                                        p->sample_tab, label);
    gtk_widget_show(p->sample_tab);
    gtk_widget_show(label);


    /* Amp page */
    p->amp_tab = param_tab_new();
    param_tab_set_param(PARAM_TAB(p->amp_tab), PATCH_PARAM_AMPLITUDE);
    label = gtk_label_new("Amp");
    gtk_notebook_append_page(GTK_NOTEBOOK(p->notebook),
                                        p->amp_tab, label);
    gtk_widget_show(p->amp_tab);
    gtk_widget_show(label);

    /* pitch page */
    p->pitch_tab = param_tab_new();
    param_tab_set_param(PARAM_TAB(p->pitch_tab), PATCH_PARAM_PITCH);
    label = gtk_label_new("Pitch");
    gtk_notebook_append_page(GTK_NOTEBOOK(p->notebook),
                                        p->pitch_tab, label);
    gtk_widget_show(p->pitch_tab);
    gtk_widget_show(label);

    /* filter page */
    p->filter_tab = param_tab_new();
    param_tab_set_param(PARAM_TAB(p->filter_tab), PATCH_PARAM_CUTOFF);
    label = gtk_label_new("Filter");
    gtk_notebook_append_page(GTK_NOTEBOOK(p->notebook),
                                        p->filter_tab, label);
    gtk_widget_show(p->filter_tab);
    gtk_widget_show(label);

    /* voice page */
    p->voice_tab = voice_tab_new();
    label = gtk_label_new("Voice");
    gtk_notebook_append_page(GTK_NOTEBOOK(p->notebook),
                                        p->voice_tab, label);
    gtk_widget_show(p->voice_tab);
    gtk_widget_show(label);

    /* envelope page */
    p->env_tab = envelope_tab_new();
    label = gtk_label_new("EG");
    gtk_notebook_append_page(GTK_NOTEBOOK(p->notebook),
                                        p->env_tab, label);
    gtk_widget_show(p->env_tab);
    gtk_widget_show(label);

    /* lfo page */
    p->lfo_tab = lfo_tab_new();
    label = gtk_label_new("LFO");
    gtk_notebook_append_page(GTK_NOTEBOOK(p->notebook),
                                        p->lfo_tab, label);
    gtk_widget_show(p->lfo_tab);
    gtk_widget_show(label);

    /* done */
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
