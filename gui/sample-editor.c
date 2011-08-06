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


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <gtk/gtk.h>
#include "petri-foo.h"
#include "patch_set_and_get.h"
#include "waveform.h"
#include "gui.h"
#include "mixer.h"
#include "sample-editor.h"

#include "basic_combos.h"

enum
{
     ZOOM_MIN = 1,
     ZOOM_MAX = 100
};

typedef struct _SampleEditorPrivate SampleEditorPrivate;


struct _SampleEditorPrivate
{
    GtkWidget*  window;
    GtkWidget*  waveform;
    GtkWidget*  hscroll;
    GtkObject*  hscrolladj;
    GtkWidget*  fade_spin;
    GtkWidget*  xfade_spin;
    GtkWidget*  mark_combo;
    GtkWidget*  mark_spin;
    GtkWidget*  mark_val;

    gboolean ignore_callback;
    gboolean ignore_mark_change;

    float   zoom_pc;
    float   range;

    int     old_play_start;
    int     old_play_stop;

    int     old_loop_start;
    int     old_loop_stop;
    int     old_fade;
    int     old_xfade;

    int     patch;
};


GtkWidget*  wf_thumb = 0;


SampleEditorPrivate* sample_editor_private_new()
{
    SampleEditorPrivate* p = malloc(sizeof(*p));

    if (!p)
        return 0;

    p->window =     0;
    p->waveform =   0;
    p->hscroll =    0;
    p->hscrolladj = 0;
    p->fade_spin =  0;
    p->xfade_spin = 0;
    p->mark_combo = 0;
    p->mark_spin =  0;
    p->mark_val =   0;

    p->ignore_callback =    FALSE;
    p->ignore_mark_change = FALSE;

    p->zoom_pc =    1.0;
    p->range =      1.0;

    p->old_play_start = 0;
    p->old_play_stop =  0;

    p->old_loop_start = 0;
    p->old_loop_stop =  0;
    p->old_fade =       0;
    p->old_xfade =      0;

    p->patch = -1;

    return p;
}


static SampleEditorPrivate* se = 0;


static void zoom_adj(void);
static void update_mark_spin(void);
static void update_fade_spins(void);
static void cb_mark_combo_changed(GtkWidget* combo, gpointer data);

static void cb_close(GtkWidget* widget, gpointer data)
{
    (void)widget;(void)data;
    mixer_note_off_with_id(se->patch, patch_get_note(se->patch));
    gtk_widget_hide(se->window);
}

static void cb_play(GtkWidget* widget, gpointer data)
{
    (void)widget;(void)data;
    mixer_note_off_with_id(se->patch, patch_get_note(se->patch));
    mixer_note_on_with_id(se->patch, patch_get_note(se->patch), 1.0);
}

static void cb_stop(GtkWidget* widget, gpointer data)
{
    (void)widget;(void)data;
    mixer_note_off_with_id(se->patch, patch_get_note(se->patch));
}

static void cb_reset(GtkWidget* widget, gpointer data)
{
    (void)widget;(void)data;
    patch_set_mark_frame(se->patch, WF_MARK_PLAY_START, se->old_play_start);
    patch_set_mark_frame(se->patch, WF_MARK_PLAY_STOP,  se->old_play_stop);
    patch_set_mark_frame(se->patch, WF_MARK_LOOP_START, se->old_loop_start);
    patch_set_mark_frame(se->patch, WF_MARK_LOOP_STOP,  se->old_loop_stop);
    cb_mark_combo_changed(se->mark_combo, 0);
    gtk_widget_queue_draw(se->waveform);
    gtk_widget_queue_draw(wf_thumb);
}

