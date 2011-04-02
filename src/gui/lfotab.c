#include <gtk/gtk.h>
#include <phat/phat.h>
#include "lfotab.h"
#include "gui.h"
#include "idselector.h"
#include "lfo.h"
#include "mod_src.h"
#include "names.h"
#include "patch_set_and_get.h"
#include "basic_combos.h"


enum
{
    SINE,
    TRIANGLE,
    SAW,
    SQUARE
};


enum
{
    MAXSTEPS = PATCH_MAX_PITCH_STEPS,
};



typedef struct _LfoTabPrivate LfoTabPrivate;

#define LFO_TAB_GET_PRIVATE(obj)        \
    (G_TYPE_INSTANCE_GET_PRIVATE((obj), \
        LFO_TAB_TYPE, LfoTabPrivate))


struct _LfoTabPrivate
{
    int patch_id;
    int lfo_id;
    GtkWidget* idsel;
    GtkWidget* shape_combo;
    GtkWidget* lfo_check;
    GtkWidget* free_radio;
    GtkWidget* sync_radio;
    GtkWidget* beats_sb;
    GtkWidget* pos_check;
    GtkWidget* freq_fan;
    GtkWidget* delay_fan;
    GtkWidget* attack_fan;
    GtkWidget* mod1_combo;
    GtkWidget* mod2_combo;
    GtkWidget* mod1_amount;
    GtkWidget* mod2_amount;
};


G_DEFINE_TYPE(LfoTab, lfo_tab, GTK_TYPE_HBOX)

static void update_lfo(LfoTabPrivate* p);


static void lfo_tab_class_init(LfoTabClass* klass)
{
    GtkObjectClass *object_class = GTK_OBJECT_CLASS(klass);
    lfo_tab_parent_class = g_type_class_peek_parent(klass);
    g_type_class_add_private(object_class, sizeof(LfoTabPrivate));
}


static void set_sensitive(LfoTabPrivate* p, gboolean val)
{
    gboolean active;

    gtk_widget_set_sensitive(p->shape_combo, val);
    gtk_widget_set_sensitive(p->free_radio, val);
    gtk_widget_set_sensitive(p->sync_radio, val);
    gtk_widget_set_sensitive(p->beats_sb, val);
    gtk_widget_set_sensitive(p->pos_check, val);
    gtk_widget_set_sensitive(p->delay_fan, val);
    gtk_widget_set_sensitive(p->attack_fan, val);
    gtk_widget_set_sensitive(p->mod1_combo, val);
    gtk_widget_set_sensitive(p->mod2_combo, val);
    gtk_widget_set_sensitive(p->mod1_amount, val);
    gtk_widget_set_sensitive(p->mod2_amount, val);

    active  = gtk_toggle_button_get_active(
                                GTK_TOGGLE_BUTTON(p->sync_radio));

    gtk_widget_set_sensitive(p->freq_fan, !active && val);
    gtk_widget_set_sensitive(p->beats_sb, active && val);
}


static void idsel_cb(IDSelector* ids, LfoTabPrivate* p)
{
    (void)ids;
    update_lfo(p);
}


static void on_cb(GtkToggleButton* button, LfoTabPrivate* p)
{
    patch_set_lfo_on(p->patch_id, p->lfo_id,
                    gtk_toggle_button_get_active(button));
}


static void on_cb2(GtkToggleButton* button, LfoTabPrivate* p)
{
    set_sensitive(p, gtk_toggle_button_get_active(button));
}


static void shape_cb(GtkWidget* combo, LfoTabPrivate* p)
{
    (void)combo;

    LFOShape shape;

    switch(basic_combo_get_active(p->shape_combo))
    {
    case TRIANGLE:  shape = LFO_SHAPE_TRIANGLE; break;
    case SAW:       shape = LFO_SHAPE_SAW;      break;
    case SQUARE:    shape = LFO_SHAPE_SQUARE;   break;
    default:        shape = LFO_SHAPE_SINE;     break;
    }

    patch_set_lfo_shape(p->patch_id, p->lfo_id, shape);
}


static void sync_cb(GtkToggleButton* button, LfoTabPrivate* p)
{
    patch_set_lfo_sync(p->patch_id, p->lfo_id,
            gtk_toggle_button_get_active(button));
}


static void sync_cb2(GtkToggleButton* button, LfoTabPrivate* p)
{
    (void)button;
    set_sensitive(p,
            gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(p->lfo_check)));
}


