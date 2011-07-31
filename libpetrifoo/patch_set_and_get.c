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


#include "patch_set_and_get.h"


#include <string.h>
#include <math.h>


#include "sample.h"
#include "adsr.h"
#include "lfo.h"


#include "patch_private/patch_data.h"
#include "patch_private/patch_defs.h"
#include "patch_private/patch_macros.h"


INLINE_ISOK_DEF


inline static int mod_src_to_eg_index(int patch_id, int id)
{
    if (!isok(patch_id))
        return PATCH_ID_INVALID;

    id -= MOD_SRC_EG;

    return (id >= 0 && id < VOICE_MAX_ENVS)
                ? id
                : PATCH_ENV_ID_INVALID;
}


static int mod_src_ok(int id)
{
    return (id & MOD_SRC_ALL || id == MOD_SRC_NONE)
            ? 0 
            : PATCH_MOD_SRC_INVALID;
}


inline static bool mark_ok(int id)
{
    if (id < WF_MARK_START || id > WF_MARK_STOP)
        return false;

    return true;
}

inline static bool mark_settable(int id)
{
    if (id < WF_MARK_PLAY_START || id > WF_MARK_PLAY_STOP)
        return false;

    return true;
}


static inline void set_mark_frame(int patch, int mark, int frame)
{
    *(patches[patch]->marks[mark]) = frame;
}


static inline int get_mark_frame(int patch, int mark)
{
    return *(patches[patch]->marks[mark]);
}


static int get_mark_frame_range(int patch, int mark, int* min, int* max)
{
    int xfade = patches[patch]->xfade_samples;

    if (mark == WF_MARK_START || mark == WF_MARK_STOP)
    {
        *min = *max = get_mark_frame(patch, mark);
        /* indicate non-editable! */
        return -1;
    }

    /* potential range: */
    *min = get_mark_frame(patch, mark - 1);
    *max = get_mark_frame(patch, mark + 1);

    /* tweak if necessary */
    switch(mark)
    {
    case WF_MARK_PLAY_START:
        *max -= xfade;
        break;
    case WF_MARK_PLAY_STOP:
        *min += xfade;
        break;
    case WF_MARK_LOOP_START:
        *min += xfade;
        *max -= xfade;
        break;
    case WF_MARK_LOOP_STOP:
        *min += xfade;
        *max -= xfade;
        break;
    }

    return get_mark_frame(patch, mark);
}


static int get_patch_param(int patch_id, PatchParamType param,
                                                            PatchParam** p)
{
    if (!isok(patch_id))
        return PATCH_ID_INVALID;

    switch(param)
    {
    case PATCH_PARAM_AMPLITUDE: *p = &patches[patch_id]->vol;      break;
    case PATCH_PARAM_PANNING:   *p = &patches[patch_id]->pan;      break;
    case PATCH_PARAM_CUTOFF:    *p = &patches[patch_id]->ffreq;    break;
    case PATCH_PARAM_RESONANCE: *p = &patches[patch_id]->freso;    break;
    case PATCH_PARAM_PITCH:     *p = &patches[patch_id]->pitch;    break;
    default:
        debug ("Invalid request for address of patch param\n");
        return PATCH_PARAM_INVALID;
    }

    return 0;
}


static int get_patch_bool(int patch_id, PatchBoolType booltype,
                                                            PatchBool** b)
{
    if (!isok(patch_id))
        return PATCH_ID_INVALID;

    switch(booltype)
    {
    case PATCH_BOOL_PORTAMENTO: *b = &patches[patch_id]->porta;     break;
    case PATCH_BOOL_LEGATO:     *b = &patches[patch_id]->legato;    break;
    default:
        debug("Invalid request for address of patch bool\n");
        return PATCH_BOOL_INVALID;
    }

    return 0;
}


static int get_patch_float(int patch_id, PatchFloatType floattype,
                                                            PatchFloat** f)
{
    if (!isok(patch_id))
        return PATCH_ID_INVALID;

    switch(floattype)
    {
    case PATCH_FLOAT_PORTAMENTO_TIME:
        *f = &patches[patch_id]->porta_secs;
        break;
    default:
        debug("Invalid request for address of patch float\n");
        return PATCH_FLOAT_INVALID;
    }

    return 0;
}



/* inline static function def macro, see private/patch_data.h */
INLINE_PATCH_TRIGGER_GLOBAL_LFO_DEF



/**************************************************************************/
/************************* ENVELOPE SETTERS *******************************/
/**************************************************************************/


int patch_set_env_on (int patch_id, int eg, bool state)
{
    eg = mod_src_to_eg_index(patch_id, eg);

    if (eg < 0)
        return eg; /* as error code */

    patches[patch_id]->env_params[eg].env_on = state;
    return 0;
}


/* sets the delay length in seconds */
int patch_set_env_delay (int patch_id, int eg, float secs)
{
    eg = mod_src_to_eg_index(patch_id, eg);

    if (eg < 0)
        return eg; /* as error code */

    if (secs < 0.0)
        return PATCH_PARAM_INVALID;

    patches[patch_id]->env_params[eg].delay = secs;
    return 0;
}

/* sets the attack length in seconds */
int patch_set_env_attack (int patch_id, int eg, float secs)
{
    eg = mod_src_to_eg_index(patch_id, eg);

    if (eg < 0)
        return eg; /* as error code */

    if (secs < 0.0)
        return PATCH_PARAM_INVALID;

    patches[patch_id]->env_params[eg].attack = secs;
    return 0;
}

/* sets the hold length in seconds */
int patch_set_env_hold (int patch_id, int eg, float secs)
{
    eg = mod_src_to_eg_index(patch_id, eg);

    if (eg < 0)
        return eg; /* as error code */

    if (secs < 0.0)
        return PATCH_PARAM_INVALID;

    patches[patch_id]->env_params[eg].hold = secs;
    return 0;
}

