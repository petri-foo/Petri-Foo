#include <gtk/gtk.h>
#include <phat/phat.h>
#include <libgnomecanvas/libgnomecanvas.h>
#include "midisection.h"
#include "petri-foo.h"
#include "gui.h"
#include "patchlist.h"
#include "midi.h"
#include "mixer.h"
#include "patch_set_and_get.h"
#include "driver.h"


/* magic numbers */
enum
{
    HEIGHT = 8,
    BG_COLOR = 0x888888FF,
    NOTE_COLOR = 0xEECC00FF,
    RANGE_COLOR = 0xDD8800FF,
};


typedef struct _MidiSectionPrivate MidiSectionPrivate;

#define MIDI_SECTION_GET_PRIVATE(obj)   \
    (G_TYPE_INSTANCE_GET_PRIVATE((obj), \
        MIDI_SECTION_TYPE, MidiSectionPrivate))

struct _MidiSectionPrivate
{
    int                 patch;
    GtkAdjustment*      adj;
    GtkWidget*          keyboard;
    GnomeCanvasItem*    range;
    GnomeCanvasItem*    note;
    GnomeCanvas*        canvas;
    gboolean            ignore;
};


G_DEFINE_TYPE(MidiSection, midi_section, GTK_TYPE_VBOX);


static void midi_section_class_init(MidiSectionClass* klass)
{
    GtkObjectClass *object_class = GTK_OBJECT_CLASS(klass);
    midi_section_parent_class = g_type_class_peek_parent(klass);
    g_type_class_add_private(object_class, sizeof(MidiSectionPrivate));
}


static void pressed_cb(GtkWidget* widget, int key, MidiSectionPrivate* p)
{
    (void)widget;

    /* a ghetto form of set-insensitive */
    if (p->patch < 0)
        return;

    if (driver_running())
        mixer_note_on_with_id(p->patch, key, 1.0);
}


static void released_cb(GtkWidget* widget, int key, MidiSectionPrivate* p)
{
    (void)widget;

    /* a ghetto form of set-insensitive */
    if (p->patch < 0)
        return;

    if (driver_running())
        mixer_note_off_with_id(p->patch, key);
}


static gboolean range_cb(GnomeCanvasItem* item, GdkEvent* event,
                                                MidiSectionPrivate* p)
{
    (void)item;
    int clicked;
    int note;
    int lower;
    int upper;
    gboolean change_range = FALSE;
    PatchList* list;

    /* a ghetto form of set-insensitive */
    if (p->patch < 0)
        return FALSE;

    if (event->type != GDK_BUTTON_PRESS)
        return FALSE;

    list = gui_get_patch_list();

    clicked = event->button.x / PHAT_KEYBOARD_KEY_WIDTH;
    note = patch_get_note(p->patch);
    lower = patch_get_lower_note(p->patch);
    upper = patch_get_upper_note(p->patch);

    /* process the click */
    if (event->button.button == 1)
    {
        lower = clicked;
        change_range = TRUE;
    }
    else if (event->button.button == 2)
    {
        note = clicked;
    }
    else if (event->button.button == 3)
    {
        upper = clicked;
        change_range = TRUE;
    }

    /* clamp the parameters */
    if (note < lower)
        lower = note;

    if (note > upper)
        upper = note;

    /* if the range is off, and a range adjusting button wasn't
     * pressed, then clamp the range down to nothing (which will
     * result in it remaining disabled) */
    if (!change_range && lower == upper)
        lower = upper = note;

    /* reposition note */
    gnome_canvas_item_set(p->note, "x1",
                    (gdouble)(note * PHAT_KEYBOARD_KEY_WIDTH - 1),  NULL);
    gnome_canvas_item_set(p->note, "x2",
                    (gdouble) (note * PHAT_KEYBOARD_KEY_WIDTH
                                    + PHAT_KEYBOARD_KEY_WIDTH - 1), NULL);

    /* reposition range */
    gnome_canvas_item_set(p->range, "x1",
                    (gdouble)(lower * PHAT_KEYBOARD_KEY_WIDTH - 1), NULL);
    gnome_canvas_item_set(p->range, "x2",
                    (gdouble)(upper * PHAT_KEYBOARD_KEY_WIDTH
                                    + PHAT_KEYBOARD_KEY_WIDTH - 1), NULL);

    /* apply changes */
    patch_set_note(p->patch, note);
    patch_set_lower_note(p->patch, lower);
    patch_set_upper_note(p->patch, upper);

    if (lower == upper)
        gnome_canvas_item_hide(p->range);
    else
        gnome_canvas_item_show(p->range);

    /* we might have moved this patch around relative to the list;
     * update the list and try to keep this patch selected */
    p->ignore = TRUE;
    patch_list_update(list, patch_list_get_current_patch(list),
                                                        PATCH_LIST_PATCH);

    return FALSE;
}


