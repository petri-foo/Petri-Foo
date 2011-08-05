/*  Phin is a fork of the PHAT Audio Toolkit.
    Phin is part of Petri-Foo. Petri-Foo is a fork of Specimen.

    Original author Pete Bessman
    Copyright 2005 Pete Bessman
    Copyright 2011 James W. Morris

    This file is part of Phin.

    Phin is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License version 2 as
    published by the Free Software Foundation.

    Phin is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with Phin.  If not, see <http://www.gnu.org/licenses/>.

    This file is a derivative of a PHAT original, modified 2011
*/
#include <stdio.h>
#include <gtk/gtk.h>
#include <libgnomecanvas/libgnomecanvas.h>
#include "phinkeyboard.h"


/* properties */
enum
{
    PROP_0,
    PROP_ORIENTATION,
    PROP_NUMKEYS,
    PROP_SHOWLABELS,
};

/* signals */
enum
{
    KEY_PRESSED,
    KEY_RELEASED,
    LAST_SIGNAL,
};

static int signals[LAST_SIGNAL];


/* magic numbers */
enum
{
    NUMKEYS = 128,
    MINKEYS = 1,
    MAXKEYS = 1000,
    TEXT_POINTS = 7,
};

/* all colors are 32bits, RGBA, 8 bits (2 hex digits) per channel */

/* natural (white) key colors */
static const guint KEY_NAT_BG =     0xEEEEEEFF;
static const guint KEY_NAT_HI =     0xFFFFFFFF;
static const guint KEY_NAT_LOW =    0x000000FF;
static const guint KEY_NAT_PRE =    0xFFFFFFFF;
static const guint KEY_NAT_ON =     0xD7D7D7FF;
static const guint KEY_NAT_SHAD =   0xAAAAAAFF;

/* accidental (black) key colors */
static const guint KEY_ACC_BG =     0x949494FF;
static const guint KEY_ACC_HI =     0xC9C9C9FF;
static const guint KEY_ACC_LOW =    0x000000FF;
static const guint KEY_ACC_PRE =    0xA5A5A5FF;
static const guint KEY_ACC_ON =     0x767676FF;
static const guint KEY_ACC_SHAD =   0x4D4D4DFF;

/* c text label color */
static const guint KEY_TEXT_BG =    0x000000FF;


typedef struct __Key _Key;

struct __Key
{
    int index;
    PhinKeyboard* keyboard;     /* the keyboard we belong to */
    GnomeCanvasGroup* group;    /* the group this key belongs to */
    GnomeCanvasItem* pre;       /* prelight rectangle */
    GnomeCanvasItem* on;        /* active (depressed) rectangle */
    GnomeCanvasItem* shad;      /* active shadow */
};


typedef struct _PhinKeyboardPrivate PhinKeyboardPrivate;

#define PHIN_KEYBOARD_GET_PRIVATE(obj) \
    (G_TYPE_INSTANCE_GET_PRIVATE((obj), PHIN_TYPE_KEYBOARD, \
                                        PhinKeyboardPrivate))

struct _PhinKeyboardPrivate
{
    _Key    *keys;
    int     nkeys;
    int     label;

    GnomeCanvas*    canvas;
    GtkOrientation  orientation;
};


G_DEFINE_TYPE(PhinKeyboard, phin_keyboard, GTK_TYPE_VIEWPORT)


static void phin_keyboard_class_init(PhinKeyboardClass* klass);
static void phin_keyboard_init(PhinKeyboard* self);
static void phin_keyboard_dispose(GObject* object);
static void phin_keyboard_set_property(GObject          *object,
                                       guint             prop_id,
                                       const GValue     *value,
                                       GParamSpec       *pspec);
static void phin_keyboard_get_property(GObject          *object,
                                       guint             prop_id,
                                       GValue           *value,
                                       GParamSpec       *pspec);


