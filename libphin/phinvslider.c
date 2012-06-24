/*  Phin is a fork of the PHAT Audio Toolkit.
    Phin is part of Petri-Foo. Petri-Foo is a fork of Specimen.

    Original author Pete Bessman
    Copyright 2005 Pete Bessman
    Copyright 2011 James W. Morris

    This file is part of Phin.

    Phin is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License version 2 as
    published by the Free Software Foundation.

    Phin is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with Phin.  If not, see <http://www.gnu.org/licenses/>.

    This file is a derivative of a PHAT original, modified 2011
*/
#include <gtk/gtk.h>


#include "phinprivate.h"
#include "phinslider.h"
#include "phinvslider.h"


G_DEFINE_TYPE(PhinVSlider, phin_vslider, PHIN_TYPE_SLIDER);


GtkWidget* phin_vslider_new (GtkAdjustment* adjustment)
{
    PhinVSlider* slider;

    int adj_lower = gtk_adjustment_get_lower(adjustment);
    int adj_upper = gtk_adjustment_get_upper(adjustment);
    int adj_value = gtk_adjustment_get_value(adjustment);

    g_assert (adj_lower < adj_upper);
    g_assert((adj_value >= adj_lower)
          && (adj_value <= adj_upper));

    slider = g_object_new (PHIN_TYPE_VSLIDER, NULL);

    phin_slider_set_orientation(PHIN_SLIDER(slider),
                                    GTK_ORIENTATION_VERTICAL);

    phin_slider_set_adjustment (PHIN_SLIDER (slider), adjustment);

    return (GtkWidget*) slider;
}


GtkWidget* phin_vslider_new_with_range (double value, double lower,
                                            double upper, double step)
{
    GtkAdjustment* adj;

    adj = (GtkAdjustment*)gtk_adjustment_new (value, lower, upper,
                                                    step, step, 0);
    return phin_vslider_new (adj);
}


static void phin_vslider_class_init (PhinVSliderClass* klass)
{
    phin_vslider_parent_class = g_type_class_peek_parent(klass);
}


static void phin_vslider_init(PhinVSlider* slider)
{
    (void)slider;
    return;
}
