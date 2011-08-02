#ifndef __PHIN_VFAN_SLIDER_H__
#define __PHIN_VFAN_SLIDER_H__

#include <gtk/gtk.h>

#include "phinfanslider.h"


G_BEGIN_DECLS

#define PHIN_TYPE_VFAN_SLIDER            (phin_vfan_slider_get_type ( ))
#define PHIN_VFAN_SLIDER(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), PHIN_TYPE_VFAN_SLIDER, PhinVFanSlider))
#define PHIN_VFAN_SLIDER_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), PHIN_TYPE_VFAN_SLIDER, PhinVFanSliderClass))
#define PHIN_IS_VFAN_SLIDER(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), PHIN_TYPE_VFAN_SLIDER))
#define PHIN_IS_VFAN_SLIDER_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), PHIN_TYPE_VFAN_SLIDER))

typedef struct _PhinVFanSliderClass PhinVFanSliderClass;
typedef struct _PhinVFanSlider      PhinVFanSlider;

struct _PhinVFanSlider
{
    PhinFanSlider parent;
};

struct _PhinVFanSliderClass
{
    PhinFanSliderClass parent_class;
};

GType phin_vfan_slider_get_type ( );

GtkWidget* phin_vfan_slider_new (GtkAdjustment* adjustment);

GtkWidget* phin_vfan_slider_new_with_range (double value,
                                            double lower,
                                            double upper,
                                            double step);
G_END_DECLS

#endif /* __PHIN_VFAN_SLIDER_H__ */
