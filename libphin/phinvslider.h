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
#ifndef __PHIN_VSLIDER_H__
#define __PHIN_VSLIDER_H__

#include <gtk/gtk.h>

#include "phinslider.h"


G_BEGIN_DECLS

#define PHIN_TYPE_VSLIDER           (phin_vslider_get_type())

#define PHIN_VSLIDER(obj)           (G_TYPE_CHECK_INSTANCE_CAST((obj), \
                                        PHIN_TYPE_VSLIDER, \
                                        PhinVSlider))

#define PHIN_VSLIDER_CLASS(klass)   (G_TYPE_CHECK_CLASS_CAST((klass), \
                                        PHIN_TYPE_VSLIDER, \
                                        PhinVSliderClass))

#define PHIN_IS_VSLIDER(obj)        (G_TYPE_CHECK_INSTANCE_TYPE((obj), \
                                        PHIN_TYPE_VSLIDER))

#define PHIN_IS_VSLIDER_CLASS(klass) \
    (G_TYPE_CHECK_CLASS_TYPE ((klass),  PHIN_TYPE_VSLIDER))


typedef struct _PhinVSliderClass PhinVSliderClass;
typedef struct _PhinVSlider      PhinVSlider;


struct _PhinVSlider
{
    PhinSlider parent;
};

struct _PhinVSliderClass
{
    PhinSliderClass parent_class;
};


GType       phin_vslider_get_type(void);
GtkWidget*  phin_vslider_new (GtkAdjustment*);
GtkWidget*  phin_vslider_new_with_range(double value,
                                            double lower,
                                            double upper,
                                            double step);

G_END_DECLS

#endif /* __PHIN_VSLIDER_H__ */