/* sets the decay length in seconds */
int patch_set_env_decay (int patch_id, int eg, float secs)
{
    eg = mod_src_to_eg_index(patch_id, eg);

    if (eg < 0)
        return eg; /* as error code */

    if (secs < 0.0)
        return PATCH_PARAM_INVALID;

    patches[patch_id]->env_params[eg].decay = secs;
    return 0;
}

/* sets the sustain level */
int patch_set_env_sustain (int patch_id, int eg, float level)
{
    eg = mod_src_to_eg_index(patch_id, eg);

    if (eg < 0)
        return eg; /* as error code */

    if (level < 0.0 || level > 1.0)
        return PATCH_PARAM_INVALID;

    patches[patch_id]->env_params[eg].sustain = level;
    return 0;
}

/* sets the release length in seconds */
int patch_set_env_release (int patch_id, int eg, float secs)
{
    eg = mod_src_to_eg_index(patch_id, eg);

    if (eg < 0)
        return eg; /* as error code */

    if (secs < 0.0)
        return PATCH_PARAM_INVALID;

    if (secs < PATCH_MIN_RELEASE)
        secs = PATCH_MIN_RELEASE;

    patches[patch_id]->env_params[eg].release = secs;
    return 0;
}

/* sets key tracking amount */
int patch_set_env_key_amt(int patch_id, int eg, float val)
{
    eg = mod_src_to_eg_index(patch_id, eg);

    if (eg < 0)
        return eg; /* as error code */

    if (val < -1.0 || val > 1.0)
        return PATCH_PARAM_INVALID;

    patches[patch_id]->env_params[eg].key_amt = val;
    return 0;
}

/*
int patch_set_env_vel_amt(int patch_id, int eg, float val)
{
    eg = mod_src_to_eg_index(patch_id, eg);

    if (eg < 0)
        return eg;

    if (val < -1.0 || val > 1.0)
        return PATCH_PARAM_INVALID;

    patches[patch_id]->env_params[eg].vel_amt = val;
    return 0;
}
*/

/**************************************************************************/
/************************* ENVELOPE GETTERS *******************************/
/**************************************************************************/

/* places the delay length in seconds into val */
int patch_get_env_on(int patch_id, int eg, bool* val)
{
    eg = mod_src_to_eg_index(patch_id, eg);

    if (eg < 0)
        return eg; /* as error code */

    *val = patches[patch_id]->env_params[eg].env_on;
    return 0;
}



/* places the delay length in seconds into val */
int patch_get_env_delay (int patch_id, int eg, float* val)
{
    eg = mod_src_to_eg_index(patch_id, eg);

    if (eg < 0)
        return eg; /* as error code */

    *val = patches[patch_id]->env_params[eg].delay;
    return 0;
}


/* places the attack length in seconds into val */
int patch_get_env_attack (int patch_id, int eg, float* val)
{
    eg = mod_src_to_eg_index(patch_id, eg);

    if (eg < 0)
        return eg; /* as error code */

    *val = patches[patch_id]->env_params[eg].attack;
    return 0;
}


/* places the hold length in seconds into val */
int patch_get_env_hold (int patch_id, int eg, float* val)
{
    eg = mod_src_to_eg_index(patch_id, eg);

    if (eg < 0)
        return eg; /* as error code */

    *val = patches[patch_id]->env_params[eg].hold;
    return 0;
}


/* places the decay length in seconds into val */
int patch_get_env_decay (int patch_id, int eg, float* val)
{
    eg = mod_src_to_eg_index(patch_id, eg);

    if (eg < 0)
        return eg; /* as error code */

    *val = patches[patch_id]->env_params[eg].decay;
    return 0;
}


/* places the sustain level into val */
int patch_get_env_sustain (int patch_id, int eg, float* val)
{
    eg = mod_src_to_eg_index(patch_id, eg);

    if (eg < 0)
        return eg; /* as error code */

    *val = patches[patch_id]->env_params[eg].sustain;
    return 0;
}

/* places the release length in seconds into val */
int patch_get_env_release (int patch_id, int eg, float* val)
{
    eg = mod_src_to_eg_index(patch_id, eg);

    if (eg < 0)
        return eg; /* as error code */

    *val = patches[patch_id]->env_params[eg].release;

    /* we hide the fact that we have a min release value from the
     * outside world (they're on a need-to-know basis) */
    if (*val <= PATCH_MIN_RELEASE)
        *val = 0;
    return 0;
}


int patch_get_env_key_amt (int patch_id, int eg, float* val)
{
    eg = mod_src_to_eg_index(patch_id, eg);

    if (eg < 0)
        return eg; /* as error code */

    *val = patches[patch_id]->env_params[eg].key_amt;
    return 0;
}

/*
int patch_get_env_vel_amt (int patch_id, int eg, float* val)
{
    eg = mod_src_to_eg_index(patch_id, eg);

    if (eg < 0)
        return eg;

    *val = patches[patch_id]->env_params[eg].vel_amt;
    return 0;
}
*/
/************************************************************************/
/*************************** LFO SETTERS ********************************/
/************************************************************************/

static int lfo_from_id(int patch_id, int id, LFO** lfo, LFOParams** lfopar)
{
    if (lfo)
        *lfo = 0;

    *lfopar = 0;

    if (!isok(patch_id))
        return PATCH_ID_INVALID;

    if ((id & MOD_SRC_VLFO) && (id & MOD_SRC_GLFO))
        return PATCH_LFO_ID_INVALID;

    if (id & MOD_SRC_VLFO)
    {
        id -= MOD_SRC_VLFO;

        if (id < VOICE_MAX_LFOS)
            *lfopar = &patches[patch_id]->vlfo_params[id];

        return 0;
    }

    if (id & MOD_SRC_GLFO)
    {
        id -= MOD_SRC_GLFO;

        if (id < PATCH_MAX_LFOS)
        {
            if (lfo)
                *lfo = patches[patch_id]->glfo[id];

            *lfopar = &patches[patch_id]->glfo_params[id];
        }

        return 0;
    }

    return PATCH_LFO_ID_INVALID;
}

#define LFO_CHECKS                                              \
    LFO*        lfo;                                            \
    LFOParams*  lfopar;                                         \
    int         err;                                            \
    if ((err = lfo_from_id(patch_id, lfo_id, &lfo, &lfopar)))   \
        return err;