static void freq_cb(PhatFanSlider* fan, LfoTabPrivate* p)
{
    patch_set_lfo_freq( p->patch_id, p->lfo_id,
                        phat_fan_slider_get_value(fan));
}


static void beats_cb(PhatSliderButton* button, LfoTabPrivate* p)
{
    patch_set_lfo_beats(p->patch_id, p->lfo_id,
                        phat_slider_button_get_value(button));
}

static void positive_cb(GtkToggleButton* button, LfoTabPrivate* p)
{
    patch_set_lfo_positive(p->patch_id, p->lfo_id,
                        gtk_toggle_button_get_active(button));
}


static void delay_cb(PhatFanSlider* fan, LfoTabPrivate* p)
{
    patch_set_lfo_delay(p->patch_id, p->lfo_id,
                        phat_fan_slider_get_value(fan));
}


static void attack_cb(PhatFanSlider* fan, LfoTabPrivate* p)
{
    patch_set_lfo_attack(p->patch_id, p->lfo_id,
                        phat_fan_slider_get_value(fan));
}

static void mod_src_cb(GtkComboBox* combo, LfoTabPrivate* p)
{
    int input_id;

    if (combo == GTK_COMBO_BOX(p->mod1_combo))
        input_id = MOD_IN1;
    else if (combo == GTK_COMBO_BOX(p->mod2_combo))
        input_id = MOD_IN2;
    else
    {
        debug ("mod_src_cb called from unrecognised combo box\n");
        return;
    }

    mod_src_callback_helper_lfo(p->patch_id, input_id, combo, p->lfo_id);
}

static void mod_amount_cb(GtkWidget* w, LfoTabPrivate* p)
{
    float val = phat_fan_slider_get_value(PHAT_FAN_SLIDER(w));

    if (w == p->mod1_amount)
        patch_set_lfo_mod1_amt(p->patch_id, p->lfo_id, val);
    else if (w == p->mod2_amount)
        patch_set_lfo_mod2_amt(p->patch_id, p->lfo_id, val);
    else
    {
        debug ("mod_amount_cb called from unrecognised widget\n");
    }
}

static void connect(LfoTabPrivate* p)
{
    g_signal_connect(G_OBJECT(p->idsel), "changed",
                    G_CALLBACK(idsel_cb), (gpointer)p);
    g_signal_connect(G_OBJECT(p->lfo_check), "toggled",
                    G_CALLBACK(on_cb), (gpointer)p);
    g_signal_connect(G_OBJECT(p->lfo_check), "toggled",
                    G_CALLBACK(on_cb2), (gpointer)p);
    g_signal_connect(G_OBJECT(p->shape_combo), "changed",
                    G_CALLBACK(shape_cb), (gpointer)p);
    g_signal_connect(G_OBJECT(p->freq_fan), "value-changed",
                    G_CALLBACK(freq_cb), (gpointer)p);
    g_signal_connect(G_OBJECT(p->sync_radio), "toggled",
                    G_CALLBACK(sync_cb), (gpointer)p);
    g_signal_connect(G_OBJECT(p->sync_radio), "toggled",
                    G_CALLBACK(sync_cb2), (gpointer)p);
    g_signal_connect(G_OBJECT(p->pos_check), "toggled",
                    G_CALLBACK(positive_cb), (gpointer)p);
    g_signal_connect(G_OBJECT(p->beats_sb), "value-changed",
                    G_CALLBACK(beats_cb), (gpointer)p);
    g_signal_connect(G_OBJECT(p->delay_fan), "value-changed",
                    G_CALLBACK(delay_cb), (gpointer)p);
    g_signal_connect(G_OBJECT(p->attack_fan), "value-changed",
                    G_CALLBACK(attack_cb), (gpointer)p);

    g_signal_connect(G_OBJECT(p->mod1_combo),    "changed",
                        G_CALLBACK(mod_src_cb),    (gpointer)p);

    g_signal_connect(G_OBJECT(p->mod1_amount),   "value-changed",
                        G_CALLBACK(mod_amount_cb), (gpointer)p);

    g_signal_connect(G_OBJECT(p->mod2_combo),    "changed",
                        G_CALLBACK(mod_src_cb),    (gpointer)p);

    g_signal_connect(G_OBJECT(p->mod2_amount),   "value-changed",
                        G_CALLBACK(mod_amount_cb), (gpointer)p);
}


