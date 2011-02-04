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


static void vol_cb(PhatFanSlider* fan, PatchSection* self)
{
    float val;

    val = phat_fan_slider_get_value(fan);
    patch_set_volume(self->patch, val);
}


static void pan_cb(PhatFanSlider* fan, PatchSection* self)
{
    float val;

    val = phat_fan_slider_get_value(fan);
    patch_set_panning(self->patch, val);
}


static void pitch_cb(PhatFanSlider* fan, PatchSection* self)
{
    float val;

    val = phat_fan_slider_get_value(fan);
    patch_set_pitch(self->patch, val);
}


static void range_cb(PhatSliderButton* button, PatchSection* self)
{
    int val;

    val = phat_slider_button_get_value(button);
    patch_set_pitch_steps(self->patch, val);
}


static void connect(PatchSection* self)
{
    g_signal_connect(G_OBJECT(self->volume_fan), "value-changed",
		     G_CALLBACK(vol_cb), (gpointer) self);
    g_signal_connect(G_OBJECT(self->pan_fan), "value-changed",
		     G_CALLBACK(pan_cb), (gpointer) self);
    g_signal_connect(G_OBJECT(self->pitch_fan), "value-changed",
		     G_CALLBACK(pitch_cb), (gpointer) self);
    g_signal_connect(G_OBJECT(self->range_sb), "value-changed",
		     G_CALLBACK(range_cb), (gpointer) self);
}


static void block(PatchSection* self)
{
    g_signal_handlers_block_by_func(self->volume_fan, vol_cb, self);
    g_signal_handlers_block_by_func(self->pan_fan, pan_cb, self);
    g_signal_handlers_block_by_func(self->pitch_fan, pitch_cb, self);
    g_signal_handlers_block_by_func(self->range_sb, range_cb, self);
}


static void unblock(PatchSection* self)
{
    g_signal_handlers_unblock_by_func(self->volume_fan, vol_cb, self);
    g_signal_handlers_unblock_by_func(self->pan_fan, pan_cb, self);
    g_signal_handlers_unblock_by_func(self->pitch_fan, pitch_cb, self);
    g_signal_handlers_unblock_by_func(self->range_sb, range_cb, self);
}


static void set_sensitive(PatchSection* self, gboolean val)
{
    gtk_widget_set_sensitive(self->volume_fan, val);
    gtk_widget_set_sensitive(self->pan_fan, val);
    gtk_widget_set_sensitive(self->pitch_fan, val);
    gtk_widget_set_sensitive(self->range_sb, val);
    gtk_widget_set_sensitive(self->notebook, val);
    
}


