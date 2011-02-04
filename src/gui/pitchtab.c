#include <gtk/gtk.h>
#include <phat/phat.h>
#include "pitchtab.h"
#include "gui.h"
#include "patch.h"

#include "mod_src_opt.h"

static GtkVBoxClass* parent_class;

static void pitch_tab_class_init(PitchTabClass* klass);
static void pitch_tab_init(PitchTab* self);
static void pitch_tab_destroy(GtkObject* object);


GType pitch_tab_get_type(void)
{
    static GType type = 0;

    if (!type)
    {
	static const GTypeInfo info =
	    {
		sizeof (PitchTabClass),
		NULL,
		NULL,
		(GClassInitFunc) pitch_tab_class_init,
		NULL,
		NULL,
		sizeof (PitchTab),
		0,
		(GInstanceInitFunc) pitch_tab_init,
	    };

	type = g_type_register_static(GTK_TYPE_VBOX, "PitchTab", &info, 0);
    }

    return type;
}


static void pitch_tab_class_init(PitchTabClass* klass)
{
    parent_class = g_type_class_peek_parent(klass);
    GTK_OBJECT_CLASS(klass)->destroy = pitch_tab_destroy;
}

static void tuning_cb(PhatFanSlider* fan, PitchTab* self)
{
    float val = phat_fan_slider_get_value(fan);
    patch_set_mod1_amt(self->patch, PATCH_PARAM_PITCH, val);
}

static void mod1_src_cb(GtkComboBox* combo, PitchTab* self)
{
/*    float val = phat_fan_slider_get_value(fan);
    patch_set_mod1_amt(self->patch, PATCH_PARAM_PITCH, val);
*/
    printf("Hi there!\n");
}

static void mod1_amt_cb(PhatFanSlider* fan, PitchTab* self)
{
    float val = phat_fan_slider_get_value(fan);
    patch_set_mod1_amt(self->patch, PATCH_PARAM_PITCH, val);
}

static void mod2_amt_cb(PhatFanSlider* fan, PitchTab* self)
{
    float val = phat_fan_slider_get_value(fan);
    patch_set_mod1_amt(self->patch, PATCH_PARAM_PITCH, val);
}


static void connect(PitchTab* self)
{
    g_signal_connect(G_OBJECT(self->tuning_fan), "value-changed",
                        G_CALLBACK(tuning_cb), (gpointer) self);

/*
    g_signal_connect(G_OBJECT(self->range_sb), "value-changed",
                        G_CALLBACK(range_cb), (gpointer) self);
*/
    g_signal_connect(G_OBJECT(self->mod1_combo), "changed",
                        G_CALLBACK(mod1_src_cb), (gpointer) self);
/*
    g_signal_connect(G_OBJECT(self->mod1_fan), "value-changed",
                        G_CALLBACK(mod1_amt_cb), (gpointer) self);

    g_signal_connect(G_OBJECT(self->mod2_combo), "changed",
                        G_CALLBACK(mod2_src_cb), (gpointer) self);

    g_signal_connect(G_OBJECT(self->mod2_fan), "value-changed",
                        G_CALLBACK(mod2_amt_cb), (gpointer) self);
*/
}


static void block(PitchTab* self)
{

    g_signal_handlers_block_by_func(self->mod1_combo, mod1_src_cb, self);
/*
    g_signal_handlers_block_by_func(self->tuning_fan, tuning_cb,   self);
    g_signal_handlers_block_by_func(self->range_sb,   range_cb,    self);
    g_signal_handlers_block_by_func(self->mod1_combo, mod1_opt_cb, self);
    g_signal_handlers_block_by_func(self->mod1_fan,   mod1_fan_cb, self);
    g_signal_handlers_block_by_func(self->mod2_combo, mod1_opt_cb, self);
    g_signal_handlers_block_by_func(self->mod2_fan,   mod1_fan_cb, self);
*/
}


static void unblock(PitchTab* self)
{
    g_signal_handlers_unblock_by_func(self->mod1_combo, mod1_src_cb, self);
/*
    g_signal_handlers_unblock_by_func(self->tuning_fan, tuning_cb,   self);
    g_signal_handlers_unblock_by_func(self->range_sb,   range_cb,    self);
    g_signal_handlers_unblock_by_func(self->mod1_combo, mod1_opt_cb, self);
    g_signal_handlers_unblock_by_func(self->mod1_fan,   mod1_fan_cb, self);
    g_signal_handlers_unblock_by_func(self->mod2_combo, mod1_opt_cb, self);
    g_signal_handlers_unblock_by_func(self->mod2_fan,   mod1_fan_cb, self);
*/
}


