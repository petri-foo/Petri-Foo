#include <stdio.h>
#include <unistd.h>
#include <pthread.h>
#include <gtk/gtk.h>
#include "petri-foo.h"
#include "driver.h"
#include "mixer.h"
#include "sync.h"
#include "gui/gui.h"

#define ALSA_PCM_NEW_HW_PARAMS_API	/* fuckstickers shoulda made the importance of this evident */
#define ALSA_PCM_NEW_SW_PARAMS_API
#include <alsa/asoundlib.h>

#define FAIL -1
#define REDUNDANT -2

/* gtk stuff */
static GtkWidget* frame;
static GtkWidget* frame_hbox;
static GtkWidget* frame_vbox;
static GtkWidget* radio_format_s16le;
static GtkWidget* radio_format_s24le;
static GtkWidget* radio_rate_44100;
static GtkWidget* radio_rate_96000;
static GtkWidget* entry_device;
static GtkWidget* label_format;
static GtkWidget* label_rate;
static GtkWidget* label_device;
static GtkWidget* button_restart;
static GtkWidget* button_stop;

/* alsa stuff */
static Atomic           running = 0;
static snd_pcm_t*       handle;	/* playback handle */
static void*            buffer;
static snd_pcm_format_t format = SND_PCM_FORMAT_S16_LE;
static int              rate = 44100;
static int              periodsize = 2048;
static pthread_t        process_thread;
static GString*         device;

#define CONVERT(x) ((void (*)(void* , float* , int))(x))
static void (*convert) (void* , float* , int);

static int xrun (int err)
{
     if (err == -EPIPE)
     {
	  errmsg ("xrun\n");
	  if ((err = snd_pcm_prepare (handle)) < 0)
	  {
	       errmsg ("Unable to recover from xrun (%s)\n",
		       snd_strerror (err));
	       return -1;
	  }
     }
     else if (err == -ESTRPIPE)
     {
	  errmsg ("Suspended\n");
	  while ((err = snd_pcm_resume (handle)) == -EAGAIN)
	       sleep (1);
	  if (err < 0)
	  {
	       if ((err = snd_pcm_prepare (handle)) < 0)
	       {
		    errmsg ("Failed to recover from suspend (%s)\n",
			    snd_strerror (err));
		    return -1;
	       }
	  }
	  errmsg ("Resumed\n");
     }
     else
     {
	  errmsg ("Unrecognized error code (%s)\n", snd_strerror (err));
	  return -1;
     }

     return 0;
}

static void convert_s24le (gint32* dest, float* src, int frames)
{
     int i;

     for (i = 0; i < frames * 2; i++)
	  dest[i] = src[i] * (0x7FFFFF);
}

static void convert_s16le (gint16* dest, float* src, int frames)
{
     int i;

     for (i = 0; i < frames * 2; i++)
	  dest[i] = src[i] * (0x7FFF);
}

static void process (void* arg)
{
     int avail, write, wrote, err, quit;
     float* buf;

     quit = 0;
     buf = g_new0 (float, periodsize * 2);
     while (!quit)
     {
	  if ((err = snd_pcm_wait (handle, -1)) < 0)
	  {
	       if (xrun (err) < 0)
	       {
		    break;
	       }
	  }
	  else if (err == 0)
	  {
	       errmsg ("Timed out somehow... odd\n");
	       break;
	  }

	  while ((avail = snd_pcm_avail_update (handle)) < 0)
	  {
	       if (xrun (avail) < 0)
	       {
		    quit = 1;
		    break;
	       }
	  }
	  if (quit)
	       break;

	  while (avail)
	  {
	       write = avail > periodsize ? periodsize : avail;
	       mixer_mixdown (buf, write);
	       convert (buffer, buf, write);
	       if ((wrote = snd_pcm_writei (handle, buffer, write)) < 0)
	       {
		    if (xrun (wrote) < 0)
		    {
			 quit = 1;
			 break;
		    }
	       }
	       else
	       {
		    if (wrote != write)
			 errmsg ("%d frames written of %d expected\n", wrote,
				 write);
		    avail -= wrote;
	       }
	  }

	  if (!running)
	       quit = 1;
     }

     if ((err = snd_pcm_close (handle)) < 0)
	  errmsg ("Unable to close device (%s)\n", snd_strerror (err));
     if (buffer != NULL)
	  free (buffer);
     if (buf != NULL)
	  free (buf);

     debug ("Halted\n");
}

/* update preference to reflect config gui */
static void update_prefs ( )
{
     int old_rate;

     /* format */
     if (gtk_toggle_button_get_active
	 (GTK_TOGGLE_BUTTON (radio_format_s16le)))
     {
	  format = SND_PCM_FORMAT_S16_LE;
     }
     else if (gtk_toggle_button_get_active
	      (GTK_TOGGLE_BUTTON (radio_format_s24le)))
     {
	  format = SND_PCM_FORMAT_S24_LE;
     }

     /* rate */
     old_rate = rate;
     if (gtk_toggle_button_get_active
	 (GTK_TOGGLE_BUTTON (radio_rate_44100)))
     {
	  rate = 44100;
     }
     else if (gtk_toggle_button_get_active
	      (GTK_TOGGLE_BUTTON (radio_rate_96000)))
     {
	  rate = 96000;
     }
     
     /* device */
     g_string_assign (device, (char*) gtk_entry_get_text (GTK_ENTRY (entry_device)));
}