static void connect(MidiSectionPrivate* p)
{
    g_signal_connect(G_OBJECT(p->keyboard), "key-pressed",
		     G_CALLBACK(pressed_cb), (gpointer)p);
    g_signal_connect(G_OBJECT(p->keyboard), "key-released",
		     G_CALLBACK(released_cb), (gpointer)p);
}


static void block(MidiSectionPrivate* p)
{
    g_signal_handlers_block_by_func(p->keyboard, pressed_cb, p);
    g_signal_handlers_block_by_func(p->keyboard, released_cb, p);
}


static void unblock(MidiSectionPrivate* p)
{
    g_signal_handlers_unblock_by_func(p->keyboard, pressed_cb, p);
    g_signal_handlers_unblock_by_func(p->keyboard, released_cb, p);
}


static void set_sensitive(MidiSectionPrivate* p, gboolean val)
{
    (void)p;(void)val;
    debug("we're here where it does nothing what so ever!\n");
    /* nada */
}


static void midi_section_init(MidiSection* self)
{
    MidiSectionPrivate* p = MIDI_SECTION_GET_PRIVATE(self);
    GtkBox* box = GTK_BOX(self);
    GtkWidget* pad;
    GtkWidget* view;
    GtkWidget* scroll;
    GnomeCanvas* canvas;
    GnomeCanvasPoints* points;
    int x1, x2, y1, y2;

    p->patch = -1;
    p->ignore = FALSE;
    x1 = 0;
    y1 = 0;
    x2 = (PHAT_KEYBOARD_KEY_WIDTH * MIDI_NOTES);
    y2 = HEIGHT;

    /* adjustment */
    p->adj = (GtkAdjustment*) gtk_adjustment_new(0, 0, 0, 0, 0, 0);

    /* viewport */
    view = gtk_viewport_new(p->adj, NULL);
    gtk_box_pack_start(box, view, FALSE, FALSE, 0);
    gtk_viewport_set_shadow_type(GTK_VIEWPORT(view), GTK_SHADOW_NONE);
    gtk_widget_set_size_request(view, 0, y2);
    gtk_widget_show(view);

    /* canvas */
    canvas = (GnomeCanvas*) gnome_canvas_new();
    gtk_widget_set_size_request(GTK_WIDGET(canvas), x2,	y2);
    gnome_canvas_set_scroll_region(canvas, 0, 0, x2 - 1, y2 -1);
    gtk_container_add(GTK_CONTAINER(view), GTK_WIDGET(canvas));
    g_signal_connect(G_OBJECT(canvas), "event",
		     G_CALLBACK(range_cb), (gpointer)p);
    gtk_widget_show(GTK_WIDGET(canvas));

    /* range display backdrop */
    gnome_canvas_item_new(gnome_canvas_root(canvas),
			  gnome_canvas_rect_get_type(),
			  "x1", (gdouble)0,
			  "y1", (gdouble)0,
			  "x2", (gdouble)x2,
			  "y2", (gdouble)y2,
			  "fill-color-rgba", BG_COLOR,
			  NULL);
    /* range */
    p->range = gnome_canvas_item_new(gnome_canvas_root(canvas),
					gnome_canvas_rect_get_type(),
					"x1", (gdouble)x1,
					"y1", (gdouble)y1,
					"x2", (gdouble)PHAT_KEYBOARD_KEY_WIDTH,
					"y2", (gdouble)y2,
					"fill-color-rgba", RANGE_COLOR,
					"outline-color", "black",
					NULL);
    gnome_canvas_item_hide(p->range);
    
    /* range root note */
    p->note = gnome_canvas_item_new(gnome_canvas_root(canvas),
				       gnome_canvas_rect_get_type(),
				       "x1", (gdouble)x1,
				       "y1", (gdouble)y1,
				       "x2", (gdouble)PHAT_KEYBOARD_KEY_WIDTH,
				       "y2", (gdouble)y2,
				       "fill-color-rgba", NOTE_COLOR,
				       "outline-color", "black",
				       NULL);
    gnome_canvas_item_hide(p->note);

    p->canvas = canvas;

    /* range display border */
    points = gnome_canvas_points_new(4);

    points->coords[0] = x1;
    points->coords[1] = y2;
    points->coords[2] = x1;
    points->coords[3] = y1;
    points->coords[4] = x2-1;
    points->coords[5] = y1;
    points->coords[6] = x2-1;
    points->coords[7] = y2;

    gnome_canvas_item_new(gnome_canvas_root(canvas),
			  gnome_canvas_line_get_type(),
			  "points", points,
			  "width-units", (gdouble)1,
			  "fill-color-rgba", 0,
			  NULL);
    gnome_canvas_points_unref(points);


    /* keyboard */
    p->keyboard = phat_hkeyboard_new(p->adj, MIDI_NOTES, TRUE);
    gtk_box_pack_start(box, p->keyboard, FALSE, FALSE, 0);
    gtk_widget_show(p->keyboard);

    /* vpad */
    pad = gui_vpad_new(GUI_SCROLLSPACE);
    gtk_box_pack_start(box, pad, FALSE, FALSE, 0);
    gtk_widget_show(pad);

    /* scrollbar */
    scroll = gtk_hscrollbar_new(p->adj);
    gtk_box_pack_start(box, scroll, FALSE, FALSE, 0);
    gtk_widget_show(scroll);

    /* done */
    connect(p);
}


