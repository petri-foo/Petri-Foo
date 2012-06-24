#include <pthread.h>
#include <gtk/gtk.h>
#include <getopt.h>
#include <string.h>

#include <petri-foo.h>
#include <phin.h>


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


int main(int argc, char *argv[])
{
    GtkWidget* window;
    GtkWidget* vbox;
    GtkWidget* hbox;
    GtkWidget* label;
    GtkWidget* hslider;
    GtkWidget* vslider;

    gtk_init(&argc, &argv);

    window = gtk_window_new (GTK_WINDOW_TOPLEVEL);

    g_signal_connect(G_OBJECT(window), "delete-event",
                            G_CALLBACK (cb_delete), NULL);

    g_signal_connect(G_OBJECT(window), "destroy",
                            G_CALLBACK(cb_quit), NULL);

    vbox = gtk_vbox_new (FALSE, 0);
    gtk_container_add (GTK_CONTAINER(window), vbox);

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
    gtk_widget_show(window);

    gtk_main();

    return 0;
}