int patch_set_lfo_on (int patch_id, int lfo_id, bool state)
{
    LFO_CHECKS
    lfopar->lfo_on = state;
    if (lfo)
        lfo_trigger(lfo, lfopar);
    return 0;
}

/* set the attack time of the param's LFO */
int patch_set_lfo_attack (int patch_id, int lfo_id, float secs)
{
    LFO_CHECKS
    if (secs < 0.0)
        return PATCH_PARAM_INVALID;
    lfopar->attack = secs;
    if (lfo)
        lfo_rigger(lfo, lfopar);
    return 0;
}

/* set the period length of the param's lfo in beats */
int patch_set_lfo_beats (int patch_id, int lfo_id, float beats)
{
    LFO_CHECKS
    if (beats < 0.0)
        return PATCH_PARAM_INVALID;
    lfopar->sync_beats = beats;
    if (lfo)
        lfo_rigger(lfo, lfopar);
    return 0;
}

/* set the delay time of the param's LFO */
int patch_set_lfo_delay (int patch_id, int lfo_id, float secs)
{
    LFO_CHECKS
    if (secs < 0.0)
        return PATCH_PARAM_INVALID;
    lfopar->delay = secs;
    if (lfo)
        lfo_rigger(lfo, lfopar);
    return 0;
}

/* set the frequency of the param's lfo */
int patch_set_lfo_freq (int patch_id, int lfo_id, float freq)
{
    LFO_CHECKS
    if (freq < 0.0)
        return PATCH_PARAM_INVALID;
    lfopar->freq = freq;
    if (lfo)
        lfo_rigger(lfo, lfopar);
    return 0;
}

/* set whether to constrain the param's LFOs to positive values or not */
int patch_set_lfo_positive (int patch_id, int lfo_id, bool state)
{
    LFO_CHECKS
    lfopar->positive = state;
    if (lfo)
        lfo_rigger(lfo, lfopar);
    return 0;
}

/* set the param's lfo shape */
int patch_set_lfo_shape (int patch_id, int lfo_id, LFOShape shape)
{
    LFO_CHECKS
    lfopar->shape = shape;
    if (lfo)
        lfo_rigger(lfo, lfopar);
    return 0;
}

/* set whether to the param's lfo should sync to tempo or not */
int patch_set_lfo_sync (int patch_id, int lfo_id, bool state)
{
    LFO_CHECKS
    lfopar->sync = state;
    if (lfo)
        lfo_rigger(lfo, lfopar);
    return 0;
}

/************************************************************************/
/*************************** LFO GETTERS ********************************/
/************************************************************************/

#define LFO_GET_CHECK                                           \
    LFOParams*  lfopar;                                         \
    int         err;                                            \
    if ((err = lfo_from_id(patch_id, lfo_id, NULL, &lfopar)))   \
        return err;


int patch_get_lfo_on(int patch_id, int lfo_id, bool* val)
{
    LFO_GET_CHECK
    *val = lfopar->lfo_on;
    return 0;
}


/* get the attack time of the param's LFO */
int patch_get_lfo_attack (int patch_id, int lfo_id, float* val)
{
    LFO_GET_CHECK
    *val = lfopar->attack;
    return 0;
}

/* get the param's lfo period length in beats */
int patch_get_lfo_beats (int patch_id, int lfo_id, float* val)
{
    LFO_GET_CHECK
    *val = lfopar->sync_beats;
    return 0;
}

/* get the delay time of the param's LFO */
int patch_get_lfo_delay (int patch_id, int lfo_id, float* val)
{
    LFO_GET_CHECK
    *val = lfopar->delay;
    return 0;
}

/* get the param's lfo frequency */
int patch_get_lfo_freq (int patch_id, int lfo_id, float* val)
{
    LFO_GET_CHECK
    *val = lfopar->freq;
    return 0;
}

int patch_get_lfo_positive (int patch_id, int lfo_id, bool* val)
{
    LFO_GET_CHECK
    *val = lfopar->positive;
    return 0;
}

/* get param's lfo shape */
int patch_get_lfo_shape (int patch_id, int lfo_id, LFOShape* val)
{
    LFO_GET_CHECK
    *val = lfopar->shape;
    return 0;
}

/* get whether param's lfo is tempo synced or not */
int patch_get_lfo_sync (int patch_id, int lfo_id, bool* val)
{
    LFO_GET_CHECK
    *val = lfopar->sync;
    return 0;
}


/**************************************************************************/
/************************ PARAMETER SETTERS *******************************/
/**************************************************************************/

/* sets channel patch listens on */
int patch_set_channel (int id, int channel)
{
    if (!isok (id))
        return PATCH_ID_INVALID;

    if (channel < 0 || channel > 15)
        return PATCH_CHANNEL_INVALID;

    patches[id]->channel = channel;
    return 0;
}

/* sets the cut signal this patch emits when activated */
int patch_set_cut (int id, int cut)
{
    if (!isok (id))
        return PATCH_ID_INVALID;
    patches[id]->cut = cut;
    return 0;
}

/* sets the cut signal that terminates this patch if active */
int patch_set_cut_by (int id, int cut_by)
{
    if (!isok (id))
        return PATCH_ID_INVALID;
    patches[id]->cut_by = cut_by;
    return 0;
}

/* sets filter cutoff frequency */
int patch_set_cutoff (int id, float freq)
{
    if (!isok (id))
        return PATCH_ID_INVALID;

    if (freq < 0.0 || freq > 1.0)
	return PATCH_PARAM_INVALID;

    patches[id]->ffreq.val = freq;
    return 0;
}

/* set whether this patch should be played legato or not */
int patch_set_legato(int id, bool val)
{
    if (!isok (id))
        return PATCH_ID_INVALID;
    patches[id]->legato.on = val;
    return 0;
}


