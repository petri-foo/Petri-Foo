#include <pthread.h>
#include <gtk/gtk.h>
#include <getopt.h>
#include <string.h>

#include "instance.h"
#include "petri-foo.h"
#include "gui/gui.h"
#include "midi.h"
#include "driver.h"
#include "lfo.h"
#include "patch.h"
#include "mixer.h"
#include "beef.h"

#ifdef HAVE_LASH
#include "lashdriver.h"
#endif

#include "jackdriver.h"

void show_usage (void)
{
	printf ("Usage: petri-foo [options] [bankname]\n\n");
	printf ("Options:\n");
	printf ("  -n, --name <name>  Specify instance name, defaults to \"petri-foo\"\n");
	printf ("  -h, --help         Display this help message\n\n");
	printf ("For more information, please see:\n");
	printf ("http://petri-foo.sourceforge.net/\n");
}

int main(int argc, char *argv[])
{
#ifdef HAVE_LASH
	/*
	 * Handle lash arguments here to prevent getopt_long() from
	 * going bonkers over arguments it hasn't been told about.
	 */
	lash_args_t *lash_args = lash_extract_args(&argc, &argv);
#endif

	int opt;
	int longopt_index;
	static struct option long_options[] = 
	{
		{ "name", 1, 0, 'n'},
		{ "uuid", 1, 0, 'U'},
		{ "help", 0, 0, 'h'},
		{ 0, 0, 0, 0}
	};

	/* command line argument processing */
	while((opt = getopt_long(argc, argv, "n:U:h", long_options,
	                         &longopt_index)) > 0)
	{
		switch (opt)
		{
			case 'n':
				set_instance_name(optarg);
				break;
			case 'U':
				jackdriver_set_uuid(strdup(optarg) );
				break;
			case 'h':
				show_usage();
				return 0;
				break;
			default:
				show_usage();
				return 1;
				break;
		}
	}

	/* constructors */
#ifdef HAVE_LASH
	lashdriver_init(lash_args);
#endif
	gtk_init(&argc, &argv);
	driver_init();
	lfo_init();
	mixer_init();
	patch_init();
	gui_init();

	if (optind < argc) 
	{
		beef_read(argv[optind]);
		gui_refresh();
	}

	/* start */
	driver_start();
	midi_start();
#ifdef HAVE_LASH
	lashdriver_start();
	lashdriver_set_jack_name((char*)driver_get_client_name());
	lashdriver_set_alsa_id(midi_get_client_id());
#endif
	gtk_main();

	/* stop */
	midi_stop();
	driver_stop();

	/* destructors */
	patch_shutdown();
	mixer_shutdown();
    free_instance_name();

	debug("Goodbye.\n");
	return 0;
}