static void cb_clear(GtkWidget* widget, gpointer data)
{
    (void)widget;
    char *op = data;

    if (strcmp(op, "loop") == 0)
    {
        int play_start = patch_get_mark_frame(se->patch,WF_MARK_PLAY_START);
        int play_stop = patch_get_mark_frame(se->patch, WF_MARK_PLAY_STOP);
        patch_set_mark_frame(se->patch, WF_MARK_LOOP_START, play_start);
        patch_set_mark_frame(se->patch, WF_MARK_LOOP_STOP, play_stop);
     }
     else if (strcmp(op, "play") == 0)
     {
        int frames = patch_get_mark_frame(se->patch, WF_MARK_STOP);
        patch_set_mark_frame(se->patch, WF_MARK_PLAY_START, 0);
        patch_set_mark_frame(se->patch, WF_MARK_PLAY_STOP, frames - 1);
     }

    cb_mark_combo_changed(se->mark_combo, 0);
    gtk_widget_queue_draw(se->waveform);
    gtk_widget_queue_draw(wf_thumb);
}


static void update_mark_val(int val)
{
    const float* wav = patch_get_sample(se->patch);
    char buf[40];
    snprintf(buf, 40, "%2.6f", wav[val * 2]);
    gtk_label_set_label(GTK_LABEL(se->mark_val), buf);
}


static void cb_scroll(GtkWidget* scroll, gpointer data)
{
    (void)data;
    float val = gtk_range_get_value(GTK_RANGE(scroll));
    waveform_set_range(WAVEFORM(se->waveform), val, val + se->range);
}


static void cb_mark_spin_changed(GtkWidget* spin, gpointer data)
{
    (void)spin;(void)data;
    int val;
    int mark;

    val = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(se->mark_spin));
    mark = waveform_get_mark(WAVEFORM(se->waveform));

    patch_set_mark_frame(se->patch, mark, val);
    update_mark_val(val);
    update_fade_spins(); /* so xfade spin button range is set correctly */
    gtk_widget_queue_draw(se->waveform);
    gtk_widget_queue_draw(wf_thumb);
}


static void cb_fade_spin_changed(GtkWidget* spin, gpointer data)
{
    (void)spin;(void)data;
    int val = gtk_spin_button_get_value_as_int(
                                        GTK_SPIN_BUTTON(se->fade_spin));
    patch_set_fade_samples(se->patch, val);
}


static void cb_xfade_spin_changed(GtkWidget* spin, gpointer data)
{
    (void)spin;(void)data;
    int val = gtk_spin_button_get_value_as_int(
                                        GTK_SPIN_BUTTON(se->xfade_spin));
    patch_set_xfade_samples(se->patch, val);
    update_mark_spin(); /* so mark spin range is set correctly */
}


static void update_mark_spin(void)
{
    int min;
    int max;

    int mark = waveform_get_mark(WAVEFORM(se->waveform));
    int val =  patch_get_mark_frame_range(se->patch, mark, &min, &max);

    if (val < 0)
    {
        val = patch_get_mark_frame(se->patch, mark);
        gtk_widget_set_sensitive(se->mark_spin, FALSE);
    }
    else
        gtk_widget_set_sensitive(se->mark_spin, TRUE);

    g_signal_handlers_block_by_func(se->mark_spin, cb_mark_spin_changed, 0);
    gtk_spin_button_set_range(GTK_SPIN_BUTTON(se->mark_spin), min, max);
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(se->mark_spin), val);
    g_signal_handlers_unblock_by_func(se->mark_spin,cb_mark_spin_changed,0);

    update_fade_spins(); /* ie set the range of xfade */
    update_mark_val(val);
}


static void update_fade_spins(void)
{
    int fade =      patch_get_fade_samples(se->patch);
    int max_fade =  patch_get_max_fade_samples(se->patch);
    int xfade =     patch_get_xfade_samples(se->patch);
    int max_xfade = patch_get_max_xfade_samples(se->patch);

    g_signal_handlers_block_by_func(se->fade_spin, cb_fade_spin_changed, 0);
    gtk_spin_button_set_range(GTK_SPIN_BUTTON(se->fade_spin), 0, max_fade);
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(se->fade_spin), fade);
    g_signal_handlers_unblock_by_func(  se->fade_spin,
                                        cb_fade_spin_changed,
                                        0);

    g_signal_handlers_block_by_func(se->xfade_spin,
                                    cb_xfade_spin_changed,
                                    0);

    gtk_spin_button_set_range(GTK_SPIN_BUTTON(se->xfade_spin), 0,
                                                            max_xfade);
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(se->xfade_spin), xfade);
    g_signal_handlers_unblock_by_func(se->xfade_spin,
                                                cb_xfade_spin_changed, 0);
}


