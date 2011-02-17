#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <gtk/gtk.h>
#include "gui.h"
#include "petri-foo.h"
#include "mixer.h"
#include "patch_util.h"

static int patch;
static char *old_file = NULL;
static GtkWidget *filesel = NULL;

static void cb_load (GtkWidget * filesel)
{
     GtkWidget *msg;
     char *name =
	  (char *)
	  gtk_file_selection_get_filename (GTK_FILE_SELECTION (filesel));
     char *curname = patch_get_sample_name (patch);
     int val;

     /* don't load the sample if it's already loaded */
     if (strcmp (name, curname) == 0)
     {
	  if (curname != NULL)
	       free (curname);
	  return;
     }

     if ((val = patch_sample_load (patch, name)) < 0)
     {
	  errmsg ("Failed to load sample %s for patch %d (%s)\n", name,
		  patch, patch_strerror (val));
	  msg = gtk_message_dialog_new (GTK_WINDOW (filesel),
					GTK_DIALOG_DESTROY_WITH_PARENT,
					GTK_MESSAGE_ERROR, GTK_BUTTONS_CLOSE,
					"Failed to load sample %s", name);
	  gtk_dialog_run (GTK_DIALOG (msg));
	  gtk_widget_destroy (msg);
     }
     else
     {
	  debug ("Successfully loaded sample %s\n", name);
     }

     if (curname != NULL)
	  free (curname);
}

static void cb_preview (GtkWidget * filesel)
{
     char *name;

     name =
	  (char *)
	  gtk_file_selection_get_filename (GTK_FILE_SELECTION (filesel));
     mixer_preview (name);

     return;
}

static void cb_cancel ( )
{
     char *cur_file;

     if (old_file == NULL)
     {
	  return;
     }
     else if (strcmp (old_file, "\0") == 0)
     {
	  patch_sample_unload (patch);
     }
     else
     {				/* set patch to old sample if another one is currently loaded */
	  cur_file = patch_get_sample_name (patch);
	  if (strcmp (old_file, cur_file) != 0)
	       patch_sample_load (patch, old_file);
	  if (cur_file != NULL)
	       free (cur_file);
     }

     return;
}

static void filesel_init ( )
{
     GtkWidget *load, *preview;
     char *filename;

     filesel = gtk_file_selection_new ("Load Sample");
     filename = patch_get_sample_name (patch);
     gtk_file_selection_set_filename (GTK_FILE_SELECTION (filesel),
				      filename);

     gtk_window_set_modal (GTK_WINDOW (filesel), TRUE);

     if (filename != NULL)
	  free (filename);

     g_signal_connect_swapped (G_OBJECT
			       (GTK_FILE_SELECTION (filesel)->ok_button),
			       "clicked", G_CALLBACK (cb_load),
			       G_OBJECT (filesel));
     g_signal_connect (G_OBJECT
		       (GTK_FILE_SELECTION (filesel)->cancel_button),
		       "clicked", G_CALLBACK (cb_cancel), NULL);

     load = gtk_dialog_add_button (GTK_DIALOG (filesel), "Load", 0);
     g_signal_connect_swapped (G_OBJECT (load), "clicked",
			       G_CALLBACK (cb_load), G_OBJECT (filesel));

     preview = gtk_dialog_add_button (GTK_DIALOG (filesel), "Pre_view", 0);
     g_signal_connect_swapped (G_OBJECT (preview), "clicked",
			       G_CALLBACK (cb_preview), G_OBJECT (filesel));
}

int sample_selector_show (int id)
{
     int response;

     patch = id;
     
     /* create filselector if this is our first time */
     if (filesel == NULL)
	  filesel_init ( );

     old_file = patch_get_sample_name (patch);

     /* "I've been singing to myself, 'There's got to be a better way.'" */
     while (1)
     {
	  response = gtk_dialog_run (GTK_DIALOG (filesel));
	  if (response != 0)
	  {
	       gtk_widget_hide (filesel);
	       break;
	  }
     }				/* maybe I'm just a retard, but I think the dialog/filesel
				 * scheme in GTK sucks and I hope it gets fixed */

     if (old_file != NULL)
     {
	  free (old_file);
	  old_file = NULL;
     }

     return 0;
}