static void block(LfoTabPrivate* p)
{
    g_signal_handlers_block_by_func(p->lfo_check,   on_cb,      p);
    g_signal_handlers_block_by_func(p->shape_combo, shape_cb,   p);
    g_signal_handlers_block_by_func(p->freq_fan,    freq_cb,    p);
    g_signal_handlers_block_by_func(p->sync_radio,  sync_cb,    p);
    g_signal_handlers_block_by_func(p->pos_check,   positive_cb,p);
    g_signal_handlers_block_by_func(p->beats_sb,    beats_cb,   p);
    g_signal_handlers_block_by_func(p->delay_fan,   delay_cb,   p);
    g_signal_handlers_block_by_func(p->attack_fan,  attack_cb,  p);
}


static void unblock(LfoTabPrivate* p)
{
    g_signal_handlers_unblock_by_func(p->lfo_check,  on_cb,      p);
    g_signal_handlers_unblock_by_func(p->shape_combo,shape_cb,   p);
    g_signal_handlers_unblock_by_func(p->freq_fan,   freq_cb,    p);
    g_signal_handlers_unblock_by_func(p->sync_radio, sync_cb,    p);
    g_signal_handlers_unblock_by_func(p->pos_check,  positive_cb,p);
    g_signal_handlers_unblock_by_func(p->beats_sb,   beats_cb,   p);
    g_signal_handlers_unblock_by_func(p->delay_fan,  delay_cb,   p);
    g_signal_handlers_unblock_by_func(p->attack_fan, attack_cb,  p);
}


