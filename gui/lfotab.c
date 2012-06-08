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

#include "lfotab.h"
#include "gui.h"
#include "idselector.h"
#include "lfo.h"
#include "mod_src_gui.h"
#include "names.h"
#include "patch_set_and_get.h"
#include "basic_combos.h"
#include "mod_src.h"
#include "maths.h"


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
    GtkWidget* lfo_check;
    GtkWidget* lfo_table;

    GtkWidget* shape_combo;
    GtkWidget* free_radio;
    GtkWidget* sync_radio;
    GtkWidget* beats_sb;
    GtkWidget* pos_check;
    GtkWidget* freq_fan;
    GtkWidget* delay_fan;
    GtkWidget* delay_label;
    GtkWidget* attack_fan;
    GtkWidget* attack_label;

    GtkWidget* fm1_combo;
    GtkWidget* fm2_combo;
    GtkWidget* fm1_amount;
    GtkWidget* fm2_amount;

    GtkWidget* am1_combo;
    GtkWidget* am2_combo;
    GtkWidget* am1_amount;
    GtkWidget* am2_amount;
};


G_DEFINE_TYPE(LfoTab, lfo_tab, GTK_TYPE_HBOX)

static void update_lfo(LfoTabPrivate* p);


static void lfo_tab_class_init(LfoTabClass* klass)
{
    GtkObjectClass *object_class = GTK_OBJECT_CLASS(klass);
    lfo_tab_parent_class = g_type_class_peek_parent(klass);
    g_type_class_add_private(object_class, sizeof(LfoTabPrivate));
}


static void set_lfo_sensitive(LfoTabPrivate* p, gboolean val)
{
    gboolean active = gtk_toggle_button_get_active(
                                GTK_TOGGLE_BUTTON(p->sync_radio));

    /*  setting the table itself takes care of the labels,
     *  but still need to set the phin widgets...
     */
    gtk_widget_set_sensitive(p->lfo_table,  val);

    gtk_widget_set_sensitive(p->shape_combo,val);
    gtk_widget_set_sensitive(p->free_radio, val);
    gtk_widget_set_sensitive(p->sync_radio, val);
    gtk_widget_set_sensitive(p->beats_sb,   val);
    gtk_widget_set_sensitive(p->pos_check,  val);
    gtk_widget_set_sensitive(p->delay_fan,  val);
    gtk_widget_set_sensitive(p->attack_fan, val);

    gtk_widget_set_sensitive(p->fm1_combo,  val);
    gtk_widget_set_sensitive(p->fm2_combo,  val);

    gtk_widget_set_sensitive(p->am1_combo,  val);
    gtk_widget_set_sensitive(p->am2_combo,  val);

    if (val)
    {
        gtk_widget_set_sensitive(p->fm1_amount,
            mod_src_callback_helper_lfo(p->patch_id, FM1,
                                GTK_COMBO_BOX(p->fm1_combo), p->lfo_id));
        gtk_widget_set_sensitive(p->fm2_amount,
            mod_src_callback_helper_lfo(p->patch_id, FM2,
                                GTK_COMBO_BOX(p->fm2_combo), p->lfo_id));
        gtk_widget_set_sensitive(p->am1_amount,
            mod_src_callback_helper_lfo(p->patch_id, AM1,
                                GTK_COMBO_BOX(p->am1_combo), p->lfo_id));
        gtk_widget_set_sensitive(p->am2_amount,
            mod_src_callback_helper_lfo(p->patch_id, AM2,
                                GTK_COMBO_BOX(p->am2_combo), p->lfo_id));
    }
    else
    {
        gtk_widget_set_sensitive(p->fm1_amount, FALSE);
        gtk_widget_set_sensitive(p->fm2_amount, FALSE);
        gtk_widget_set_sensitive(p->am1_amount, FALSE);
        gtk_widget_set_sensitive(p->am2_amount, FALSE);
    }

    gtk_widget_set_sensitive(p->freq_fan, !active && val);
    gtk_widget_set_sensitive(p->beats_sb,  active && val);
}


static void idsel_cb(IDSelector* ids, LfoTabPrivate* p)
{
    (void)ids;
    update_lfo(p);
}


static void on_cb(GtkToggleButton* button, LfoTabPrivate* p)
{
    patch_set_lfo_active(p->patch_id, p->lfo_id,
                    gtk_toggle_button_get_active(button));
}