static void phin_keyboard_class_init(PhinKeyboardClass* klass)
{
    GObjectClass* object_class = G_OBJECT_CLASS(klass);

    phin_keyboard_parent_class = g_type_class_peek_parent(klass);

    object_class->dispose = phin_keyboard_dispose;

    g_type_class_add_private(object_class, sizeof(PhinKeyboardPrivate));

    object_class->set_property = phin_keyboard_set_property;
    object_class->get_property = phin_keyboard_get_property;

    g_object_class_install_property(object_class,
                                    PROP_ORIENTATION,
                                    g_param_spec_enum("orientation",
                                                      "Orientation",
                    "How the keyboard should be arranged on the screen",
                                    GTK_TYPE_ORIENTATION,
                                    GTK_ORIENTATION_VERTICAL,
                                    G_PARAM_READWRITE
                                  | G_PARAM_CONSTRUCT_ONLY));

    g_object_class_install_property(object_class,
                                    PROP_NUMKEYS,
                                    g_param_spec_int("numkeys",
                                                     "Number of Keys",
                    "How many keys this keyboard should have",
                                    MINKEYS,
                                    MAXKEYS,
                                    NUMKEYS,
                                    G_PARAM_READWRITE
                                  | G_PARAM_CONSTRUCT_ONLY));

    g_object_class_install_property(object_class,
                                    PROP_SHOWLABELS,
                                    g_param_spec_boolean("show-labels",
                                                         "Show Labels",
                    "Whether C keys should be labeled or not",
                                    TRUE,
                                    G_PARAM_READWRITE
                                  | G_PARAM_CONSTRUCT_ONLY));

    /**
     * PhinKeyboard::key-pressed
     * @keyboard: the object on which the signal was emitted
     * @key: the index of the key that was pressed
     *
     * The "key-pressed" signal is emitted whenever a key is pressed.
     *
     */
    signals[KEY_PRESSED] =
        g_signal_new ("key-pressed",
                      G_TYPE_FROM_CLASS (klass),
                      G_SIGNAL_RUN_FIRST | G_SIGNAL_ACTION,
                      G_STRUCT_OFFSET (PhinKeyboardClass, key_pressed),
                      NULL, NULL,
                      g_cclosure_marshal_VOID__INT,
                      G_TYPE_NONE, 1, G_TYPE_INT);

    /**
     * PhinKeyboard::key-released
     * @keyboard: the object on which the signal was emitted
     * @key: the index of the key that was pressed
     *
     * The "key-released" signal is emitted whenever a key is released.
     *
     */
    signals[KEY_RELEASED] =
        g_signal_new ("key-released",
                      G_TYPE_FROM_CLASS (klass),
                      G_SIGNAL_RUN_FIRST | G_SIGNAL_ACTION,
                      G_STRUCT_OFFSET (PhinKeyboardClass, key_released),
                      NULL, NULL,
                      g_cclosure_marshal_VOID__INT,
                      G_TYPE_NONE, 1, G_TYPE_INT);

    klass->key_pressed = NULL;
    klass->key_released = NULL;
}


static gboolean
key_press_cb(GnomeCanvasItem* item, GdkEvent* event, _Key* key)
{
    (void)item;

    switch (event->type)
    {
    case GDK_BUTTON_PRESS:
        gnome_canvas_item_show(key->on);
        gnome_canvas_item_show(key->shad);
        g_signal_emit(key->keyboard, signals[KEY_PRESSED], 0, key->index);
        break;

    case GDK_BUTTON_RELEASE:
        gnome_canvas_item_hide(key->on);
        gnome_canvas_item_hide(key->shad);
        g_signal_emit(key->keyboard, signals[KEY_RELEASED], 0, key->index);
        break;

    case GDK_ENTER_NOTIFY:
        gnome_canvas_item_show(key->pre);
        break;

    case GDK_LEAVE_NOTIFY:
        gnome_canvas_item_hide(key->pre);
        break;

    default:
        break;
    }

    return FALSE;
}


/* so much gayness in here, either I suck or gnome-canvas does; most
 * likely, we both do */