int patch_set_fade_samples(int id, int samples)
{
debug("set fade samples id:%d samples:%d\n",id,samples);
    if (!isok (id))
        return PATCH_ID_INVALID;

    if (patches[id]->sample->sp == NULL)
        return 0;

    if (samples < 0)
    {
        debug ("refusing to set negative fade length\n");
        return PATCH_PARAM_INVALID;
    }

    if (patches[id]->play_start + samples * 2 >= patches[id]->play_stop)
    {
        debug ("refusing to set fade length greater than half the "
               " number of samples between play start and play stop\n");
        return PATCH_PARAM_INVALID;
    }

    patches[id]->fade_samples = samples;
    return 0;
}

int patch_set_xfade_samples(int id, int samples)
{
    if (!isok (id))
        return PATCH_ID_INVALID;

    if (patches[id]->sample->sp == NULL)
        return 0;

    if (samples < 0)
    {
        debug ("refusing to set negative xfade length\n");
        return PATCH_PARAM_INVALID;
    }

    if (patches[id]->loop_start + samples > patches[id]->loop_stop)
    {
        debug ("refusing to set xfade length greater than samples"
               " between play start and play stop\n");
        return PATCH_PARAM_INVALID;
    }

    if (patches[id]->loop_stop + samples > patches[id]->play_stop)
    {
        debug ("refusing to set xfade length greater than samples"
               " between loop stop and play stop\n");
        return PATCH_PARAM_INVALID;
    }

debug("setting xfade to %d\n",samples);
    patches[id]->xfade_samples = samples;
    return 0;
}


/* sets the lower note of a patch's range */
int patch_set_lower_note (int id, int note)
{
    if (!isok (id))
	return PATCH_ID_INVALID;

    if (note < 0 || note > 127)
	return PATCH_NOTE_INVALID;

    patches[id]->lower_note = note;
    return 0;
}


int patch_set_mark_frame(int patch, int mark, int frame)
{
    if (!isok(patch))
        return -1;

    if (patches[patch]->sample->sp == NULL)
        return -1;

    if (!mark_settable(mark))
        return -1;

    int min;
    int max;

    get_mark_frame_range(patch, mark, &min, &max);

    if (frame < min || frame > max)
        return -1;

    set_mark_frame(patch, mark, frame);
    return mark;
}


int patch_set_mark_frame_expand(int patch, int mark, int frame,
                                                     int* also_changed)
{
    int also = 0;
    int also_frame = -1;
    int xfade = patches[patch]->xfade_samples;
    int fade = patches[patch]->fade_samples;

    if (!isok(patch))
        return -1;

    if (patches[patch]->sample->sp == NULL)
        return -1;

    /*  if callee wishes not to be informed about which marks get changed
        as a result of changing this one, also_changed will be NULL. It
        needs to be a valid pointer.
     */
    if (!also_changed)
        also_changed = &also;

    *also_changed = -1;

    switch(mark)
    {
    case WF_MARK_PLAY_START:
        also_frame = frame + xfade; /* pot loop start pos */

        if (frame + fade * 2 >= get_mark_frame(patch, WF_MARK_PLAY_STOP)
         || also_frame >= get_mark_frame(patch, WF_MARK_LOOP_STOP))
        {
            mark = -1;
        }
        else if (also_frame > get_mark_frame(patch, WF_MARK_LOOP_START))
        {   /* moving play start along pushes loop start along... */
            if (also_frame + xfade
                < get_mark_frame(patch, WF_MARK_LOOP_STOP))
            {
                *also_changed = WF_MARK_LOOP_START;
            }
            else
                mark = -1;
        }
        break;

    case WF_MARK_PLAY_STOP:
        also_frame = frame - xfade; /* pot loop stop pos */

        if (frame - fade * 2<= get_mark_frame(patch, WF_MARK_PLAY_START)
         || also_frame <= get_mark_frame(patch, WF_MARK_LOOP_START))
        {
            mark = -1;
        }
        else if (also_frame < get_mark_frame(patch, WF_MARK_LOOP_STOP))
        {
            if (also_frame - xfade
                > get_mark_frame(patch, WF_MARK_LOOP_START))
            {
                *also_changed = WF_MARK_LOOP_STOP;
            }
            else
                mark = -1;
        }
        break;

    case WF_MARK_LOOP_START:
        also_frame = frame - xfade; /* pot play start pos */

        if (frame + xfade >= get_mark_frame(patch, WF_MARK_LOOP_STOP))
            mark = -1;
        else if (also_frame < get_mark_frame(patch, WF_MARK_PLAY_START))
        {
            if (also_frame > 0)
                *also_changed = WF_MARK_PLAY_START;
            else
                mark = -1;
        }
        break;

    case WF_MARK_LOOP_STOP:
        also_frame = frame + xfade /* pot play stop pos */;

        if (frame - xfade <= get_mark_frame(patch, WF_MARK_LOOP_START))
            mark = -1;
        else if (also_frame > get_mark_frame(patch, WF_MARK_PLAY_STOP))
        {
            if (also_frame < get_mark_frame(patch, WF_MARK_STOP))
                *also_changed = WF_MARK_PLAY_STOP;
            else
                mark = -1;
        }
        break;

    default:
        mark = -1;
    }

    if (mark != -1)
    {
        set_mark_frame(patch, mark, frame);
    }

    if (*also_changed != -1)
    {
        set_mark_frame(patch, *also_changed, also_frame);
    }

    return mark;
}


/* set whether the patch is monophonic or not */
int patch_set_monophonic(int id, bool val)
{
    if (!isok (id))
	return PATCH_ID_INVALID;

    patches[id]->mono = val;
    return 0;
}

/* sets the name */
int patch_set_name (int id, const char *name)
{
    if (!isok (id))
	return PATCH_ID_INVALID;
    strncpy (patches[id]->name, name, PATCH_MAX_NAME);
    return 0;
}

/* sets the root note */
int patch_set_note (int id, int note)
{
    if (!isok (id))
	return PATCH_ID_INVALID;

    if (note < 0 || note > 127)
	return PATCH_NOTE_INVALID;

    patches[id]->note = note;
    return 0;
}

