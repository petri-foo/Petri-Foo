#include <gtk/gtk.h>
#include <phat/phat.h>
#include "lfotab.h"
#include "gui.h"
#include "idselector.h"
#include "lfo.h"
#include "mod_src.h"
#include "patch_set_and_get.h"


/* must match order of items in menu */
enum
{
    SINE,
    TRIANGLE,
    SAW,
    SQUARE,
};

enum
{
    MAXSTEPS = PATCH_MAX_PITCH_STEPS,
};

static GtkHBoxClass* parent_class;

static void lfo_tab_class_init(LfoTabClass* klass);
static void lfo_tab_init(LfoTab* self);
static void update_lfo(LfoTab* self);

GType lfo_tab_get_type(void)
{
    static GType type = 0;

    if (!type)
    {
	static const GTypeInfo info =
	    {
		sizeof (LfoTabClass),
		NULL,
		NULL,
		(GClassInitFunc) lfo_tab_class_init,
		NULL,
		NULL,
		sizeof (LfoTab),
		0,
		(GInstanceInitFunc) lfo_tab_init,
	    };

	type = g_type_register_static(GTK_TYPE_HBOX, "LfoTab", &info, 0);
    }

    return type;
}


static void lfo_tab_class_init(LfoTabClass* klass)
{
    parent_class = g_type_class_peek_parent(klass);
}


static void set_sensitive(LfoTab* self, gboolean val)
{
    gboolean active;
    
    gtk_widget_set_sensitive(self->shape_opt, val);
    gtk_widget_set_sensitive(self->free_radio, val);
    gtk_widget_set_sensitive(self->sync_radio, val);
    gtk_widget_set_sensitive(self->beats_sb, val);
    gtk_widget_set_sensitive(self->pos_check, val);
    gtk_widget_set_sensitive(self->delay_fan, val);
    gtk_widget_set_sensitive(self->attack_fan, val);
    gtk_widget_set_sensitive(self->mod1_combo, val);
    gtk_widget_set_sensitive(self->mod2_combo, val);
    gtk_widget_set_sensitive(self->mod1_amount, val);
    gtk_widget_set_sensitive(self->mod2_amount, val);

    active  = gtk_toggle_button_get_active(
                                GTK_TOGGLE_BUTTON(self->sync_radio));

    gtk_widget_set_sensitive(self->freq_fan, !active && val);
    gtk_widget_set_sensitive(self->beats_sb, active && val);
}


static void idsel_cb(IDSelector* ids, LfoTab* self)
{
    update_lfo(self);
}


static void on_cb(GtkToggleButton* button, LfoTab* self)
{
    patch_set_lfo_on(self->patch_id, self->lfo_id,
                    gtk_toggle_button_get_active(button));
}


static void on_cb2(GtkToggleButton* button, LfoTab* self)
{
    set_sensitive(self, gtk_toggle_button_get_active(button));
}


static void shape_cb(GtkOptionMenu* opt, LfoTab* self)
{
    int val;
    LFOShape shape;

    val = gtk_option_menu_get_history(opt);

    switch(val)
    {
    case TRIANGLE:  shape = LFO_SHAPE_TRIANGLE; break;
    case SAW:       shape = LFO_SHAPE_SAW;      break;
    case SQUARE:    shape = LFO_SHAPE_SQUARE;   break;
    default:        shape = LFO_SHAPE_SINE;     break;
    }

    patch_set_lfo_shape(self->patch_id, self->lfo_id, shape);
}


static void sync_cb(GtkToggleButton* button, LfoTab* self)
{
    patch_set_lfo_sync(self->patch_id, self->lfo_id,
            gtk_toggle_button_get_active(button));
}


static void sync_cb2(GtkToggleButton* button, LfoTab* self)
{
    set_sensitive(self,
            gtk_toggle_button_get_active(
                    GTK_TOGGLE_BUTTON(self->lfo_check)));
}


