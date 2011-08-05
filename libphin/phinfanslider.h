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

#ifndef __PHIN_FAN_SLIDER_H__
#define __PHIN_FAN_SLIDER_H__

#include <gtk/gtk.h>

G_BEGIN_DECLS

#define PHIN_TYPE_FAN_SLIDER            (phin_fan_slider_get_type())
#define PHIN_FAN_SLIDER(obj) \
    (G_TYPE_CHECK_INSTANCE_CAST((obj),  PHIN_TYPE_FAN_SLIDER, \
                                        PhinFanSlider))

#define PHIN_FAN_SLIDER_CLASS(klass)    \
    (G_TYPE_CHECK_CLASS_CAST((klass),   PHIN_TYPE_FAN_SLIDER, \
                                        PhinFanSliderClass))

#define PHIN_IS_FAN_SLIDER(obj) \
    (G_TYPE_CHECK_INSTANCE_TYPE((obj),  PHIN_TYPE_FAN_SLIDER))

#define PHIN_IS_FAN_SLIDER_CLASS(klass) \
    (G_TYPE_CHECK_CLASS_TYPE((klass),   PHIN_TYPE_FAN_SLIDER))


typedef struct _PhinFanSliderClass PhinFanSliderClass;
typedef struct _PhinFanSlider      PhinFanSlider;


struct _PhinFanSlider
{
    GtkWidget parent;
};


struct _PhinFanSliderClass
{
    GtkWidgetClass parent_class;
    /* private */
    void (*value_changed) (PhinFanSlider* slider);
    void (*changed)       (PhinFanSlider* slider);
};

/* whether to use fans on the sliders or not (requires compositing).*/
void        phin_fan_slider_set_fans_active(gboolean enable_fans);
gboolean    phin_fan_slider_get_fans_active(void);


GType       phin_fan_slider_get_type(   void );
void        phin_fan_slider_set_value(  PhinFanSlider*, double );
void        phin_fan_slider_set_log(    PhinFanSlider*, gboolean );
gboolean    phin_fan_slider_is_log(     PhinFanSlider* );
double      phin_fan_slider_get_value(  PhinFanSlider* );

void        phin_fan_slider_set_range(  PhinFanSlider*, double lower,
                                                        double upper );
void        phin_fan_slider_get_range(  PhinFanSlider*, double* lower,
                                                        double* upper );

void            phin_fan_slider_set_adjustment( PhinFanSlider*,
                                                GtkAdjustment* );
GtkAdjustment*  phin_fan_slider_get_adjustment( PhinFanSlider* );

void        phin_fan_slider_set_inverted(   PhinFanSlider*, gboolean); 
gboolean    phin_fan_slider_get_inverted(   PhinFanSlider*);

void        phin_fan_slider_set_default_value(  PhinFanSlider*, gdouble ); 

/* run once */
void        phin_fan_slider_set_orientation(    PhinFanSlider*,
                                                GtkOrientation );

G_END_DECLS

#endif /* __PHIN_FAN_SLIDER_H__ */

