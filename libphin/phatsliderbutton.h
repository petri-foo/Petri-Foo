#ifndef __PHAT_SLIDER_BUTTON_H__
#define __PHAT_SLIDER_BUTTON_H__

#include <gtk/gtk.h>

G_BEGIN_DECLS

#define PHAT_TYPE_SLIDER_BUTTON            (phat_slider_button_get_type ( ))
#define PHAT_SLIDER_BUTTON(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), PHAT_TYPE_SLIDER_BUTTON, PhatSliderButton))
#define PHAT_SLIDER_BUTTON_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), PHAT_TYPE_SLIDER_BUTTON, PhatSliderButtonClass))
#define PHAT_IS_SLIDER_BUTTON(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), PHAT_TYPE_SLIDER_BUTTON))
#define PHAT_IS_SLIDER_BUTTON_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), PHAT_TYPE_SLIDER_BUTTON))

typedef struct _PhatSliderButtonClass PhatSliderButtonClass;
typedef struct _PhatSliderButton      PhatSliderButton;

struct _PhatSliderButton
{
    GtkHBox parent;

    GdkCursor* arrow_cursor;
    GdkCursor* empty_cursor;
    GdkWindow* event_window;
    GtkWidget* left_arrow;
    GtkWidget* right_arrow;
    GtkWidget* label;
    GtkWidget* prefix_label;
    GtkWidget* postfix_label;
    GtkWidget* entry;
    GtkAdjustment* adjustment;
    char* prefix;
    char* postfix;
    int digits;
    int hilite;
    int state;
    int xpress_root, ypress_root;
    int xpress, ypress;
    int firstrun;
    guint threshold;
    gboolean slid;
};

struct _PhatSliderButtonClass
{
    GtkHBoxClass parent_class;

    void (*value_changed) (PhatSliderButton* slider);
    void (*changed)       (PhatSliderButton* slider);
};

GType phat_slider_button_get_type ( );

GtkWidget* phat_slider_button_new (GtkAdjustment* adjustment,
                                   int digits);

GtkWidget* phat_slider_button_new_with_range (double value,
                                              double lower,
                                              double upper,
                                              double step,
                                              int digits);

void phat_slider_button_set_value (PhatSliderButton* button, double value);

double phat_slider_button_get_value (PhatSliderButton* button);

void phat_slider_button_set_range (PhatSliderButton* button,
                                   double lower, double upper);

void phat_slider_button_get_range (PhatSliderButton* button,
                                   double* lower, double* upper);

void phat_slider_button_set_adjustment (PhatSliderButton* button,
                                        GtkAdjustment* adjustment);

GtkAdjustment* phat_slider_button_get_adjustment (PhatSliderButton* button);

void phat_slider_button_set_increment (PhatSliderButton* button,
                                       double step, double page);

void phat_slider_button_get_increment (PhatSliderButton* button,
                                       double* step, double* page);

void phat_slider_button_set_format (PhatSliderButton* button,
                                    int digits,
                                    const char* prefix,
                                    const char* postfix);

void phat_slider_button_get_format (PhatSliderButton* button,
                                    int* digits,
                                    char** prefix,
                                    char** postfix);

void phat_slider_button_set_threshold (PhatSliderButton* button,
                                       guint threshold);

int phat_slider_button_get_threshold (PhatSliderButton* button);

G_END_DECLS

#endif /* __PHAT_SLIDER_BUTTON_H__ */