static void draw_key(PhinKeyboard* self, int index, int pos, guint bg,
                     guint hi, guint low, guint pre, guint on, guint shad)
{
    PhinKeyboardPrivate* p = PHIN_KEYBOARD_GET_PRIVATE(self);

    _Key* key = &p->keys[index];
    GnomeCanvasPoints* points;
    int x1;
    int y1;
    int x2;
    int y2;

    if (p->orientation == GTK_ORIENTATION_VERTICAL)
    {
        x1 = 0;
        y1 = pos + 1;           /* teh gayz0r */
        x2 = PHIN_KEYBOARD_KEY_LENGTH - 1;
        y2 = pos - PHIN_KEYBOARD_KEY_WIDTH + 1;
    }
    else
    {
        x1 = pos + PHIN_KEYBOARD_KEY_WIDTH - 1;
        y1 = 0;
        x2 = pos;
        y2 = PHIN_KEYBOARD_KEY_LENGTH - 1;
    }

    /* key group */
    key->group = (GnomeCanvasGroup*)
        gnome_canvas_item_new(  gnome_canvas_root(p->canvas),
                                gnome_canvas_group_get_type(), NULL);

    g_signal_connect(G_OBJECT(key->group), "event",
                     G_CALLBACK(key_press_cb), (gpointer)key);

    key->index = index;
    key->keyboard = self;

    /* draw main key rect */
    gnome_canvas_item_new(key->group,
                          gnome_canvas_rect_get_type(),
                          "x1", (gdouble)x1,
                          "y1", (gdouble)y1,
                          "x2", (gdouble)x2,
                          "y2", (gdouble)y2,
                          "fill-color-rgba", bg,
                          NULL);

    /* draw prelight rect */
    key->pre = gnome_canvas_item_new(key->group,
                                     gnome_canvas_rect_get_type(),
                                     "x1", (gdouble)x1,
                                     "y1", (gdouble)y1,
                                     "x2", (gdouble)x2,
                                     "y2", (gdouble)y2,
                                     "fill-color-rgba", pre,
                                     NULL);

    gnome_canvas_item_hide(key->pre);

    /* draw key highlight */
    points = gnome_canvas_points_new(3);

    if (p->orientation == GTK_ORIENTATION_VERTICAL)
    {
        points->coords[0] = x1+1;   points->coords[1] = y1;
        points->coords[2] = x1+1;   points->coords[3] = y2+1;
        points->coords[4] = x2;     points->coords[5] = y2+1;
    }
    else
    {
        points->coords[0] = x1;     points->coords[1] = y1+1;
        points->coords[2] = x2;     points->coords[3] = y1+1;
        points->coords[4] = x2;     points->coords[5] = y2;
    }

    gnome_canvas_item_new(key->group,
                          gnome_canvas_line_get_type(),
                          "points", points,
                          "width-units", (gdouble)1,
                          "fill-color-rgba", hi,
                          NULL);

    gnome_canvas_points_unref(points);
                        
    /* draw key border */
    points = gnome_canvas_points_new(4);
    
    if (p->orientation == GTK_ORIENTATION_VERTICAL)
    {
        points->coords[0] = x1;     points->coords[1] = y1;
        points->coords[2] = x1;     points->coords[3] = y2;
        points->coords[4] = x2;     points->coords[5] = y2;
        points->coords[6] = x2;     points->coords[7] = y1;
    }
    else
    {
        points->coords[0] = x2;     points->coords[1] = y1;
        points->coords[2] = x1;     points->coords[3] = y1;
        points->coords[4] = x1;     points->coords[5] = y2;
        points->coords[6] = x2;     points->coords[7] = y2;
    }

    gnome_canvas_item_new(key->group,
                          gnome_canvas_line_get_type(),
                          "points", points,
                          "width-units", (gdouble)1,
                          "fill-color-rgba", low,
                          NULL);

    gnome_canvas_points_unref(points);

    if (p->orientation == GTK_ORIENTATION_VERTICAL)
    {
        /* draw active rect */
        key->on = gnome_canvas_item_new(key->group,
                                        gnome_canvas_rect_get_type(),
                                        "x1", (gdouble)x1+1,
                                        "y1", (gdouble)y1,
                                        "x2", (gdouble)x2,
                                        "y2", (gdouble)y2+1,
                                        "fill-color-rgba", on,
                                        NULL);
    }
    else
    {
        /* draw active rect */
        key->on = gnome_canvas_item_new(key->group,
                                        gnome_canvas_rect_get_type(),
                                        "x1", (gdouble)x1,
                                        "y1", (gdouble)y1+1,
                                        "x2", (gdouble)x2,
                                        "y2", (gdouble)y2,
                                        "fill-color-rgba", on,
                                        NULL);
    }

    gnome_canvas_item_hide(key->on);

    /* draw active shadow */
    points = gnome_canvas_points_new(6);

    if (p->orientation == GTK_ORIENTATION_VERTICAL)
    {
        points->coords[0] = x1+1;   points->coords[1] = y1;
        points->coords[2] = x1+1;   points->coords[3] = y2+1;
        points->coords[4] = x2;     points->coords[5] = y2+1;
        points->coords[6] = x2;     points->coords[7] = y2 + 3;
        points->coords[8] = x1 + 3; points->coords[9] = y2 + 3;
        points->coords[10] = x1 + 3;points->coords[11] = y1;
    }
    else
    {
        points->coords[0] = x1;     points->coords[1] = y1 + 1;
        points->coords[2] = x2;     points->coords[3] = y1 + 1;
        points->coords[4] = x2;     points->coords[5] = y2;
        points->coords[6] = x2 + 2; points->coords[7] = y2;
        points->coords[8] = x2 + 2; points->coords[9] = y1 + 3;
        points->coords[10] = x1;    points->coords[11] = y1 + 3;
    }

    key->shad = gnome_canvas_item_new(key->group,
                                      gnome_canvas_polygon_get_type(),
                                      "points", points,
                                      "fill-color-rgba", shad,
                                      NULL);
    gnome_canvas_item_hide(key->shad);
    gnome_canvas_points_unref(points);

    /* draw label if applicable */
    if (p->label && (index % 12) == 0)
    {
        char* s = g_strdup_printf("%d", index / 12);

        if (p->orientation == GTK_ORIENTATION_VERTICAL)
        {
            gnome_canvas_item_new(key->group,
                        gnome_canvas_text_get_type(),
                        "text", s,
                        "x", (gdouble)(x2 - 2),
                        "y", (gdouble)(y1 - (PHIN_KEYBOARD_KEY_WIDTH / 2)),
                        "anchor", GTK_ANCHOR_EAST,
                        "fill-color-rgba", (gint)KEY_TEXT_BG,
                        "font", "sans",
                        "size-points", (gdouble)TEXT_POINTS, NULL);
        }
        else
        {
            gnome_canvas_item_new(key->group,
                        gnome_canvas_text_get_type(),
                        "text", s,
                        "x", (gdouble)(x1 - (PHIN_KEYBOARD_KEY_WIDTH / 2)),
                        "y", (gdouble)(y2 - 2),
                        "anchor", GTK_ANCHOR_SOUTH,
                        "fill-color-rgba", (gint)KEY_TEXT_BG,
                        "font", "sans",
                        "size-points", (gdouble)TEXT_POINTS,
                        "justification", GTK_JUSTIFY_CENTER, NULL);
        }

        g_free(s);
    }
}