/* sets the panorama */
int patch_set_panning (int id, float pan)
{
    if (!isok (id))
	return PATCH_ID_INVALID;

    if (pan < -1.0 || pan > 1.0)
	return PATCH_PAN_INVALID;

    patches[id]->pan.val = pan;
    return 0;
}

/* set the pitch */
int patch_set_pitch (int id, float pitch)
{
    if (!isok (id))
	return PATCH_ID_INVALID;

    if (pitch < -1.0 || pitch > 1.0)
	return PATCH_PARAM_INVALID;

    patches[id]->pitch.val = pitch;
    return 0;
}

/* set the pitch range */
int patch_set_pitch_steps (int id, int steps)
{
    if (!isok (id))
	return PATCH_ID_INVALID;

    if (steps < -PATCH_MAX_PITCH_STEPS
	|| steps > PATCH_MAX_PITCH_STEPS)
	return PATCH_PARAM_INVALID;

    patches[id]->pitch_steps = steps;
    return 0;
}

/* sets the play mode */
int patch_set_play_mode (int id, PatchPlayMode mode)
{
    if (!isok (id))
	return PATCH_ID_INVALID;

    /* verify direction */
    if (mode & PATCH_PLAY_FORWARD)
    {
	if (mode & PATCH_PLAY_REVERSE)
	{
	    return PATCH_PLAY_MODE_INVALID;
	}
    }
    else if (mode & PATCH_PLAY_REVERSE)
    {
	if (mode & PATCH_PLAY_FORWARD)
	{
	    return PATCH_PLAY_MODE_INVALID;
	}
    }
    else
    {
	return PATCH_PLAY_MODE_INVALID;
    }

    /* verify duration */
    if (mode & PATCH_PLAY_SINGLESHOT)
    {
	if ((mode & PATCH_PLAY_TRIM) || (mode & PATCH_PLAY_LOOP))
	{
	    return PATCH_PLAY_MODE_INVALID;
	}
    }
    else if (mode & PATCH_PLAY_TRIM)
    {
	if ((mode & PATCH_PLAY_SINGLESHOT) || (mode & PATCH_PLAY_LOOP))
	{
	    return PATCH_PLAY_MODE_INVALID;
	}
    }
    else if (mode & PATCH_PLAY_LOOP)
    {
	if ((mode & PATCH_PLAY_SINGLESHOT) || (mode & PATCH_PLAY_TRIM))
	{
	    return PATCH_PLAY_MODE_INVALID;
	}
    }

    /* make sure pingpong isn't frivolously set (just for style
     * points) */
    if ((mode & PATCH_PLAY_PINGPONG) && !(mode && PATCH_PLAY_LOOP))
    {
	return PATCH_PLAY_MODE_INVALID;
    }

    patches[id]->play_mode = mode;
    return 0;
}

/* set whether portamento is being used or not */
int patch_set_portamento (int id, bool val)
{
    if (!isok (id))
        return PATCH_ID_INVALID;

    patches[id]->porta.on = val;
    return 0;
}

/* set length of portamento slides in seconds */
int patch_set_portamento_time (int id, float secs)
{
    if (!isok (id))
        return PATCH_ID_INVALID;

    if (secs < 0.0)
        return PATCH_PARAM_INVALID;

    patches[id]->porta_secs.assign = secs;
    return 0;
}


/* set the filter's resonance */
int patch_set_resonance (int id, float reso)
{
    if (!isok (id))
	return PATCH_ID_INVALID;

    if (reso < 0.0 || reso > 1.0)
	return PATCH_PARAM_INVALID;

    patches[id]->freso.val = reso;
    return 0;
}


/* set the upper note of a patch's range */
int patch_set_upper_note (int id, int note)
{
    if (!isok (id))
	return PATCH_ID_INVALID;

    if (note < 0 || note > 127)
	return PATCH_NOTE_INVALID;

    patches[id]->upper_note = note;
    return 0;
}

/* sets the patch lower velocity */ 
int patch_set_lower_vel (int id, int vel)
{
    if (!isok (id))
	return PATCH_ID_INVALID;

    if (vel < 0 || vel > 127)
	return PATCH_VELOCITY_INVALID;

    patches[id]->lower_vel = vel;
    return 0;
}

/* sets the patch upper velocity */ 
int patch_set_upper_vel (int id, int vel)
{
    if (!isok (id))
	return PATCH_ID_INVALID;

    if (vel < 0 || vel > 127)
	return PATCH_VELOCITY_INVALID;

    patches[id]->upper_vel = vel;
    return 0;
}


/* set the amplitude */
int patch_set_amplitude (int id, float vol)
{

    if (!isok (id))
	return PATCH_ID_INVALID;

    if (vol < 0 || vol > 1.0)
	return PATCH_VOL_INVALID;

    patches[id]->vol.val = vol;
    return 0;
}

/**************************************************************************/
/************************* PARAMETER GETTERS*******************************/
/**************************************************************************/

/* get the channel the patch listens on */
int patch_get_channel (int id)
{
    if (!isok (id))
        return PATCH_ID_INVALID;

    return patches[id]->channel;
}

/* get the cut signal */
int patch_get_cut (int id)
{
    if (!isok (id))
        return PATCH_ID_INVALID;

    return patches[id]->cut;
}

/* get the cut-by signal */
int patch_get_cut_by (int id)
{
    if (!isok (id))
        return PATCH_ID_INVALID;

    return patches[id]->cut_by;
}

/* get the filter cutoff value */
float patch_get_cutoff (int id)
{
    if (!isok (id))
        return PATCH_ID_INVALID;

    return patches[id]->ffreq.val;
}

/* get the display index */
int patch_get_display_index (int id)
{
    if (!isok (id))
        return PATCH_ID_INVALID;

    return patches[id]->display_index;
}

/* get the number of frame in the sample */
int patch_get_frames (int id)
{
    if (!isok (id))
        return PATCH_ID_INVALID;

    if (patches[id]->sample->sp == NULL)
        return 0;

   debug("patches[%d].sample->frames:%d\n", id, patches[id]->sample->frames);

    return patches[id]->sample->frames;
}