static void freq_cb(PhatFanSlider* fan, LfoTab* self)
{
    patch_set_lfo_freq( self->patch_id, self->lfo_id,
                        phat_fan_slider_get_value(fan));
}


static void beats_cb(PhatSliderButton* button, LfoTab* self)
{
    patch_set_lfo_beats(self->patch_id, self->lfo_id,
                        phat_slider_button_get_value(button));
}

static void positive_cb(GtkToggleButton* button, LfoTab* self)
{
    patch_set_lfo_positive(self->patch_id, self->lfo_id,
                        gtk_toggle_button_get_active(button));
}


static void delay_cb(PhatFanSlider* fan, LfoTab* self)
{
    patch_set_lfo_delay(self->patch_id, self->lfo_id,
                        phat_fan_slider_get_value(fan));
}


static void attack_cb(PhatFanSlider* fan, LfoTab* self)
{
    patch_set_lfo_attack(self->patch_id, self->lfo_id,
                        phat_fan_slider_get_value(fan));
}

static void mod_src_cb(GtkComboBox* combo, LfoTab* self)
{
    int input_id;

    if (combo == GTK_COMBO_BOX(self->mod1_combo))
        input_id = MOD_IN1;
    else if (combo == GTK_COMBO_BOX(self->mod2_combo))
        input_id = MOD_IN2;
    else
    {
        debug ("mod_src_cb called from unrecognised combo box\n");
        return;
    }

    mod_src_callback_helper_lfo(self->patch_id, input_id,
                                        combo,self->lfo_id);
}

static void mod_amount_cb(GtkWidget* w, LfoTab* self)
{
    float val = phat_fan_slider_get_value(PHAT_FAN_SLIDER(w));

    if (w == self->mod1_amount)
        patch_set_lfo_mod1_amt(self->patch_id, self->lfo_id, val);
    else if (w == self->mod2_amount)
        patch_set_lfo_mod2_amt(self->patch_id, self->lfo_id, val);
    else
    {
        debug ("mod_amount_cb called from unrecognised widget\n");
    }
}

static void connect(LfoTab* self)
{
    g_signal_connect(G_OBJECT(self->idsel), "changed",
                    G_CALLBACK(idsel_cb), (gpointer) self);
    g_signal_connect(G_OBJECT(self->lfo_check), "toggled",
                    G_CALLBACK(on_cb), (gpointer)self);
    g_signal_connect(G_OBJECT(self->lfo_check), "toggled",
                    G_CALLBACK(on_cb2), (gpointer)self);
    g_signal_connect(G_OBJECT(self->shape_opt), "changed",
                    G_CALLBACK(shape_cb), (gpointer) self);
    g_signal_connect(G_OBJECT(self->freq_fan), "value-changed",
                    G_CALLBACK(freq_cb), (gpointer) self);
    g_signal_connect(G_OBJECT(self->sync_radio), "toggled",
                    G_CALLBACK(sync_cb), (gpointer) self);
    g_signal_connect(G_OBJECT(self->sync_radio), "toggled",
                    G_CALLBACK(sync_cb2), (gpointer) self);
    g_signal_connect(G_OBJECT(self->pos_check), "toggled",
                    G_CALLBACK(positive_cb), (gpointer) self);
    g_signal_connect(G_OBJECT(self->beats_sb), "value-changed",
                    G_CALLBACK(beats_cb), (gpointer) self);
    g_signal_connect(G_OBJECT(self->delay_fan), "value-changed",
                    G_CALLBACK(delay_cb), (gpointer) self);
    g_signal_connect(G_OBJECT(self->attack_fan), "value-changed",
                    G_CALLBACK(attack_cb), (gpointer) self);

    g_signal_connect(G_OBJECT(self->mod1_combo),    "changed",
                        G_CALLBACK(mod_src_cb),    (gpointer) self);

    g_signal_connect(G_OBJECT(self->mod1_amount),   "value-changed",
                        G_CALLBACK(mod_amount_cb), (gpointer) self);

    g_signal_connect(G_OBJECT(self->mod2_combo),    "changed",
                        G_CALLBACK(mod_src_cb),    (gpointer) self);

    g_signal_connect(G_OBJECT(self->mod2_amount),   "value-changed",
                        G_CALLBACK(mod_amount_cb), (gpointer) self);
}