GtkWidget* midi_section_new(void)
{
    return (GtkWidget*) g_object_new(MIDI_SECTION_TYPE, NULL);
}


void midi_section_set_patch(MidiSection* self, int patch)
{
    MidiSectionPrivate* p = MIDI_SECTION_GET_PRIVATE(self);
    int note;
    int lower;
    int upper;

    p->patch = patch;

    if (patch < 0)
    {
        set_sensitive(p, FALSE);
        gnome_canvas_item_hide(p->note);
        gnome_canvas_item_hide(p->range);

        if (p->ignore)
            p->ignore = FALSE;
        else
            gtk_adjustment_set_value(p->adj,
                (0.5 * gtk_adjustment_get_upper(p->adj)
                    - gtk_adjustment_get_page_size(p->adj) / 2));
    }
    else
    {

        set_sensitive(p, TRUE);

        note = patch_get_note(patch);
        lower = patch_get_lower_note(patch);
        upper = patch_get_upper_note(patch);

        block(p);

        gnome_canvas_item_set(p->note, "x1",
                (gdouble)(note * PHAT_KEYBOARD_KEY_WIDTH - 1), NULL);
        gnome_canvas_item_set(p->note, "x2",
                (gdouble)(note * PHAT_KEYBOARD_KEY_WIDTH
                               + PHAT_KEYBOARD_KEY_WIDTH - 1), NULL);
        gnome_canvas_item_show(p->note);

        gnome_canvas_item_set(p->range, "x1",
                (gdouble) (lower * PHAT_KEYBOARD_KEY_WIDTH - 1), NULL);
        gnome_canvas_item_set(p->range, "x2",
                (gdouble) (upper * PHAT_KEYBOARD_KEY_WIDTH
                                 + PHAT_KEYBOARD_KEY_WIDTH - 1), NULL);
        if (lower != upper)
            gnome_canvas_item_show(p->range);
        else
            gnome_canvas_item_hide(p->range);

        /* scroll the keyboard to show the root note */
        if (p->ignore)
            p->ignore = FALSE;
        else
            gtk_adjustment_set_value(p->adj,
                ((note + 1.0) / MIDI_NOTES)
                    * gtk_adjustment_get_upper(p->adj)
                    - gtk_adjustment_get_page_size(p->adj) / 2);

        unblock(p);
    }
}

