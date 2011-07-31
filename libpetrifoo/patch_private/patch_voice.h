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


#ifndef PATCH_PRIVATE_PATCH_VOICE_H
#define PATCH_PRIVATE_PATCH_VOICE_H


#include "adsr.h"
#include "lfo.h"
#include "patch.h"
#include "ticks.h"


#include <stdint.h>
#include <stdbool.h>


/* release modes */
typedef enum
{
    RELEASE_NONE,       /* release no voices */
    RELEASE_NOTEOFF,    /* release is a result of a midi noteoff */
    RELEASE_STEAL,      /* release is result of voice stealing */
    RELEASE_CUTOFF,     /* release is a result of a cut */
    RELEASE_ALL         /* release all voices for a given patch */
} release_t;


typedef enum
{
    PLAYSTATE_OFF =         0,
    PLAYSTATE_PLAY =        1 << 1,
    PLAYSTATE_LOOP =        1 << 2,
    PLAYSTATE_FADE_IN =     1 << 3,
    PLAYSTATE_FADE_OUT =    1 << 4,
} playstate_t;


/* type for currently playing notes (voices) */
typedef struct _PatchVoice
{
    bool    active;         /* whether this voice is playing or not */
    Tick    ticks;          /* at what time this voice was activated */
    int     relset;         /* how many ticks should pass before we release
                             * this voice (negative if N/A) */
    release_t   relmode;    /* how release activated (noteoff or cutoff) */

    bool    released;       /* whether we've been released or not */
    bool    to_end;         /* whether we're to go to end of sample after
                               loop or not */
    int       dir;          /* current direction
                             * (forward == 1, reverse == -1) */
    int         note;       /* the note that activated us */
    double      pitch;      /* what pitch ratio to play at */
    double      pitch_step; /* how much to increment pitch by each
                             * porta_tick */
    int         porta_ticks;/* how many ticks to increment pitch for */
    int         posi;       /* integer sample index */
    uint32_t    posf;       /* fractional sample index */
    int         stepi;      /* integer step amount */
    uint32_t    stepf;      /* fractional step amount */

    float   vel;            /* velocity; volume of this voice */
    float   key_track;      /* = (note - lower) / (upper - lower) */

    float const* vol_mod[MAX_MOD_SLOTS];
    float const* pan_mod[MAX_MOD_SLOTS];
    float const* ffreq_mod[MAX_MOD_SLOTS];
    float const* freso_mod[MAX_MOD_SLOTS];
    float const* pitch_mod[MAX_MOD_SLOTS];

    ADSR*       env[VOICE_MAX_ENVS];
    LFO*        lfo[VOICE_MAX_LFOS];

    float       fll;    /* lowpass filter buffer, left */
    float       fbl;    /* bandpass apparently, left */
    float       flr;    /* lowpass filter buffer, right */
    float       fbr;    /* bandpass right */

    /* formerly declick_vol */
    playstate_t playstate;
    bool        xfade;
    bool        loop;

    int         fade_posi;  /* position in fade ie 0 ~ fade_samples - */
    uint32_t    fade_posf;  /* used for all fades: in, out, and x */

    int         fade_out_start_pos;

    float       fade_declick;

    int         xfade_point_posi;
    uint32_t    xfade_point_posf;
    int         xfade_posi; /* position in xfade ie 0 ~ xfade_samples */
    uint32_t    xfade_posf;
    int         xfade_dir;  /* direction of continuation */

    float       xfade_declick;

} PatchVoice;


PatchVoice* patch_voice_new(void);
void        patch_voice_free(PatchVoice*);


#endif