static void block(LfoTab* self)
{
    g_signal_handlers_block_by_func(self->lfo_check, on_cb, self);
    g_signal_handlers_block_by_func(self->shape_opt, shape_cb, self);
    g_signal_handlers_block_by_func(self->freq_fan, freq_cb, self);
    g_signal_handlers_block_by_func(self->sync_radio, sync_cb, self);
    g_signal_handlers_block_by_func(self->pos_check, positive_cb, self);
    g_signal_handlers_block_by_func(self->beats_sb, beats_cb, self);
    g_signal_handlers_block_by_func(self->delay_fan, delay_cb, self);
    g_signal_handlers_block_by_func(self->attack_fan, attack_cb, self);
}


static void unblock(LfoTab* self)
{
    g_signal_handlers_unblock_by_func(self->lfo_check, on_cb, self);
    g_signal_handlers_unblock_by_func(self->shape_opt, shape_cb, self);
    g_signal_handlers_unblock_by_func(self->freq_fan, freq_cb, self);    
    g_signal_handlers_unblock_by_func(self->sync_radio, sync_cb, self);
    g_signal_handlers_unblock_by_func(self->pos_check, positive_cb, self);
    g_signal_handlers_unblock_by_func(self->beats_sb, beats_cb, self);
    g_signal_handlers_unblock_by_func(self->delay_fan, delay_cb, self);
    g_signal_handlers_unblock_by_func(self->attack_fan, attack_cb, self);
}


inline static GtkWidget* shape_opt_new(LfoTab* self)
{
    GtkWidget* menu;
    GtkWidget* opt;
    GtkWidget* item;

        
    /* lfo menu */
    menu = gtk_menu_new();
    
    item = gtk_menu_item_new_with_label("Sine");
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);
    gtk_widget_show(item);
    
    item = gtk_menu_item_new_with_label("Triangle");
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);
    gtk_widget_show(item);
    
    item = gtk_menu_item_new_with_label("Saw");
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);
    gtk_widget_show(item);
    
    item = gtk_menu_item_new_with_label("Square");
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);
    gtk_widget_show(item);

    /* lfo option menu */
    opt = gtk_option_menu_new();
    gtk_option_menu_set_menu(GTK_OPTION_MENU(opt), menu);

    return opt;
}    


