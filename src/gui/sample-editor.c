#include <stdio.h>
#include <string.h>
#include <gtk/gtk.h>
#include "petri-foo.h"
#include "patch_set_and_get.h"
#include "waveform.h"
#include "gui.h"
#include "mixer.h"
#include "sample-editor.h"

#include "basic_combos.h"

static GtkWidget *window;
static GtkWidget *waveform;
static GtkWidget *hscroll;
static GtkAdjustment *hscrolladj;

static GtkWidget* wf_thumb = 0;

enum
{
     ZOOM_MIN = 1,
     ZOOM_MAX = 100
};




static int ignore_callback = 0;
static int zoom = ZOOM_MIN;
static float range = 1.0;

static int old_play_start, old_play_stop;
static int old_loop_start, old_loop_stop;
static int patch;

static GtkWidget*   mark_combo;
static GtkWidget*   mark_spin;

/*
static int old_xfade, old_fade_in, old_fade_out;
static GtkWidget* spin_fade_in;
static GtkWidget* spin_fade_out;
static GtkWidget* spin_xfade;
*/



static void cb_mark_combo_changed(GtkWidget* combo, gpointer data);

static void cb_close (GtkWidget * widget, gpointer data)
{
    (void)widget;(void)data;
    debug ("Hiding sample editor\n");
    mixer_note_off_with_id (patch, patch_get_note (patch));
    gtk_widget_hide (window);
}

static void cb_play (GtkWidget * widget, gpointer data)
{
    (void)widget;(void)data;
     mixer_note_on_with_id (patch, patch_get_note (patch), 1.0);
}

static void cb_stop (GtkWidget * widget, gpointer data)
{
    (void)widget;(void)data;
     mixer_note_off_with_id (patch, patch_get_note (patch));
}

static void cb_reset (GtkWidget * widget, gpointer data)
{
    (void)widget;(void)data;
     debug ("Restoring initial values\n");
     patch_set_sample_start(patch, old_play_start);
     patch_set_sample_stop(patch, old_play_stop);
     patch_set_loop_start(patch, old_loop_start);
     patch_set_loop_stop(patch, old_loop_stop);
    cb_mark_combo_changed(mark_combo, 0);
     gtk_widget_queue_draw(waveform);
}

static void cb_clear (GtkWidget * widget, gpointer data)
{
    (void)widget;
    char *op = data;
    int play_start, play_stop;
    int frames;

    if (strcmp (op, "loop") == 0)
    {
        play_start = patch_get_sample_start (patch);
        play_stop = patch_get_sample_stop (patch);
        patch_set_loop_start (patch, play_start);
        patch_set_loop_stop (patch, play_stop);
     }
     else if (strcmp (op, "play") == 0)
     {
        frames = patch_get_frames (patch);
        patch_set_sample_start (patch, 0);
        patch_set_sample_stop (patch, frames - 1);
     }

    cb_mark_combo_changed(mark_combo, 0);
    gtk_widget_queue_draw(waveform);
}

static void cb_scroll (GtkWidget * scroll, gpointer data)
{
    (void)data;
     float val;

     val = gtk_range_get_value (GTK_RANGE (scroll));
     waveform_set_range (WAVEFORM (waveform), val, val + range);
}


static void cb_mark_spin_changed(GtkWidget * spin, gpointer data)
{
    waveform_set_mark_frame(WAVEFORM(waveform),
            gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(mark_spin)));
    gtk_widget_queue_draw(waveform);
}


static void cb_mark_combo_changed(GtkWidget* combo, gpointer data)
{
    int val;
    int min;
    int max;

    waveform_set_mark(WAVEFORM(waveform),
                    gtk_combo_box_get_active(GTK_COMBO_BOX(mark_combo)));

    val = waveform_get_mark_spin_range(WAVEFORM(waveform), &min, &max);

    if (val < 0)
    {
        val = waveform_get_mark_frame(WAVEFORM(waveform));
        gtk_widget_set_sensitive(mark_spin, FALSE);
    }
    else
        gtk_widget_set_sensitive(mark_spin, TRUE);

    g_signal_handlers_block_by_func(mark_spin, cb_mark_spin_changed, 0);
    gtk_spin_button_set_range(GTK_SPIN_BUTTON(mark_spin), min, max);
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(mark_spin), val);
    g_signal_handlers_unblock_by_func(mark_spin, cb_mark_spin_changed, 0);
}


