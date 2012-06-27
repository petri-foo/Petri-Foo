#include <pthread.h>
#include <gtk/gtk.h>
#include <getopt.h>
#include <string.h>

#include <petri-foo.h>
#include <phin.h>



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


int main(int argc, char *argv[])
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

/*
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
*/
    gtk_widget_show(table);
    gtk_widget_show(frame);
    gtk_widget_show(window);

    gtk_main();

    return 0;
}
