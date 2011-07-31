#include <gtk/gtk.h>
#include "phatprivate.h"
#include "phatfanslider.h"
#include "phathfanslider.h"

static PhatFanSliderClass* parent_class;

static void phat_hfan_slider_class_init (PhatHFanSliderClass* klass);
static void phat_hfan_slider_init (PhatHFanSlider* slider);
static void phat_hfan_slider_destroy (GtkObject* object);

GType phat_hfan_slider_get_type ( )
{
    static GType type = 0;

    if (!type)
    {
        static const GTypeInfo info =
            {
                sizeof (PhatHFanSliderClass),
                NULL,
                NULL,
                (GClassInitFunc) phat_hfan_slider_class_init,
                NULL,
                NULL,
                sizeof (PhatHFanSlider),
                0,
                (GInstanceInitFunc) phat_hfan_slider_init,
                NULL
            };

        type = g_type_register_static (PHAT_TYPE_FAN_SLIDER,
                                       "PhatHFanSlider",
                                       &info,
                                       0);
    }

    return type;
}

/**
 * phat_hfan_slider_new:
 * @adjustment: the #GtkAdjustment that the new slider will use
 *
 * Creates a new #PhatHFanSlider.
 *
 * Returns: a newly created #PhatHFanSlider
 * 
 */
GtkWidget* phat_hfan_slider_new (GtkAdjustment* adjustment)
{
    PhatHFanSlider* slider;

    gdouble adj_lower = gtk_adjustment_get_lower(adjustment);
    gdouble adj_upper = gtk_adjustment_get_upper(adjustment);
    gdouble adj_value = gtk_adjustment_get_value(adjustment);

    g_assert (adj_lower < adj_upper);
    g_assert ((adj_value >= adj_lower)
              && (adj_value <= adj_upper));

    slider = g_object_new (PHAT_TYPE_HFAN_SLIDER, NULL);

    PHAT_FAN_SLIDER (slider)->orientation = GTK_ORIENTATION_HORIZONTAL;

    phat_fan_slider_set_adjustment (PHAT_FAN_SLIDER (slider), adjustment);

    return (GtkWidget*) slider;
}
     
/**
 * phat_hfan_slider_new_with_range:
 * @value: the initial value the new slider should have
 * @lower: the lowest value the new slider will allow
 * @upper: the highest value the new slider will allow
 * @step: increment added or subtracted when sliding
 * 
 * Creates a new #PhatHFanSlider.  The slider will create a new #GtkAdjustment
 * from @value, @lower, and @upper.  If these parameters represent a bogus
 * configuration, the program will terminate.
 *
 * Returns: a newly created #PhatHFanSlider
 *
 */
GtkWidget* phat_hfan_slider_new_with_range (double value, double lower,
                                            double upper, double step)
{
    GtkAdjustment* adj;

    adj = (GtkAdjustment*) gtk_adjustment_new (value, lower, upper, step, step, 0);
     
    return phat_hfan_slider_new (adj);
}

static void phat_hfan_slider_class_init (PhatHFanSliderClass* klass)
{
    GtkObjectClass* object_class = (GtkObjectClass*) klass;

    parent_class = g_type_class_peek (PHAT_TYPE_FAN_SLIDER);

    object_class->destroy = phat_hfan_slider_destroy;
}

static void phat_hfan_slider_init (PhatHFanSlider* slider)
{
    return;
}

static void phat_hfan_slider_destroy (GtkObject* object)
{
    GtkObjectClass* klass;
     
    g_return_if_fail (object != NULL);
    g_return_if_fail (PHAT_IS_HFAN_SLIDER (object));

    klass = GTK_OBJECT_CLASS (parent_class);

    if (klass->destroy)
        klass->destroy (object);
}