static void cb_mark_combo_changed(GtkWidget* combo, gpointer data)
{
    (void)combo;(void)data;
    se->ignore_mark_change = TRUE;
    waveform_set_mark(WAVEFORM(se->waveform),
                gtk_combo_box_get_active(GTK_COMBO_BOX(se->mark_combo)));
    update_mark_spin();
}

static void cb_wf_play_changed(void)
{
    /*  there's some chance the play point that was changed is
        selected and shown in the mark spin. */
    update_mark_spin();
    update_fade_spins();
    gtk_widget_queue_draw(wf_thumb);
}

static void cb_wf_loop_changed(void)
{
    /*  there's some chance the play point that was changed is
        selected and shown in the mark spin. */
    update_mark_spin();
    update_fade_spins();
    gtk_widget_queue_draw(wf_thumb);
}

static void cb_wf_mark_changed(void)
{
    /* ditto cb_wf_play_changed */
    if (se->ignore_mark_change)
    {
        se->ignore_mark_change = FALSE;
        return;
    }
    gtk_combo_box_set_active(GTK_COMBO_BOX(se->mark_combo),
                                waveform_get_mark(WAVEFORM(se->waveform)));
}

static void cb_wf_view_changed(void)
{
    float start, stop;
    waveform_get_range(WAVEFORM(se->waveform), &start, &stop);

    g_signal_handlers_block_by_func(se->hscroll, cb_scroll, 0);
    zoom_adj();
    g_signal_handlers_unblock_by_func(se->hscroll, cb_scroll, 0);

}


static void zoom_adj(void)
{
    float page_size=0;
    float step_inc=0;
    float page_inc=0;
    float val=0;
    float start=0;
    float stop=0;

    waveform_get_range(WAVEFORM(se->waveform), &start, &stop);
    gtk_range_set_range(GTK_RANGE(se->hscroll), start, stop);

    val = start;
    se->range = page_size = stop - start;

    step_inc = se->range / 100;
    page_inc = step_inc / 10;
    se->hscrolladj = gtk_adjustment_new(val, 0.0, 1.0,
                                            step_inc,
                                            page_inc,
                                            page_size);

    gtk_range_set_adjustment(GTK_RANGE(se->hscroll),
                                            GTK_ADJUSTMENT(se->hscrolladj));

     /* emit value-changed signal so waveform is redrawn with
      * the new dimensions */
    g_signal_emit_by_name(G_OBJECT(se->hscroll), "value-changed");

    return;
}

static void cb_zoom_1to1(GtkWidget* widget, gpointer data)
{
    (void)widget;(void)data;
    int frames;
    int width;
    float zoom_level;
    float zoom_multiplier;

    waveform_get_size(WAVEFORM(se->waveform), &width, NULL);
    frames = patch_get_frames(se->patch);

    zoom_level = width / (double)frames;

    zoom_multiplier = se->range / zoom_level;

    waveform_zoom(WAVEFORM(se->waveform), zoom_multiplier, 0.5);
    zoom_adj();
}

static void cb_zoom_all(GtkWidget* widget, gpointer data)
{
    (void)widget;(void)data;
    waveform_set_range(WAVEFORM(se->waveform), 0.0, 1.0);
    zoom_adj();
}


static void cb_zoom_in(GtkWidget* widget, gpointer data)
{
    (void)widget;(void)data;
    waveform_zoom(WAVEFORM(se->waveform), 2.0, 0.5);
    zoom_adj();
}

static void cb_zoom_out(GtkWidget* widget, gpointer data)
{
    (void)widget;(void)data;
    waveform_zoom(WAVEFORM(se->waveform), 0.5, 0.5);
    zoom_adj();
}


