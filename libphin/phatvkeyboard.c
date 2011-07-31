#include <gtk/gtk.h>
#include "phatvkeyboard.h"

static PhatVKeyboardClass* parent_class;

static void phat_vkeyboard_class_init(PhatVKeyboardClass* klass);
static void phat_vkeyboard_init(PhatVKeyboard* self);


GType phat_vkeyboard_get_type(void)
{
    static GType type = 0;

    if (!type)
    {
        static const GTypeInfo info =
            {
                sizeof (PhatVKeyboardClass),
                NULL,
                NULL,
                (GClassInitFunc) phat_vkeyboard_class_init,
                NULL,
                NULL,
                sizeof (PhatVKeyboard),
                0,
                (GInstanceInitFunc) phat_vkeyboard_init,
            };

        type = g_type_register_static(PHAT_TYPE_KEYBOARD, "PhatVKeyboard", &info, 0);
    }

    return type;
}


static void phat_vkeyboard_class_init(PhatVKeyboardClass* klass)
{
    parent_class = g_type_class_peek_parent(klass);
}


static void phat_vkeyboard_init(PhatVKeyboard* self)
{
    /* nada */
}

/**
 * phat_vkeyboard_new:
 * @adjustment: the #GtkAdjustment that the new keyboard will use for scrolling
 * @numkeys: number of keys to create
 * @show_labels: whether to label the C keys
 *
 * Creates a new #PhatVKeyboard.
 *
 * Returns: a newly created #PhatVKeyboard
 * 
 */
GtkWidget* phat_vkeyboard_new(GtkAdjustment* adjustment, int numkeys, gboolean show_labels)
{
    if (!adjustment)
        adjustment = (GtkAdjustment*) gtk_adjustment_new(0, 0, 0, 0, 0, 0);
    
    return g_object_new(PHAT_TYPE_VKEYBOARD,
                        "vadjustment", adjustment,
                        "shadow-type", GTK_SHADOW_NONE,
                        "orientation", GTK_ORIENTATION_VERTICAL,
                        "numkeys", numkeys,
                        "show-labels", show_labels,
                        NULL);
}