static void lfo_tab_init(LfoTab* self)
{
    LfoTabPrivate* p = LFO_TAB_GET_PRIVATE(self);

    static const char* shapes[] = { "Sine", "Triangle", "Saw", "Square", 0 };

    GtkBox* box = GTK_BOX(self);
    GtkWidget* title;
    GtkWidget* table;
    GtkTable* t;
    GtkWidget* pad;
    GtkWidget* label;

    int y = 1;

    p->patch_id = -1;
    gtk_container_set_border_width(GTK_CONTAINER(self), GUI_BORDERSPACE);

    /* table */

    /* parameter selector */
    p->idsel = id_selector_new();
    id_selector_set(ID_SELECTOR(p->idsel), names_lfos_get(), ID_SELECTOR_V);
    gtk_box_pack_start(box, p->idsel, FALSE, FALSE, 0);
    gtk_widget_show(p->idsel);

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
    p->lfo_check = gtk_check_button_new();
    gtk_container_add(GTK_CONTAINER(p->lfo_check), title);
    gtk_table_attach_defaults(t, p->lfo_check, 0, 5, 0, 1);
    gtk_widget_show(title);
    gtk_widget_show(p->lfo_check);

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
    gtk_misc_set_alignment(GTK_MISC(label), 0.0, 0.5);
    gtk_table_attach(t, label, 1, 2, y, y + 1, GTK_FILL, 0, 0, 0);
    gtk_widget_show(label);

    p->shape_combo = basic_combo_create(shapes);
    gtk_table_attach(t, p->shape_combo, 3, 5, y, y + 1, GTK_FILL,
                                                         0, 0, 0);
    gtk_widget_show(p->shape_combo);

    ++y;

    /* freq */
    label = gtk_label_new("Frequency:");
    p->free_radio = gtk_radio_button_new(NULL);
    p->freq_fan = phat_hfan_slider_new_with_range(5.0, 0.0, 20.0, 0.1);

    gtk_misc_set_alignment(GTK_MISC(label), 0.0, 0.5);
    gtk_table_attach(t, label, 1, 2, y, y + 1, GTK_FILL, 0, 0, 0);
    gtk_table_attach(t, p->free_radio, 2, 3, y, y + 1, 0, 0, 0, 0);
    gtk_table_attach_defaults(t, p->freq_fan, 3, 5, y, y + 1);

    gtk_widget_show(label);
    gtk_widget_show(p->free_radio);
    gtk_widget_show(p->freq_fan);

    ++y;

    /* sync */
    p->sync_radio = gtk_radio_button_new_from_widget(
                            GTK_RADIO_BUTTON(p->free_radio));
    p->beats_sb = phat_slider_button_new_with_range(1.0, .25, 32.0, .25, 2);

    phat_slider_button_set_format(PHAT_SLIDER_BUTTON(p->beats_sb),
                                    -1, NULL, "Beats");

    phat_slider_button_set_threshold(PHAT_SLIDER_BUTTON(p->beats_sb),
                                    GUI_THRESHOLD);

    gtk_table_attach(t, p->sync_radio, 2, 3, y, y + 1, 0, 0, 0, 0);
    gtk_table_attach(t, p->beats_sb, 3, 5, y, y + 1, GTK_FILL, 0, 0, 0);

    gtk_widget_show(p->sync_radio);
    gtk_widget_show(p->beats_sb);
    gtk_widget_set_sensitive(p->beats_sb, FALSE);

    ++y;


    /* mod1 input source */
    label = gtk_label_new("Freq.Mod1:");
    gtk_misc_set_alignment(GTK_MISC(label), 0.0, 0.5);
    gtk_table_attach(t, label, 1, 2, y, y + 1, GTK_FILL, 0, 0, 0);
    gtk_widget_show(label);

    p->mod1_combo = mod_src_new_combo_with_cell();
    gtk_table_attach_defaults(t, p->mod1_combo, 3, 5, y, y + 1);
    gtk_widget_show(p->mod1_combo);

    ++y;

    label = gtk_label_new("Amount:");
    gtk_misc_set_alignment(GTK_MISC(label), 0.0, 0.5);
    gtk_table_attach(t, label, 1, 2, y, y + 1, GTK_FILL, 0, 0, 0);
    gtk_widget_show(label);

    p->mod1_amount = phat_hfan_slider_new_with_range(0.0, -1.0, 1.0, 0.1);
    gtk_table_attach_defaults(t, p->mod1_amount, 3, 5, y, y + 1);
    gtk_widget_show(p->mod1_amount);

    ++y;

    /* mod2 input source */
    label = gtk_label_new("Freq.Mod2:");
    gtk_misc_set_alignment(GTK_MISC(label), 0.0, 0.5);
    gtk_table_attach(t, label, 1, 2, y, y + 1, GTK_FILL, 0, 0, 0);
    gtk_widget_show(label);

    p->mod2_combo = mod_src_new_combo_with_cell();
    gtk_table_attach_defaults(t, p->mod2_combo, 3, 5, y, y + 1);
    gtk_widget_show(p->mod2_combo);

    ++y;

    label = gtk_label_new("Amount:");
    gtk_misc_set_alignment(GTK_MISC(label), 0.0, 0.5);
    gtk_table_attach(t, label, 1, 2, y, y + 1, GTK_FILL, 0, 0, 0);
    gtk_widget_show(label);

    p->mod2_amount = phat_hfan_slider_new_with_range(0.0, -1.0, 1.0, 0.1);
    gtk_table_attach_defaults(t, p->mod2_amount, 3, 5, y, y + 1);
    gtk_widget_show(p->mod2_amount);

    ++y;

//    gtk_table_set_row_spacing(t, y, GUI_SPACING);

    /* delay fan */
    label = gtk_label_new("Delay:");
    p->delay_fan = phat_hfan_slider_new_with_range(0.1, 0.0, 1.0, 0.01);

    gtk_misc_set_alignment(GTK_MISC(label), 0.0, 0.5);
    gtk_table_attach(t, label, 1, 2, y, y + 1, GTK_FILL, 0, 0, 0);
    gtk_table_attach_defaults(t, p->delay_fan, 3, 4, y, y + 1);

    gtk_widget_show(label);
    gtk_widget_show(p->delay_fan);

    ++y;

//    gtk_table_set_col_spacing(t, 3, GUI_SPACING*2);

    /* attack fan */
    label = gtk_label_new("Attack:");
    p->attack_fan = phat_hfan_slider_new_with_range(0.1, 0.0, 1.0, 0.01);

    gtk_misc_set_alignment(GTK_MISC(label), 0.0, 0.5);
    gtk_table_attach(t, label, 1, 2, y, y + 1, GTK_FILL, 0, 0, 0);
    gtk_table_attach_defaults(t, p->attack_fan, 3, 4, y, y + 1);

    gtk_widget_show(label);
    gtk_widget_show(p->attack_fan);

    /* positive */
    p->pos_check = gtk_check_button_new_with_label("Positive");
    gtk_table_attach(t, p->pos_check, 4, 5, y, y + 1, GTK_FILL, 0, 0, 0);
    gtk_widget_show(p->pos_check);

    ++y;

    gtk_widget_show(pad);


    /* done */
    set_sensitive(p, FALSE);
    connect(p);
}


