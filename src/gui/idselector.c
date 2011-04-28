#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>

#include <stdio.h>

#include <stdlib.h>
#include <string.h>

#include "idselector.h"


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
    int current_id;

    char**      names;
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
                GTK_TOGGLE_BUTTON(p->buttons[p->current_id]), FALSE);
            p->current_id = i;
            gtk_toggle_button_set_active(
                GTK_TOGGLE_BUTTON(p->buttons[p->current_id]), TRUE);
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
    if (p->buttons[p->current_id] == button)
        return TRUE;

    return FALSE;
}


static void id_selector_init(IDSelector* self)
{
    IDSelectorPrivate* p = ID_SELECTOR_GET_PRIVATE(self);
    p->self = self;
    p->item_count = 0;
    p->current_id = 0;
    p->names = 0;
    p->buttons = 0;
}


GtkWidget* id_selector_new(void)
{
    return (GtkWidget*)g_object_new(ID_SELECTOR_TYPE, NULL);
}


void id_selector_set(IDSelector* ids, const char** item_names, int orient)
{
    IDSelectorPrivate* p = ID_SELECTOR_GET_PRIVATE(ids);
    GtkWidget* xbox;
    GtkBox* box;
    int i;

    if (p->item_count)
        return;

    for (i = 0; item_names[i] != 0; ++i);

    p->item_count = i;

    p->names = malloc(sizeof(char*) * p->item_count);
    p->buttons = malloc(sizeof(GtkWidget*) * p->item_count);

    xbox = (orient == ID_SELECTOR_H)
                ? gtk_hbox_new(FALSE, 0)
                : gtk_vbox_new(FALSE, 0);

    gtk_box_pack_start(GTK_BOX(ids), xbox, FALSE, FALSE, 0);
    gtk_widget_show(xbox);
    box = GTK_BOX(xbox);

    for (i = 0; i < p->item_count; ++i)
    {
        p->names[i] = malloc(strlen(item_names[i]) + 1);
        strcpy(p->names[i], item_names[i]);
        p->buttons[i] = gtk_toggle_button_new_with_label(p->names[i]);
        gtk_box_pack_start(box, p->buttons[i], FALSE, FALSE, 0);
        gtk_widget_show(p->buttons[i]);
        g_signal_connect(G_OBJECT(p->buttons[i]), "button-press-event",
                            G_CALLBACK(press_cb), (gpointer)p);
        g_signal_connect(G_OBJECT(p->buttons[i]), "key-press-event",
                            G_CALLBACK(press_cb), (gpointer)p);
        g_signal_connect(G_OBJECT(p->buttons[i]), "enter-notify-event",
                            G_CALLBACK(enter_cb), (gpointer)p);
    }

    p->current_id = 0;

    gtk_toggle_button_set_active(
                GTK_TOGGLE_BUTTON(p->buttons[p->current_id]),
                TRUE);
}


int id_selector_get_id(IDSelector* self)
{
    return ID_SELECTOR_GET_PRIVATE(self)->current_id;
}


const char* id_selector_get_name(IDSelector* self)
{
    IDSelectorPrivate* p = ID_SELECTOR_GET_PRIVATE(self);
    return p->names[p->current_id];
}


const char* id_selector_get_name_by_id(IDSelector* self, int id)
{
    return ID_SELECTOR_GET_PRIVATE(self)->names[id];
}
