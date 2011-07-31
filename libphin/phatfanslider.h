#ifndef __PHAT_FAN_SLIDER_H__
#define __PHAT_FAN_SLIDER_H__

#include <gtk/gtk.h>

G_BEGIN_DECLS

#define PHAT_TYPE_FAN_SLIDER            (phat_fan_slider_get_type ( ))
#define PHAT_FAN_SLIDER(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), PHAT_TYPE_FAN_SLIDER, PhatFanSlider))
#define PHAT_FAN_SLIDER_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), PHAT_TYPE_FAN_SLIDER, PhatFanSliderClass))
#define PHAT_IS_FAN_SLIDER(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), PHAT_TYPE_FAN_SLIDER))
#define PHAT_IS_FAN_SLIDER_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), PHAT_TYPE_FAN_SLIDER))

typedef struct _PhatFanSliderClass PhatFanSliderClass;
typedef struct _PhatFanSlider      PhatFanSlider;


struct _PhatFanSlider
{
    GtkWidget parent;

    GtkAdjustment* adjustment;
    GtkAdjustment* adjustment_prv;
    double         val;
    double         center_val;
    int            xclick_root;
    int            yclick_root;
    int            xclick;
    int            yclick;
    int            fan_max_thickness;
    int            state;
    gboolean       inverted;
    int            direction;
    gboolean       is_log;
    GtkOrientation orientation;
    GtkWidget*     fan_window;
    GdkCursor*     arrow_cursor;
    GdkCursor*     empty_cursor;
    GdkWindow*     event_window;
    GtkWidget*     hint_window0;
    GtkWidget*     hint_window1;
    GdkRectangle   cur_fan;
    gboolean       use_default_value;
    gdouble        default_value;
};


struct _PhatFanSliderClass
{
    GtkWidgetClass parent_class;

    void (*value_changed) (PhatFanSlider* slider);
    void (*changed)       (PhatFanSlider* slider);
};


GType phat_fan_slider_get_type ();

void phat_fan_slider_set_value (PhatFanSlider* slider, double value);

void phat_fan_slider_set_log (PhatFanSlider* slider, gboolean is_log);

gboolean phat_fan_slider_is_log (PhatFanSlider* slider);

double phat_fan_slider_get_value (PhatFanSlider* slider);

void phat_fan_slider_set_range (PhatFanSlider* slider,
                                double lower, double upper);

void phat_fan_slider_get_range (PhatFanSlider* slider,
                                double* lower, double* upper);

void phat_fan_slider_set_adjustment (PhatFanSlider* slider,
                                     GtkAdjustment* adjustment);

GtkAdjustment* phat_fan_slider_get_adjustment (PhatFanSlider* slider);

void phat_fan_slider_set_inverted (PhatFanSlider* slider, gboolean inverted);

gboolean phat_fan_slider_get_inverted (PhatFanSlider* slider);

void phat_fan_slider_set_default_value(PhatFanSlider* slider, gdouble value);

G_END_DECLS

#endif /* __PHAT_FAN_SLIDER_H__ */

