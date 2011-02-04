#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <gtk/gtk.h>
#include "petri-foo.h"
#include "patch.h"
#include "beef.h"

static const char *last_bank = "";

static void save_bank_as_verify (GtkWidget * filesel)
{
     GtkWidget *msg;
     char *name =
	  (char *)
	  gtk_file_selection_get_filename (GTK_FILE_SELECTION (filesel));
     int val;

     if ((val = beef_write (name)) < 0)
     {
	  errmsg ("Failed to write file %s\n", name);
	  msg =
	       gtk_message_dialog_new (GTK_WINDOW (filesel), GTK_DIALOG_MODAL,
				       GTK_MESSAGE_ERROR, GTK_BUTTONS_CLOSE,
				       "Failed to write file %s\n.", name);
	  g_signal_connect_swapped (G_OBJECT (msg), "response",
				    G_CALLBACK (gtk_widget_destroy), msg);
	  gtk_widget_show (msg);
     }
     else
     {
	  debug ("Succesfully wrote file %s\n", name);
	  if (last_bank[0] != '\0')
	       free (last_bank);
	  last_bank = (const char *) strdup (name);
	  gtk_dialog_response (GTK_DIALOG (filesel), GTK_RESPONSE_ACCEPT);
     }
}

static void open_bank_verify (GtkWidget * filesel)
{
     GtkWidget *msg;
     char *name =
	  (char *)
	  gtk_file_selection_get_filename (GTK_FILE_SELECTION (filesel));
     int val;

     if ((val = beef_read (name)) < 0)
     {
	  errmsg ("Failed to open file %s\n", name);
	  msg =
	       gtk_message_dialog_new (GTK_WINDOW (filesel), GTK_DIALOG_MODAL,
				       GTK_MESSAGE_ERROR, GTK_BUTTONS_CLOSE,
				       "Failed to open file %s.", name);
	  g_signal_connect_swapped (G_OBJECT (msg), "response",
				    G_CALLBACK (gtk_widget_destroy), msg);
	  gtk_widget_show (msg);
     }
     else
     {
	  debug ("Succesfully opened file %s\n", name);
	  if (last_bank[0] != '\0')
	       free (last_bank);
	  last_bank = (const char *) strdup (name);
	  gtk_dialog_response (GTK_DIALOG (filesel), GTK_RESPONSE_ACCEPT);
     }
}

int bank_ops_save_as ( )
{
     GtkWidget *filesel;
     int val;

     filesel = gtk_file_selection_new ("Save Bank");
     gtk_file_selection_set_filename (GTK_FILE_SELECTION (filesel),
				      last_bank);

     g_signal_connect_swapped (G_OBJECT
			       (GTK_FILE_SELECTION (filesel)->ok_button),
			       "clicked", G_CALLBACK (save_bank_as_verify),
			       filesel);

     if (gtk_dialog_run (GTK_DIALOG (filesel)) == GTK_RESPONSE_ACCEPT)
	  val = 0;
     else
	  val = -1;
     gtk_widget_destroy (filesel);

     return val;
}

int bank_ops_save ( )
{
     int val;

     if (last_bank[0] == '\0')
     {
	  val = bank_ops_save_as ( );
     }
     else
     {
	  val = beef_write (last_bank);
     }

     return val;
}

int bank_ops_open ( )
{
     GtkWidget *filesel;
     int val;

     filesel = gtk_file_selection_new ("Open Bank");
     gtk_file_selection_set_filename (GTK_FILE_SELECTION (filesel),
				      last_bank);

     g_signal_connect_swapped (G_OBJECT
			       (GTK_FILE_SELECTION (filesel)->ok_button),
			       "clicked", G_CALLBACK (open_bank_verify),
			       filesel);

     if (gtk_dialog_run (GTK_DIALOG (filesel)) == GTK_RESPONSE_ACCEPT)
	  val = 0;
     else
	  val = -1;
     gtk_widget_destroy (filesel);

     return val;
}

int bank_ops_new ( )
{
     patch_destroy_all ( );
     last_bank = "";
     return 0;
}
