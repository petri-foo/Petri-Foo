#include <gtk/gtk.h>
#include "phathkeyboard.h"

static PhatHKeyboardClass* parent_class;

static void phat_hkeyboard_class_init(PhatHKeyboardClass* klass);
static void phat_hkeyboard_init(PhatHKeyboard* self);


GType phat_hkeyboard_get_type(void)
{
    static GType type = 0;

    if (!type)
    {
        static const GTypeInfo info =
            {
                sizeof (PhatHKeyboardClass),
                NULL,
                NULL,
                (GClassInitFunc) phat_hkeyboard_class_init,
                NULL,
                NULL,
                sizeof (PhatHKeyboard),
                0,
                (GInstanceInitFunc) phat_hkeyboard_init,
                NULL
            };

        type = g_type_register_static(PHAT_TYPE_KEYBOARD, "PhatHKeyboard", &info, 0);
    }

    return type;
}


static void phat_hkeyboard_class_init(PhatHKeyboardClass* klass)
{
    parent_class = g_type_class_peek_parent(klass);
}


static void phat_hkeyboard_init(PhatHKeyboard* self)
{
    (void)self;
}

/**
 * phat_hkeyboard_new:
 * @adjustment: the #GtkAdjustment that the new keyboard will use for scrolling
 * @numkeys: number of keys to create
 * @show_labels: whether to label the C keys
 *
 * Creates a new #PhatHKeyboard.
 *
 * Returns: a newly created #PhatHKeyboard
 * 
 */
GtkWidget* phat_hkeyboard_new(GtkAdjustment* adjustment, int numkeys, gboolean show_labels)
{
    if (!adjustment)
        adjustment = (GtkAdjustment*) gtk_adjustment_new(0, 0, 0, 0, 0, 0);
    
    return g_object_new(PHAT_TYPE_HKEYBOARD,
                        "hadjustment", adjustment,
                        "shadow-type", GTK_SHADOW_NONE,
                        "orientation", GTK_ORIENTATION_HORIZONTAL,
                        "numkeys", numkeys,
                        "show-labels", show_labels,
                        NULL);
}
