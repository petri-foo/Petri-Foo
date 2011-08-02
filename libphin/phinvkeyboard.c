#include <gtk/gtk.h>
#include "phinvkeyboard.h"

static PhinVKeyboardClass* parent_class;

static void phin_vkeyboard_class_init(PhinVKeyboardClass* klass);
static void phin_vkeyboard_init(PhinVKeyboard* self);


GType phin_vkeyboard_get_type(void)
{
    static GType type = 0;

    if (!type)
    {
        static const GTypeInfo info =
            {
                sizeof (PhinVKeyboardClass),
                NULL,
                NULL,
                (GClassInitFunc) phin_vkeyboard_class_init,
                NULL,
                NULL,
                sizeof (PhinVKeyboard),
                0,
                (GInstanceInitFunc) phin_vkeyboard_init,
            };

        type = g_type_register_static(PHIN_TYPE_KEYBOARD, "PhinVKeyboard", &info, 0);
    }

    return type;
}


static void phin_vkeyboard_class_init(PhinVKeyboardClass* klass)
{
    parent_class = g_type_class_peek_parent(klass);
}


static void phin_vkeyboard_init(PhinVKeyboard* self)
{
    /* nada */
}

/**
 * phin_vkeyboard_new:
 * @adjustment: the #GtkAdjustment that the new keyboard will use for scrolling
 * @numkeys: number of keys to create
 * @show_labels: whether to label the C keys
 *
 * Creates a new #PhinVKeyboard.
 *
 * Returns: a newly created #PhinVKeyboard
 * 
 */
GtkWidget* phin_vkeyboard_new(GtkAdjustment* adjustment, int numkeys, gboolean show_labels)
{
    if (!adjustment)
        adjustment = (GtkAdjustment*) gtk_adjustment_new(0, 0, 0, 0, 0, 0);
    
    return g_object_new(PHIN_TYPE_VKEYBOARD,
                        "vadjustment", adjustment,
                        "shadow-type", GTK_SHADOW_NONE,
                        "orientation", GTK_ORIENTATION_VERTICAL,
                        "numkeys", numkeys,
                        "show-labels", show_labels,
                        NULL);
}