/* get whether this patch is played legato or not */
bool patch_get_legato(int id)
{
    if (!isok (id))
        return PATCH_ID_INVALID;

    return patches[id]->legato.on;
}


/* get the lower note */
int patch_get_lower_note (int id)
{
    if (!isok (id))
        return PATCH_ID_INVALID;

    return patches[id]->lower_note;
}


int patch_get_mark_frame(int patch, int mark)
{
    if (!isok(patch))
        return -1;

    if (patches[patch]->sample->sp == NULL)
        return -1;

    if (!mark_ok(mark))
        return -1;

    return get_mark_frame(patch, mark);
}


int patch_get_mark_frame_range(int patch, int mark, int* frame_min,
                                                    int* frame_max)
{
    if (!isok(patch) || !mark_ok(mark))
        return -1;

    return get_mark_frame_range(patch, mark, frame_min, frame_max);
}

/* get whether this patch is monophonic or not */
bool patch_get_monophonic(int id)
{
    if (!isok(id))
        return PATCH_ID_INVALID;

    return patches[id]->mono;
}

/* get the name */
char *patch_get_name (int id)
{
    char *name;

    if (!isok(id))
        name = strdup ("\0");
    else
        name = strdup (patches[id]->name);

    return name;
}

/* get the root note */
int patch_get_note (int id)
{
    if (!isok(id))
        return PATCH_ID_INVALID;

    return patches[id]->note;
}

/* get the panorama */
float patch_get_panning (int id)
{
    if (!isok(id))
        return PATCH_ID_INVALID;

    return patches[id]->pan.val;
}

/* get the pitch */
float patch_get_pitch (int id)
{
    if (!isok(id))
        return PATCH_ID_INVALID;

    return patches[id]->pitch.val;
}

/* get the pitch range */
int patch_get_pitch_steps (int id)
{
    if (!isok(id))
        return PATCH_ID_INVALID;

    return patches[id]->pitch_steps;
}

/* get the play mode */
PatchPlayMode patch_get_play_mode(int id)
{
    if (!isok(id))
        return PATCH_ID_INVALID;

    return patches[id]->play_mode;
}

/* get whether portamento is used or not */
bool patch_get_portamento(int id)
{
    if (!isok(id))
        return PATCH_ID_INVALID;

    return patches[id]->porta.on;
}

/* get length of portamento slides in seconds */
float patch_get_portamento_time(int id)
{
    if (!isok(id))
        return PATCH_ID_INVALID;

    return patches[id]->porta_secs.assign;
}


/* get the filter's resonance amount */
float patch_get_resonance(int id)
{
    if (!isok(id))
        return PATCH_ID_INVALID;

    return patches[id]->freso.val;
}

/* get a pointer to the sample data */
const float *patch_get_sample (int id)
{
    if (!isok(id))
        return NULL;

    return patches[id]->sample->sp;
}

/* get the name of the sample file */
const char *patch_get_sample_name (int id)
{
    if (!isok(id))
        return 0;

    return patches[id]->sample->filename;
}

/* get the upper note */
int patch_get_upper_note (int id)
{
    if (!isok(id))
        return PATCH_ID_INVALID;

    return patches[id]->upper_note;
}

/* get the upper velocity trigger */
int patch_get_upper_vel (int id)
{
    if (!isok(id))
        return PATCH_ID_INVALID;

    return patches[id]->upper_vel;
}

/* get the lower velocity trigger */
int patch_get_lower_vel (int id)
{
    if (!isok(id))
        return PATCH_ID_INVALID;

    return patches[id]->lower_vel;
}

/* get the amplitude */
float patch_get_amplitude (int id)
{
    if (!isok(id))
        return PATCH_ID_INVALID;

    return patches[id]->vol.val;
}


int patch_get_fade_samples(int id)
{
    if (!isok (id))
        return PATCH_ID_INVALID;

    return patches[id]->fade_samples;
}


int patch_get_xfade_samples(int id)
{
    if (!isok(id))
        return PATCH_ID_INVALID;

    return patches[id]->xfade_samples;
}


int patch_get_max_fade_samples(int id)
{
    return (patches[id]->play_stop - patches[id]->play_start) / 2;
}


int patch_get_max_xfade_samples(int id)
{
    int min = patches[id]->sample->frames;
    int tmp;

    tmp = patches[id]->loop_stop - patches[id]->loop_start;
    min = (tmp < min) ? tmp : min;

    tmp = patches[id]->play_stop - patches[id]->loop_stop;
    min = (tmp < min) ? tmp : min;

    tmp = patches[id]->loop_start - patches[id]->play_start;
    min = (tmp < min) ? tmp : min;

debug("max xfade samples:%d\n", min);

    return min;
}


/******************************************************************/
/*************************** PARAM ********************************/
/******************************************************************/

int patch_param_get_value(int patch_id, PatchParamType param, float* v)
{
    if (!isok(patch_id))
        return PATCH_ID_INVALID;

    switch(param)
    {
    case PATCH_PARAM_AMPLITUDE: *v = patches[patch_id]->vol.val;    break;
    case PATCH_PARAM_PANNING:   *v = patches[patch_id]->pan.val;    break;
    case PATCH_PARAM_CUTOFF:    *v = patches[patch_id]->ffreq.val;  break;
    case PATCH_PARAM_RESONANCE: *v = patches[patch_id]->freso.val;  break;
    case PATCH_PARAM_PITCH:     *v = patches[patch_id]->pitch.val;  break;
    default:
        return PATCH_PARAM_INVALID;
    }

    return 0;
}


int patch_param_set_value(int patch_id, PatchParamType param, float v)
{
    if (!isok(patch_id))
        return PATCH_ID_INVALID;

    switch(param)
    {
    case PATCH_PARAM_AMPLITUDE: patches[patch_id]->vol.val = v;     break;
    case PATCH_PARAM_PANNING:   patches[patch_id]->pan.val = v;     break;
    case PATCH_PARAM_CUTOFF:    patches[patch_id]->ffreq.val = v;   break;
    case PATCH_PARAM_RESONANCE: patches[patch_id]->freso.val = v;   break;
    case PATCH_PARAM_PITCH:     patches[patch_id]->pitch.val = v;   break;
    default:
        return PATCH_PARAM_INVALID;
    }

    return 0;
}


