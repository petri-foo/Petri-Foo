#include <gtk/gtk.h>


#include "phinprivate.h"
#include "phinfanslider.h"
#include "phinvfanslider.h"


G_DEFINE_TYPE(PhinVFanSlider, phin_vfan_slider, PHIN_TYPE_FAN_SLIDER);


GtkWidget* phin_vfan_slider_new (GtkAdjustment* adjustment)
{
    PhinVFanSlider* slider;

    int adj_lower = gtk_adjustment_get_lower(adjustment);
    int adj_upper = gtk_adjustment_get_upper(adjustment);
    int adj_value = gtk_adjustment_get_value(adjustment);

    g_assert (adj_lower < adj_upper);
    g_assert((adj_value >= adj_lower)
          && (adj_value <= adj_upper));

    slider = g_object_new (PHIN_TYPE_VFAN_SLIDER, NULL);

    phin_fan_slider_set_orientation(PHIN_FAN_SLIDER(slider),
                                    GTK_ORIENTATION_VERTICAL);

    phin_fan_slider_set_adjustment (PHIN_FAN_SLIDER (slider), adjustment);

    return (GtkWidget*) slider;
}


GtkWidget* phin_vfan_slider_new_with_range (double value, double lower,
                                            double upper, double step)
{
    GtkAdjustment* adj;

    adj = (GtkAdjustment*)gtk_adjustment_new (value, lower, upper,
                                                    step, step, 0);
    return phin_vfan_slider_new (adj);
}


static void phin_vfan_slider_class_init (PhinVFanSliderClass* klass)
{
    phin_vfan_slider_parent_class = g_type_class_peek_parent(klass);
}


static void phin_vfan_slider_init(PhinVFanSlider* slider)
{
    (void)slider;
    return;
}
