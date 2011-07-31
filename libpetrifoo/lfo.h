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


#ifndef __LFO_H__
#define __LFO_H__


/*
 * This class implements a LUT-based LFO with various waveforms and
 * linear interpolation.
 *
 * It uses 32-bit fixed-point phase and increment, where the 8 MSB
 * represent the integer part of the number and the 24 LSB the
 * fractionnal part
 *
 * Originally written Remy Muller on 2003-08-22
 *
 * Adapted for C and Specimen by that incorrigible wannabe
 * cyber-ninja, Pete Bessman, on 2004-06-03.
 * modified by James W. Morris 2011
 */


#include <stdbool.h>
#include "ticks.h"


typedef enum
{
     LFO_SHAPE_SINE,
     LFO_SHAPE_TRIANGLE,
     LFO_SHAPE_SAW,
     LFO_SHAPE_SQUARE

} LFOShape;


typedef struct _LFOParams
{
    bool        lfo_on;
    LFOShape    shape;
    float       freq;        /* frequency in hz */
    float       sync_beats;
    bool        sync;
    bool        positive; /* oscillate from [0, 1] instead of [-1, 1] */
    float       delay;       /* in seconds */
    float       attack;      /* in seconds */

    int     fm1_id;    /* ID of modulation source */
    float   fm1_amt;   /* amount of modulation we add [-1.0, 1.0]  */
    int     fm2_id;
    float   fm2_amt;

    int     am1_id;
    float   am1_amt;
    int     am2_id;
    int     am2_amt;

} LFOParams;


typedef struct _LFO LFO;


/* initialize LFO subsystem */
void    lfo_tables_init(void);

/* set operating samplerate of all LFOs (call at least once and
 * whenever samplerate changes) */
void    lfo_set_samplerate(int rate);

/* set operating tempo in beats per minute of all tempo-synced LFOs */
void    lfo_set_tempo(float bpm);


void    lfo_params_init(LFOParams*, float freq, LFOShape);


LFO*    lfo_new(void);
void    lfo_free(LFO*);
void    lfo_init(LFO*);


/* activate an LFO using the given params; an LFO must be re-activated
 * after the samplerate/tempo changes in order for those changes to
 * take effect */
void    lfo_trigger(LFO*, LFOParams*);

/* like lfo_trigger except it don't reset phase */
void    lfo_rigger(LFO*, LFOParams*);

/* advance an LFO and return its new value */
float   lfo_tick(LFO*);


float const*    lfo_output(LFO*);
void            lfo_set_fm1(LFO*, float const*);
void            lfo_set_fm2(LFO*, float const*);

void            lfo_set_am1(LFO*,  float const*);
void            lfo_set_am2(LFO*,  float const*);

/* use to get pre-calculated lfo table values into global lfo */
void            lfo_set_output(LFO*, float);

#endif /* __LFO_H__ */
