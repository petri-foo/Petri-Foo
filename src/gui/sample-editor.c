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

static GtkWidget* window;
static GtkWidget* waveform;
static GtkWidget* hscroll;

static GtkObject* hscrolladj;

static GtkWidget* wf_thumb = 0;

enum
{
     ZOOM_MIN = 1,
     ZOOM_MAX = 100
};


static int ignore_callback = 0;

static gboolean ignore_mark_change = FALSE; /* once upon a time*/

static int zoom = ZOOM_MIN;
static float range = 1.0;

static int old_play_start, old_play_stop;
static int old_loop_start, old_loop_stop;
static int patch;

static GtkWidget*   mark_combo;
static GtkWidget*   mark_spin;
static GtkWidget*   mark_val;


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


static void update_mark_val(int val)
{
    const float* wav = patch_get_sample(patch);
    char buf[40];
    snprintf(buf, 40, "%2.6f", wav[val * 2]);
    gtk_label_set_label(GTK_LABEL(mark_val), buf);
}


static void cb_scroll (GtkWidget * scroll, gpointer data)
{
    (void)data;
     float val = gtk_range_get_value(GTK_RANGE(scroll));
     debug("scroll changing:%f\n",(float)val);
     waveform_set_range(WAVEFORM(waveform), val, val + range);
}


static void cb_mark_spin_changed(GtkWidget * spin, gpointer data)
{
    debug("spin changing\n");
    int val = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(mark_spin));
    waveform_set_mark_frame(WAVEFORM(waveform), val);
    update_mark_val(val);
    gtk_widget_queue_draw(waveform);
}


static void update_mark_spin(void)
{
    int val;
    int min;
    int max;

    val = waveform_get_mark_spin_range(WAVEFORM(waveform), &min, &max);

    debug("updating mark spin\n");

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

    update_mark_val(val);
}


static void cb_mark_combo_changed(GtkWidget* combo, gpointer data)
{
    debug("combo changing...\n");
    ignore_mark_change = TRUE;
    waveform_set_mark(WAVEFORM(waveform),
                    gtk_combo_box_get_active(GTK_COMBO_BOX(mark_combo)));
    update_mark_spin();
}

static void cb_wf_play_changed ()
{
    /*  there's some chance the play point that was changed is
        selected and shown in the mark spin. */
    debug("play changing...\n");
    update_mark_spin();
}

static void cb_wf_loop_changed ()
{
    /*  there's some chance the play point that was changed is
        selected and shown in the mark spin. */
    debug("loop changing...\n");
    update_mark_spin();
}

static void cb_wf_mark_changed()
{
    /* ditto cb_wf_play_changed */
    if (ignore_mark_change)
    {
        debug("ignoring mark change this time\n");
        ignore_mark_change = FALSE;
        return;
    }
    debug("mark changing...\n");
    gtk_combo_box_set_active(GTK_COMBO_BOX(mark_combo),
                                waveform_get_mark(WAVEFORM(waveform)));
/*    cb_mark_combo_changed(mark_combo, 0);*/
}

static void cb_wf_view_changed()
{
    float start, stop;
    waveform_get_range(WAVEFORM(waveform), &start, &stop);
    debug("view changing...\n");
    g_signal_handlers_block_by_func(hscroll, cb_scroll, 0);
    gtk_adjustment_set_value(GTK_ADJUSTMENT(hscrolladj), start);
    g_signal_handlers_unblock_by_func(hscroll, cb_scroll, 0);
}

static void cb_zoom(GtkAdjustment* adj, GtkWidget* spinbutton)
{
    (void)spinbutton;
    float max;
    float page_size;
    float step_inc;
    float page_inc;
    float val;

    float start;
    float stop;
    int frames = patch_get_frames(patch);

    waveform_get_range(WAVEFORM(waveform), &start, &stop);

    debug("zoom changing...\n");

    zoom = gtk_adjustment_get_value(adj);

    if (zoom < ZOOM_MIN)
        zoom = ZOOM_MIN;
    else if (zoom > ZOOM_MAX)
        zoom = ZOOM_MAX;

    max = 1.0 - (1.0 / zoom);
    page_size = 1.0 / zoom;
    page_inc = max / 1000;
    step_inc = max / 10;

    /* create new adjustment */

    val = start + (stop - start) / 2 - page_size / 2;

    /* range needs to be updated so that we can tell the waveform
     * object how much of itself it needs to draw when the scrollbar
     * changes */

    range = 1.0 - max;

    if (val + range > 1.0)
        val = 1.0 - range;


debug("start:%f stop:%f hscroll range value:%f\n", start,stop,val);

    gtk_range_set_range(GTK_RANGE(hscroll), start, stop);

    hscrolladj = gtk_adjustment_new(val, 0.0, 1.0,  step_inc,
                                                    page_inc,
                                                    page_size);
    gtk_range_set_adjustment(GTK_RANGE(hscroll),
                                            GTK_ADJUSTMENT(hscrolladj));


     /* emit value-changed signal so waveform is redrawn with
      * the new dimensions */
//    g_signal_emit_by_name(G_OBJECT(hscroll), "value-changed");
    g_signal_emit_by_name(G_OBJECT(hscroll), "value-changed");
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

    mark_val = gtk_label_new("");
    gtk_box_pack_start(GTK_BOX (hbox), mark_val, FALSE, FALSE, 0);
    gtk_widget_show(mark_val);


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
     g_signal_connect(G_OBJECT (waveform), "view-changed",
                            G_CALLBACK(cb_wf_view_changed), NULL);

     /* waveform scrollbar */
     hscrolladj = gtk_adjustment_new (0.0, 0.0, 1.0, 0.0, 0.0,
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
