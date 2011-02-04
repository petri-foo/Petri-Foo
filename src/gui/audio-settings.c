#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <gtk/gtk.h>
#include "petri-foo.h"
#include "driver.h"
#include "patch.h"
#include "mixer.h"
#include "lfo.h"
#include "gui.h"

/* my mom says I shouldn't play with my widget */
static GtkWidget* window;
static GtkWidget* master_vbox;
static GtkWidget* master_hbox;
static GtkWidget* frame_driver;
static GtkWidget* frame_driver_hbox;
static GtkWidget* radio_driver;
static GtkWidget* config_widget;	/* current (last) config widget */
static GtkWidget* button_close;
static GtkWidget* label;

static void cb_close (GtkWidget* widget, gpointer data)
{
     debug ("Hiding audio settings window\n");

     gtk_widget_hide (window);
}

static void cb_setdriver (GtkWidget* widget, int id)
{
     if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (widget)))
     {
	  /* handle widget */
	  gtk_widget_hide (config_widget);
	  config_widget = driver_get_widget (id);
	  gtk_widget_show (config_widget);

	  /* start the new sound system */
	  driver_stop ( );
	  driver_start (id);
     }
}

void audio_settings_show ( )
{
     debug ("Showing audio settings window\n");

     /* a hack to make sure that the alsadriver widget handles
      * displaying it's settings properly */
     g_signal_emit_by_name (G_OBJECT (config_widget), "show");
     
     gtk_widget_show (window);
}

void audio_settings_init (GtkWidget* parent)
{
     size_t i;
     GSList *driver_list = NULL;

     debug ("Initializing audio settings window\n");

     window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
     gtk_window_set_title (GTK_WINDOW (window), "Audio Settings");
     gtk_window_set_transient_for (GTK_WINDOW (window),
				   GTK_WINDOW (parent));
     gtk_window_set_modal (GTK_WINDOW (window), TRUE);
     g_signal_connect (GTK_WINDOW (window), "delete-event",
		       G_CALLBACK (cb_close), NULL);

     master_vbox = gtk_vbox_new (FALSE, GUI_SPACING);
     gtk_container_add (GTK_CONTAINER (window), master_vbox);
     gtk_container_set_border_width (GTK_CONTAINER (window), GUI_SPACING);
     gtk_widget_show (master_vbox);

     master_hbox = gtk_hbox_new (FALSE, GUI_SPACING);
     gtk_box_pack_start (GTK_BOX (master_vbox), master_hbox, FALSE, FALSE,
			 0);
     gtk_widget_show (master_hbox);

     frame_driver = gtk_frame_new ("Driver");
     gtk_box_pack_start (GTK_BOX (master_hbox), frame_driver, FALSE, FALSE,
			 0);
     gtk_widget_show (frame_driver);

     frame_driver_hbox = gtk_hbox_new (FALSE, GUI_SPACING);
     gtk_container_add (GTK_CONTAINER (frame_driver), frame_driver_hbox);
     gtk_container_set_border_width (GTK_CONTAINER (frame_driver_hbox),
				     GUI_SPACING);
     gtk_widget_show (frame_driver_hbox);

     master_hbox = gtk_hbox_new (FALSE, GUI_SPACING);
     gtk_box_pack_start (GTK_BOX (master_vbox), master_hbox, FALSE, FALSE,
			 0);
     gtk_widget_show (master_hbox);

     /* setup up config interfaces for drivers */
     for (i = 0; i < driver_get_count ( ); i++)
     {
	  gtk_box_pack_start (GTK_BOX (master_hbox), driver_get_widget (i),
			      FALSE, FALSE, 0);

	  if (driver_list == NULL)
	  {
	       radio_driver = gtk_radio_button_new_with_label (NULL,
						     driver_get_name (i));
	       gtk_box_pack_start (GTK_BOX (frame_driver_hbox), radio_driver,
				   FALSE, FALSE, 0);
	       g_signal_connect (G_OBJECT (radio_driver), "toggled",
				 G_CALLBACK (cb_setdriver),
				 (gpointer) i);
	       gtk_widget_show (radio_driver);
	       driver_list = gtk_radio_button_get_group (GTK_RADIO_BUTTON
						(radio_driver));
	  }
	  else
	  {
	       radio_driver =  gtk_radio_button_new_with_label (driver_list,
						     driver_get_name (i));
	       gtk_box_pack_start (GTK_BOX (frame_driver_hbox), radio_driver,
				   FALSE, FALSE, 0);
	       g_signal_connect (G_OBJECT (radio_driver), "toggled",
				 G_CALLBACK (cb_setdriver),
				 (gpointer) i);
	       gtk_widget_show (radio_driver);
	  }
     }

     if (i > 0)
     {				/* setup the first useable driver */
	  config_widget = driver_get_widget (0);
     }
     else
     {				/* if we don't have any configurable (i.e. real) drivers... */
	  label = gtk_label_new ("No drivers available. AIEE!\n");
	  config_widget = label;
     }
     gtk_widget_show (config_widget);

     master_hbox = gtk_hbox_new (FALSE, GUI_SPACING);
     gtk_box_pack_start (GTK_BOX (master_vbox), master_hbox, FALSE, FALSE,
			 0);
     gtk_widget_show (master_hbox);

     button_close = gtk_button_new_from_stock (GTK_STOCK_CLOSE);
     gtk_box_pack_end (GTK_BOX (master_hbox), button_close, FALSE, FALSE,
		       0);
     g_signal_connect (G_OBJECT (button_close), "clicked",
		       G_CALLBACK (cb_close), NULL);
     gtk_widget_show (button_close);
}
