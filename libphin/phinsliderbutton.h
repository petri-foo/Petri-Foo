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
#ifndef __PHIN_SLIDER_BUTTON_H__
#define __PHIN_SLIDER_BUTTON_H__

#include <gtk/gtk.h>

G_BEGIN_DECLS

#define PHIN_TYPE_SLIDER_BUTTON         (phin_slider_button_get_type())

#define PHIN_SLIDER_BUTTON(obj)         (G_TYPE_CHECK_INSTANCE_CAST((obj), \
                                        PHIN_TYPE_SLIDER_BUTTON, \
                                        PhinSliderButton))

#define PHIN_SLIDER_BUTTON_CLASS(klass) (G_TYPE_CHECK_CLASS_CAST((klass), \
                                        PHIN_TYPE_SLIDER_BUTTON, \
                                        PhinSliderButtonClass))

#define PHIN_IS_SLIDER_BUTTON(obj)      (G_TYPE_CHECK_INSTANCE_TYPE((obj), \
                                        PHIN_TYPE_SLIDER_BUTTON))

#define PHIN_IS_SLIDER_BUTTON_CLASS(klass)\
    (G_TYPE_CHECK_CLASS_TYPE ((klass),  PHIN_TYPE_SLIDER_BUTTON))


typedef struct _PhinSliderButtonClass PhinSliderButtonClass;
typedef struct _PhinSliderButton      PhinSliderButton;


struct _PhinSliderButton
{
    GtkHBox parent;
};

struct _PhinSliderButtonClass
{
    GtkHBoxClass parent_class;

    void (*value_changed) (PhinSliderButton* slider);
    void (*changed)       (PhinSliderButton* slider);
};


GType       phin_slider_button_get_type ( );
GtkWidget*  phin_slider_button_new (GtkAdjustment*, int digits);
GtkWidget*  phin_slider_button_new_with_range ( double value,
                                                double lower,
                                                double upper,
                                                double step,
                                                int digits);

void    phin_slider_button_set_value(   PhinSliderButton*, double );
double  phin_slider_button_get_value (  PhinSliderButton* );

void    phin_slider_button_set_range (  PhinSliderButton*,
                                        double lower,   double upper);
void    phin_slider_button_get_range (  PhinSliderButton*,
                                        double* lower,  double* upper);

void            phin_slider_button_set_adjustment(  PhinSliderButton*,
                                                    GtkAdjustment* );
GtkAdjustment*  phin_slider_button_get_adjustment (PhinSliderButton* ); 

void    phin_slider_button_set_increment(   PhinSliderButton*,
                                            double step, double page);
void    phin_slider_button_get_increment(   PhinSliderButton*,
                                            double* step, double* page);

void    phin_slider_button_set_format(  PhinSliderButton*,  int digits,
                                        const char* prefix,
                                        const char* postfix);
void    phin_slider_button_get_format(  PhinSliderButton*,  int* digits,
                                        char** prefix,
                                        char** postfix);

void    phin_slider_button_set_threshold(   PhinSliderButton*,
                                            guint threshold);
int     phin_slider_button_get_threshold(   PhinSliderButton*);


G_END_DECLS

#endif /* __PHIN_SLIDER_BUTTON_H__ */