/**************************************************************************/
/*********************** MODULATION SETTERS *******************************/
/**************************************************************************/

#define PATCH_PARAM_CHECKS                                  \
    PatchParam* p;                                          \
    int err;                                                \
    if (!isok(patch_id))                                    \
        return PATCH_ID_INVALID;                            \
    if ((err = get_patch_param(patch_id, param, &p)) < 0)   \
        return err;

#define PATCH_SLOT_CHECKS                         \
    if (slot < 0 || slot > MAX_MOD_SLOTS)   \
        return PATCH_MOD_SLOT_INVALID;      \


int
patch_param_set_mod_src(int patch_id, PatchParamType param, int slot,
                                                            int id)
{
    PATCH_PARAM_CHECKS
    PATCH_SLOT_CHECKS
    p->mod_id[slot] = id;
    return 0;
}


int
patch_param_set_mod_amt(int patch_id, PatchParamType param, int slot,
                                                            float amt)
{
    PATCH_PARAM_CHECKS
    PATCH_SLOT_CHECKS

    if (amt < -1.0 || amt > 1.0)
        return PATCH_MOD_AMOUNT_INVALID;

    p->mod_amt[slot] = amt;

    if (param == PATCH_PARAM_PITCH)
    {
        patches[patch_id]->mod_pitch_max[slot] =
                        pow(2, (amt * PATCH_MAX_PITCH_STEPS) / 12.0);
        patches[patch_id]->mod_pitch_min[slot] =
                        pow(2, -(amt * PATCH_MAX_PITCH_STEPS) / 12.0);
    }

    return 0;
}


int
patch_param_set_vel_amount(int patch_id, PatchParamType param, float amt)
{
    PATCH_PARAM_CHECKS

    if (amt < -1.0 || amt > 1.0)
        return PATCH_PARAM_INVALID;

    p->vel_amt = amt;
    return 0;
}


int
patch_param_set_key_amount(int patch_id, PatchParamType param, float amt)
{
    PATCH_PARAM_CHECKS

    if (amt < -1.0 || amt > 1.0)
        return PATCH_PARAM_INVALID;

    p->key_amt = amt;
    return 0;
}


/**************************************************************************/
/********************** MODULATION GETTERS ********************************/
/**************************************************************************/

int
patch_param_get_mod_src(int patch_id, PatchParamType param, int slot,
                                                            int* src_id)
{
    PATCH_PARAM_CHECKS
    PATCH_SLOT_CHECKS
    *src_id = p->mod_id[slot];
    return 0;
}


int
patch_param_get_mod_amt(int patch_id, PatchParamType param, int slot,
                                                            float* amt)
{
    PATCH_PARAM_CHECKS
    PATCH_SLOT_CHECKS
    *amt = p->mod_amt[slot];
    return 0;
}


int patch_param_get_vel_amount(int patch_id, PatchParamType param,
                                                            float* val)
{
    PATCH_PARAM_CHECKS
    *val = p->vel_amt;
    return 0;
}


int patch_param_get_key_amount(int patch_id, PatchParamType param,
                                                            float* val)
{
    PATCH_PARAM_CHECKS
    *val = p->key_amt;
    return 0;
}


#define PATCH_BOOL_CHECKS                                   \
    PatchBool* b;                                           \
    int err;                                                \
    if (!isok(patch_id))                                    \
        return PATCH_ID_INVALID;                            \
    if ((err = get_patch_bool(patch_id, booltype, &b)) < 0) \
        return err;



/* PatchBool set/get */
int patch_bool_set_on(int patch_id, PatchBoolType booltype, bool val)
{
    PATCH_BOOL_CHECKS
    b->on = val;
    return 0;
}


int patch_bool_set_thresh(int patch_id, PatchBoolType booltype, float val)
{
    PATCH_BOOL_CHECKS
    b->thresh = val;
    return 0;
}


int patch_bool_set_mod_src(int patch_id, PatchBoolType booltype, int mod_id)
{
    PATCH_BOOL_CHECKS
    b->mod_id = mod_id;
    return 0;
}


int patch_bool_get_all(int patch_id, PatchBoolType booltype,
                        bool* on, float* thresh, int* mod_id)
{
    PATCH_BOOL_CHECKS
    *on =       b->on;
    *thresh =   b->thresh;
    *mod_id =   b->mod_id;
    return 0;
}


int patch_bool_get_on(int patch_id, PatchBoolType booltype, bool* on)
{
    PATCH_BOOL_CHECKS;
    *on = b->on;
    return 0;
}

int
patch_bool_get_thresh(int patch_id, PatchBoolType booltype, float* thresh)
{
    PATCH_BOOL_CHECKS
    *thresh =   b->thresh;
    return 0;
}

int
patch_bool_get_mod_src(int patch_id, PatchBoolType booltype, int* mod_id)
{
    PATCH_BOOL_CHECKS
    *mod_id =   b->mod_id;
    return 0;
}

#define PATCH_FLOAT_CHECKS                                      \
    PatchFloat* f;                                              \
    int err;                                                    \
    if (!isok(patch_id))                                        \
        return PATCH_ID_INVALID;                                \
    if ((err = get_patch_float(patch_id, floattype, &f)) < 0)   \
        return err;


/* PatchFloat set/get */
int
patch_float_set_assign(int patch_id, PatchFloatType floattype, float val)
{
    PATCH_FLOAT_CHECKS
    f->assign = val;
    return 0;
}


int
patch_float_set_mod_amt(int patch_id, PatchFloatType floattype, float val)
{
    PATCH_FLOAT_CHECKS
    f->mod_amt = val;
    return 0;
}


int
patch_float_set_mod_src(int patch_id, PatchFloatType floattype, int mod_id)
{
    PATCH_FLOAT_CHECKS
    f->mod_id = mod_id;
    return 0;
}


