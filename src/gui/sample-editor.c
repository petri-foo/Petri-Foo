#include <stdio.h>
#include <string.h>
#include <gtk/gtk.h>
#include "petri-foo.h"
#include "patch.h"
#include "waveform.h"
#include "gui.h"
#include "mixer.h"

static GtkWidget *window;
static GtkWidget *waveform;
static GtkWidget *hscroll;
static GtkAdjustment *hscrolladj;

enum
{
     ZOOM_MIN = 1,
     ZOOM_MAX = 100
};

static int zoom = ZOOM_MIN;
static float range = 1.0;
static int old_play_start, old_play_stop;
static int old_loop_start, old_loop_stop;
static int patch;

static void cb_close (GtkWidget * widget, gpointer data)
{
     debug ("Hiding sample editor\n");
     mixer_note_off_with_id (patch, patch_get_note (patch));
     gtk_widget_hide (window);
}

static void cb_play (GtkWidget * widget, gpointer data)
{
     mixer_note_on_with_id (patch, patch_get_note (patch), 1.0);
}

static void cb_stop (GtkWidget * widget, gpointer data)
{
     mixer_note_off_with_id (patch, patch_get_note (patch));
}

static void cb_reset (GtkWidget * widget, gpointer data)
{
     debug ("Restoring initial values\n");
     patch_set_sample_start (patch, old_play_start);
     patch_set_sample_stop (patch, old_play_stop);
     patch_set_loop_start (patch, old_loop_start);
     patch_set_loop_stop (patch, old_loop_stop);

     gtk_widget_queue_draw (waveform);
}

static void cb_clear (GtkWidget * widget, char *op)
{
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

     gtk_widget_queue_draw (waveform);
}

static void cb_scroll (GtkWidget * scroll, gpointer data)
{
     float val;

     val = gtk_range_get_value (GTK_RANGE (scroll));
     waveform_set_range (WAVEFORM (waveform), val, val + range);
}

static void cb_zoom (GtkAdjustment * adj, GtkWidget * spinbutton)
{
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
      * the new dimensions */
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

     gtk_widget_show (window);
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
     g_signal_connect (G_OBJECT (w), "delete-event", G_CALLBACK (cb_close),
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
     image = gtk_image_new_from_file (PIXMAPSDIR "play.png");
     button = gtk_button_new ( );
     gtk_container_add (GTK_CONTAINER (button), image);
     gtk_box_pack_start (GTK_BOX (hbox), button, FALSE, FALSE, 0);
     g_signal_connect_swapped (G_OBJECT (button), "clicked",
			       G_CALLBACK (cb_play), NULL);
     gtk_widget_show (image);
     gtk_widget_show (button);

     /* stop button */
     image = gtk_image_new_from_file (PIXMAPSDIR "stop.png");
     button = gtk_button_new ( );
     gtk_container_add (GTK_CONTAINER (button), image);
     gtk_box_pack_start (GTK_BOX (hbox), button, FALSE, FALSE, 0);
     g_signal_connect_swapped (G_OBJECT (button), "clicked",
			       G_CALLBACK (cb_stop), NULL);
     gtk_widget_show (image);
     gtk_widget_show (button);

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
     waveform = waveform_new (-1, 512, 256, TRUE);
     gtk_box_pack_start (GTK_BOX (master_vbox), waveform, TRUE, TRUE, 0);
     gtk_widget_show (waveform);

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