static gboolean refresh(gpointer data)
{
    PatchSection* self = PATCH_SECTION(data);
    float vol, pan;

    if (self->patch < 0)
	return TRUE;
    
    vol = patch_get_volume(self->patch);
    pan = patch_get_panning(self->patch);

    block(self);

    phat_fan_slider_set_value(PHAT_FAN_SLIDER(self->volume_fan), vol);
    phat_fan_slider_set_value(PHAT_FAN_SLIDER(self->pan_fan), pan);

    unblock(self);

    return TRUE;
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
    gtk_table_attach(GTK_TABLE(table), self->title, 0, 8, 0, 1, GTK_FILL, 0, 0, 0);
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

    /* volume */
    label = gtk_label_new("Volume:");
    self->volume_fan = phat_hfan_slider_new_with_range(DEFAULT_VOLUME, 0.0, 1.0, 0.1);

    gtk_misc_set_alignment(GTK_MISC(label), 0.0, 0.5);
    gtk_table_attach(GTK_TABLE(table), label, 1, 2, 1, 2, GTK_FILL, 0, 0, 0);
    gtk_table_attach(GTK_TABLE(table), self->volume_fan, 3, 4, 1, 2, GTK_EXPAND | GTK_FILL, 0, 0, 0);

    gtk_widget_show(label);
    gtk_widget_show(self->volume_fan);

    /* panning */
    label = gtk_label_new("Panning:");
    self->pan_fan = phat_hfan_slider_new_with_range(0.0, -1.0, 1.0, 0.1);

    gtk_misc_set_alignment(GTK_MISC(label), 0.0, 0.5);
    gtk_table_attach(GTK_TABLE(table), label, 1, 2, 2, 3, GTK_FILL, 0, 0, 0);
    gtk_table_attach(GTK_TABLE(table), self->pan_fan, 3, 4, 2, 3, GTK_EXPAND | GTK_FILL, 0, 0, 0);

    gtk_widget_show(label);
    gtk_widget_show(self->pan_fan);

    /* pitch */
    label = gtk_label_new("Pitch:");
    self->pitch_fan = phat_hfan_slider_new_with_range(0.0, -1.0, 1.0, 0.1);

    gtk_misc_set_alignment(GTK_MISC(label), 0.0, 0.5);
    gtk_table_attach(GTK_TABLE(table), label, 5, 6, 1, 2, GTK_FILL, 0, 0, 0);
    gtk_table_attach(GTK_TABLE(table), self->pitch_fan, 7, 8, 1, 2, GTK_EXPAND | GTK_FILL, 0, 0, 0);

    gtk_widget_show(label);
    gtk_widget_show(self->pitch_fan);

    /* range */
    label = gtk_label_new("Steps:");
    self->range_sb = phat_slider_button_new_with_range(2.0, 0.0, 48.0, 1.0, 0);

    phat_slider_button_set_threshold(PHAT_SLIDER_BUTTON(self->range_sb), GUI_THRESHOLD);
    gtk_misc_set_alignment(GTK_MISC(label), 0.0, 0.5);

    gtk_table_attach(GTK_TABLE(table), label, 5, 6, 2, 3, GTK_FILL, 0, 0, 0);
    gtk_table_attach(GTK_TABLE(table), self->range_sb, 7, 8, 2, 3, GTK_EXPAND | GTK_FILL, 0, 0, 0);

    gtk_widget_show(label);
    gtk_widget_show(self->range_sb);

    /* notebook */
    self->notebook = gtk_notebook_new();
    gtk_table_attach_defaults(GTK_TABLE(table), self->notebook, 0, 8, 3, 4);
    gtk_widget_show(self->notebook);

    /* sample page */
    self->sample_tab = sample_tab_new();
    label = gtk_label_new("SMP");
    gtk_notebook_append_page(GTK_NOTEBOOK(self->notebook), self->sample_tab, label);
    gtk_widget_show(self->sample_tab);
    gtk_widget_show(label);
    
    /* voice page */
    self->voice_tab = voice_tab_new();
    label = gtk_label_new("VOICE");
    gtk_notebook_append_page(GTK_NOTEBOOK(self->notebook), self->voice_tab, label);
    gtk_widget_show(self->voice_tab);
    gtk_widget_show(label);

    /* filter page */
    self->filter_tab = filter_tab_new();
    label = gtk_label_new("FILT");
    gtk_notebook_append_page(GTK_NOTEBOOK(self->notebook), self->filter_tab, label);
    gtk_widget_show(self->filter_tab);
    gtk_widget_show(label);

    /* velocity page */
    self->vel_tab = velocity_tab_new();
    label = gtk_label_new("VEL");
    gtk_notebook_append_page(GTK_NOTEBOOK(self->notebook), self->vel_tab, label);
    gtk_widget_show(self->vel_tab);
    gtk_widget_show(label);

    /* envelope page */
    self->env_tab = envelope_tab_new();
    label = gtk_label_new("ENV");
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
    connect(self);
    self->refresh = g_timeout_add(GUI_REFRESH_TIMEOUT, refresh, (gpointer) self);
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
    float vol, pan, pitch;
    int range;
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

	vol = patch_get_volume(patch);
	pan = patch_get_panning(patch);
	pitch = patch_get_pitch(patch);
	range = patch_get_pitch_steps(patch);
	name = patch_get_name(patch);

	title = g_strdup_printf("<b>%s</b>", name);
	gtk_label_set_markup(GTK_LABEL(self->title), title);
	g_free(title);
	g_free(name);

	block(self);

	phat_fan_slider_set_value(PHAT_FAN_SLIDER(self->volume_fan), vol);
	phat_fan_slider_set_value(PHAT_FAN_SLIDER(self->pan_fan), pan);
	phat_fan_slider_set_value(PHAT_FAN_SLIDER(self->pitch_fan), pitch);
	phat_slider_button_set_value(PHAT_SLIDER_BUTTON(self->range_sb), range);
	
	unblock(self);
    }

    sample_tab_set_patch(SAMPLE_TAB(self->sample_tab), patch);
    voice_tab_set_patch(VOICE_TAB(self->voice_tab), patch);
    filter_tab_set_patch(FILTER_TAB(self->filter_tab), patch);
    velocity_tab_set_patch(VELOCITY_TAB(self->vel_tab), patch);
    envelope_tab_set_patch(ENVELOPE_TAB(self->env_tab), patch);
    lfo_tab_set_patch(LFO_TAB(self->lfo_tab), patch);
}
