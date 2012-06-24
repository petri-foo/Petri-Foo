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
/*  Phin is a fork of PHAT

    Original Specimen author Pete Bessman
    Copyright 2005 Pete Bessman
    Copyright 2011 James W. Morris

    This file is part of Phin

    Phin is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License version 2 as
    published by the Free Software Foundation.

    Phin is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with Phin.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef __PHIN_SLIDER_H__
#define __PHIN_SLIDER_H__

#include <gtk/gtk.h>

G_BEGIN_DECLS

#define PHIN_TYPE_SLIDER            (phin_slider_get_type())
#define PHIN_SLIDER(obj) \
    (G_TYPE_CHECK_INSTANCE_CAST((obj),  PHIN_TYPE_SLIDER, \
                                        PhinSlider))

#define PHIN_SLIDER_CLASS(klass)    \
    (G_TYPE_CHECK_CLASS_CAST((klass),   PHIN_TYPE_SLIDER, \
                                        PhinSliderClass))

#define PHIN_IS_SLIDER(obj) \
    (G_TYPE_CHECK_INSTANCE_TYPE((obj),  PHIN_TYPE_SLIDER))

#define PHIN_IS_SLIDER_CLASS(klass) \
    (G_TYPE_CHECK_CLASS_TYPE((klass),   PHIN_TYPE_SLIDER))


typedef struct _PhinSliderClass PhinSliderClass;
typedef struct _PhinSlider      PhinSlider;


struct _PhinSlider
{
    GtkWidget parent;
};


struct _PhinSliderClass
{
    GtkWidgetClass parent_class;
    /* private */
    void (*value_changed) (PhinSlider* slider);
    void (*changed)       (PhinSlider* slider);
};


GType       phin_slider_get_type(   void );
void        phin_slider_set_value(  PhinSlider*, double );
void        phin_slider_set_log(    PhinSlider*, gboolean );
gboolean    phin_slider_is_log(     PhinSlider* );
double      phin_slider_get_value(  PhinSlider* );

void        phin_slider_set_range(  PhinSlider*, double lower,
                                                        double upper );
void        phin_slider_get_range(  PhinSlider*, double* lower,
                                                        double* upper );

void            phin_slider_set_adjustment( PhinSlider*,
                                                GtkAdjustment* );
GtkAdjustment*  phin_slider_get_adjustment( PhinSlider* );

void        phin_slider_set_inverted(   PhinSlider*, gboolean); 
gboolean    phin_slider_get_inverted(   PhinSlider*);

void        phin_slider_set_default_value(  PhinSlider*, gdouble ); 

/* run once */
void        phin_slider_set_orientation(    PhinSlider*,
                                                GtkOrientation );

G_END_DECLS

#endif /* __PHIN_SLIDER_H__ */