static void draw_keyboard(PhinKeyboard* self)
{
    PhinKeyboardPrivate* p = PHIN_KEYBOARD_GET_PRIVATE(self);
    int i, pos, note;

    /* make sure our construction properties are set (this is
     * _lame_ass_) */
    if (p->nkeys < 0 || p->label < 0)
        return;

    p->keys = g_new(_Key, p->nkeys);

    /* orientation */
    if (p->orientation == GTK_ORIENTATION_VERTICAL)
    {
        gtk_widget_set_size_request(GTK_WIDGET(self),
                                    PHIN_KEYBOARD_KEY_LENGTH, 0);
        gtk_widget_set_size_request(GTK_WIDGET(p->canvas),
                                    PHIN_KEYBOARD_KEY_LENGTH,
                                    (PHIN_KEYBOARD_KEY_WIDTH * p->nkeys));
        gnome_canvas_set_scroll_region(p->canvas, 0, 0,
                                    PHIN_KEYBOARD_KEY_LENGTH - 1,
                                    (PHIN_KEYBOARD_KEY_WIDTH * p->nkeys)-1);
    }
    else
    {
        gtk_widget_set_size_request(GTK_WIDGET(self), 0,
                                    PHIN_KEYBOARD_KEY_LENGTH);
        gtk_widget_set_size_request(GTK_WIDGET(p->canvas),
                                    (PHIN_KEYBOARD_KEY_WIDTH * p->nkeys),
                                    PHIN_KEYBOARD_KEY_LENGTH);
        gnome_canvas_set_scroll_region(p->canvas, 0, 0,
                                    (PHIN_KEYBOARD_KEY_WIDTH * p->nkeys)-1,
                                    PHIN_KEYBOARD_KEY_LENGTH - 1);
    }

    if (p->orientation == GTK_ORIENTATION_VERTICAL)
    {
        pos = (PHIN_KEYBOARD_KEY_WIDTH * p->nkeys) - 1;

        for (i = note = 0; i < p->nkeys; ++i, ++note)
        {
            if (note > 11)
                note = 0;

            switch (note)
            {
            case 0: case 2: case 4: case 5: case 7: case 9: case 11:
                draw_key(self, i, pos, KEY_NAT_BG, KEY_NAT_HI,
                         KEY_NAT_LOW, KEY_NAT_PRE,
                         KEY_NAT_ON, KEY_NAT_SHAD);
                break;

            default:
                draw_key(self, i, pos, KEY_ACC_BG, KEY_ACC_HI,
                         KEY_ACC_LOW, KEY_ACC_PRE,
                         KEY_ACC_ON, KEY_ACC_SHAD);
                break;
            }
            pos -= PHIN_KEYBOARD_KEY_WIDTH;
        }
    }
    else
    {
        pos = 0;

        for (i = note = 0; i < p->nkeys; ++i, ++note)
        {
            if (note > 11)
                note = 0;

            switch (note)
            {
            case 0: case 2: case 4: case 5: case 7: case 9: case 11:
                draw_key(self, i, pos, KEY_NAT_BG, KEY_NAT_HI,
                         KEY_NAT_LOW, KEY_NAT_PRE,
                         KEY_NAT_ON, KEY_NAT_SHAD);
                break;

            default:
                draw_key(self, i, pos, KEY_ACC_BG, KEY_ACC_HI,
                         KEY_ACC_LOW, KEY_ACC_PRE,
                         KEY_ACC_ON, KEY_ACC_SHAD);
                break;
            }

            pos += PHIN_KEYBOARD_KEY_WIDTH;
        }
    }
}



