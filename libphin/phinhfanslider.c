#include <gtk/gtk.h>


#include "phinprivate.h"
#include "phinfanslider.h"
#include "phinhfanslider.h"


G_DEFINE_TYPE(PhinHFanSlider, phin_hfan_slider, PHIN_TYPE_FAN_SLIDER);


GtkWidget* phin_hfan_slider_new (GtkAdjustment* adjustment)
{
    PhinHFanSlider* slider;

    gdouble adj_lower = gtk_adjustment_get_lower(adjustment);
    gdouble adj_upper = gtk_adjustment_get_upper(adjustment);
    gdouble adj_value = gtk_adjustment_get_value(adjustment);

    g_assert (adj_lower < adj_upper);
    g_assert((adj_value >= adj_lower)
          && (adj_value <= adj_upper));

    slider = g_object_new (PHIN_TYPE_HFAN_SLIDER, NULL);

    phin_fan_slider_set_orientation(PHIN_FAN_SLIDER(slider),
                                    GTK_ORIENTATION_HORIZONTAL);

    phin_fan_slider_set_adjustment( PHIN_FAN_SLIDER (slider), adjustment);

    return (GtkWidget*) slider;
}


GtkWidget* phin_hfan_slider_new_with_range (double value, double lower,
                                            double upper, double step)
{
    GtkAdjustment* adj;

    adj = (GtkAdjustment*)gtk_adjustment_new(value, lower, upper,
                                                    step, step, 0);
    return phin_hfan_slider_new (adj);
}


static void phin_hfan_slider_class_init(PhinHFanSliderClass* klass)
{
    phin_hfan_slider_parent_class = g_type_class_peek_parent(klass);
}


static void phin_hfan_slider_init(PhinHFanSlider* slider)
{
    (void)slider;
    return;
}
