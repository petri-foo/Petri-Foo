#ifndef __PHIN_HFAN_SLIDER_H__
#define __PHIN_HFAN_SLIDER_H__

#include <gtk/gtk.h>

#include "phinfanslider.h"


G_BEGIN_DECLS

#define PHIN_TYPE_HFAN_SLIDER \
    (phin_hfan_slider_get_type())

#define PHIN_HFAN_SLIDER(obj) \
    (G_TYPE_CHECK_INSTANCE_CAST((obj),  PHIN_TYPE_HFAN_SLIDER, \
                                        PhinHFanSlider))

#define PHIN_IS_HFAN_SLIDER(obj) \
    (G_TYPE_CHECK_INSTANCE_TYPE((obj),  PHIN_TYPE_HFAN_SLIDER))


#define PHIN_HFAN_SLIDER_CLASS(klass) \
    (G_TYPE_CHECK_CLASS_CAST ((klass),  PHIN_TYPE_HFAN_SLIDER, \
                                        PhinHFanSliderClass))

#define PHIN_IS_HFAN_SLIDER_CLASS(klass) \
    (G_TYPE_CHECK_INSTANCE_TYPE ((klass), PHIN_TYPE_HFAN_SLIDER))


typedef struct _PhinHFanSliderClass PhinHFanSliderClass;
typedef struct _PhinHFanSlider      PhinHFanSlider;

struct _PhinHFanSlider
{
    PhinFanSlider parent;
};

struct _PhinHFanSliderClass
{
    PhinFanSliderClass parent_class;
};

GType phin_hfan_slider_get_type ( );

GtkWidget* phin_hfan_slider_new (GtkAdjustment* adjustment);

GtkWidget* phin_hfan_slider_new_with_range (double value,
                                            double lower,
                                            double upper,
                                            double step);
G_END_DECLS

#endif /* __PHIN_HFAN_SLIDER_H__ */
