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


#include "idselector.h"

#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>

#include <stdio.h>

#include <stdlib.h>
#include <string.h>

#include "petri-foo.h"



enum
{
    CHANGED,
    LAST_SIGNAL,
};

static int signals[LAST_SIGNAL];


typedef struct _IDSelectorPrivate IDSelectorPrivate;

#define ID_SELECTOR_GET_PRIVATE(obj)     \
    (G_TYPE_INSTANCE_GET_PRIVATE((obj), \
        ID_SELECTOR_TYPE, IDSelectorPrivate))


struct _IDSelectorPrivate
{
    int item_count;
    int index;

    id_name*    ids_names;
    GtkWidget** buttons;
    IDSelector* self;
};


G_DEFINE_TYPE(IDSelector, id_selector, GTK_TYPE_HBOX)


static void id_selector_class_init(IDSelectorClass* klass)
{
    GtkObjectClass* object_class = GTK_OBJECT_CLASS(klass);
    id_selector_parent_class = g_type_class_peek_parent(klass);
    g_type_class_add_private(object_class, sizeof(IDSelectorPrivate));

    signals[CHANGED] =
        g_signal_new (  "changed",
                        G_TYPE_FROM_CLASS (klass),
                        G_SIGNAL_RUN_FIRST | G_SIGNAL_ACTION,
                        G_STRUCT_OFFSET (IDSelectorClass, changed),
                        NULL, NULL,
                        g_cclosure_marshal_VOID__VOID, G_TYPE_NONE, 0);

    klass->changed = NULL;
}


static gboolean
press_cb(GtkToggleButton* button, GdkEvent* event, IDSelectorPrivate* p)
{
    int i;
    gboolean toggle = FALSE;

    switch (event->type)
    {
    case GDK_KEY_PRESS:
        if (event->key.keyval == GDK_Return ||
            event->key.keyval == GDK_KP_Enter ||
            event->key.keyval == GDK_space)
        {
            toggle = TRUE;
        }
        break;

    case GDK_BUTTON_PRESS:
        if (event->button.button == 1)
            toggle = TRUE;
        break;

    default:
        break;
    }

    if (!toggle)
        return FALSE;

    if (gtk_toggle_button_get_active(button))
        return TRUE;

    /* convert button to index number */
    for (i = 0; i < p->item_count; ++i)
    {
        if (GTK_WIDGET(button) == p->buttons[i])
        {
            gtk_toggle_button_set_active(
                GTK_TOGGLE_BUTTON(p->buttons[p->index]), FALSE);
            p->index = i;
            gtk_toggle_button_set_active(
                GTK_TOGGLE_BUTTON(p->buttons[p->index]), TRUE);

            debug("selected index:%d id:%d name:%s\n",
                    i, p->ids_names[i].id, p->ids_names[i].name);

            g_signal_emit_by_name(G_OBJECT(p->self), "changed");
            return TRUE;
        }
    }

    return FALSE;
}


static gboolean
enter_cb(GtkWidget* button, GdkEvent* event, IDSelectorPrivate* p)
{
    (void)event;
    if (p->buttons[p->index] == button)
        return TRUE;

    return FALSE;
}


static void id_selector_init(IDSelector* self)
{
    IDSelectorPrivate* p = ID_SELECTOR_GET_PRIVATE(self);
    p->self = self;
    p->item_count = 0;
    p->index = 0;
    p->ids_names = 0;
    p->buttons = 0;
}


GtkWidget* id_selector_new(void)
{
    return (GtkWidget*)g_object_new(ID_SELECTOR_TYPE, NULL);
}


void id_selector_set(IDSelector* ids, const id_name* ids_names, int orient)
{
    IDSelectorPrivate* p = ID_SELECTOR_GET_PRIVATE(ids);
    GtkWidget* xbox;
    GtkBox* box;
    int i;

    if (p->item_count)
    {
        debug("IDSelector has been set already.\n");
        return;
    }

    for (i = 0; ids_names[i].name != 0; ++i);

    p->item_count = i;

    p->ids_names = malloc(sizeof(*p->ids_names) * p->item_count);
    p->buttons = malloc(sizeof(GtkWidget*) * p->item_count);

    xbox = (orient == ID_SELECTOR_H)
                ? gtk_hbox_new(FALSE, 0)
                : gtk_vbox_new(FALSE, 0);

    gtk_box_pack_start(GTK_BOX(ids), xbox, FALSE, FALSE, 0);
    gtk_widget_show(xbox);
    box = GTK_BOX(xbox);

    for (i = 0; i < p->item_count; ++i, ++ids_names)
    {
        id_name_init(&p->ids_names[i], ids_names->id, ids_names->name);
        p->buttons[i] =
                gtk_toggle_button_new_with_label(p->ids_names[i].name);
        gtk_box_pack_start(box, p->buttons[i], FALSE, FALSE, 0);
        gtk_widget_show(p->buttons[i]);
        g_signal_connect(G_OBJECT(p->buttons[i]), "button-press-event",
                            G_CALLBACK(press_cb), (gpointer)p);
        g_signal_connect(G_OBJECT(p->buttons[i]), "key-press-event",
                            G_CALLBACK(press_cb), (gpointer)p);
        g_signal_connect(G_OBJECT(p->buttons[i]), "enter-notify-event",
                            G_CALLBACK(enter_cb), (gpointer)p);
    }

    p->index = 0;

    gtk_toggle_button_set_active(
                GTK_TOGGLE_BUTTON(p->buttons[p->index]),
                TRUE);
}


int id_selector_get_id(IDSelector* self)
{
    IDSelectorPrivate* p = ID_SELECTOR_GET_PRIVATE(self);
    return p->ids_names[p->index].id;
}


const char* id_selector_get_name(IDSelector* self)
{
    IDSelectorPrivate* p = ID_SELECTOR_GET_PRIVATE(self);
    return p->ids_names[p->index].name;
}


const char* id_selector_get_name_by_id(IDSelector* self, int id)
{
    int i;
    IDSelectorPrivate* p = ID_SELECTOR_GET_PRIVATE(self);

    for (i = 0; i < p->item_count; ++i)
        if (p->ids_names[i].id == id)
            return p->ids_names[i].name;

    return 0;
}