int patch_float_get_all(int patch_id, PatchFloatType floattype,
                        float* assign, float* mod_amt, int* mod_id)
{
    PATCH_FLOAT_CHECKS
    *assign =   f->assign;
    *mod_amt =  f->mod_amt;
    *mod_id =   f->mod_id;
    return 0;
}


int
patch_float_get_assign(int patch_id, PatchFloatType floattype,
                                                float* assign)
{
    PATCH_FLOAT_CHECKS;
    *assign = f->assign;
    return 0;
}

int
patch_float_get_mod_amt(int patch_id, PatchFloatType floattype,
                                                float* mod_amt)
{
    PATCH_FLOAT_CHECKS
    *mod_amt =   f->mod_amt;
    return 0;
}

int
patch_float_get_mod_src(int patch_id, PatchFloatType floattype, int* mod_id)
{
    PATCH_FLOAT_CHECKS
    *mod_id =   f->mod_id;
    return 0;
}



/**************************************************************************/
/****************** LFO FREQ MODULATION SETTERS ***************************/
/**************************************************************************/

#define PATCH_LFO_CHECKS                                        \
    LFO* lfo;                                                   \
    LFOParams* lfopar;                                          \
    int err;                                                    \
    if ((err = lfo_from_id(patch_id, lfo_id, &lfo, &lfopar)))   \
        return err;

#define PATCH_NULL_LFO_CHECKS                                   \
    LFOParams* lfopar;                                          \
    int err;                                                    \
    if ((err = lfo_from_id(patch_id, lfo_id, NULL, &lfopar)))   \
        return err;


int patch_set_lfo_fm1_src(int patch_id, int lfo_id, int modsrc_id)
{
    PATCH_LFO_CHECKS
    lfopar->fm1_id = modsrc_id;

    if (lfo)
        patch_trigger_global_lfo(patch_id, lfo, lfopar);

    return 0;
}

int patch_set_lfo_fm2_src(int patch_id, int lfo_id, int modsrc_id)
{
    PATCH_LFO_CHECKS
    lfopar->fm2_id = modsrc_id;

    if (lfo)
        patch_trigger_global_lfo(patch_id, lfo, lfopar);

    return 0;
}

int patch_set_lfo_fm1_amt(int patch_id, int lfo_id, float amount)
{
    PATCH_LFO_CHECKS
    lfopar->fm1_amt = amount;

    if (lfo)
        patch_trigger_global_lfo(patch_id, lfo, lfopar);

    return 0;
}

int patch_set_lfo_fm2_amt(int patch_id, int lfo_id, float amount)
{
    PATCH_LFO_CHECKS
    lfopar->fm2_amt = amount;

    if (lfo)
        patch_trigger_global_lfo(patch_id, lfo, lfopar);

    return 0;
}



/**************************************************************************/
/****************** LFO FREQ MODULATION GETTERS ***************************/
/**************************************************************************/

int patch_get_lfo_fm1_src(int patch_id, int lfo_id, int* modsrc_id)
{
    PATCH_NULL_LFO_CHECKS
    *modsrc_id = lfopar->fm1_id;
    return 0;
}

int patch_get_lfo_fm2_src(int patch_id, int lfo_id, int* modsrc_id)
{
    PATCH_NULL_LFO_CHECKS
    *modsrc_id = lfopar->fm2_id;
    return 0;
}

int patch_get_lfo_fm1_amt(int patch_id, int lfo_id, float* amount)
{
    PATCH_NULL_LFO_CHECKS
    *amount = lfopar->fm1_amt;
    return 0;
}

int patch_get_lfo_fm2_amt(int patch_id, int lfo_id, float* amount)
{
    PATCH_NULL_LFO_CHECKS
    *amount = lfopar->fm2_amt;
    return 0;
}


/**************************************************************************/
/******************* LFO AMP MODULATION SETTERS ***************************/
/**************************************************************************/

int patch_set_lfo_am1_src(int patch_id, int lfo_id, int modsrc_id)
{
    PATCH_LFO_CHECKS
    lfopar->am1_id = modsrc_id;

    if (lfo)
        patch_trigger_global_lfo(patch_id, lfo, lfopar);

    return 0;
}

int patch_set_lfo_am2_src(int patch_id, int lfo_id, int modsrc_id)
{
    PATCH_LFO_CHECKS
    lfopar->am2_id = modsrc_id;

    if (lfo)
        patch_trigger_global_lfo(patch_id, lfo, lfopar);

    return 0;
}

int patch_set_lfo_am1_amt(int patch_id, int lfo_id, float amount)
{
    PATCH_LFO_CHECKS
    lfopar->am1_amt = amount;

    if (lfo)
        patch_trigger_global_lfo(patch_id, lfo, lfopar);

    return 0;
}

int patch_set_lfo_am2_amt(int patch_id, int lfo_id, float amount)
{
    PATCH_LFO_CHECKS
    lfopar->am2_amt = amount;

    if (lfo)
        patch_trigger_global_lfo(patch_id, lfo, lfopar);

    return 0;
}



/**************************************************************************/
/******************* LFO AMP MODULATION GETTERS ***************************/
/**************************************************************************/

int patch_get_lfo_am1_src(int patch_id, int lfo_id, int* modsrc_id)
{
    PATCH_NULL_LFO_CHECKS
    *modsrc_id = lfopar->am1_id;
    return 0;
}

int patch_get_lfo_am2_src(int patch_id, int lfo_id, int* modsrc_id)
{
    PATCH_NULL_LFO_CHECKS
    *modsrc_id = lfopar->am2_id;
    return 0;
}

int patch_get_lfo_am1_amt(int patch_id, int lfo_id, float* amount)
{
    PATCH_NULL_LFO_CHECKS
    *amount = lfopar->am1_amt;
    return 0;
}

int patch_get_lfo_am2_amt(int patch_id, int lfo_id, float* amount)
{
    PATCH_NULL_LFO_CHECKS
    *amount = lfopar->am2_amt;
    return 0;
}


