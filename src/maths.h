#ifndef __MATHS_H__
#define __MATHS_H__

#include <glib.h>

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
float cerp(float y0, float y1, float y2, float y3, guint8 d);

/* convert a floating-point linear volume value to its logarithmic
 * equivalent */
float log_volume(float x);


#endif /* __MATHS_H__ */
