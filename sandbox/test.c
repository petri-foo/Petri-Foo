#include <getopt.h>
#include <gtk/gtk.h>
#include <libgen.h>
#include <pthread.h>
#include <string.h>

#include <petri-foo.h>
#include <phin.h>
#include <file_ops.h>





#include <assert.h>







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

#include <pthread.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>

#include "dish_file.h"

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




#include <getopt.h>
#include <pthread.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>

#include "dish_file.h"
#include "driver.h"
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
    dish_file_state_cleanup();
    midi_stop();
    driver_stop();
    patch_shutdown();
    mixer_shutdown();
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
    driver_init();
    lfo_tables_init();
    mixer_init();
    patch_control_init();
    dish_file_state_init();
/*
    driver_start();
    midi_start();

    dish_file_read( "/home/sirrom/Audio_Sketchbook/"
                    "pf-banks/ftest001/ftest001.petri-foo");
*/
    for (n = 0; n < SC; ++n)
    {
        s[n].sa_handler = sighandlers[n];
        sigfillset(&s[n].sa_mask);
        s[n].sa_flags = 0;
        sigaction(sigs[n], &s[n], NULL);
    }

    sleep(1);

    cleanup();

    return 0;
}



/*

int main(int argc, char *argv[])
{

    char* path[] = {    "/1234/noise.wav",
                        "/noise.wav",
                        "noise.wav",
                        "/some/path/",
                        "/path",            0 };
    char* dir = 0;
    char* file = 0;
    int i;

    for (i = 0; path[i] != 0; ++i)
    {
        char* p = strdup(path[i]);
        size_t lc = strlen(p) - 1;

        if (*(p + lc) == '/')
            *(p + lc) = '\0';

        if (file_ops_split_path(p, &dir, &file) == 0)
        {
            char* np = file_ops_join_path(dir, file);
            debug("path:'%s' dir:'%s' file:'%s'\n", path[i], dir, file);
            debug("reconstituted: '%s'\n", np);
            debug("\n");
            free(dir);
            free(file);
            free(np);
        }

        char* name;
        char* ext;

        if (file_ops_split_file(path[i], &name, &ext) == 0)
        {
            char* np = file_ops_join_ext(name, ext);
            debug("filename:'%s' name:'%s' ext:'%s'\n", path[i], name, ext);
            debug("reconstituted: '%s'\n", np);
            debug("\n");
            free(name);
            free(ext);
            free(np);
        }
    }

    char* p1 = "/home/sirrom/zero/samples/dir1";
    char* p2 = "/home/sirrom/zero/samples/";

    if (strstr(p1, p2) == p1)
    {
        debug("'%s' contains '%s' at start\n", p1, p2);
    }

    file_ops_make_relative("/home/sirrom/zero/samples/", "/home/sirrom/");
    file_ops_make_relative("/home/sirrom/zero/samples/", "/home/sirrom");
    file_ops_make_relative("/home/sirrom/zero/samples", "/home/sirrom/");
    file_ops_make_relative("/home/sirrom/zero/samples", "/home/sirrom");

    return 0;
}


*/


/*
int main(int argc, char *argv[])
{
    mod_src_create();

    lfo_tables_init();

    patch_control_init();

    dish_file_state_init();

    patch_set_samplerate(44100);

    dish_file_read( "/home/sirrom/Audio_Sketchbook/"
                    "pf-banks/ftest001/ftest001.petri-foo");

    dish_file_write_basic("/home/sirrom/ftest001.petri-foo");

    dish_file_write();

    msg_log(MSG_MESSAGE, "Cleanup...\n");

    dish_file_state_cleanup();

    patch_shutdown();

    free_instance_name();

    mod_src_destroy();

    msg_log(MSG_MESSAGE, "Goodbye!\n");

    return 0;
}

*/




/*

void gui_attach(GtkTable* table, GtkWidget* widget, guint l, guint r,
                                                    guint t, guint b)
{
    gtk_table_attach(table, widget, l, r, t, b, GTK_FILL | GTK_EXPAND,
                                                GTK_SHRINK,
                                                0, 0);
    gtk_widget_show(widget);
}

static gboolean cb_delete(GtkWidget* w, GdkEvent* ev, gpointer data)
{
    (void)w;(void)ev;(void)data;
    debug("delete\n");
    return FALSE;
}


static void cb_quit(GtkWidget* w, gpointer data)
{
    (void)w;(void)data;
    debug("quit\n");
    gtk_main_quit();
}





int main_old(int argc, char *argv[])
{
    GtkWidget* window;
    GtkWidget* vbox;
    GtkWidget* hbox;
    GtkWidget* label;
    GtkWidget* hslider;
    GtkWidget* vslider;
    GtkWidget* frame;
    GtkWidget* table;
    GtkTable* t;

    gtk_init(&argc, &argv);

    window = gtk_window_new (GTK_WINDOW_TOPLEVEL);

    g_signal_connect(G_OBJECT(window), "delete-event",
                            G_CALLBACK (cb_delete), NULL);

    g_signal_connect(G_OBJECT(window), "destroy",
                            G_CALLBACK(cb_quit), NULL);

    frame = gtk_frame_new("Test");
    gtk_container_add(GTK_CONTAINER(window), frame);
    gtk_container_set_border_width(GTK_CONTAINER(frame), 4);

    table = gtk_table_new(8, 3, FALSE);
    t = GTK_TABLE(table);
    gtk_container_add(GTK_CONTAINER(frame), table);

    hslider = phin_hslider_new_with_range(0.0, -1.0, 1.0, 0.1);
    gui_attach(t, hslider, 0, 1, 0, 1);
    gtk_widget_show(hslider);

    vslider = phin_vslider_new_with_range(0.0, -1.0, 1.0, 0.1);
    gui_attach(t, vslider, 1, 2, 0, 1);
    gtk_widget_show(vslider);

*//*
    gui_pack(box, table);
    vbox = gtk_vbox_new (FALSE, 0);
    gtk_container_add (GTK_CONTAINER(frame), vbox);

    hbox = gtk_hbox_new(FALSE, 0);
    gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 0);

    label = gtk_label_new("Test");
    gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 0);

    hslider = phin_hslider_new_with_range(0.0, -1.0, 1.0, 0.1);
    gtk_box_pack_start(GTK_BOX(hbox), hslider, TRUE, TRUE, 0);
    gtk_widget_show(hslider);

    hslider = phin_hslider_new_with_range(0.0, 0.0, 1.0, 0.1);
    gtk_box_pack_start(GTK_BOX(hbox), hslider, TRUE, TRUE, 0);
    gtk_widget_show(hslider);

    vslider = phin_vslider_new_with_range(0.0, -1.0, 1.0, 0.1);
    gtk_box_pack_start(GTK_BOX(vbox), vslider, TRUE, TRUE, 0);
    gtk_widget_show(vslider);

    vslider = phin_vslider_new_with_range(0.0, 0.0, 1.0, 0.1);
    gtk_box_pack_start(GTK_BOX(vbox), vslider, TRUE, TRUE, 0);

    gtk_widget_show(vslider);
    gtk_widget_show(label);
    gtk_widget_show(hbox);
    gtk_widget_show(vbox);
*//*
    gtk_widget_show(table);
    gtk_widget_show(frame);
    gtk_widget_show(window);

    gtk_main();

    return 0;
}
*/