static void lfo_tab_init(LfoTab* self)
{
    GtkBox* box = GTK_BOX(self);
    GtkWidget* title;
    GtkWidget* table;
    GtkTable* t;
    GtkWidget* pad;
    GtkWidget* label;

    int y = 1;

    self->patch_id = -1;
    gtk_container_set_border_width(GTK_CONTAINER(self), GUI_BORDERSPACE);

    /* table */

    /* parameter selector */
    self->idsel = id_selector_new();
    id_selector_set(ID_SELECTOR(self->idsel), mod_src_lfo_names(),
                                                ID_SELECTOR_V);
    gtk_box_pack_start(box, self->idsel, FALSE, FALSE, 0);
    gtk_widget_show(self->idsel);

    /* selector padding */
    pad = gui_hpad_new(GUI_SECSPACE);
    gtk_box_pack_start(box, pad, FALSE, FALSE, 0);
    gtk_widget_show(pad);

    table = gtk_table_new(9, 5, FALSE);
    t = (GtkTable*) table;
    gtk_box_pack_start(box, table, FALSE, FALSE, 0);
    gtk_widget_show(table);


    /* lfo title */
    title = gui_title_new("Low Frequency Oscillator");
    self->lfo_check = gtk_check_button_new();
    gtk_container_add(GTK_CONTAINER(self->lfo_check), title);
    gtk_table_attach_defaults(t, self->lfo_check, 0, 5, 0, 1);
    gtk_widget_show(title);
    gtk_widget_show(self->lfo_check);

    /* indentation */
    pad = gui_hpad_new(GUI_INDENT);
    gtk_table_attach(t, pad, 0, 1, y, y + 1, 0, 0, 0, 0);
    gtk_widget_show(pad);
    
    /* lfo title padding */
    pad = gui_vpad_new(GUI_TITLESPACE);
    gtk_table_attach(t, pad, 1, 2, y, y + 1, 0, 0, 0, 0);
    gtk_widget_show(pad);

    /* label-fan column spacing */
    pad = gui_hpad_new(GUI_TEXTSPACE);
    gtk_table_attach(t, pad, 2, 3, y, y + 1, 0, 0, 0, 0);
    gtk_widget_show(pad);

    ++y;

    /* shape */
    label = gtk_label_new("Shape:");
    self->shape_opt = shape_opt_new(self);

    gtk_misc_set_alignment(GTK_MISC(label), 0.0, 0.5);
    gtk_table_attach(t, label, 1, 2, y, y + 1, GTK_FILL, 0, 0, 0);
    gtk_table_attach(t, self->shape_opt, 3, 5, y, y + 1, GTK_FILL, 0, 0, 0);

    gtk_widget_show(label);
    gtk_widget_show(self->shape_opt);

    ++y;

    /* freq */
    label = gtk_label_new("Frequency:");
    self->free_radio = gtk_radio_button_new(NULL);
    self->freq_fan = phat_hfan_slider_new_with_range(5.0, 0.0, 20.0, 0.1);

    gtk_misc_set_alignment(GTK_MISC(label), 0.0, 0.5);
    gtk_table_attach(t, label, 1, 2, y, y + 1, GTK_FILL, 0, 0, 0);
    gtk_table_attach(t, self->free_radio, 2, 3, y, y + 1, 0, 0, 0, 0);
    gtk_table_attach_defaults(t, self->freq_fan, 3, 5, y, y + 1);

    gtk_widget_show(label);
    gtk_widget_show(self->free_radio);
    gtk_widget_show(self->freq_fan);

    ++y;

    /* sync */
    self->sync_radio = gtk_radio_button_new_from_widget(
                            GTK_RADIO_BUTTON(self->free_radio));
    self->beats_sb =
            phat_slider_button_new_with_range(1.0, .25, 32.0, .25, 2);

    phat_slider_button_set_format(PHAT_SLIDER_BUTTON(self->beats_sb),
                                    -1, NULL, "Beats");

    phat_slider_button_set_threshold(PHAT_SLIDER_BUTTON(self->beats_sb),
                                    GUI_THRESHOLD);

    gtk_table_attach(t, self->sync_radio, 2, 3, y, y + 1, 0, 0, 0, 0);
    gtk_table_attach(t, self->beats_sb, 3, 5, y, y + 1, GTK_FILL, 0, 0, 0);

    gtk_widget_show(self->sync_radio);
    gtk_widget_show(self->beats_sb);
    gtk_widget_set_sensitive(self->beats_sb, FALSE);

    ++y;


    /* mod1 input source */
    label = gtk_label_new("Freq.Mod1:");
    gtk_misc_set_alignment(GTK_MISC(label), 0.0, 0.5);
    gtk_table_attach(t, label, 1, 2, y, y + 1, GTK_FILL, 0, 0, 0);
    gtk_widget_show(label);

    self->mod1_combo = mod_src_new_combo_with_cell();
    gtk_table_attach_defaults(t, self->mod1_combo, 3, 5, y, y + 1);
    gtk_widget_show(self->mod1_combo);

    ++y;

    label = gtk_label_new("Amount:");
    gtk_misc_set_alignment(GTK_MISC(label), 0.0, 0.5);
    gtk_table_attach(t, label, 1, 2, y, y + 1, GTK_FILL, 0, 0, 0);
    gtk_widget_show(label);

    self->mod1_amount = phat_hfan_slider_new_with_range(0.0, -1.0,
                                                        1.0, 0.1);
    gtk_table_attach_defaults(t, self->mod1_amount, 3, 5, y, y + 1);
    gtk_widget_show(self->mod1_amount);

    ++y;

    /* mod2 input source */
    label = gtk_label_new("Freq.Mod2:");
    gtk_misc_set_alignment(GTK_MISC(label), 0.0, 0.5);
    gtk_table_attach(t, label, 1, 2, y, y + 1, GTK_FILL, 0, 0, 0);
    gtk_widget_show(label);

    self->mod2_combo = mod_src_new_combo_with_cell();
    gtk_table_attach_defaults(t, self->mod2_combo, 3, 5, y, y + 1);
    gtk_widget_show(self->mod2_combo);

    ++y;

    label = gtk_label_new("Amount:");
    gtk_misc_set_alignment(GTK_MISC(label), 0.0, 0.5);
    gtk_table_attach(t, label, 1, 2, y, y + 1, GTK_FILL, 0, 0, 0);
    gtk_widget_show(label);

    self->mod2_amount = phat_hfan_slider_new_with_range(0.0, -1.0,
                                                        1.0, 0.1);
    gtk_table_attach_defaults(t, self->mod2_amount, 3, 5, y, y + 1);
    gtk_widget_show(self->mod2_amount);

    ++y;

//    gtk_table_set_row_spacing(t, y, GUI_SPACING);

    /* delay fan */
    label = gtk_label_new("Delay:");
    self->delay_fan = phat_hfan_slider_new_with_range(0.1, 0.0, 1.0, 0.01);

    gtk_misc_set_alignment(GTK_MISC(label), 0.0, 0.5);
    gtk_table_attach(t, label, 1, 2, y, y + 1, GTK_FILL, 0, 0, 0);
    gtk_table_attach_defaults(t, self->delay_fan, 3, 4, y, y + 1);

    gtk_widget_show(label);
    gtk_widget_show(self->delay_fan);

    ++y;

//    gtk_table_set_col_spacing(t, 3, GUI_SPACING*2);

    /* attack fan */
    label = gtk_label_new("Attack:");
    self->attack_fan = phat_hfan_slider_new_with_range(0.1, 0.0, 1.0, 0.01);

    gtk_misc_set_alignment(GTK_MISC(label), 0.0, 0.5);
    gtk_table_attach(t, label, 1, 2, y, y + 1, GTK_FILL, 0, 0, 0);
    gtk_table_attach_defaults(t, self->attack_fan, 3, 4, y, y + 1);

    gtk_widget_show(label);
    gtk_widget_show(self->attack_fan);

    /* positive */
    self->pos_check = gtk_check_button_new_with_label("Positive");
    gtk_table_attach(t, self->pos_check, 4, 5, y, y + 1, GTK_FILL, 0, 0, 0);
    gtk_widget_show(self->pos_check);

    ++y;

    gtk_widget_show(pad);


    /* done */
    set_sensitive(self, FALSE);
    connect(self);
}


