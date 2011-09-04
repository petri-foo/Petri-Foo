/*  Petri-Foo is a fork of the Specimen audio sampler.

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
*/


#ifndef PATCH_EVENT_H
#define PATCH_EVENT_H


#include <stdbool.h>


typedef enum _EventType
{
    EV_TYPE_EG,     /* patch_id, eg_id,  var_id */
    EV_TYPE_LFO,    /* patch_id, lfo_id, var_id */
    EV_TYPE_LFO_AM, /* patch_id, lfo_id, AM slot_id, var_id */
    EV_TYPE_LFO_FM, /* patch_id, lfo_id, FM slot_id, var_id */
    EV_TYPE_MAIN,   /* patch_id, var_id */
    EV_TYPE_MARK,   /* patch_id, mark_id */

} EvType;



enum
{
    EV_INVALID_EVENT = -1,

    /* bool settings */
    EV__ACTIVE,
    EV__LEGATO,
    EV__MONO,
    EV__ON,
    EV__PORTAMENTO,
    EV__POSITIVE,
    EV__SYNC,

    /* char settings */
    EV__NAME,

    /* float settings */
    EV__AMPLITUDE,
    EV__ASSIGN,
    EV__ATTACK,
    EV__CUTOFF,
    EV__DECAY,
    EV__DELAY,
    EV__FREQ,
    EV__HOLD,
    EV__KEY_AMT,
    EV__MOD_AMT,
    EV__PAN,
    EV__PORTAMENTO_TIME,
    EV__PITCH,
    EV__RELEASE,
    EV__RESONANCE,
    EV__SUSTAIN,
    EV__SYNC_BEATS,
    EV__THRESH,
    EV__VALUE,
    EV__VEL_AMT,

    /* int settings */
    EV__CHANNEL,
    EV__CUT,
    EV__CUT_BY,
    EV__FADE_SAMPLES,
    EV__LFO_SHAPE,
    EV__LOOP_START,
    EV__LOOP_STOP,
    EV__LOWER_NOTE,
    EV__LOWER_VEL,
    EV__MOD_SRC,
    EV__PITCH_STEPS,
    EV__PLAY_MODE,
    EV__PLAY_START,
    EV__PLAY_STOP,
    EV__ROOT_NOTE,
    EV__UPPER_NOTE,
    EV__UPPER_VEL,
    EV__XFADE_SAMPLES,

    EV_LAST_EVENT_XXX
};



typedef struct _BaseEvent BaseEvent;


int         patch_event_set(    int patch_id, EvType, ...);
BaseEvent*  patch_event_get(    int patch_id, EvType, ...);

/* assertions on these to make sure correct type used */
bool    base_event_get_bool(    BaseEvent*);
float   base_event_get_float(   BaseEvent*);
int     base_event_get_int(     BaseEvent*);

#endif