/*
static void cb_xfade_changed (GtkWidget * spin, gpointer data)
{
    (void)spin;(void)data;
    if (ignore_callback)
        return;
 
    int xfade = patch_get_sample_xfade(patch);
    int val;

    val = gtk_spin_button_get_value_as_int(
                            GTK_SPIN_BUTTON (spin_xfade));
    if (val != xfade)
    {
        patch_set_sample_xfade(patch, val);
        xfade = val;
        gtk_spin_button_set_range(
                    GTK_SPIN_BUTTON(spin_xfade), xfade - 1, xfade + 1);
    }

    gtk_widget_queue_draw(waveform);
}


static void cb_fade_in_changed (GtkWidget * spin, gpointer data)
{
    (void)spin;(void)data;
    if (ignore_callback)
        return;
 
    int fade_in = patch_get_sample_fade_in(patch);
    int val;

    val = gtk_spin_button_get_value_as_int(
                            GTK_SPIN_BUTTON(spin_fade_in));
    if (val != fade_in)
    {
        patch_set_sample_fade_in(patch, val);
        fade_in = val;
        gtk_spin_button_set_range(
                        GTK_SPIN_BUTTON(spin_fade_in),
                                            fade_in - 1, fade_in + 1);
    }

    gtk_widget_queue_draw(waveform);
}


static void cb_fade_out_changed (GtkWidget * spin, gpointer data)
{
    (void)spin;(void)data;
    if (ignore_callback)
        return;
 
    int fade_out = patch_get_sample_fade_out(patch);
    int val;

    val = gtk_spin_button_get_value_as_int(
                            GTK_SPIN_BUTTON(spin_fade_out));
    if (val != fade_out)
    {
        patch_set_sample_fade_out(patch, val);
        fade_out = val;
        gtk_spin_button_set_range(
                        GTK_SPIN_BUTTON(spin_fade_out),
                                            fade_out - 1, fade_out + 1);
    }

    gtk_widget_queue_draw(waveform);
}
*/


static void cb_wf_play_changed ()
{
    /* there's a good chance this effects the mark_spin selected
       by the mark_combo, (either the value or range). but it
       there's also a chance it might not
     */
    cb_mark_combo_changed(mark_combo, 0);
}

static void cb_wf_loop_changed ()
{
    /* ditto cb_wf_play_changed */
    cb_mark_combo_changed(mark_combo, 0);
}

static void cb_wf_mark_changed()
{
    /* ditto cb_wf_play_changed */
    cb_mark_combo_changed(mark_combo, 0);
}

static void cb_zoom (GtkAdjustment * adj, GtkWidget * spinbutton)
{
    (void)spinbutton;
     float max;
     float page_size;
     float step_inc;
     float page_inc;
     float val;

     /* validate new zoom value, isn't a step anymore because of using spinbutton */
     zoom = gtk_adjustment_get_value (adj);
     if (zoom < ZOOM_MIN)
	  zoom = ZOOM_MIN;
     else if (zoom > ZOOM_MAX)
	  zoom = ZOOM_MAX;

     /* setup new adjustment values */
     max = 1.0 - (1.0 / zoom);	/* the maximum value we'll be able to obtain */
     page_size = 1.0 / zoom;
     page_inc = max / 1000;
     step_inc = max / 10;

     /* create new adjustment */
     val = gtk_range_get_value (GTK_RANGE (hscroll));
     if (val > max)
	  val = max;
     hscrolladj =
	  (GtkAdjustment *) gtk_adjustment_new (val, 0.0, 1.0, step_inc,
						page_inc, page_size);
     gtk_range_set_adjustment (GTK_RANGE (hscroll), hscrolladj);

     /* range needs to be updated so that we can tell the waveform
      * object how much of itself it needs to draw when the scrollbar
      * thumb (who the fuck named _that_?) moves */
     range = 1.0 - max;

     /* we emit this signal so that the waveform will get redrawn with
      * the new dimensions 
      */
    g_signal_emit_by_name (G_OBJECT (hscroll), "value-changed");
}



void sample_editor_show (int id)
{
    waveform_set_patch (WAVEFORM (waveform), id);

    patch = id;

    old_play_start = patch_get_sample_start (id);
    old_play_stop = patch_get_sample_stop (id);

    old_loop_start = patch_get_loop_start (id);
    old_loop_stop = patch_get_loop_stop (id);

    gtk_combo_box_set_active(GTK_COMBO_BOX(mark_combo), WF_MARK_PLAY_START);

    gtk_widget_show (window);
}


void sample_editor_set_thumb(GtkWidget* thumb)
{
    wf_thumb = thumb;
}


