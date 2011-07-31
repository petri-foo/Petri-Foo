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


#ifndef PRIVATE_PATCH_DEFS_H
#define PRIVATE_PATCH_DEFS_H


#include "patch.h"


/* Don't add #includes in here. */


/* the minimum envelope release value we'll allow (to prevent clicks) 
 *  FIXME: this may become deprecated, we'll see... */
extern const float  PATCH_MIN_RELEASE;


/* how much time to wait before actually releasing legato patches;
 * this is to make sure that noteons immediately following noteoffs
 * stay "connected" */
extern const float  PATCH_LEGATO_LAG;


/* in certain places, we consider values with an absolute value less
 * than or equal to this to be equivalent to zero  - standard stuff */
extern const float  ALMOST_ZERO;


/* what sample rate we think the audio interface is running at */
extern int      patch_samplerate;
extern int      patch_buffersize;


/* how many ticks legato releases lag; calculated to take
 * PATCH_LEGATO_LAG seconds */
extern int      patch_legato_lag;



/* the patches */
extern Patch*   patches[PATCH_COUNT];



#define DEFAULT_FADE_SAMPLES 100


#endif