static void on_cb2(GtkToggleButton* button, LfoTabPrivate* p)
{
    set_lfo_sensitive(p, gtk_toggle_button_get_active(button));
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
    gboolean active = gtk_toggle_button_get_active(
                                GTK_TOGGLE_BUTTON(button));
    gtk_widget_set_sensitive(p->freq_fan, !active);
    gtk_widget_set_sensitive(p->beats_sb,  active);
}


static void freq_cb(PhinFanSlider* fan, LfoTabPrivate* p)
{
    patch_set_lfo_freq( p->patch_id, p->lfo_id,
                        phin_fan_slider_get_value(fan));
}


static void beats_cb(PhinSliderButton* button, LfoTabPrivate* p)
{
    patch_set_lfo_sync_beats(p->patch_id, p->lfo_id,
                        phin_slider_button_get_value(button));
}

static void positive_cb(GtkToggleButton* button, LfoTabPrivate* p)
{
    patch_set_lfo_positive(p->patch_id, p->lfo_id,
                        gtk_toggle_button_get_active(button));
}


static void delay_cb(PhinFanSlider* fan, LfoTabPrivate* p)
{
    patch_set_lfo_delay(p->patch_id, p->lfo_id,
                        phin_fan_slider_get_value(fan));
}


static void attack_cb(PhinFanSlider* fan, LfoTabPrivate* p)
{
    patch_set_lfo_attack(p->patch_id, p->lfo_id,
                        phin_fan_slider_get_value(fan));
}

static void mod_src_cb(GtkComboBox* combo, LfoTabPrivate* p)
{
    int input_id;
    bool sens;

    if (combo == GTK_COMBO_BOX(p->fm1_combo))
        input_id = FM1;
    else if (combo == GTK_COMBO_BOX(p->fm2_combo))
        input_id = FM2;
    else if (combo == GTK_COMBO_BOX(p->am1_combo))
        input_id = AM1;
    else if (combo == GTK_COMBO_BOX(p->am2_combo))
        input_id = AM2;
    else
    {
        debug ("mod_src_cb called from unrecognised combo box\n");
        return;
    }

    sens = mod_src_callback_helper_lfo(p->patch_id, input_id,
                                                    combo, p->lfo_id);

    switch(input_id)
    {
    case FM1:   gtk_widget_set_sensitive(p->fm1_amount, sens);  break;
    case FM2:   gtk_widget_set_sensitive(p->fm2_amount, sens);  break;
    case AM1:   gtk_widget_set_sensitive(p->am1_amount, sens);  break;
    case AM2:   gtk_widget_set_sensitive(p->am2_amount, sens);  break;
    default:
        break;
    }
}

