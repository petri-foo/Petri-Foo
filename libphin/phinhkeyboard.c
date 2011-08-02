#include <gtk/gtk.h>
#include "phinhkeyboard.h"

static PhinHKeyboardClass* parent_class;

static void phin_hkeyboard_class_init(PhinHKeyboardClass* klass);
static void phin_hkeyboard_init(PhinHKeyboard* self);


GType phin_hkeyboard_get_type(void)
{
    static GType type = 0;

    if (!type)
    {
        static const GTypeInfo info =
            {
                sizeof (PhinHKeyboardClass),
                NULL,
                NULL,
                (GClassInitFunc) phin_hkeyboard_class_init,
                NULL,
                NULL,
                sizeof (PhinHKeyboard),
                0,
                (GInstanceInitFunc) phin_hkeyboard_init,
                NULL
            };

        type = g_type_register_static(PHIN_TYPE_KEYBOARD, "PhinHKeyboard", &info, 0);
    }

    return type;
}


static void phin_hkeyboard_class_init(PhinHKeyboardClass* klass)
{
    parent_class = g_type_class_peek_parent(klass);
}


static void phin_hkeyboard_init(PhinHKeyboard* self)
{
    (void)self;
}

/**
 * phin_hkeyboard_new:
 * @adjustment: the #GtkAdjustment that the new keyboard will use for scrolling
 * @numkeys: number of keys to create
 * @show_labels: whether to label the C keys
 *
 * Creates a new #PhinHKeyboard.
 *
 * Returns: a newly created #PhinHKeyboard
 * 
 */
GtkWidget* phin_hkeyboard_new(GtkAdjustment* adjustment, int numkeys, gboolean show_labels)
{
    if (!adjustment)
        adjustment = (GtkAdjustment*) gtk_adjustment_new(0, 0, 0, 0, 0, 0);
    
    return g_object_new(PHIN_TYPE_HKEYBOARD,
                        "hadjustment", adjustment,
                        "shadow-type", GTK_SHADOW_NONE,
                        "orientation", GTK_ORIENTATION_HORIZONTAL,
                        "numkeys", numkeys,
                        "show-labels", show_labels,
                        NULL);
}
