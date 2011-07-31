#ifndef __PHAT_HFAN_SLIDER_H__
#define __PHAT_HFAN_SLIDER_H__

#include <gtk/gtk.h>
#include <phat/phatfanslider.h>

G_BEGIN_DECLS

#define PHAT_TYPE_HFAN_SLIDER            (phat_hfan_slider_get_type ( ))
#define PHAT_HFAN_SLIDER(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), PHAT_TYPE_HFAN_SLIDER, PhatHFanSlider))
#define PHAT_HFAN_SLIDER_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), PHAT_TYPE_HFAN_SLIDER, PhatHFanSliderClass))
#define PHAT_IS_HFAN_SLIDER(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), PHAT_TYPE_HFAN_SLIDER))
#define PHAT_IS_HFAN_SLIDER_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), PHAT_TYPE_HFAN_SLIDER))

typedef struct _PhatHFanSliderClass PhatHFanSliderClass;
typedef struct _PhatHFanSlider      PhatHFanSlider;

struct _PhatHFanSlider
{
    PhatFanSlider parent;
};

struct _PhatHFanSliderClass
{
    PhatFanSliderClass parent_class;
};

GType phat_hfan_slider_get_type ( );

GtkWidget* phat_hfan_slider_new (GtkAdjustment* adjustment);

GtkWidget* phat_hfan_slider_new_with_range (double value,
                                            double lower,
                                            double upper,
                                            double step);
G_END_DECLS

#endif /* __PHAT_HFAN_SLIDER_H__ */