static int start ( )
{
     snd_pcm_hw_params_t* hwparams;
     snd_pcm_access_t access = SND_PCM_ACCESS_RW_INTERLEAVED;
     int channels = 2;
     int err;

     if (running)
     {
	  errmsg ("Alsa Driver is already running, so not re-initializing\n");
	  return REDUNDANT;
     }

     debug ("Initializing Alsa Driver...\n");
     update_prefs ( );
     if ((err = snd_pcm_open (&handle, device->str, SND_PCM_STREAM_PLAYBACK,
			SND_PCM_NONBLOCK)) < 0)
     {
	  errmsg ("Failed to open device %s for playback (%s)\n", device->str,
		  snd_strerror (err));
	  running = 0;
	  errmsg ("Failed to initialize\n");
	  return FAIL;
     }

     /* setup the device */
     snd_pcm_hw_params_alloca (&hwparams);
     if (hwparams == NULL)
     {
	  errmsg ("Failed to allocate space for hardware parameters structure (%s)\n",
		  snd_strerror (err));
	  goto fail;
     }
     if ((err = snd_pcm_hw_params_any (handle, hwparams)) < 0)
     {
	  errmsg ("Failed to initialize hardware parameters structure (%s)\n",
		  snd_strerror (err));
	  goto fail;
     }
     if ((err = snd_pcm_hw_params_set_access (handle, hwparams, access)) < 0)
     {
	  errmsg ("Failed to set access mode (%s)\n", snd_strerror (err));
	  goto fail;
     }
     if ((err = snd_pcm_hw_params_set_format (handle, hwparams, format)) < 0)
     {
	  errmsg ("Failed to set format (%s)\n", snd_strerror (err));
	  goto fail;
     }
     if ((err = snd_pcm_hw_params_set_rate (handle, hwparams, rate, 0)) < 0)
     {
	  errmsg ("Failed to set sample rate to %d (%s)\n", rate,
		  snd_strerror (err));
	  goto fail;
     }
     if ((err = snd_pcm_hw_params_set_channels (handle, hwparams, channels)) < 0)
     {
	  errmsg ("Failed to set channels to %d (%s)\n", 2,
		  snd_strerror (err));
	  goto fail;
     }
     if ((err = snd_pcm_hw_params_set_buffer_size (handle, hwparams,
					     periodsize * 2)) < 0)
     {
	  errmsg ("Failed to set buffer size to %d (%s)\n", periodsize * 2,
		  snd_strerror (err));
	  goto fail;
     }
     if ((err = snd_pcm_hw_params_set_period_size (handle, hwparams, periodsize,
					     0)) < 0)
     {
	  errmsg ("Failed to set period size to %d (%s)\n", periodsize,
		  snd_strerror (err));
	  goto fail;

     }
     if ((err = snd_pcm_hw_params (handle, hwparams)) < 0)
     {
	  errmsg ("Failed to apply hardware parameters (%s)\n",
		  snd_strerror (err));
	  goto fail;
     }

     /* allocate write buffer */
     if (format == SND_PCM_FORMAT_S16_LE)
     {
	  buffer = g_new0 (gint16, channels * periodsize);
	  convert = CONVERT (convert_s16le);
     }
     else if (format == SND_PCM_FORMAT_S24_LE)
     {
	  buffer = g_new0 (gint32, channels * periodsize);
	  convert = CONVERT (convert_s24le);
     }
     else
     {
	  errmsg ("Invalid internal format... AIEEE!\n");
	  goto fail;
     }

/* success */
     driver_set_samplerate (rate);
     driver_set_buffersize (periodsize);
     mixer_flush();
     running = 1;
     pthread_create (&process_thread, NULL, (void* ) process, NULL);
     debug ("Initialization complete\n");
     return 0;

fail:
     snd_pcm_close (handle);
     running = 0;
     errmsg ("Failed to initialize\n");
     return FAIL;
}

static int stop ( )
{
     if (!running)
     {
	  errmsg ("Not running, so not shutting down\n");
	  return 0;
     }
     else
     {
	  debug ("Shutting down...\n");
	  running = 0;
     }

     pthread_join (process_thread, NULL);
     debug ("Shutdown successful\n");
     return 0;
}

static void restart ( )
{
     stop ( );
     start ( );
}

static void cb_show (GtkWidget* widget, gpointer data)
{
     if (rate == 96000)
     {
	  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (radio_rate_96000), TRUE);
     }
     else
     {
	  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (radio_rate_44100), TRUE);
     }

     if (format == SND_PCM_FORMAT_S24_LE)
     {
	  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (radio_format_s24le), TRUE);
     }
     else
     {
	  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (radio_format_s16le), TRUE);
     }

     gtk_entry_set_text (GTK_ENTRY (entry_device), device->str);
     sync_set_method (SYNC_METHOD_MIDI);
}