void sample_editor_show(int id)
{
    waveform_set_patch(WAVEFORM(se->waveform), id);

    se->patch = id;

    se->old_play_start = patch_get_mark_frame(se->patch,
                                                WF_MARK_PLAY_START);
    se->old_play_stop =  patch_get_mark_frame(se->patch,
                                                WF_MARK_PLAY_STOP);
    se->old_loop_start = patch_get_mark_frame(se->patch,
                                                WF_MARK_LOOP_START);
    se->old_loop_stop =  patch_get_mark_frame(se->patch,
                                                WF_MARK_LOOP_STOP);

    gtk_combo_box_set_active(GTK_COMBO_BOX(se->mark_combo),
                                                WF_MARK_PLAY_START);
    update_fade_spins();
    gtk_widget_show(se->window);
}


void sample_editor_update(void)
{
    waveform_set_range(WAVEFORM(se->waveform), 0.0, 1.0);
    gtk_widget_queue_draw(se->waveform);
}



void sample_editor_set_thumb(GtkWidget* thumb)
{
    wf_thumb = thumb;
}


void sample_editor_init(GtkWidget * parent)
{
    GtkWindow* w;
    GtkWidget* master_vbox;
    GtkWidget* hbox;
    GtkWidget* button;
    GtkWidget* image;
    GtkWidget* label;
    GtkWidget* tmp;

    se = sample_editor_private_new();

    /* main window */
    se->window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    w = GTK_WINDOW(se->window);
    gtk_window_set_title(w, "Edit Sample");
    gtk_window_set_resizable(w, TRUE);
    gtk_window_set_transient_for(w, GTK_WINDOW(parent));
    gtk_window_set_modal(w, FALSE);
    g_signal_connect(G_OBJECT(w), "delete-event", G_CALLBACK(cb_close),
                                                                   NULL);

    /* master vbox */
    master_vbox = gtk_vbox_new(FALSE, GUI_SPACING);
    gtk_container_add(GTK_CONTAINER(se->window), master_vbox);
    gtk_container_set_border_width(GTK_CONTAINER(se->window), GUI_SPACING);
    gtk_widget_show(master_vbox);


    /* top row hbox */
    hbox = gtk_hbox_new(FALSE, GUI_SPACING);
    gtk_box_pack_start(GTK_BOX(master_vbox), hbox, FALSE, FALSE, 0);
    gtk_widget_show(hbox);

    /* play button */
    image = gtk_image_new_from_stock(GTK_STOCK_MEDIA_PLAY,
                                     GTK_ICON_SIZE_SMALL_TOOLBAR);
    button = gtk_button_new();
    gtk_container_add(GTK_CONTAINER(button), image);
    gtk_box_pack_start(GTK_BOX (hbox), button, FALSE, FALSE, 0);
    g_signal_connect_swapped(G_OBJECT (button), "clicked",
                                    G_CALLBACK(cb_play), NULL);
    gtk_widget_show(image);
    gtk_widget_show(button);

    /* stop button */
    image = gtk_image_new_from_stock(GTK_STOCK_MEDIA_STOP,
                                     GTK_ICON_SIZE_SMALL_TOOLBAR);
    button = gtk_button_new ( );
    gtk_container_add (GTK_CONTAINER (button), image);
    gtk_box_pack_start (GTK_BOX (hbox), button, FALSE, FALSE, 0);
    g_signal_connect_swapped (G_OBJECT (button), "clicked",
                                    G_CALLBACK (cb_stop), NULL);
    gtk_widget_show(image);
    gtk_widget_show(button);

    /* separator */
    tmp = gtk_vseparator_new();
    gtk_box_pack_start(GTK_BOX(hbox), tmp, FALSE, FALSE, 0);
    gtk_widget_show(tmp);

    /* fade spin button */
    label = gtk_label_new("Fade:");
    gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 0);
    gtk_widget_show(label);

    se->fade_spin = gtk_spin_button_new_with_range(0, 1, 1);
    gtk_box_pack_start(GTK_BOX(hbox), se->fade_spin, FALSE, FALSE, 0);
    gtk_widget_show(se->fade_spin);
    g_signal_connect(G_OBJECT(se->fade_spin), "value-changed",
                            G_CALLBACK(cb_fade_spin_changed), NULL);

    /* X-fade spin button */
    label = gtk_label_new("X-Fade:");
    gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 0);
    gtk_widget_show(label);

    se->xfade_spin = gtk_spin_button_new_with_range(0, 1, 1);
    gtk_box_pack_start(GTK_BOX(hbox), se->xfade_spin, FALSE, FALSE, 0);
    gtk_widget_show(se->xfade_spin);
    g_signal_connect(G_OBJECT(se->xfade_spin), "value-changed",
                            G_CALLBACK(cb_xfade_spin_changed), NULL);

    /* separator */
    tmp = gtk_vseparator_new();
    gtk_box_pack_start(GTK_BOX(hbox), tmp, FALSE, FALSE, 0);
    gtk_widget_show(tmp);


    /* mark combo */
    se->mark_combo = basic_combo_create(waveform_get_mark_names());
    gtk_box_pack_start(GTK_BOX(hbox), se->mark_combo, FALSE, FALSE, 0);
    gtk_widget_show(se->mark_combo);
    g_signal_connect(G_OBJECT(se->mark_combo), "changed",
                            G_CALLBACK(cb_mark_combo_changed), NULL);

    se->mark_spin = gtk_spin_button_new_with_range(0, 1, 1);
    gtk_box_pack_start(GTK_BOX(hbox), se->mark_spin, FALSE, FALSE, 0);
    gtk_widget_show(se->mark_spin);
    g_signal_connect(G_OBJECT(se->mark_spin), "value-changed",
                            G_CALLBACK(cb_mark_spin_changed), NULL);

    /* mark spin value label */
    se->mark_val = gtk_label_new("");
    gtk_box_pack_start(GTK_BOX(hbox), se->mark_val, FALSE, FALSE, 0);
    gtk_widget_show(se->mark_val);

    /* separator */
    tmp = gtk_vseparator_new();
    gtk_box_pack_start(GTK_BOX(hbox), tmp, FALSE, FALSE, 0);
    gtk_widget_show(tmp);

     /* loop points clear button */
     button = gtk_button_new_with_label ("Loop");
     gtk_box_pack_end (GTK_BOX (hbox), button, FALSE, FALSE, 0);
     g_signal_connect (G_OBJECT (button), "clicked", G_CALLBACK (cb_clear),
           (gpointer) "loop");
     gtk_widget_show (button);

     /* play points clear button */
     button = gtk_button_new_with_label ("Play");
     gtk_box_pack_end (GTK_BOX (hbox), button, FALSE, FALSE, 0);
     g_signal_connect (G_OBJECT (button), "clicked", G_CALLBACK (cb_clear),
                                                        (gpointer) "play");
     gtk_widget_show (button);


     /* clear label */
     label = gtk_label_new ("Clear Points:");
     gtk_box_pack_end (GTK_BOX (hbox), label, FALSE, FALSE, 0);
     gtk_widget_show (label);

     /* waveform */
     se->waveform = waveform_new();
     waveform_set_patch(        WAVEFORM(se->waveform), -1);
     waveform_set_size(         WAVEFORM(se->waveform),  512, 256);
     waveform_set_interactive(  WAVEFORM(se->waveform), TRUE);
     gtk_box_pack_start(GTK_BOX(master_vbox), se->waveform, TRUE, TRUE, 0);
     gtk_widget_show(se->waveform);
     g_signal_connect(G_OBJECT(se->waveform), "play-changed",
                            G_CALLBACK(cb_wf_play_changed), NULL);
     g_signal_connect(G_OBJECT(se->waveform), "loop-changed",
                            G_CALLBACK(cb_wf_loop_changed), NULL);
     g_signal_connect(G_OBJECT(se->waveform), "mark-changed",
                            G_CALLBACK(cb_wf_mark_changed), NULL);
     g_signal_connect(G_OBJECT(se->waveform), "view-changed",
                            G_CALLBACK(cb_wf_view_changed), NULL);

     /* waveform scrollbar */
     se->hscrolladj = gtk_adjustment_new(0.0, 0.0, 1.0, 0.0, 0.0, 1.0);
     se->hscroll = gtk_hscrollbar_new(GTK_ADJUSTMENT(se->hscrolladj));
     gtk_box_pack_start(GTK_BOX(master_vbox), se->hscroll, FALSE, FALSE, 0);
     g_signal_connect(G_OBJECT(se->hscroll), "value-changed",
                                            G_CALLBACK (cb_scroll), NULL);
     gtk_widget_show (se->hscroll);

     /* hbox */
     hbox = gtk_hbox_new (FALSE, GUI_SPACING);
     gtk_box_pack_start (GTK_BOX (master_vbox), hbox, FALSE, FALSE, 0);
     gtk_widget_show (hbox);

    /* zoom-out button */
    image = gtk_image_new_from_stock(GTK_STOCK_ZOOM_OUT,
                                     GTK_ICON_SIZE_SMALL_TOOLBAR);
    button = gtk_button_new();
    gtk_container_add(GTK_CONTAINER(button), image);
    gtk_box_pack_start(GTK_BOX (hbox), button, FALSE, FALSE, 0);
    g_signal_connect_swapped(G_OBJECT (button), "clicked",
                                    G_CALLBACK(cb_zoom_out), NULL);
    gtk_widget_show(image);
    gtk_widget_show(button);

    /* zoom-in button */
    image = gtk_image_new_from_stock(GTK_STOCK_ZOOM_IN,
                                     GTK_ICON_SIZE_SMALL_TOOLBAR);
    button = gtk_button_new();
    gtk_container_add(GTK_CONTAINER(button), image);
    gtk_box_pack_start(GTK_BOX (hbox), button, FALSE, FALSE, 0);
    g_signal_connect_swapped(G_OBJECT (button), "clicked",
                                    G_CALLBACK(cb_zoom_in), NULL);
    gtk_widget_show(image);
    gtk_widget_show(button);

    /* zoom-1:1 button */
    image = gtk_image_new_from_stock(GTK_STOCK_ZOOM_100,
                                     GTK_ICON_SIZE_SMALL_TOOLBAR);
    button = gtk_button_new();
    gtk_container_add(GTK_CONTAINER(button), image);
    gtk_box_pack_start(GTK_BOX (hbox), button, FALSE, FALSE, 0);
    g_signal_connect_swapped(G_OBJECT (button), "clicked",
                                    G_CALLBACK(cb_zoom_1to1), NULL);
    gtk_widget_show(image);
    gtk_widget_show(button);

    /* zoom-all button */
    image = gtk_image_new_from_stock(GTK_STOCK_ZOOM_FIT,
                                     GTK_ICON_SIZE_SMALL_TOOLBAR);
    button = gtk_button_new();
    gtk_container_add(GTK_CONTAINER(button), image);
    gtk_box_pack_start(GTK_BOX (hbox), button, FALSE, FALSE, 0);
    g_signal_connect_swapped(G_OBJECT (button), "clicked",
                                    G_CALLBACK(cb_zoom_all), NULL);
    gtk_widget_show(image);
    gtk_widget_show(button);

     /* close button */
     button = gtk_button_new_from_stock (GTK_STOCK_CLOSE);
     gtk_box_pack_end (GTK_BOX (hbox), button, FALSE, FALSE, 0);
     g_signal_connect (G_OBJECT (button), "clicked", G_CALLBACK (cb_close),
		       NULL);
     gtk_widget_show (button);

     /* reset button */
     button = gtk_button_new_with_label ("Reset");
     gtk_box_pack_end (GTK_BOX (hbox), button, FALSE, FALSE, 0);
     g_signal_connect (G_OBJECT (button), "clicked", G_CALLBACK (cb_reset),
		       NULL);
     gtk_widget_show (button);
}