static gboolean refresh(gpointer data)
{
    PitchTab* self = PITCH_TAB(data);
    float cut, res;

    if (self->patch < 0)
        return TRUE;

/*
    cut = patch_get_cutoff(self->patch);
    res = patch_get_resonance(self->patch);

    block(self);
    
    phat_fan_slider_set_value(PHAT_FAN_SLIDER(self->freq_fan), cut);
    phat_fan_slider_set_value(PHAT_FAN_SLIDER(self->reso_fan), res);
    
    unblock(self);
*/
    return TRUE;
}


static void pitch_tab_init(PitchTab* self)
{
    GtkBox* box = GTK_BOX(self);
    GtkWidget* title;
    GtkWidget* table;
    GtkTable* t;
    GtkWidget* pad;
    GtkWidget* label;
    GtkTreeModel* mod_src_list = mod_src_opt_new();

    self->patch = -1;
    gtk_container_set_border_width(GTK_CONTAINER(self), GUI_BORDERSPACE);

    /* table */
    table = gtk_table_new(4, 4, FALSE);
    t = (GtkTable*) table;
    gtk_box_pack_start(box, table, FALSE, FALSE, 0);
    gtk_widget_show(table);
    
    /* pitch title */
    title = gui_title_new("Pitch");
    gtk_table_attach_defaults(t, title, 0, 4, 0, 1);
    gtk_widget_show(title);

    /* indentation */
    pad = gui_hpad_new(GUI_INDENT);
    gtk_table_attach(t, pad, 0, 1, 1, 2, 0, 0, 0, 0);
    gtk_widget_show(pad);
    
    /* pitch title padding */
    pad = gui_vpad_new(GUI_TITLESPACE);
    gtk_table_attach(t, pad, 1, 2, 1, 2, 0, 0, 0, 0);
    gtk_widget_show(pad);

    /* label-fan column spacing */
    pad = gui_hpad_new(GUI_TEXTSPACE);
    gtk_table_attach(t, pad, 2, 3, 1, 2, 0, 0, 0, 0);
    gtk_widget_show(pad);

    /* mod1 input source */

    label = gtk_label_new("Mod1:");
    self->mod1_combo = gtk_combo_box_new_with_model(mod_src_list);
    gtk_misc_set_alignment(GTK_MISC(label), 0.0, 0.5);
    gtk_table_attach(t, label, 1, 2, 2, 3, GTK_FILL, 0, 0, 0);
    gtk_table_attach_defaults(t, self->mod1_combo, 3, 4, 2, 3);
    gtk_widget_show(label);
    gtk_widget_show(self->mod1_combo);

/* Create cell renderer. */
GtkWidget* cell = gtk_cell_renderer_text_new();
gtk_cell_layout_pack_start( GTK_CELL_LAYOUT( self->mod1_combo ), cell, TRUE );
gtk_cell_layout_set_attributes( GTK_CELL_LAYOUT( self->mod1_combo ), cell, "text", 0, NULL );
/*
    label = gtk_label_new("Cutoff:");
    self->freq_fan = phat_hfan_slider_new_with_range(1.0, 0.0, 1.0, 0.01);
    gtk_misc_set_alignment(GTK_MISC(label), 0.0, 0.5);
    gtk_table_attach(t, label, 1, 2, 2, 3, GTK_FILL, 0, 0, 0);
    gtk_table_attach_defaults(t, self->freq_fan, 3, 4, 2, 3);
    gtk_widget_show(label);
    gtk_widget_show(self->freq_fan);
*/
    /* reso fan */
/*
    label = gtk_label_new("Resonance:");
    self->reso_fan = phat_hfan_slider_new_with_range(0.0, 0.0, 1.0, 0.01);
    gtk_misc_set_alignment(GTK_MISC(label), 0.0, 0.5);
    gtk_table_attach(t, label, 1, 2, 3, 4, GTK_FILL, 0, 0, 0);
    gtk_table_attach_defaults(t, self->reso_fan, 3, 4, 3, 4);
    gtk_widget_show(label);
    gtk_widget_show(self->reso_fan);
*/
    /* done */
    connect(self);
    self->refresh = g_timeout_add(GUI_REFRESH_TIMEOUT, refresh, (gpointer) self);
}


static void pitch_tab_destroy(GtkObject* object)
{
    PitchTab* self = PITCH_TAB(object);
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


GtkWidget* pitch_tab_new(void)
{
    return (GtkWidget*) g_object_new(PITCH_TAB_TYPE, NULL);
}


void pitch_tab_set_patch(PitchTab* self, int patch)
{
    float freq, reso;

    self->patch = patch;

    if (patch < 0)
        return;
/*
    freq = patch_get_cutoff(patch);
    reso = patch_get_resonance(patch);

    block(self);
    
    phat_fan_slider_set_value(PHAT_FAN_SLIDER(self->freq_fan), freq);
    phat_fan_slider_set_value(PHAT_FAN_SLIDER(self->reso_fan), reso);

    unblock(self);
*/
}