void sample_editor_init (GtkWidget * parent)
{
     GtkWindow *w;
     GtkWidget *master_vbox;
     GtkWidget *hbox;
     GtkWidget *button;
     GtkWidget *image;
     GtkWidget *label;
     GtkWidget *spinbutton;
     GtkAdjustment *zoom_adj;

     debug ("Initializing sample editor window\n");

     /* main window */
     window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
     w = GTK_WINDOW (window);
     gtk_window_set_title (w, "Edit Sample");
     gtk_window_set_resizable (w, TRUE);
     gtk_window_set_transient_for (w, GTK_WINDOW (parent));
     gtk_window_set_modal (w, TRUE);
     g_signal_connect (G_OBJECT(w), "delete-event", G_CALLBACK(cb_close),
                                                                    NULL);

     /* master vbox */
     master_vbox = gtk_vbox_new (FALSE, GUI_SPACING);
     gtk_container_add (GTK_CONTAINER (window), master_vbox);
     gtk_container_set_border_width (GTK_CONTAINER (window), GUI_SPACING);
     gtk_widget_show (master_vbox);


     /* top row hbox */
     hbox = gtk_hbox_new (FALSE, GUI_SPACING);
     gtk_box_pack_start (GTK_BOX (master_vbox), hbox, FALSE, FALSE, 0);
     gtk_widget_show (hbox);

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

    mark_combo = basic_combo_create(waveform_get_mark_names());
    gtk_box_pack_start(GTK_BOX (hbox), mark_combo, FALSE, FALSE, 0);
    gtk_widget_show(mark_combo);
    g_signal_connect(G_OBJECT(mark_combo), "changed",
                            G_CALLBACK(cb_mark_combo_changed), NULL);

    mark_spin = gtk_spin_button_new_with_range(0, 1, 1);
    gtk_box_pack_start(GTK_BOX (hbox), mark_spin, FALSE, FALSE, 0);
    gtk_widget_show(mark_spin);
    g_signal_connect(G_OBJECT(mark_spin), "value-changed",
                            G_CALLBACK(cb_mark_spin_changed), NULL);


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
     waveform = waveform_new();
     waveform_set_patch(        WAVEFORM(waveform), -1);
     waveform_set_size(         WAVEFORM(waveform),  512, 256);
     waveform_set_interactive(  WAVEFORM(waveform), TRUE);
     gtk_box_pack_start (GTK_BOX (master_vbox), waveform, TRUE, TRUE, 0);
     gtk_widget_show (waveform);
     g_signal_connect(G_OBJECT (waveform), "play-changed",
                            G_CALLBACK(cb_wf_play_changed), NULL);
     g_signal_connect(G_OBJECT (waveform), "loop-changed",
                            G_CALLBACK(cb_wf_loop_changed), NULL);
     g_signal_connect(G_OBJECT (waveform), "mark-changed",
                            G_CALLBACK(cb_wf_mark_changed), NULL);

     /* waveform scrollbar */
     hscrolladj =
	  (GtkAdjustment *) gtk_adjustment_new (0.0, 0.0, 1.0, 0.0, 0.0,
						1.0);
     hscroll = gtk_hscrollbar_new (GTK_ADJUSTMENT (hscrolladj));
     gtk_box_pack_start (GTK_BOX (master_vbox), hscroll, FALSE, FALSE, 0);
     g_signal_connect (G_OBJECT (hscroll), "value-changed",
		       G_CALLBACK (cb_scroll), NULL);
     gtk_widget_show (hscroll);

     /* hbox */
     hbox = gtk_hbox_new (FALSE, GUI_SPACING);
     gtk_box_pack_start (GTK_BOX (master_vbox), hbox, FALSE, FALSE, 0);
     gtk_widget_show (hbox);


     /* zoom spinbutton */

     label = gtk_label_new ("Zoom:");
     gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);
     gtk_widget_show (label);

     zoom_adj =
	  (GtkAdjustment *) gtk_adjustment_new (1.0, ZOOM_MIN, ZOOM_MAX, 1,
						5, 0.0);
     spinbutton = gtk_spin_button_new (GTK_ADJUSTMENT (zoom_adj), 1, 2);
     gtk_box_pack_start (GTK_BOX (hbox), spinbutton, FALSE, FALSE, 0);
     g_signal_connect (G_OBJECT (zoom_adj), "value_changed",
		       G_CALLBACK (cb_zoom), (gpointer) spinbutton);
     gtk_widget_show (spinbutton);


    /* fade in */
/*
     label = gtk_label_new ("Fade In:");
     gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);
     gtk_widget_show (label);

     spin_fade_in = gtk_spin_button_new_with_range(0, 1, 1);
     gtk_box_pack_start (GTK_BOX (hbox), spin_fade_in, FALSE, FALSE, 0);
     gtk_widget_show (spin_fade_in);
     g_signal_connect (G_OBJECT (spin_fade_in), "value-changed",
		       G_CALLBACK (cb_fade_in_changed), NULL);
*/
    /* fade out */
/*
     label = gtk_label_new ("Fade Out:");
     gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);
     gtk_widget_show (label);

     spin_fade_out = gtk_spin_button_new_with_range(0, 1, 1);
     gtk_box_pack_start (GTK_BOX (hbox), spin_fade_out, FALSE, FALSE, 0);
     gtk_widget_show (spin_fade_out);
     g_signal_connect (G_OBJECT (spin_fade_out), "value-changed",
		       G_CALLBACK (cb_fade_out_changed), NULL);
*/
    /* xfade*/
/*
     label = gtk_label_new ("X-Fade:");
     gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);
     gtk_widget_show (label);

     spin_xfade = gtk_spin_button_new_with_range(0, 1, 1);
     gtk_box_pack_start (GTK_BOX (hbox), spin_xfade, FALSE, FALSE, 0);
     gtk_widget_show (spin_xfade);
     g_signal_connect (G_OBJECT (spin_xfade), "value-changed",
		       G_CALLBACK (cb_xfade_changed), NULL);
*/

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
