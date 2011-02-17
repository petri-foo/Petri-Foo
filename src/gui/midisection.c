#include <gtk/gtk.h>
#include <phat/phat.h>
#include <libgnomecanvas/libgnomecanvas.h>
#include "midisection.h"
#include "petri-foo.h"
#include "gui.h"
#include "patchlist.h"
#include "midi.h"
#include "mixer.h"
#include "patch_util.h"


static GtkVBoxClass* parent_class;

static void midi_section_class_init(MidiSectionClass* klass);
static void midi_section_init(MidiSection* self);


/* magic numbers */
enum
{
    HEIGHT = 8,
    BG_COLOR = 0x888888FF,
    NOTE_COLOR = 0xEECC00FF,
    RANGE_COLOR = 0xDD8800FF,
};


GType midi_section_get_type(void)
{
    static GType type = 0;

    if (!type)
    {
	static const GTypeInfo info =
	    {
		sizeof (MidiSectionClass),
		NULL,
		NULL,
		(GClassInitFunc) midi_section_class_init,
		NULL,
		NULL,
		sizeof (MidiSection),
		0,
		(GInstanceInitFunc) midi_section_init,
	    };

	/* replace PARENT_CLASS_TYPE with whatever's appropriate for your widget */
	type = g_type_register_static(GTK_TYPE_VBOX, "MidiSection", &info, 0);
    }

    return type;
}


static void pressed_cb(GtkWidget* widget, int key, MidiSection* self)
{
    /* a ghetto form of set-insensitive */
    if (self->patch < 0)
	return;

    mixer_note_on_with_id(self->patch, key, 1.0);
}


static void released_cb(GtkWidget* widget, int key, MidiSection* self)
{
    /* a ghetto form of set-insensitive */
    if (self->patch < 0)
	return;

    mixer_note_off_with_id(self->patch, key);
}


static gboolean range_cb(GnomeCanvasItem* item, GdkEvent* event, MidiSection* self)
{
    int clicked;
    int note;
    int lower;
    int upper;
    int range;
    int change_range = 0;
    PatchList* list = gui_get_patch_list();

    /* a ghetto form of set-insensitive */
    if (self->patch < 0)
	return FALSE;
    
    switch (event->type)
    {
    case GDK_BUTTON_PRESS:
	clicked = event->button.x / PHAT_KEYBOARD_KEY_WIDTH;
	note = patch_get_note(self->patch);
	lower = patch_get_lower_note(self->patch);
	upper = patch_get_upper_note(self->patch);
	range = patch_get_range(self->patch);

	/* process the click */
	if (event->button.button == 1)
	{
	    lower = clicked;
	    change_range = 1;
	}
	else if (event->button.button == 2)
	{
	    note = clicked;
	}
	else if (event->button.button == 3)
	{
	    upper = clicked;
	    change_range = 1;
	}

	/* clamp the parameters */
	if (note < lower)
	{
	    lower = note;
	}
	if (note > upper)
	{
	    upper = note;
	}

	/* if the range is off, and a range adjusting button wasn't
	 * pressed, then clamp the range down to nothing (which will
	 * result in it remaining disabled) */
	if (!range && !change_range)
	{
	    lower = upper = note;
	}
	    
	/* reposition note */
	gnome_canvas_item_set(self->note, "x1",
			      (gdouble) (note * PHAT_KEYBOARD_KEY_WIDTH - 1),
			      NULL);
	gnome_canvas_item_set(self->note, "x2",
			      (gdouble) (note * PHAT_KEYBOARD_KEY_WIDTH + PHAT_KEYBOARD_KEY_WIDTH - 1),
			      NULL);

	/* reposition range */
	gnome_canvas_item_set(self->range, "x1",
			      (gdouble) (lower * PHAT_KEYBOARD_KEY_WIDTH - 1),
			      NULL);
	gnome_canvas_item_set(self->range, "x2",
			      (gdouble) (upper * PHAT_KEYBOARD_KEY_WIDTH + PHAT_KEYBOARD_KEY_WIDTH - 1),
			      NULL);

	/* apply changes */
	patch_set_note(self->patch, note);
	patch_set_lower_note(self->patch, lower);
	patch_set_upper_note(self->patch, upper);
	if (lower == upper)
	{
	    patch_set_range(self->patch, FALSE);
	    gnome_canvas_item_hide(self->range);
	}
	else
	{
	    patch_set_range(self->patch, TRUE);
	    gnome_canvas_item_show(self->range);
	}

	/* we might have moved this patch around relative to the list;
	 * update the list and try to keep this patch selected */
	self->ignore = TRUE;
	patch_list_update(list, patch_list_get_current_patch(list), PATCH_LIST_PATCH);
        break;
    default:
	break;
    };

    return FALSE;		/* make sure my fellow jiggas pickup up da shizzle */
}


/* what the fuck is wrong with me... did I just say shizzle? */

static void connect(MidiSection* self)
{
    g_signal_connect(G_OBJECT(self->keyboard), "key-pressed",
		     G_CALLBACK(pressed_cb), (gpointer) self);
    g_signal_connect(G_OBJECT(self->keyboard), "key-released",
		     G_CALLBACK(released_cb), (gpointer) self);
}


/* why am I having a conversation with myself in my program through comments? */

static void block(MidiSection* self)
{
    g_signal_handlers_block_by_func(self->keyboard, pressed_cb, self);
    g_signal_handlers_block_by_func(self->keyboard, released_cb, self);
}

