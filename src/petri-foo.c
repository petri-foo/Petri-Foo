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
#include "patch_util.h"
#include "mixer.h"
#include "mod_src.h"
#include "names.h"
#include "dish_file.h"
#include "jackdriver.h"

void show_usage (void)
{
    printf ("Usage: petri-foo [options] [bankname]\n\n");
    printf ("Options:\n");
    printf("  -n, --name <name>  Specify instance name, "
                                "defaults to \"petri-foo\"\n" );
    printf("  -u, --unconnected  Don't auto-connect to JACK\n");
    printf("  -h, --help         Display this help message\n\n");
    printf ("For more information, please see:\n");
    printf ("http://petri-foo.sourceforge.net/\n");
}


int main(int argc, char *argv[])
{
    int opt;
    int longopt_index;

    static struct option long_options[] = 
    {
        { "name",           1, 0, 'n'},
        { "unconnected",    0, 0, 'u'},
        { "uuid",           1, 0, 'U'},
        { "help",           0, 0, 'h'},
        { 0, 0, 0, 0}
    };


    while((opt = getopt_long(argc, argv, "n:u:U:h",
                                        long_options, &longopt_index)) > 0)
    {
        switch (opt)
        {
        case 'n':
            set_instance_name(optarg);
            break;

        case 'u':
            jackdriver_set_unconnected();
            break;

        case 'U':
            jackdriver_set_uuid(strdup(optarg) );
            break;

        case 'h':
            show_usage();
            return 0;

        default:
            show_usage();
            return 1;
        }
    }

    mod_src_create();
    gtk_init(&argc, &argv);
    gui_init();
    driver_init();
    lfo_tables_init();
    mixer_init();
    patch_control_init();
    driver_start();
    midi_start();

    if (optind < argc)
        dish_file_read(argv[optind]);
    else
        patch_create_default();

    gui_refresh();

    if (optind < argc)
        gui_set_window_title(argv[optind]);

    gtk_main();

    /* shutdown... */
    midi_stop();
    driver_stop();
    patch_shutdown();
    mixer_shutdown();
    free_instance_name();
    mod_src_destroy();

    debug("Goodbye.\n");

    return 0;
}