static void init ( )
{
     /* alsa settings frame */
     frame = gtk_frame_new ("ALSA");

     device = g_string_new ("plughw:0");
     
     frame_vbox = gtk_vbox_new (FALSE, GUI_SPACING);
     gtk_container_set_border_width (GTK_CONTAINER (frame_vbox), GUI_SPACING);
     gtk_container_add (GTK_CONTAINER (frame), frame_vbox);
     gtk_widget_show (frame_vbox);

     frame_hbox = gtk_hbox_new (FALSE, GUI_SPACING);
     gtk_box_pack_start (GTK_BOX (frame_vbox), frame_hbox, FALSE, FALSE, 0);
     gtk_widget_show (frame_hbox);

     label_format = gtk_label_new ("Format:");
     gtk_box_pack_start (GTK_BOX (frame_hbox), label_format, FALSE, FALSE,
			 0);
     gtk_widget_show (label_format);

     radio_format_s16le = gtk_radio_button_new_with_label (NULL, "S16LE");
     gtk_box_pack_start (GTK_BOX (frame_hbox), radio_format_s16le,
			 FALSE, FALSE, 0);
     gtk_widget_show (radio_format_s16le);

     radio_format_s24le =
	  gtk_radio_button_new_with_label_from_widget (GTK_RADIO_BUTTON
						       (radio_format_s16le),
						       "S24LE");
     gtk_box_pack_start (GTK_BOX (frame_hbox), radio_format_s24le, FALSE,
			 FALSE, 0);
     gtk_widget_show (radio_format_s24le);

     frame_hbox = gtk_hbox_new (FALSE, GUI_SPACING);
     gtk_box_pack_start (GTK_BOX (frame_vbox), frame_hbox, FALSE, FALSE, 0);
     gtk_widget_show (frame_hbox);

     label_rate = gtk_label_new ("Rate:");
     gtk_box_pack_start (GTK_BOX (frame_hbox), label_rate, FALSE, FALSE, 0);
     gtk_widget_show (label_rate);

     radio_rate_44100 = gtk_radio_button_new_with_label (NULL, "44100");
     gtk_box_pack_start (GTK_BOX (frame_hbox), radio_rate_44100,
			 FALSE, FALSE, 0);
     gtk_widget_show (radio_rate_44100);

     radio_rate_96000 =
	  gtk_radio_button_new_with_label_from_widget (GTK_RADIO_BUTTON
						       (radio_rate_44100),
						       "96000");
     gtk_box_pack_start (GTK_BOX (frame_hbox), radio_rate_96000, FALSE,
			 FALSE, 0);
     gtk_widget_show (radio_rate_96000);

     frame_hbox = gtk_hbox_new (FALSE, GUI_SPACING);
     gtk_box_pack_start (GTK_BOX (frame_vbox), frame_hbox, FALSE, FALSE, 0);
     gtk_widget_show (frame_hbox);

     label_device = gtk_label_new ("Device:");
     gtk_box_pack_start (GTK_BOX (frame_hbox), label_device, FALSE, FALSE,
			 0);
     gtk_widget_show (label_device);

     entry_device = gtk_entry_new ( );
     gtk_box_pack_start (GTK_BOX (frame_hbox), entry_device, FALSE, FALSE,
			 0);
     gtk_entry_set_text (GTK_ENTRY (entry_device), "plughw:0");
     gtk_widget_show (entry_device);

     frame_hbox = gtk_hbox_new (FALSE, GUI_SPACING);
     gtk_box_pack_start (GTK_BOX (frame_vbox), frame_hbox, FALSE, FALSE, 0);
     gtk_widget_show (frame_hbox);

     button_restart = gtk_button_new_with_label ("Restart");
     gtk_box_pack_start (GTK_BOX (frame_hbox), button_restart, FALSE, FALSE,
			 0);
     g_signal_connect (G_OBJECT (button_restart), "clicked",
		       G_CALLBACK (restart), NULL);
     gtk_widget_show (button_restart);

     button_stop = gtk_button_new_with_label ("Stop");
     gtk_box_pack_start (GTK_BOX (frame_hbox), button_stop, FALSE, FALSE,
			 0);
     g_signal_connect (G_OBJECT (button_stop), "clicked", G_CALLBACK (stop),
		       NULL);
     gtk_widget_show (button_stop);

     /* we only allow midi syncing */
     g_signal_connect_swapped (G_OBJECT (frame), "show", G_CALLBACK (cb_show),
			       NULL);

     return;
}

static GtkWidget* getwidget ( )
{
     return frame;
}

static int getrate ( )
{
     return rate;
}

static int getperiodsize ( )
{
     return periodsize;
}

static const char* getname ( )
{
     return "ALSA";
}

static void* getid ( )
{
     return NULL;
}

Driver alsa_driver = {
     init,
     start,
     stop,
     getrate,
     getperiodsize,
     getwidget,
     getname,
     getid
};