/* I need crack. */

static void unblock(MidiSection* self)
{
    g_signal_handlers_unblock_by_func(self->keyboard, pressed_cb, self);
    g_signal_handlers_unblock_by_func(self->keyboard, released_cb, self);
}


static void set_sensitive(MidiSection* self, gboolean val)
{
    /* nada */
}


static void midi_section_class_init(MidiSectionClass* klass)
{
    parent_class = g_type_class_peek_parent(klass);
}


static void midi_section_init(MidiSection* self)
{
    GtkBox* box = GTK_BOX(self);
    GtkWidget* pad;
    GtkWidget* view;
    GtkWidget* scroll;
    GnomeCanvas* canvas;
    GnomeCanvasPoints* points;
    int x1, x2, y1, y2;

    self->patch = -1;
    self->ignore = FALSE;
    x1 = 0;
    y1 = 0;
    x2 = (PHAT_KEYBOARD_KEY_WIDTH * MIDI_NOTES);
    y2 = HEIGHT;

    /* adjustment */
    self->adj = (GtkAdjustment*) gtk_adjustment_new(0, 0, 0, 0, 0, 0);

    /* viewport */
    view = gtk_viewport_new(self->adj, NULL);
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
		     G_CALLBACK(range_cb), (gpointer)self);
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
    self->range = gnome_canvas_item_new(gnome_canvas_root(canvas),
					gnome_canvas_rect_get_type(),
					"x1", (gdouble)x1,
					"y1", (gdouble)y1,
					"x2", (gdouble)PHAT_KEYBOARD_KEY_WIDTH,
					"y2", (gdouble)y2,
					"fill-color-rgba", RANGE_COLOR,
					"outline-color", "black",
					NULL);
    gnome_canvas_item_hide(self->range);
    
    /* range root note */
    self->note = gnome_canvas_item_new(gnome_canvas_root(canvas),
				       gnome_canvas_rect_get_type(),
				       "x1", (gdouble)x1,
				       "y1", (gdouble)y1,
				       "x2", (gdouble)PHAT_KEYBOARD_KEY_WIDTH,
				       "y2", (gdouble)y2,
				       "fill-color-rgba", NOTE_COLOR,
				       "outline-color", "black",
				       NULL);
    gnome_canvas_item_hide(self->note);

    self->canvas = canvas;

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
    self->keyboard = phat_hkeyboard_new(self->adj, MIDI_NOTES, TRUE);
    gtk_box_pack_start(box, self->keyboard, FALSE, FALSE, 0);
    gtk_widget_show(self->keyboard);

    /* vpad */
    pad = gui_vpad_new(GUI_SCROLLSPACE);
    gtk_box_pack_start(box, pad, FALSE, FALSE, 0);
    gtk_widget_show(pad);

    /* scrollbar */
    scroll = gtk_hscrollbar_new(self->adj);
    gtk_box_pack_start(box, scroll, FALSE, FALSE, 0);
    gtk_widget_show(scroll);

    /* done */
    connect(self);
}


GtkWidget* midi_section_new(void)
{
    return (GtkWidget*) g_object_new(MIDI_SECTION_TYPE, NULL);
}


void midi_section_set_patch(MidiSection* self, int patch)
{
    int note;
    int lower;
    int upper;
    int range;

    self->patch = patch;

    if (patch < 0)
    {
	set_sensitive(self, FALSE);
	gnome_canvas_item_hide(self->note);
	gnome_canvas_item_hide(self->range);

	if (self->ignore)
	    self->ignore = FALSE;
	else
	    gtk_adjustment_set_value(self->adj,
				     (0.5 * self->adj->upper)
				     - (self->adj->page_size / 2));
    }
    else
    {
	set_sensitive(self, TRUE);

	note = patch_get_note(patch);
	lower = patch_get_lower_note(patch);
	upper = patch_get_upper_note(patch);
	range = patch_get_range(patch);

	block(self);

	/* so let me get this straight... if I set more than one
	 * property at once, shit gets fux0red? Fags. */
	gnome_canvas_item_set(self->note, "x1",
			      (gdouble) (note * PHAT_KEYBOARD_KEY_WIDTH - 1),
			      NULL);
	gnome_canvas_item_set(self->note, "x2",
			      (gdouble) (note * PHAT_KEYBOARD_KEY_WIDTH + PHAT_KEYBOARD_KEY_WIDTH - 1),
			      NULL);
	gnome_canvas_item_show(self->note);

	/* do the same dumbshit for the range */
	gnome_canvas_item_set(self->range, "x1",
			      (gdouble) (lower * PHAT_KEYBOARD_KEY_WIDTH - 1),
			      NULL);
	gnome_canvas_item_set(self->range, "x2",
			      (gdouble) (upper * PHAT_KEYBOARD_KEY_WIDTH + PHAT_KEYBOARD_KEY_WIDTH - 1),
			      NULL);
	if (range)
	    gnome_canvas_item_show(self->range);
	else
	    gnome_canvas_item_hide(self->range);

	/* scroll the keyboard to show the root note */
	if (self->ignore)
	    self->ignore = FALSE;
	else
	    gtk_adjustment_set_value(self->adj,
				     ((note+1.0) / MIDI_NOTES)
				     * self->adj->upper
				     - (self->adj->page_size / 2));
	
	unblock(self);
    }
}