static void update_lfo(LfoTab* self)
{
    LFOShape shape;
    float freq, beats, delay, attack;
    gboolean sync, positive;
    gboolean on;

    int   mod1src, mod2src;
    float mod1amt, mod2amt;

    GtkTreeIter m1iter, m2iter;

    self->lfo_id = id_selector_get_id(ID_SELECTOR(self->idsel));

    int mod_srcs = (patch_lfo_is_global(self->lfo_id)
                        ? MOD_SRC_INPUTS_GLOBAL
                        : MOD_SRC_INPUTS_ALL);

    block(self);
    mod_src_combo_set_model(GTK_COMBO_BOX(self->mod1_combo), mod_srcs);
    mod_src_combo_set_model(GTK_COMBO_BOX(self->mod2_combo), mod_srcs);
    unblock(self);

    patch_get_lfo_shape(    self->patch_id, self->lfo_id, &shape);
    patch_get_lfo_freq(     self->patch_id, self->lfo_id, &freq);
    patch_get_lfo_beats(    self->patch_id, self->lfo_id, &beats);
    patch_get_lfo_delay(    self->patch_id, self->lfo_id, &delay);
    patch_get_lfo_attack(   self->patch_id, self->lfo_id, &attack);
    patch_get_lfo_sync(     self->patch_id, self->lfo_id, &sync);
    patch_get_lfo_positive( self->patch_id, self->lfo_id, &positive);
    patch_get_lfo_on(       self->patch_id, self->lfo_id, &on);

    patch_get_lfo_mod1_src( self->patch_id, self->lfo_id, &mod1src);
    patch_get_lfo_mod1_amt( self->patch_id, self->lfo_id, &mod1amt);
    patch_get_lfo_mod2_src( self->patch_id, self->lfo_id, &mod2src);
    patch_get_lfo_mod2_amt( self->patch_id, self->lfo_id, &mod2amt);

    if (!mod_src_combo_get_iter_with_id(GTK_COMBO_BOX(self->mod1_combo),
                                                        mod1src, &m1iter))
    {
        debug("failed to get lfo mod1 source id from combo box\n");
    }

    if (!mod_src_combo_get_iter_with_id(GTK_COMBO_BOX(self->mod2_combo),
                                                        mod2src, &m2iter))
    {
        debug("failed to get lfo mod2 source id from combo box\n");
    }

    block(self);

    phat_fan_slider_set_value(PHAT_FAN_SLIDER(self->freq_fan), freq);
    phat_fan_slider_set_value(PHAT_FAN_SLIDER(self->delay_fan), delay);
    phat_fan_slider_set_value(PHAT_FAN_SLIDER(self->attack_fan), attack);

    phat_slider_button_set_value(PHAT_SLIDER_BUTTON(self->beats_sb), beats);

    if (sync)
        gtk_toggle_button_set_active(
                    GTK_TOGGLE_BUTTON(self->sync_radio), TRUE);
    else
        gtk_toggle_button_set_active(
                    GTK_TOGGLE_BUTTON(self->free_radio), TRUE);

    gtk_toggle_button_set_active(
                    GTK_TOGGLE_BUTTON(self->pos_check), positive);

    switch (shape)
    {
    case LFO_SHAPE_TRIANGLE:
        gtk_option_menu_set_history(
                    GTK_OPTION_MENU(self->shape_opt), TRIANGLE);
        break;
    case LFO_SHAPE_SAW:
        gtk_option_menu_set_history(
                    GTK_OPTION_MENU(self->shape_opt), SAW);
        break;
    case LFO_SHAPE_SQUARE:
        gtk_option_menu_set_history(
                    GTK_OPTION_MENU(self->shape_opt), SQUARE);
        break;
    default:
        gtk_option_menu_set_history(
                    GTK_OPTION_MENU(self->shape_opt), SINE);
        break;
    }

    gtk_combo_box_set_active_iter(GTK_COMBO_BOX(self->mod1_combo), &m1iter);
    gtk_combo_box_set_active_iter(GTK_COMBO_BOX(self->mod2_combo), &m2iter);

    phat_fan_slider_set_value(PHAT_FAN_SLIDER(self->mod1_amount), mod1amt);
    phat_fan_slider_set_value(PHAT_FAN_SLIDER(self->mod2_amount), mod2amt);

    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(self->lfo_check), on);

    unblock(self);
}


GtkWidget* lfo_tab_new(void)
{
    return (GtkWidget*) g_object_new(LFO_TAB_TYPE, NULL);
}


void lfo_tab_set_patch(LfoTab* self, int patch_id)
{
    self->patch_id = patch_id;

    if (patch_id < 0)
	return;

    update_lfo(self);
}
