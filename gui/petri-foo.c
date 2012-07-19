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

#include <getopt.h>
#include <gtk/gtk.h>
#include <pthread.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>

#include "dish_file.h"
#include "driver.h"
#include "global_settings.h"
#include "gui.h"
#include "instance.h"
#include "jackdriver.h"
#include "lfo.h"
#include "midi.h"
#include "mixer.h"
#include "mod_src.h"
#include "msg_log.h"
#include "names.h"
#include "patch.h"
#include "patch_util.h"
#include "petri-foo.h"
#include "session.h"


void show_usage (void)
{
    printf ("Usage: petri-foo [options] [bank]\n");
    printf("(Bank files use .petri-foo extension)\n\n");

    printf ("Options:\n");
    printf("  -a, --autoconnect         Auto-connect through JACK to "
                                        "system playback ports\n");
    printf("  -j, --jack-name <name>    Specify JACK client name, "
                                        "defaults to \"Petri-Foo\"\n" );
    printf("  -u, --unconnected         Don't auto-connect to JACK system "
                                        "playback ports (deprecated)\n");
    printf("  -U, --uuid <uuid>         Set UUID for JACK session\n");
    printf("  -h, --help                Display this help message\n\n");
    printf("For more information, please see:"
            "http://petri-foo.sourceforge.net/\n");
}


void cleanup(void)
{
    msg_log(MSG_MESSAGE, "Cleanup...\n");
    session_cleanup();
    midi_stop();
    driver_stop();
    patch_shutdown();
    mixer_shutdown();
    settings_write();
    settings_free();
    free_instance_name();
    mod_src_destroy();

    msg_log(MSG_MESSAGE, "Goodbye!\n");

    exit(0);
}


void sigint_handler()
{
    puts("\n");
    msg_log(MSG_MESSAGE, "Caught SIGINT signal\n");
    cleanup();
}

void sighup_handler()
{
    puts("\n");
    msg_log(MSG_MESSAGE, "Caught SIGHUP signal\n");
    cleanup();
}

void sigterm_handler()
{
    puts("\n");
    msg_log(MSG_MESSAGE, "Caught SIGTERM signal\n");
    cleanup();
}


int main(int argc, char *argv[])
{
    enum { SC = 3 };
    int opt, n;
    int sigs[] = { SIGINT, SIGHUP, SIGTERM };
    void (*sighandlers[])() = { sigint_handler,
                                sighup_handler,
                                sigterm_handler };
    struct sigaction s[SC];

    for (n = 0; n < SC; ++n)
    {
        s[n].sa_handler = SIG_IGN;
        sigfillset(&s[n].sa_mask);
        s[n].sa_flags = 0;
        sigaction(sigs[n], &s[n], NULL);
    }

    for (opt = 1; opt < argc; ++opt)
    {
        if (strcmp(argv[opt], "-h") == 0
         || strcmp(argv[opt], "--help") == 0)
        {
            show_usage();
            return 0;
        }
    }

    mod_src_create();
    gtk_init(&argc, &argv);
    settings_init();
    driver_init();
    lfo_tables_init();
    mixer_init();
    patch_control_init();
    session_init(argc, argv);
    gui_init();

    for (n = 0; n < SC; ++n)
    {
        s[n].sa_handler = sighandlers[n];
        sigfillset(&s[n].sa_mask);
        s[n].sa_flags = 0;
        sigaction(sigs[n], &s[n], NULL);
    }

    gtk_main();

    dish_file_write_full("/home/sirrom/zero");

    cleanup();


    return 0;
}