static void update_lfo(LfoTabPrivate* p)
{
    LFOShape lfoshape;
    int shape;
    float freq, beats, delay, attack;
    gboolean sync, positive;
    gboolean on;
    GtkTreeIter iter;

    int   mod1src, mod2src;
    float mod1amt, mod2amt;

    GtkTreeIter m1iter, m2iter;

    p->lfo_id = id_selector_get_id(ID_SELECTOR(p->idsel));

    int mod_srcs = (patch_lfo_is_global(p->lfo_id)
                        ? MOD_SRC_INPUTS_GLOBAL
                        : MOD_SRC_INPUTS_ALL);

    block(p);
    mod_src_combo_set_model(GTK_COMBO_BOX(p->mod1_combo), mod_srcs);
    mod_src_combo_set_model(GTK_COMBO_BOX(p->mod2_combo), mod_srcs);
    unblock(p);

    patch_get_lfo_shape(    p->patch_id, p->lfo_id, &lfoshape);
    patch_get_lfo_freq(     p->patch_id, p->lfo_id, &freq);
    patch_get_lfo_beats(    p->patch_id, p->lfo_id, &beats);
    patch_get_lfo_delay(    p->patch_id, p->lfo_id, &delay);
    patch_get_lfo_attack(   p->patch_id, p->lfo_id, &attack);
    patch_get_lfo_sync(     p->patch_id, p->lfo_id, &sync);
    patch_get_lfo_positive( p->patch_id, p->lfo_id, &positive);
    patch_get_lfo_on(       p->patch_id, p->lfo_id, &on);

    patch_get_lfo_mod1_src( p->patch_id, p->lfo_id, &mod1src);
    patch_get_lfo_mod1_amt( p->patch_id, p->lfo_id, &mod1amt);
    patch_get_lfo_mod2_src( p->patch_id, p->lfo_id, &mod2src);
    patch_get_lfo_mod2_amt( p->patch_id, p->lfo_id, &mod2amt);

    if (!mod_src_combo_get_iter_with_id(GTK_COMBO_BOX(p->mod1_combo),
                                                        mod1src, &m1iter))
    {
        debug("failed to get lfo mod1 source id from combo box\n");
    }

    if (!mod_src_combo_get_iter_with_id(GTK_COMBO_BOX(p->mod2_combo),
                                                        mod2src, &m2iter))
    {
        debug("failed to get lfo mod2 source id from combo box\n");
    }

    block(p);

    phat_fan_slider_set_value(PHAT_FAN_SLIDER(p->freq_fan), freq);
    phat_fan_slider_set_value(PHAT_FAN_SLIDER(p->delay_fan), delay);
    phat_fan_slider_set_value(PHAT_FAN_SLIDER(p->attack_fan), attack);

    phat_slider_button_set_value(PHAT_SLIDER_BUTTON(p->beats_sb), beats);

    if (sync)
        gtk_toggle_button_set_active(
                    GTK_TOGGLE_BUTTON(p->sync_radio), TRUE);
    else
        gtk_toggle_button_set_active(
                    GTK_TOGGLE_BUTTON(p->free_radio), TRUE);

    gtk_toggle_button_set_active(
                    GTK_TOGGLE_BUTTON(p->pos_check), positive);

    switch (lfoshape)
    {
    case LFO_SHAPE_TRIANGLE:    shape = TRIANGLE;       break;
    case LFO_SHAPE_SAW:         shape = SAW;            break;
    case LFO_SHAPE_SQUARE:      shape = SQUARE;         break;
    default:                    shape = SINE;           break;
    }

    if (basic_combo_get_iter_at_index(p->shape_combo, shape, &iter))
    {
        gtk_combo_box_set_active_iter(GTK_COMBO_BOX(p->shape_combo),
                                                                &iter); 
    }

    gtk_combo_box_set_active_iter(GTK_COMBO_BOX(p->mod1_combo), &m1iter);
    gtk_combo_box_set_active_iter(GTK_COMBO_BOX(p->mod2_combo), &m2iter);

    phat_fan_slider_set_value(PHAT_FAN_SLIDER(p->mod1_amount), mod1amt);
    phat_fan_slider_set_value(PHAT_FAN_SLIDER(p->mod2_amount), mod2amt);

    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(p->lfo_check), on);

    unblock(p);
}


GtkWidget* lfo_tab_new(void)
{
    return (GtkWidget*) g_object_new(LFO_TAB_TYPE, NULL);
}


void lfo_tab_set_patch(LfoTab* self, int patch_id)
{
    LfoTabPrivate* p = LFO_TAB_GET_PRIVATE(self);
    
    p->patch_id = patch_id;

    if (patch_id < 0)
	return;

    update_lfo(p);
}
