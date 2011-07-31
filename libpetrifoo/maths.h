/*  Petri-Foo is a fork of the Specimen audio sampler.

    Original Specimen author Pete Bessman
    Copyright 2005 Pete Bessman
    Copyright 2011 James W. Morris

    This file is part of Petri-Foo.

    Petri-Foo is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License version 2 as
    published by the Free Software Foundation.

    Petri-Foo is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with Petri-Foo.  If not, see <http://www.gnu.org/licenses/>.

    This file is a derivative of a Specimen original, modified 2011
*/


#ifndef __MATHS_H__
#define __MATHS_H__

#include <stdint.h>

/* old fashioned linear interpolator */
inline static float lerp(float y0, float y1, float d)
{
     return y0 * (1 - d) + y1 * d;
}

/* return the distance between y0 and y1 given some value y */
inline static float delerp(float y0, float y1, float y)
{
     return (y - y0) / (y1 - y);
}

/* cubic interpolation routine:
   y0 = sample previous to current sample
   y1 = current sample
   y2 = sample after current sample
   y3 = sample after y2
   x  = fractional distance between y1 and y2 in unsigned integer form */
float cerp(float y0, float y1, float y2, float y3, uint8_t d);

/* convert a floating-point linear amplitude value to its logarithmic
 * equivalent */
float log_amplitude(float x);


#endif /* __MATHS_H__ */
