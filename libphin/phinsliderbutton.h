#ifndef __PHIN_SLIDER_BUTTON_H__
#define __PHIN_SLIDER_BUTTON_H__

#include <gtk/gtk.h>

G_BEGIN_DECLS

#define PHIN_TYPE_SLIDER_BUTTON            (phin_slider_button_get_type ( ))
#define PHIN_SLIDER_BUTTON(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), PHIN_TYPE_SLIDER_BUTTON, PhinSliderButton))
#define PHIN_SLIDER_BUTTON_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), PHIN_TYPE_SLIDER_BUTTON, PhinSliderButtonClass))
#define PHIN_IS_SLIDER_BUTTON(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), PHIN_TYPE_SLIDER_BUTTON))
#define PHIN_IS_SLIDER_BUTTON_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), PHIN_TYPE_SLIDER_BUTTON))

typedef struct _PhinSliderButtonClass PhinSliderButtonClass;
typedef struct _PhinSliderButton      PhinSliderButton;

struct _PhinSliderButton
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

struct _PhinSliderButtonClass
{
    GtkHBoxClass parent_class;

    void (*value_changed) (PhinSliderButton* slider);
    void (*changed)       (PhinSliderButton* slider);
};

GType phin_slider_button_get_type ( );

GtkWidget* phin_slider_button_new (GtkAdjustment* adjustment,
                                   int digits);

GtkWidget* phin_slider_button_new_with_range (double value,
                                              double lower,
                                              double upper,
                                              double step,
                                              int digits);

void phin_slider_button_set_value (PhinSliderButton* button, double value);

double phin_slider_button_get_value (PhinSliderButton* button);

void phin_slider_button_set_range (PhinSliderButton* button,
                                   double lower, double upper);

void phin_slider_button_get_range (PhinSliderButton* button,
                                   double* lower, double* upper);

void phin_slider_button_set_adjustment (PhinSliderButton* button,
                                        GtkAdjustment* adjustment);

GtkAdjustment* phin_slider_button_get_adjustment (PhinSliderButton* button);

void phin_slider_button_set_increment (PhinSliderButton* button,
                                       double step, double page);

void phin_slider_button_get_increment (PhinSliderButton* button,
                                       double* step, double* page);

void phin_slider_button_set_format (PhinSliderButton* button,
                                    int digits,
                                    const char* prefix,
                                    const char* postfix);

void phin_slider_button_get_format (PhinSliderButton* button,
                                    int* digits,
                                    char** prefix,
                                    char** postfix);

void phin_slider_button_set_threshold (PhinSliderButton* button,
                                       guint threshold);

int phin_slider_button_get_threshold (PhinSliderButton* button);

G_END_DECLS

#endif /* __PHIN_SLIDER_BUTTON_H__ */
