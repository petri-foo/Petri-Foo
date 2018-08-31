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


#include <gtk/gtk.h>
#include <goocanvas-2.0/goocanvas.h>

#include "phin.h"

#include "midisection.h"
#include "petri-foo.h"
#include "gui.h"
#include "patchlist.h"
#include "midi.h"
#include "mixer.h"
#include "patch_set_and_get.h"
#include "driver.h"

static void goo_canvas_item_set(GooCanvasItem *item, const char *property, GValue *value){
	GooCanvasStyle *current_style = goo_canvas_item_get_style(item);
	GooCanvasStyle *clone = goo_canvas_style_copy(current_style);
	goo_canvas_style_set_property(clone, g_quark_from_string(property), value);
	goo_canvas_item_set_style(item, clone);
}

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
    GooCanvasItem*    range;
    GooCanvasItem*    note;
    GooCanvas*        canvas;
    gboolean            ignore;
};


G_DEFINE_TYPE(MidiSection, midi_section, GTK_TYPE_VBOX);


static void midi_section_class_init(MidiSectionClass* klass)
{
    GtkWidgetClass *object_class = GTK_WIDGET_CLASS(klass);
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


static gboolean range_cb(GooCanvasItem* item, GdkEvent* event,
                                                MidiSectionPrivate* p)
{
    (void)item;
    int clicked;
    int note;
    int lower;
    int upper;
    gboolean change_range = FALSE;
    PatchList* list;
	GValue style_value = G_VALUE_INIT;

    /* a ghetto form of set-insensitive */
    if (p->patch < 0)
        return FALSE;

    if (event->type != GDK_BUTTON_PRESS)
        return FALSE;

    list = gui_get_patch_list();

    clicked = event->button.x / PHIN_KEYBOARD_KEY_WIDTH;
    note =  patch_get_root_note(p->patch);
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
    goo_canvas_item_set_simple_transform(p->note, 
			(gdouble)(note * PHIN_KEYBOARD_KEY_WIDTH - 1),
			0,
			PHIN_KEYBOARD_KEY_WIDTH,
			1);

    /* reposition range */
    goo_canvas_item_set_simple_transform(p->range, 
                    (gdouble)(lower * PHIN_KEYBOARD_KEY_WIDTH - 1),
					1,
                    (gdouble)((upper - lower) * PHIN_KEYBOARD_KEY_WIDTH),
					1);

    /* apply changes */
    patch_set_root_note(p->patch, note);
    patch_set_lower_note(p->patch, lower);
    patch_set_upper_note(p->patch, upper);

    if (lower == upper) {
		g_value_init(&style_value, G_TYPE_INT);
		g_value_set_int(&style_value, GOO_CANVAS_ITEM_HIDDEN);
        goo_canvas_item_set(p->range, "visibility", &style_value);
	} else {
		g_value_init(&style_value, G_TYPE_INT);
		g_value_set_int(&style_value, GOO_CANVAS_ITEM_VISIBLE);
        goo_canvas_item_set(p->range, "visibility", &style_value);
	}

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
    /*debug("we're here where it does nothing what so ever!\n");*/
}


static void midi_section_init(MidiSection* self)
{
    MidiSectionPrivate* p = MIDI_SECTION_GET_PRIVATE(self);
    GtkBox* box = GTK_BOX(self);
    GtkWidget* pad;
    GtkWidget* view;
    GtkWidget* scroll;
    GooCanvas* canvas;
    GooCanvasPoints* points;
    int x1, x2, y1, y2;
	GValue style_value = G_VALUE_INIT;

    p->patch = -1;
    p->ignore = FALSE;
    x1 = 0;
    y1 = 0;
    x2 = (PHIN_KEYBOARD_KEY_WIDTH * MIDI_NOTES);
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
    canvas = (GooCanvas*) goo_canvas_new();
    gtk_widget_set_size_request(GTK_WIDGET(canvas), x2,	y2);
    /*TODO: goo_canvas_set_scroll_region(canvas, 0, 0, x2 - 1, y2 -1);*/
    gtk_container_add(GTK_CONTAINER(view), GTK_WIDGET(canvas));
    g_signal_connect(G_OBJECT(canvas), "event",
		     G_CALLBACK(range_cb), (gpointer)p);
    gtk_widget_show(GTK_WIDGET(canvas));

    /* range display backdrop */
    goo_canvas_rect_new(goo_canvas_get_root_item(canvas),
			   (gdouble)0,
			   (gdouble)0,
			   (gdouble)x2,
			   (gdouble)y2,
			  "fill-color-rgba", BG_COLOR,
			  NULL);
    /* range */
    p->range = goo_canvas_rect_new(goo_canvas_get_root_item(canvas),
					 (gdouble)x1,
					 (gdouble)y1,
					 (gdouble)PHIN_KEYBOARD_KEY_WIDTH,
					 (gdouble)y2,
					"fill-color-rgba", RANGE_COLOR,
					"stroke-color", "black",
					NULL);
	g_value_init(&style_value, G_TYPE_INT);
	g_value_set_int(&style_value, GOO_CANVAS_ITEM_HIDDEN);
	goo_canvas_item_set(p->range, "visibility", &style_value);

    /* range root note */
    p->note = goo_canvas_rect_new(goo_canvas_get_root_item(canvas),
				        (gdouble)x1,
				        (gdouble)y1,
				        (gdouble)PHIN_KEYBOARD_KEY_WIDTH,
				        (gdouble)y2,
				       "fill-color-rgba", NOTE_COLOR,
				       "stroke-color", "black",
				       NULL);
	g_value_set_int(&style_value, GOO_CANVAS_ITEM_HIDDEN);
	goo_canvas_item_set(p->note, "visibility", &style_value);

    p->canvas = canvas;

    /* range display border */
    points = goo_canvas_points_new(4);

    points->coords[0] = x1;
    points->coords[1] = y2;
    points->coords[2] = x1;
    points->coords[3] = y1;
    points->coords[4] = x2-1;
    points->coords[5] = y1;
    points->coords[6] = x2-1;
    points->coords[7] = y2;

	char path[512];
	g_snprintf(&path, 511, "M %d %d L %d %d L %d %d Z", 
			points->coords[0], 
			points->coords[1], 
			points->coords[2], 
			points->coords[3], 
			points->coords[4], 
			points->coords[5], 
			points->coords[6], 
			points->coords[7]);

    goo_canvas_path_new(goo_canvas_get_root_item(canvas), path,
			  "fill-color-rgba", 0,
			  NULL);
    goo_canvas_points_unref(points);


    /* keyboard */
    p->keyboard = phin_hkeyboard_new(p->adj, MIDI_NOTES, TRUE);
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
	GValue visible_value = G_VALUE_INIT;
	GValue width_value = G_VALUE_INIT;

	g_value_init(&visible_value, G_TYPE_INT);
	g_value_init(&width_value, G_TYPE_DOUBLE);
	p->patch = patch;

    if (patch < 0)
    {
        set_sensitive(p, FALSE);
		g_value_set_int(&visible_value, GOO_CANVAS_ITEM_HIDDEN);
		goo_canvas_item_set(p->note, "visibility", &visible_value);
		goo_canvas_item_set(p->range, "visibility", &visible_value);

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

        note =  patch_get_root_note(patch);
        lower = patch_get_lower_note(patch);
        upper = patch_get_upper_note(patch);

        block(p);
        set_sensitive(p, FALSE);
		g_value_set_double(&width_value, (gdouble)(note * PHIN_KEYBOARD_KEY_WIDTH - 1));
        goo_canvas_item_set(p->note, "x1", &width_value);
		g_value_set_double(&width_value,
				(gdouble)(note * PHIN_KEYBOARD_KEY_WIDTH
					+ PHIN_KEYBOARD_KEY_WIDTH - 1));
		goo_canvas_item_set(p->note, "x2", &width_value);
		g_value_set_int(&visible_value, GOO_CANVAS_ITEM_VISIBLE);
		goo_canvas_item_set(p->note, "visibility", &visible_value);
		g_value_set_double(&width_value, (gdouble) (lower * PHIN_KEYBOARD_KEY_WIDTH - 1));
        goo_canvas_item_set(p->range, "x1", &width_value);
		g_value_set_double(&width_value,
                (gdouble) (upper * PHIN_KEYBOARD_KEY_WIDTH
                                 + PHIN_KEYBOARD_KEY_WIDTH - 1));
        goo_canvas_item_set(p->range, "x2", &width_value);
		if (lower != upper) {
			g_value_set_int(&visible_value, GOO_CANVAS_ITEM_VISIBLE);
			goo_canvas_item_set(p->range, "visibility", &visible_value);
		} else {
			g_value_set_int(&visible_value, GOO_CANVAS_ITEM_HIDDEN);
			goo_canvas_item_set(p->range, "visibility", &visible_value);
		}

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