static void mod_amount_cb(GtkWidget* w, LfoTabPrivate* p)
{
    float val = phin_fan_slider_get_value(PHIN_FAN_SLIDER(w));

    if (w == p->fm1_amount)
        patch_set_lfo_fm1_amt(p->patch_id, p->lfo_id, val);
    else if (w == p->fm2_amount)
        patch_set_lfo_fm2_amt(p->patch_id, p->lfo_id, val);
    else if (w == p->am1_amount)
        patch_set_lfo_am1_amt(p->patch_id, p->lfo_id, val);
    else if (w == p->am2_amount)
        patch_set_lfo_am2_amt(p->patch_id, p->lfo_id, val);
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

    g_signal_connect(G_OBJECT(p->fm1_combo),    "changed",
                        G_CALLBACK(mod_src_cb),    (gpointer)p);

    g_signal_connect(G_OBJECT(p->fm1_amount),   "value-changed",
                        G_CALLBACK(mod_amount_cb), (gpointer)p);

    g_signal_connect(G_OBJECT(p->fm2_combo),    "changed",
                        G_CALLBACK(mod_src_cb),    (gpointer)p);

    g_signal_connect(G_OBJECT(p->fm2_amount),   "value-changed",
                        G_CALLBACK(mod_amount_cb), (gpointer)p);

    g_signal_connect(G_OBJECT(p->am1_combo),    "changed",
                        G_CALLBACK(mod_src_cb),    (gpointer)p);

    g_signal_connect(G_OBJECT(p->am1_amount),   "value-changed",
                        G_CALLBACK(mod_amount_cb), (gpointer)p);

    g_signal_connect(G_OBJECT(p->am2_combo),    "changed",
                        G_CALLBACK(mod_src_cb),    (gpointer)p);

    g_signal_connect(G_OBJECT(p->am2_amount),   "value-changed",
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

    static const char* shapes[] = {
        "Sine", "Triangle", "Saw", "Square", 0
    };

    GtkBox*     selfbox = GTK_BOX(self);
    GtkWidget*  title;
    GtkTable*   t;
    GtkWidget*  pad;
    GtkWidget*  label;
    GtkBox*     box = GTK_BOX(gtk_vbox_new(FALSE, 0));

    id_name* lfo_ids = mod_src_get(MOD_SRC_LFOS);

    int y = 1;

    /* collumns */
    int a1 = 0, a2 = 1;
    int b1 = 1, b2 = 2;
    int c1 = 2, c2 = 3;

    p->patch_id = -1;
    gtk_container_set_border_width(GTK_CONTAINER(self), GUI_BORDERSPACE);

    /* table */

    /* parameter selector */
    p->idsel = id_selector_new();

    id_selector_set(ID_SELECTOR(p->idsel),
                    lfo_ids,
                    ID_SELECTOR_V);

    mod_src_free(lfo_ids);

    gui_pack(selfbox, p->idsel);
    gui_pack(selfbox, gui_hpad_new(GUI_SECSPACE));
    gui_pack(selfbox, GTK_WIDGET(box));

    /* lfo title */
    title = gui_title_new("Low Frequency Oscillator");
    p->lfo_check = gtk_check_button_new();
    gtk_container_add(GTK_CONTAINER(p->lfo_check), title);
    gui_pack(box, p->lfo_check);
    gtk_widget_show(title); /* title requires additional showing */

    /* title padding */
    gui_pack(box, gui_vpad_new(GUI_SECSPACE));

    /* table */
    p->lfo_table = gtk_table_new(9, 3, FALSE);
    t = (GtkTable*) p->lfo_table;
    gui_pack(box, p->lfo_table);

    /* Frequency title */
    title = gui_title_new("Frequency");
    gui_attach(t, title, a1, c2, y, y + 1);
    gtk_widget_show(title);
    ++y;

    /* freq */
    gui_label_attach("Hrtz:", t, a1, a2, y, y + 1);
    p->free_radio = gtk_radio_button_new(NULL);
    p->freq_fan = phin_hfan_slider_new_with_range(5.0, 0.0, 50.0, 0.1);
    gui_attach(t, p->free_radio, b1, b2, y, y + 1);
    gui_attach(t, p->freq_fan, c1, c2, y, y + 1);
    ++y;

    /* sync */
    p->sync_radio = gtk_radio_button_new_from_widget(
                            GTK_RADIO_BUTTON(p->free_radio));
    p->beats_sb = phin_slider_button_new_with_range(1.0, .25, 32.0, .25, 2);
    phin_slider_button_set_format(PHIN_SLIDER_BUTTON(p->beats_sb),
                                    -1, NULL, "Beats");
    phin_slider_button_set_threshold(PHIN_SLIDER_BUTTON(p->beats_sb),
                                    GUI_THRESHOLD);
    gui_attach(t, p->sync_radio, b1, b2, y, y + 1);
    gui_attach(t, p->beats_sb, c1, c2, y, y + 1);
    gtk_widget_set_sensitive(p->beats_sb, FALSE);
    ++y;

    /* mod1 input source */
    gui_label_attach("Mod1:", t, a1, a2, y, y + 1);
    p->fm1_combo = mod_src_new_combo_with_cell();
    gui_attach(t, p->fm1_combo, c1, c2, y, y + 1);
    ++y;

    gui_label_attach("Amount:", t, a1, a2, y, y + 1);
    p->fm1_amount = phin_hfan_slider_new_with_range(0.0, -1.0, 1.0, 0.1);
    gui_attach(t, p->fm1_amount, c1, c2, y, y + 1);
    ++y;

    /* mod2 input source */
    gui_label_attach("Mod2:", t, a1, a2, y, y + 1);
    p->fm2_combo = mod_src_new_combo_with_cell();
    gui_attach(t, p->fm2_combo, c1, c2, y, y + 1);
    ++y;

    gui_label_attach("Amount:", t, a1, a2, y, y + 1);
    p->fm2_amount = phin_hfan_slider_new_with_range(0.0, -1.0, 1.0, 0.1);
    gui_attach(t, p->fm2_amount, c1, c2, y, y + 1);
    ++y;

    /* selector padding */
    pad = gui_vpad_new(GUI_SECSPACE);
    gui_attach(t, pad, a1, c2, y, y + 1);
    ++y;

    /* Frequency title */
    title = gui_title_new("Amplitude");
    gui_attach(t, title, a1, c2, y, y + 1);
    gtk_widget_show(title);
    ++y;

    /* shape */
    gui_label_attach("Shape:", t, a1, a2, y, y + 1);
    p->shape_combo = basic_combo_create(shapes);
    gui_attach(t, p->shape_combo, c1, c2, y, y + 1);
    ++y;

    /* positive */
    p->pos_check = gtk_check_button_new_with_label("Positive");
    gui_attach(t, p->pos_check, a1, a2, y, y + 1);
    ++y;

    /* delay fan */
    p->delay_label = label = gui_label_attach("Delay:", t, a1, a2, y, y+1);
    p->delay_fan = phin_hfan_slider_new_with_range(0.1, 0.0, 1.0, 0.01);
    gui_attach(t, p->delay_fan, c1, c2, y, y + 1);
    ++y;

    /* attack fan */
    p->attack_label = label = gui_label_attach("Attack:", t, a1, a2, y,y+1);
    p->attack_fan = phin_hfan_slider_new_with_range(0.1, 0.0, 1.0, 0.01);
    gui_attach(t, p->attack_fan, c1, c2, y, y + 1);
    ++y;

    /* mod1 input source */
    gui_label_attach("Mod1:", t, a1, a2, y, y + 1);
    p->am1_combo = mod_src_new_combo_with_cell();
    gui_attach(t, p->am1_combo, c1, c2, y, y + 1);
    ++y;

    gui_label_attach("Amount:", t, a1, a2, y, y + 1);
    p->am1_amount = phin_hfan_slider_new_with_range(0.0, -1.0, 1.0, 0.1);
    gui_attach(t, p->am1_amount, c1, c2, y, y + 1);
    ++y;

    /* mod2 input source */
    gui_label_attach("Mod2:", t, a1, a2, y, y + 1);
    p->am2_combo = mod_src_new_combo_with_cell();
    gui_attach(t, p->am2_combo, c1, c2, y, y + 1);
    ++y;

    gui_label_attach("Amount:", t, a1, a2, y, y + 1);
    p->am2_amount = phin_hfan_slider_new_with_range(0.0, -1.0, 1.0, 0.1);
    gui_attach(t, p->am2_amount, c1, c2, y, y + 1);
    ++y;

    /* done */
    set_lfo_sensitive(p, FALSE);
    connect(p);
}


static void update_lfo(LfoTabPrivate* p)
{
    LFOShape lfoshape;
    int shape;
    float freq, beats, delay, attack;
    bool active, sync, positive;
    GtkTreeIter iter;

    int   fm1src, fm2src;
    float fm1amt, fm2amt;
    int   am1src, am2src;
    float am1amt, am2amt;

    GtkTreeIter fm1iter, fm2iter;
    GtkTreeIter am1iter, am2iter;

    p->lfo_id = id_selector_get_id(ID_SELECTOR(p->idsel));

    debug("UPDATE LFO\t\tgot lfo id:%d\n", p->lfo_id);

    int mod_srcs = (mod_src_is_global(p->lfo_id)
                        ? MOD_SRC_GLOBALS
                        : MOD_SRC_ALL);

    block(p);
    mod_src_combo_set_model(GTK_COMBO_BOX(p->fm1_combo), mod_srcs);
    mod_src_combo_set_model(GTK_COMBO_BOX(p->fm2_combo), mod_srcs);
    mod_src_combo_set_model(GTK_COMBO_BOX(p->am1_combo), mod_srcs);
    mod_src_combo_set_model(GTK_COMBO_BOX(p->am2_combo), mod_srcs);
    unblock(p);

    debug("getting lfo (id:%d) data from patch:%d\n",
                                p->lfo_id,p->patch_id);

    lfoshape = patch_get_lfo_shape(     p->patch_id, p->lfo_id);
    freq = patch_get_lfo_freq(          p->patch_id, p->lfo_id);
    beats = patch_get_lfo_sync_beats(   p->patch_id, p->lfo_id);

    if (!mod_src_is_global(p->lfo_id))
    {
        delay = patch_get_lfo_delay(    p->patch_id, p->lfo_id);
        attack = patch_get_lfo_attack(  p->patch_id, p->lfo_id);
    }

    sync = patch_get_lfo_sync(          p->patch_id, p->lfo_id);
    positive = patch_get_lfo_positive(  p->patch_id, p->lfo_id);
    active = patch_get_lfo_active(      p->patch_id, p->lfo_id);

    fm1src = patch_get_lfo_fm1_src(  p->patch_id, p->lfo_id);
    fm1amt = patch_get_lfo_fm1_amt(  p->patch_id, p->lfo_id);
    fm2src = patch_get_lfo_fm2_src(  p->patch_id, p->lfo_id);
    fm2amt = patch_get_lfo_fm2_amt(  p->patch_id, p->lfo_id);

    am1src = patch_get_lfo_am1_src(  p->patch_id, p->lfo_id);
    am1amt = patch_get_lfo_am1_amt(  p->patch_id, p->lfo_id);
    am2src = patch_get_lfo_am2_src(  p->patch_id, p->lfo_id);
    am2amt = patch_get_lfo_am2_amt(  p->patch_id, p->lfo_id);

    debug("getting mod src combo iter with id\n");

    if (!mod_src_combo_get_iter_with_id(GTK_COMBO_BOX(p->fm1_combo),
                                                        fm1src, &fm1iter))
    {
        debug("failed to get lfo fm1 source id from combo box\n");
    }

    if (!mod_src_combo_get_iter_with_id(GTK_COMBO_BOX(p->fm2_combo),
                                                        fm2src, &fm2iter))
    {
        debug("failed to get lfo fm2 source id from combo box\n");
    }

    if (!mod_src_combo_get_iter_with_id(GTK_COMBO_BOX(p->am1_combo),
                                                        am1src, &am1iter))
    {
        debug("failed to get lfo fm1 source id from combo box\n");
    }

    if (!mod_src_combo_get_iter_with_id(GTK_COMBO_BOX(p->am2_combo),
                                                        am2src, &am2iter))
    {
        debug("failed to get lfo fm2 source id from combo box\n");
    }


    debug("...\n");

    block(p);

    phin_fan_slider_set_value(PHIN_FAN_SLIDER(p->freq_fan), freq);


    if (mod_src_is_global(p->lfo_id))
    {
        gtk_widget_hide(p->delay_fan);
        gtk_widget_hide(p->delay_label);
        gtk_widget_hide(p->attack_fan);
        gtk_widget_hide(p->attack_label);
    }
    else
    {
        phin_fan_slider_set_value(PHIN_FAN_SLIDER(p->delay_fan), delay);
        phin_fan_slider_set_value(PHIN_FAN_SLIDER(p->attack_fan), attack);
        gtk_widget_show(p->delay_fan);
        gtk_widget_show(p->delay_label);
        gtk_widget_show(p->attack_fan);
        gtk_widget_show(p->attack_label);
    }

    phin_slider_button_set_value(PHIN_SLIDER_BUTTON(p->beats_sb), beats);

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

    gtk_combo_box_set_active_iter(GTK_COMBO_BOX(p->fm1_combo), &fm1iter);
    gtk_combo_box_set_active_iter(GTK_COMBO_BOX(p->fm2_combo), &fm2iter);
    gtk_combo_box_set_active_iter(GTK_COMBO_BOX(p->am1_combo), &am1iter);
    gtk_combo_box_set_active_iter(GTK_COMBO_BOX(p->am2_combo), &am2iter);

    phin_fan_slider_set_value(PHIN_FAN_SLIDER(p->fm1_amount), fm1amt);
    phin_fan_slider_set_value(PHIN_FAN_SLIDER(p->fm2_amount), fm2amt);
    phin_fan_slider_set_value(PHIN_FAN_SLIDER(p->am1_amount), am1amt);
    phin_fan_slider_set_value(PHIN_FAN_SLIDER(p->am2_amount), am2amt);

    gtk_widget_set_sensitive(p->fm1_amount, fm1src != MOD_SRC_NONE);
    gtk_widget_set_sensitive(p->fm2_amount, fm2src != MOD_SRC_NONE);
    gtk_widget_set_sensitive(p->am1_amount, am1src != MOD_SRC_NONE);
    gtk_widget_set_sensitive(p->am2_amount, am2src != MOD_SRC_NONE);

    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(p->lfo_check), active);

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
