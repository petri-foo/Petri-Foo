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
static GtkHBoxClass* parent_class;

static void id_selector_class_init(IDSelectorClass* klass);
static void id_selector_init(IDSelector* self);


GType id_selector_get_type(void)
{
    static GType type = 0;

    if (!type)
    {
        static const GTypeInfo info =
            {
                sizeof (IDSelectorClass),
                NULL,
                NULL,
                (GClassInitFunc) id_selector_class_init,
                NULL,
                NULL,
                sizeof (IDSelector),
                0,
                (GInstanceInitFunc) id_selector_init,
                NULL
            };

        type = g_type_register_static(GTK_TYPE_HBOX,
                                        "IDSelector", &info, 0);
    }

    return type;
}


static void id_selector_class_init(IDSelectorClass* klass)
{
    parent_class = g_type_class_peek_parent(klass);

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
press_cb(GtkToggleButton* button, GdkEvent* event, IDSelector* self)
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
    for (i = 0; i < self->item_count; ++i)
    {
        if (GTK_WIDGET(button) == self->buttons[i])
        {
            gtk_toggle_button_set_active(
                GTK_TOGGLE_BUTTON(self->buttons[self->current_id]), FALSE);
            self->current_id = i;
            gtk_toggle_button_set_active(
                GTK_TOGGLE_BUTTON(self->buttons[self->current_id]), TRUE);
            g_signal_emit_by_name(G_OBJECT(self), "changed");
            return TRUE;
        }
    }

    return FALSE;
}


static gboolean
enter_cb(GtkWidget* button, GdkEvent* event, IDSelector* self)
{
    (void)event;
    if (self->buttons[self->current_id] == button)
        return TRUE;

    return FALSE;
}


static void id_selector_init(IDSelector* self)
{
    self->item_count = 0;
    self->current_id = 0;
    self->names = 0;
    self->buttons = 0;
}


GtkWidget* id_selector_new(void)
{
    return (GtkWidget*)g_object_new(ID_SELECTOR_TYPE, NULL);
}


void id_selector_set(IDSelector* ids, const char** item_names, int orient)
{
    GtkWidget* xbox;
    GtkBox* box;
    int i;

    if (ids->item_count)
        return;

    for (i = 0; item_names[i] != 0; ++i);

    ids->item_count = i;

    ids->names = malloc(sizeof(char*) * ids->item_count);
    ids->buttons = malloc(sizeof(GtkWidget*) * ids->item_count);

    xbox = (orient == ID_SELECTOR_H)
                ? gtk_hbox_new(FALSE, 0)
                : gtk_vbox_new(FALSE, 0);

    gtk_box_pack_start(GTK_BOX(ids), xbox, TRUE, FALSE, 0);
    gtk_widget_show(xbox);
    box = GTK_BOX(xbox);

    for (i = 0; i < ids->item_count; ++i)
    {
        ids->names[i] = malloc(strlen(item_names[i]) + 1);
        strcpy(ids->names[i], item_names[i]);
        ids->buttons[i] = gtk_toggle_button_new_with_label(ids->names[i]);
        gtk_box_pack_start(box, ids->buttons[i], TRUE, TRUE, 0);
        gtk_widget_show(ids->buttons[i]);
        g_signal_connect(G_OBJECT(ids->buttons[i]), "button-press-event",
                            G_CALLBACK(press_cb), (gpointer)ids);
        g_signal_connect(G_OBJECT(ids->buttons[i]), "key-press-event",
                            G_CALLBACK(press_cb), (gpointer)ids);
        g_signal_connect(G_OBJECT(ids->buttons[i]), "enter-notify-event",
                            G_CALLBACK(enter_cb), (gpointer)ids);
    }

    ids->current_id = 0;

    gtk_toggle_button_set_active(
                GTK_TOGGLE_BUTTON(ids->buttons[ids->current_id]),
                TRUE);
}


int id_selector_get_id(IDSelector* self)
{
    return self->current_id;
}


const char* id_selector_get_name(IDSelector* self)
{
    return self->names[self->current_id];
}


const char* id_selector_get_name_by_id(IDSelector* self, int id)
{
    return self->names[id];
}
