#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>
#include <phat/phat.h>
#include <stdlib.h>
#include "patchsection.h"
#include "petri-foo.h"
#include "gui.h"
#include "sampletab.h"
#include "voicetab.h"
#include "filtertab.h"
#include "velocitytab.h"
#include "envelopetab.h"
#include "lfotab.h"
#include "patch.h"
#include "mixer.h"

#include "pitchtab.h"
#include "amptab.h"


const char* deftitle = "<b>Empty Bank</b>";

static GtkVBoxClass* parent_class;

static void patch_section_class_init(PatchSectionClass* klass);
static void patch_section_init(PatchSection* self);
static void patch_section_destroy(GtkObject* object);


typedef struct _MenuItem
{
    GtkWidget *item;
    PatchSection* self;
    int patch;
} MenuItem;


GType patch_section_get_type(void)
{
    static GType type = 0;

    if (!type)
    {
	static const GTypeInfo info =
	    {
		sizeof (PatchSectionClass),
		NULL,
		NULL,
		(GClassInitFunc) patch_section_class_init,
		NULL,
		NULL,
		sizeof (PatchSection),
		0,
		(GInstanceInitFunc) patch_section_init,
	    };

	type = g_type_register_static(GTK_TYPE_VBOX, "PatchSection", &info, 0);
    }

    return type;
}


static void patch_section_class_init(PatchSectionClass* klass)
{
    parent_class = g_type_class_peek_parent(klass);

    GTK_OBJECT_CLASS(klass)->destroy = patch_section_destroy;
}



static void set_sensitive(PatchSection* self, gboolean val)
{
    gtk_widget_set_sensitive(self->notebook, val);
}




static void patch_section_init(PatchSection* self)
{
    GtkBox* box = GTK_BOX(self);
    GtkWidget* label;
    GtkWidget* table;
    GtkWidget* pad;

    self->patch = -1;
    
    /* table */
    table = gtk_table_new(4, 8, FALSE);
    gtk_box_pack_start(box, table, TRUE, TRUE, 0);
    gtk_widget_show(table);

    /* title */
    self->title = gtk_label_new(NULL);
    gtk_misc_set_alignment(GTK_MISC(self->title), 0.0, 0.0);
    gtk_label_set_markup(GTK_LABEL(self->title), deftitle);
    gtk_table_attach(GTK_TABLE(table), self->title, 0, 8, 0, 1, 
                                                GTK_FILL, 0, 0, 0);
    gtk_widget_show(self->title);

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
    self->notebook = gtk_notebook_new();
    gtk_table_attach_defaults(GTK_TABLE(table), self->notebook, 0, 8, 3, 4);
    gtk_widget_show(self->notebook);

    /* sample page */
    self->sample_tab = sample_tab_new();
    label = gtk_label_new("Sample");
    gtk_notebook_append_page(GTK_NOTEBOOK(self->notebook), self->sample_tab, label);
    gtk_widget_show(self->sample_tab);
    gtk_widget_show(label);
    
    /* voice page */
    self->voice_tab = voice_tab_new();
    label = gtk_label_new("Voice");
    gtk_notebook_append_page(GTK_NOTEBOOK(self->notebook), self->voice_tab, label);
    gtk_widget_show(self->voice_tab);
    gtk_widget_show(label);


    /* Amp page */
    self->amp_tab = amp_tab_new();
    label = gtk_label_new("Amp");
    gtk_notebook_append_page(GTK_NOTEBOOK(self->notebook), self->amp_tab, label);
    gtk_widget_show(self->amp_tab);
    gtk_widget_show(label);

    /* pitch page */
    self->pitch_tab = pitch_tab_new();
    label = gtk_label_new("Pitch");
    gtk_notebook_append_page(GTK_NOTEBOOK(self->notebook), self->pitch_tab, label);
    gtk_widget_show(self->pitch_tab);
    gtk_widget_show(label);


    /* filter page */
    self->filter_tab = filter_tab_new();
    label = gtk_label_new("Filter");
    gtk_notebook_append_page(GTK_NOTEBOOK(self->notebook), self->filter_tab, label);
    gtk_widget_show(self->filter_tab);
    gtk_widget_show(label);

    /* velocity page */
    self->vel_tab = velocity_tab_new();
    label = gtk_label_new("Velocity");
    gtk_notebook_append_page(GTK_NOTEBOOK(self->notebook), self->vel_tab, label);
    gtk_widget_show(self->vel_tab);
    gtk_widget_show(label);

    /* envelope page */
    self->env_tab = envelope_tab_new();
    label = gtk_label_new("EG");
    gtk_notebook_append_page(GTK_NOTEBOOK(self->notebook), self->env_tab, label);
    gtk_widget_show(self->env_tab);
    gtk_widget_show(label);

    /* lfo page */
    self->lfo_tab = lfo_tab_new();
    label = gtk_label_new("LFO");
    gtk_notebook_append_page(GTK_NOTEBOOK(self->notebook), self->lfo_tab, label);
    gtk_widget_show(self->lfo_tab);
    gtk_widget_show(label);

    /* done */
}


static void patch_section_destroy(GtkObject* object)
{
    PatchSection* self = PATCH_SECTION(object);
    GtkObjectClass* klass = GTK_OBJECT_CLASS(parent_class);

    if (!g_source_remove(self->refresh))
    {
	debug("failed to remove refresh function from idle loop: %u\n", self->refresh);
    }
    else
    {
	debug("refresh function removed\n");
    }
    
    if (klass->destroy)
	klass->destroy(object);
}


GtkWidget* patch_section_new(void)
{
    return (GtkWidget*) g_object_new(PATCH_SECTION_TYPE, NULL);
}


void patch_section_set_patch(PatchSection* self, int patch)
{
    char* name;
    char* title;

    self->patch = patch;

    if (patch < 0)
    {
        set_sensitive(self, FALSE);
        gtk_label_set_markup(GTK_LABEL(self->title), deftitle);
    }
    else
    {
        set_sensitive(self, TRUE);
        name = patch_get_name(patch);
        title = g_strdup_printf("<b>%s</b>", name);
        gtk_label_set_markup(GTK_LABEL(self->title), title);
        g_free(title);
        g_free(name);
    }

    sample_tab_set_patch(SAMPLE_TAB(self->sample_tab), patch);
    voice_tab_set_patch(VOICE_TAB(self->voice_tab), patch);
    filter_tab_set_patch(FILTER_TAB(self->filter_tab), patch);
    velocity_tab_set_patch(VELOCITY_TAB(self->vel_tab), patch);
    envelope_tab_set_patch(ENVELOPE_TAB(self->env_tab), patch);
    lfo_tab_set_patch(LFO_TAB(self->lfo_tab), patch);
    pitch_tab_set_patch(PITCH_TAB(self->pitch_tab), patch);
    amp_tab_set_patch(AMP_TAB(self->amp_tab), patch);
}