static void phin_keyboard_init(PhinKeyboard* self)
{
    PhinKeyboardPrivate* p = PHIN_KEYBOARD_GET_PRIVATE(self);
    p->keys = NULL;
    p->nkeys = -1;
    p->orientation = -1;
    p->label = -1;

    p->canvas = (GnomeCanvas*) gnome_canvas_new();
    gtk_container_add(GTK_CONTAINER(self), GTK_WIDGET(p->canvas));
    gtk_widget_show(GTK_WIDGET(p->canvas));
}
 

static void phin_keyboard_set_property(GObject          *object,
                                       guint             prop_id,
                                       const GValue     *value,
                                       GParamSpec       *pspec)
{
    PhinKeyboardPrivate* p = PHIN_KEYBOARD_GET_PRIVATE(object);

    switch (prop_id)
    {
    case PROP_ORIENTATION:
        p->orientation = g_value_get_enum(value);
        break;

    case PROP_NUMKEYS:
        p->nkeys = g_value_get_int(value);
        break;

    case PROP_SHOWLABELS:
        p->label = g_value_get_boolean(value);
        break;

    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
        return;
    }

    draw_keyboard(PHIN_KEYBOARD(object));
}


static void phin_keyboard_get_property(GObject          *object,
                                       guint             prop_id,
                                       GValue           *value,
                                       GParamSpec       *pspec)
{
    PhinKeyboardPrivate* p = PHIN_KEYBOARD_GET_PRIVATE(object);

    switch (prop_id)
    {
    case PROP_ORIENTATION:
        g_value_set_enum (value, p->orientation);
        break;

    case PROP_NUMKEYS:
        g_value_set_int(value, p->nkeys);
        break;

    case PROP_SHOWLABELS:
        g_value_set_boolean(value, p->label);
        break;

    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
        break;
    }
}


static void phin_keyboard_dispose(GObject* object)
{
    PhinKeyboardPrivate* p = PHIN_KEYBOARD_GET_PRIVATE(object);

    g_free(p->keys);
    p->keys = NULL;
}


/**
 * phin_keyboard_get_adjustment:
 * @keyboard: a #PhinKeyboard
 *
 * Retrives the current adjustment in use by @keyboard.
 *
 * Returns: @keyboard's current #GtkAdjustment
 *
 */
GtkAdjustment* phin_keyboard_get_adjustment(PhinKeyboard* kb)
{
    PhinKeyboardPrivate* p = PHIN_KEYBOARD_GET_PRIVATE(kb);
    GtkAdjustment* adj;

    if (p->orientation == GTK_ORIENTATION_VERTICAL)
        g_object_get(kb, "vadjustment", &adj, (char *)NULL);
    else
        g_object_get(kb, "hadjustment", &adj, (char *)NULL);

    return adj;
}


/**
 * phin_keyboard_set_adjustment:
 * @keyboard: a #PhinKeyboard
 * @adjustment: a #GtkAdjustment
 *
 * Sets the adjustment used by @keyboard.
 *
 */
void phin_keyboard_set_adjustment(PhinKeyboard* kb, GtkAdjustment* adj)
{
    PhinKeyboardPrivate* p = PHIN_KEYBOARD_GET_PRIVATE(kb);

    if (!adj)
        return;

    if (p->orientation == GTK_ORIENTATION_VERTICAL)
        g_object_set(kb, "vadjustment", adj, (char *)NULL);
    else
        g_object_set(kb, "hadjustment", adj, (char *)NULL);
}
