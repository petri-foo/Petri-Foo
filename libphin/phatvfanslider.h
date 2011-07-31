#ifndef __PHAT_VFAN_SLIDER_H__
#define __PHAT_VFAN_SLIDER_H__

#include <gtk/gtk.h>
#include <phat/phatfanslider.h>

G_BEGIN_DECLS

#define PHAT_TYPE_VFAN_SLIDER            (phat_vfan_slider_get_type ( ))
#define PHAT_VFAN_SLIDER(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), PHAT_TYPE_VFAN_SLIDER, PhatVFanSlider))
#define PHAT_VFAN_SLIDER_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), PHAT_TYPE_VFAN_SLIDER, PhatVFanSliderClass))
#define PHAT_IS_VFAN_SLIDER(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), PHAT_TYPE_VFAN_SLIDER))
#define PHAT_IS_VFAN_SLIDER_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), PHAT_TYPE_VFAN_SLIDER))

typedef struct _PhatVFanSliderClass PhatVFanSliderClass;
typedef struct _PhatVFanSlider      PhatVFanSlider;

struct _PhatVFanSlider
{
    PhatFanSlider parent;
};

struct _PhatVFanSliderClass
{
    PhatFanSliderClass parent_class;
};

GType phat_vfan_slider_get_type ( );

GtkWidget* phat_vfan_slider_new (GtkAdjustment* adjustment);

GtkWidget* phat_vfan_slider_new_with_range (double value,
                                            double lower,
                                            double upper,
                                            double step);
G_END_DECLS

#endif /* __PHAT_VFAN_SLIDER_H__ */
