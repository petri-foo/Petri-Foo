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

#include <assert.h>
#include <math.h>
#include <string.h>
#include <unistd.h>


#include "sample.h"
#include "adsr.h"
#include "lfo.h"
#include "pf_error.h"


#include "patch_private/err_msg.h"
#include "patch_private/patch_data.h"
#include "patch_private/patch_defs.h"
#include "patch_private/patch_macros.h"


INLINE_PATCHOK_DEF

inline static bool markok(int id)
{
    return (id >= WF_MARK_START && id <= WF_MARK_STOP);
}


inline static bool marksetok(int id)
{
    return (id >= WF_MARK_PLAY_START && id <= WF_MARK_PLAY_STOP);
}


static inline void set_mark_frame(int patch_id, int mark, int frame)
{
    *(patches[patch_id]->marks[mark]) = frame;
}


static inline int get_mark_frame(int patch_id, int mark)
{
    return *(patches[patch_id]->marks[mark]);
}


static int get_mark_frame_range(int patch_id, int mark, int* min, int* max)
{
    int xfade;

    assert(patchok(patch_id));
    assert(markok(mark));

    xfade = patches[patch_id]->xfade_samples;

    if (mark == WF_MARK_START || mark == WF_MARK_STOP)
    {
        *min = *max = get_mark_frame(patch_id, mark);
        /* indicate non-editable! */
        return -1;
    }

    /* potential range: */
    *min = get_mark_frame(patch_id, mark - 1);
    *max = get_mark_frame(patch_id, mark + 1);

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
    default:
        assert(0);
    }

    return get_mark_frame(patch_id, mark);
}


static PatchParam* get_patch_param(int patch_id, PatchParamType param)
{
    switch(param)
    {
    case PATCH_PARAM_AMPLITUDE: return &patches[patch_id]->amp;
    case PATCH_PARAM_PANNING:   return &patches[patch_id]->pan;
    case PATCH_PARAM_CUTOFF:    return &patches[patch_id]->ffreq;
    case PATCH_PARAM_RESONANCE: return &patches[patch_id]->freso;
    case PATCH_PARAM_PITCH:     return &patches[patch_id]->pitch;
    default:
        assert(0);
    }
    return 0;
}


static PatchBool* get_patch_bool(int patch_id, PatchBoolType booltype)
{
    assert(patchok(patch_id));
    switch(booltype)
    {
    case PATCH_BOOL_PORTAMENTO: return &patches[patch_id]->porta;
    case PATCH_BOOL_LEGATO:     return &patches[patch_id]->legato;
    default:
        assert(0);
    }
    return 0;
}


static PatchFloat* get_patch_float(int patch_id, PatchFloatType floattype)
{
    assert(patchok(patch_id));
    switch(floattype)
    {
    case PATCH_FLOAT_PORTAMENTO_TIME:
        return &patches[patch_id]->porta_secs;
    default:
        assert(0);
    }
    return 0;
}


/* inline static function def macro, see private/patch_data.h */
INLINE_PATCH_TRIGGER_GLOBAL_LFO_DEF



/**************************************************************************/
/************************* ENVELOPE SETTERS *******************************/
/**************************************************************************/

inline static int mod_src_to_eg_index(int id)
{
    assert(id >= MOD_SRC_EG);
    id -= MOD_SRC_EG;
    assert(id < VOICE_MAX_ENVS);
    return id;
}


int patch_set_env_active(int patch_id, int eg, bool state)
{
    assert(patchok(patch_id));
    eg = mod_src_to_eg_index(eg);
    patches[patch_id]->env_params[eg].active = state;
    return 0;
}


#define PATCH_SET_ENV_TIME( _EGPAR )                            \
int patch_set_env_##_EGPAR(int patch_id, int eg, float secs)    \
{                                                       \
    assert(patchok(patch_id));                          \
    if (secs < 0.0f)                                    \
    {                                                   \
        pf_error(PF_ERR_PATCH_VALUE_NEGATIVE);          \
        return -1;                                      \
    }                                                   \
    eg = mod_src_to_eg_index(eg);                       \
    patches[patch_id]->env_params[eg]._EGPAR = secs;    \
    return 0;                                           \
}

PATCH_SET_ENV_TIME( delay  )
PATCH_SET_ENV_TIME( attack )
PATCH_SET_ENV_TIME( hold   )
PATCH_SET_ENV_TIME( decay  )

/* special cases: */

int patch_set_env_sustain (int patch_id, int eg, float level)
{
    assert(patchok(patch_id));
    if (level < 0.0 || level > 1.0)
    {
        pf_error(PF_ERR_PATCH_VALUE_LEVEL);
        return -1;
    }
    eg = mod_src_to_eg_index(eg);
    patches[patch_id]->env_params[eg].sustain = level;
    return 0;
}


int patch_set_env_release (int patch_id, int eg, float secs)
{
    assert(patchok(patch_id));
    if (secs < 0.0f)
    {
        pf_error(PF_ERR_PATCH_VALUE_NEGATIVE);
        return -1;
    }
    eg = mod_src_to_eg_index(eg);
    /* use of min release time should remain hidden */
    if (secs < PATCH_MIN_RELEASE)
        secs = PATCH_MIN_RELEASE;
    patches[patch_id]->env_params[eg].release = secs;
    return 0;
}


int patch_set_env_key_amt(int patch_id, int eg, float val)
{
    assert(patchok(patch_id));
    if (val < -1.0 || val > 1.0)
    {
        pf_error(PF_ERR_PATCH_PARAM_VALUE);
        return -1;
    }
    eg = mod_src_to_eg_index(eg);
    patches[patch_id]->env_params[eg].key_amt = val;
    return 0;
}


/**************************************************************************/
/************************* ENVELOPE GETTERS *******************************/
/**************************************************************************/

#define PATCH_GET_ENV_PARAM( _EGPAR, _EGPARTYPE)        \
_EGPARTYPE patch_get_env_##_EGPAR(int patch_id, int eg) \
{                                                       \
    assert(patchok(patch_id));                             \
    eg = mod_src_to_eg_index(eg);                       \
    return patches[patch_id]->env_params[eg]._EGPAR;    \
}

PATCH_GET_ENV_PARAM( active,    bool  )
PATCH_GET_ENV_PARAM( delay,     float )
PATCH_GET_ENV_PARAM( attack,    float )
PATCH_GET_ENV_PARAM( hold,      float )
PATCH_GET_ENV_PARAM( decay,     float )
PATCH_GET_ENV_PARAM( sustain,   float )
PATCH_GET_ENV_PARAM( key_amt,   float )

/* special cases: */
float patch_get_env_release (int patch_id, int eg)
{
    float val;
    assert(patchok(patch_id));
    eg = mod_src_to_eg_index(eg);
    val = patches[patch_id]->env_params[eg].release;
    /* hide usage of min-release-value from outside world */
    if (val <= PATCH_MIN_RELEASE)
        val = 0;
    return val;
}


/************************************************************************/
/*************************** LFO SETTERS ********************************/
/************************************************************************/

static LFOParams* lfopar_from_id(int patch_id, int id, LFO** lfo)
{
    assert(patchok(patch_id));
    assert( ((id & MOD_SRC_VLFO) && (id & MOD_SRC_GLFO)) == 0);
    assert( ((id & MOD_SRC_VLFO) || (id & MOD_SRC_GLFO)) != 0);

    if (lfo)
        *lfo = 0;

    if (id & MOD_SRC_VLFO)
    {
        id -= MOD_SRC_VLFO;
        assert (id < VOICE_MAX_LFOS);
        return &patches[patch_id]->vlfo_params[id];
    }

    id -= MOD_SRC_GLFO;
    assert(id < PATCH_MAX_LFOS);

    if (lfo)
        *lfo = patches[patch_id]->glfo[id];

    return &patches[patch_id]->glfo_params[id];
}


#define PATCH_SET_LFO_VAR( _LFOVAR, _LFOVARTYPE )                       \
int patch_set_lfo_##_LFOVAR(int patch_id, int lfo_id, _LFOVARTYPE val)  \
{                                                   \
    LFO*        lfo;                                \
    LFOParams*  lfopar;                             \
    lfopar = lfopar_from_id(patch_id, lfo_id, &lfo);\
    lfopar->_LFOVAR = val;                          \
    if (lfo)                                        \
        lfo_update_params(lfo, lfopar);             \
    return 0;                                       \
}

PATCH_SET_LFO_VAR( active,   bool)
PATCH_SET_LFO_VAR( positive, bool)
PATCH_SET_LFO_VAR( shape,    LFOShape)
PATCH_SET_LFO_VAR( sync,     bool)


#define PATCH_SET_LFO_VAR_BOUNDED( _LFOVAR, _LFOVARTYPE )               \
int patch_set_lfo_##_LFOVAR(int patch_id, int lfo_id, _LFOVARTYPE val)  \
{                                                   \
    LFO*        lfo;                                \
    LFOParams*  lfopar;                             \
    lfopar = lfopar_from_id(patch_id, lfo_id, &lfo);\
    if (val < 0.0)                                  \
    {                                               \
        pf_error(PF_ERR_PATCH_VALUE_NEGATIVE);      \
        return -1;                                  \
    }                                               \
    lfopar->_LFOVAR = val;                          \
    if (lfo)                                        \
        lfo_update_params(lfo, lfopar);             \
    return 0;                                       \
}

PATCH_SET_LFO_VAR_BOUNDED( attack,     float )
PATCH_SET_LFO_VAR_BOUNDED( sync_beats, float )
PATCH_SET_LFO_VAR_BOUNDED( delay,      float )
PATCH_SET_LFO_VAR_BOUNDED( freq,       float )


/************************************************************************/
/*************************** LFO GETTERS ********************************/
/************************************************************************/

#define PATCH_GET_LFO_VAR( _LFOVAR, _LFOVARTYPE )               \
_LFOVARTYPE patch_get_lfo_##_LFOVAR(int patch_id, int lfo_id)   \
{                                                               \
    return lfopar_from_id(patch_id, lfo_id, NULL)->_LFOVAR;     \
}

PATCH_GET_LFO_VAR( active,     bool )
PATCH_GET_LFO_VAR( attack,     float )
PATCH_GET_LFO_VAR( sync_beats, float )
PATCH_GET_LFO_VAR( delay,      float )
PATCH_GET_LFO_VAR( freq,       float )
PATCH_GET_LFO_VAR( positive,   bool )
PATCH_GET_LFO_VAR( shape,      LFOShape )
PATCH_GET_LFO_VAR( sync,       bool )

/**************************************************************************/
/************************ PARAMETER SETTERS *******************************/
/**************************************************************************/


/* sets the cut signal this patch emits when activated */
int patch_set_cut (int patch_id, int cut)
{
    assert(patchok(patch_id));
    patches[patch_id]->cut = cut;
    return 0;
}

/* sets the cut signal that terminates this patch if active */
int patch_set_cut_by (int patch_id, int cut_by)
{
    assert(patchok(patch_id));
    patches[patch_id]->cut_by = cut_by;
    return 0;
}

/* set whether this patch should be played legato or not */
int patch_set_legato(int patch_id, bool val)
{
    assert(patchok(patch_id));
    patches[patch_id]->legato.active = val;
    return 0;
}


int patch_set_fade_samples(int patch_id, int samples)
{
    assert(patchok(patch_id));

    if (patches[patch_id]->sample->sp == NULL)
        return 0;

    if (samples < 0)
    {
        pf_error(PF_ERR_PATCH_PARAM_VALUE);
        return -1;
    }

    if (patches[patch_id]->play_start + samples * 2
        >= patches[patch_id]->play_stop)
    {
        pf_error(PF_ERR_PATCH_PARAM_VALUE);
        return -1;
    }

    patches[patch_id]->fade_samples = samples;
    return 0;
}

int patch_set_xfade_samples(int patch_id, int samples)
{
    assert(patchok(patch_id));

    if (patches[patch_id]->sample->sp == NULL)
        return 0;

    if (samples < 0)
    {
        pf_error(PF_ERR_PATCH_PARAM_VALUE);
        return -1;
    }

    if (patches[patch_id]->loop_start + samples
      > patches[patch_id]->loop_stop)
    {
        pf_error(PF_ERR_PATCH_PARAM_VALUE);
        return -1;
    }

    if (patches[patch_id]->loop_stop + samples
      > patches[patch_id]->play_stop)
    {
        pf_error(PF_ERR_PATCH_PARAM_VALUE);
        return -1;
    }

    patches[patch_id]->xfade_samples = samples;
    return 0;
}


int patch_set_mark_frame(int patch_id, int mark, int frame)
{
    /* FIXME: perhaps this complexity better placed in libpetrifui? */
    assert(patchok(patch_id));
    assert(marksetok(mark));

    if (patches[patch_id]->sample->sp == NULL)
    {   /* FIXME: assert here? */
        errmsg("sample not set\n");
        return -1;
    }

/*    if (!mark_settable(mark))
    {
        debug("mark %d not settable\n", mark);
        return -1;
    }
*/
    int min;
    int max;

    get_mark_frame_range(patch_id, mark, &min, &max);

    if (frame < min || frame > max)
        return -1;

    set_mark_frame(patch_id, mark, frame);
    return mark;
}


int patch_set_mark_frame_expand(int patch_id, int mark, int frame,
                                                     int* also_changed)
{
    int also = 0;
    int also_frame = -1;
    int xfade = patches[patch_id]->xfade_samples;
    int fade = patches[patch_id]->fade_samples;

    assert(patchok(patch_id));

    if (patches[patch_id]->sample->sp == NULL)
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

        if (frame + fade * 2 >= get_mark_frame(patch_id, WF_MARK_PLAY_STOP)
         || also_frame >= get_mark_frame(patch_id, WF_MARK_LOOP_STOP))
        {
            mark = -1;
        }
        else if (also_frame > get_mark_frame(patch_id, WF_MARK_LOOP_START))
        {   /* moving play start along pushes loop start along... */
            if (also_frame + xfade
                < get_mark_frame(patch_id, WF_MARK_LOOP_STOP))
            {
                *also_changed = WF_MARK_LOOP_START;
            }
            else
                mark = -1;
        }
        break;

    case WF_MARK_PLAY_STOP:
        also_frame = frame - xfade; /* pot loop stop pos */

        if (frame - fade * 2<= get_mark_frame(patch_id, WF_MARK_PLAY_START)
         || also_frame <= get_mark_frame(patch_id, WF_MARK_LOOP_START))
        {
            mark = -1;
        }
        else if (also_frame < get_mark_frame(patch_id, WF_MARK_LOOP_STOP))
        {
            if (also_frame - xfade
                > get_mark_frame(patch_id, WF_MARK_LOOP_START))
            {
                *also_changed = WF_MARK_LOOP_STOP;
            }
            else
                mark = -1;
        }
        break;

    case WF_MARK_LOOP_START:
        also_frame = frame - xfade; /* pot play start pos */

        if (frame + xfade >= get_mark_frame(patch_id, WF_MARK_LOOP_STOP))
            mark = -1;
        else if (also_frame < get_mark_frame(patch_id, WF_MARK_PLAY_START))
        {
            if (also_frame > 0)
                *also_changed = WF_MARK_PLAY_START;
            else
                mark = -1;
        }
        break;

    case WF_MARK_LOOP_STOP:
        also_frame = frame + xfade /* pot play stop pos */;

        if (frame - xfade <= get_mark_frame(patch_id, WF_MARK_LOOP_START))
            mark = -1;
        else if (also_frame > get_mark_frame(patch_id, WF_MARK_PLAY_STOP))
        {
            if (also_frame < get_mark_frame(patch_id, WF_MARK_STOP))
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
        set_mark_frame(patch_id, mark, frame);
    }

    if (*also_changed != -1)
    {
        set_mark_frame(patch_id, *also_changed, also_frame);
    }

    return mark;
}


/* sets the name */
int patch_set_name (int patch_id, const char *name)
{
    assert(patchok(patch_id));
    strncpy (patches[patch_id]->name, name, PATCH_MAX_NAME);
    return 0;
}


#define PATCH_SET_VAR( _VAR, _VARMIN, _VARMAX ) \
int patch_set_##_VAR(int patch_id, int val)     \
{                                               \
    assert(patchok(patch_id));                     \
    if (val < _VARMIN || val > _VARMAX)         \
    {                                           \
        pf_error(PF_ERR_PATCH_PARAM_VALUE);     \
        return -1;                              \
    }                                           \
    patches[patch_id]->_VAR = val;              \
    return 0;                                   \
}

PATCH_SET_VAR( channel,     0,  15 )
PATCH_SET_VAR( root_note,   0,  127 )
PATCH_SET_VAR( lower_note,  0,  127 )
PATCH_SET_VAR( upper_note,  0,  127 )
PATCH_SET_VAR( lower_vel,   0,  127 )
PATCH_SET_VAR( upper_vel,   0,  127 )

PATCH_SET_VAR( pitch_steps, -PATCH_MAX_PITCH_STEPS, PATCH_MAX_PITCH_STEPS )

/* set whether the patch is monophonic or not */
int patch_set_monophonic(int patch_id, bool val)
{
    assert(patchok(patch_id));
    patches[patch_id]->mono = val;
    return 0;
}


#define PATCH_SET_PARAM( _PARAM, _PARMIN, _PARMAX ) \
int patch_set_##_PARAM(int patch_id, float val)     \
{                                                   \
    assert(patchok(patch_id));                         \
    if (val < _PARMIN || val > _PARMAX)             \
    {                                               \
        pf_error(PF_ERR_PATCH_PARAM_VALUE);         \
        return -1;                                  \
    }                                               \
    patches[patch_id]->_PARAM.val = val;            \
    return 0;                                       \
}

PATCH_SET_PARAM( amp,       0.0,    1.0 )
PATCH_SET_PARAM( pan,      -1.0,    1.0 )
PATCH_SET_PARAM( ffreq,     0.0,    1.0 )
PATCH_SET_PARAM( freso,     0.0,    1.0 )
PATCH_SET_PARAM( pitch,    -1.0,    1.0 )


/* sets the play mode */
int patch_set_play_mode (int patch_id, PatchPlayMode mode)
{
    assert(patchok(patch_id));

    if (mode & PATCH_PLAY_SINGLESHOT)
    {
        assert( ((mode & PATCH_PLAY_TRIM)
               | (mode & PATCH_PLAY_LOOP)) == 0);
    }
    else if (mode & PATCH_PLAY_TRIM)
    {
        assert( ((mode & PATCH_PLAY_SINGLESHOT)
               | (mode & PATCH_PLAY_LOOP)) == 0);
    }
    else if (mode & PATCH_PLAY_LOOP)
    {
        assert( ((mode & PATCH_PLAY_SINGLESHOT)
               | (mode & PATCH_PLAY_TRIM)) == 0);
    }

    if (!(mode & PATCH_PLAY_LOOP))
    {
        assert( ((mode & PATCH_PLAY_PINGPONG)
               | (mode & PATCH_PLAY_TO_END)) == 0);
    }

    patches[patch_id]->play_mode = mode;
    return 0;
}

/* set whether portamento is being used or not */
int patch_set_portamento (int patch_id, bool val)
{
    assert(patchok(patch_id));
    patches[patch_id]->porta.active = val;
    return 0;
}

/* set length of portamento slides in seconds */
int patch_set_portamento_time (int patch_id, float secs)
{
    assert(patchok(patch_id));
    if (secs < 0.0)
    {
        pf_error(PF_ERR_PATCH_PARAM_VALUE);
        return -1;
    }
    patches[patch_id]->porta_secs.val = secs;
    return 0;
}



/**************************************************************************/
/************************* PARAMETER GETTERS*******************************/
/**************************************************************************/


#define PATCH_GET_VAR( _VAR )       \
int patch_get_##_VAR(int patch_id)  \
{                                   \
    assert(patchok(patch_id));         \
    return patches[patch_id]->_VAR; \
}

PATCH_GET_VAR( channel )
PATCH_GET_VAR( cut )
PATCH_GET_VAR( cut_by )
PATCH_GET_VAR( display_index )
PATCH_GET_VAR( root_note )
PATCH_GET_VAR( lower_note )
PATCH_GET_VAR( upper_note )
PATCH_GET_VAR( lower_vel )
PATCH_GET_VAR( upper_vel )
PATCH_GET_VAR( pitch_steps )



/* get the filter cutoff value */
float patch_get_cutoff(int patch_id)
{
    assert(patchok(patch_id));
    return patches[patch_id]->ffreq.val;
}


/* get the number of frame in the sample */
int patch_get_frames(int patch_id)
{
    assert(patchok(patch_id));
    if (patches[patch_id]->sample->sp == NULL)
        return 0;
    return patches[patch_id]->sample->frames;
}

/* get whether this patch is played legato or not */
bool patch_get_legato(int patch_id)
{
    assert(patchok(patch_id));
    return patches[patch_id]->legato.active;
}


int patch_get_mark_frame(int patch_id, int mark)
{
    assert(patchok(patch_id));
    assert(markok(mark));

    /* FIXME: should this be an assert or not ? */
    assert(patches[patch_id]->sample->sp != NULL);

    return get_mark_frame(patch_id, mark);
}


int patch_get_mark_frame_range(int patch_id, int mark, int* frame_min,
                                                    int* frame_max)
{
    assert(patchok(patch_id));
    assert(markok(mark));

    /* FIXME: should this be an assert or not ? */
    assert(patches[patch_id]->sample->sp != NULL);

    return get_mark_frame_range(patch_id, mark, frame_min, frame_max);
}

/* get whether this patch is monophonic or not */
bool patch_get_monophonic(int patch_id)
{
    assert(patchok(patch_id));
    return patches[patch_id]->mono;
}

/* get the name */
const char *patch_get_name(int patch_id)
{
    assert(patchok(patch_id));
    return patches[patch_id]->name;
}


/* get the panorama */
float patch_get_panning(int patch_id)
{
    assert(patchok(patch_id));
    return patches[patch_id]->pan.val;
}

/* get the pitch */
float patch_get_pitch(int patch_id)
{
    assert(patchok(patch_id));
    return patches[patch_id]->pitch.val;
}

/* get the play mode */
PatchPlayMode patch_get_play_mode(int patch_id)
{
    assert(patchok(patch_id));
    return patches[patch_id]->play_mode;
}

/* get whether portamento is used or not */
bool patch_get_portamento(int patch_id)
{
    assert(patchok(patch_id));
    return patches[patch_id]->porta.active;
}

/* get length of portamento slides in seconds */
float patch_get_portamento_time(int patch_id)
{
    assert(patchok(patch_id));
    return patches[patch_id]->porta_secs.val;
}


/* get the filter's resonance amount */
float patch_get_resonance(int patch_id)
{
    assert(patchok(patch_id));
    return patches[patch_id]->freso.val;
}

/* get a pointer to the sample data */
const float *patch_get_sample(int patch_id)
{
    assert(patchok(patch_id));
    return patches[patch_id]->sample->sp;
}

/* get the name of the sample file */
const char *patch_get_sample_name(int patch_id)
{
    assert(patchok(patch_id));
    return patches[patch_id]->sample->filename;
}


/* get the amplitude */
float patch_get_amplitude(int patch_id)
{
    assert(patchok(patch_id));
    return patches[patch_id]->amp.val;
}


int patch_get_fade_samples(int patch_id)
{
    assert(patchok(patch_id));
    return patches[patch_id]->fade_samples;
}


int patch_get_xfade_samples(int patch_id)
{
    assert(patchok(patch_id));
    return patches[patch_id]->xfade_samples;
}


int patch_get_max_fade_samples(int patch_id)
{
    assert(patchok(patch_id));
    return (patches[patch_id]->play_stop - patches[patch_id]->play_start) / 2;
}


int patch_get_max_xfade_samples(int patch_id)
{
    int min;
    int tmp;

    assert(patchok(patch_id));

    min = patches[patch_id]->sample->frames;

    tmp = patches[patch_id]->loop_stop - patches[patch_id]->loop_start;
    min = (tmp < min) ? tmp : min;

    tmp = patches[patch_id]->play_stop - patches[patch_id]->loop_stop;
    min = (tmp < min) ? tmp : min;

    tmp = patches[patch_id]->loop_start - patches[patch_id]->play_start;
    min = (tmp < min) ? tmp : min;

    return min;
}


/******************************************************************/
/*************************** PARAM ********************************/
/******************************************************************/

float patch_param_get_value(int patch_id, PatchParamType param)
{
    assert(patchok(patch_id));

    switch(param)
    {
    case PATCH_PARAM_AMPLITUDE: return patches[patch_id]->amp.val;
    case PATCH_PARAM_PANNING:   return patches[patch_id]->pan.val;
    case PATCH_PARAM_CUTOFF:    return patches[patch_id]->ffreq.val;
    case PATCH_PARAM_RESONANCE: return patches[patch_id]->freso.val;
    case PATCH_PARAM_PITCH:     return patches[patch_id]->pitch.val;
    default:
        assert(0);
    }
    /* prevent "program returns random data in a function" */
    return 0;
}


void patch_param_set_value(int patch_id, PatchParamType param, float v)
{
    assert(patchok(patch_id));

    switch(param)
    {
    case PATCH_PARAM_AMPLITUDE: patches[patch_id]->amp.val = v;     break;
    case PATCH_PARAM_PANNING:   patches[patch_id]->pan.val = v;     break;
    case PATCH_PARAM_CUTOFF:    patches[patch_id]->ffreq.val = v;   break;
    case PATCH_PARAM_RESONANCE: patches[patch_id]->freso.val = v;   break;
    case PATCH_PARAM_PITCH:     patches[patch_id]->pitch.val = v;   break;
    default:
        assert(0);
    }
}


/**************************************************************************/
/*********************** MODULATION SETTERS *******************************/
/**************************************************************************/

#define PATCH_PARAM_CHECKS  \
    PatchParam* p;          \
    assert(patchok(patch_id)); \
    p = get_patch_param(patch_id, param);

int
patch_param_set_mod_src(int patch_id, PatchParamType param, int slot,
                                                            int id)
{
    PATCH_PARAM_CHECKS
    assert(slot >=0 && slot <= MAX_MOD_SLOTS);
    p->mod_id[slot] = id;
    return 0;
}


int
patch_param_set_mod_amt(int patch_id, PatchParamType param, int slot,
                                                            float amt)
{
    PATCH_PARAM_CHECKS
    assert(slot >=0 && slot <= MAX_MOD_SLOTS);

    if (amt < -1.0 || amt > 1.0)
    {
        pf_error(PF_ERR_PATCH_PARAM_VALUE);
        return -1;
    }

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


#define PATCH_PARAM_SET_AMOUNT( _PARAM )            \
int patch_param_set_##_PARAM##_amount               \
    (int patch_id, PatchParamType param, float amt) \
{                                           \
    PATCH_PARAM_CHECKS                      \
    if (amt < -1.0 || amt > 1.0)            \
    {                                       \
        pf_error(PF_ERR_PATCH_PARAM_VALUE); \
        return -1;          \
    }                       \
    p->_PARAM##_amt = amt;  \
    return 0;               \
}

PATCH_PARAM_SET_AMOUNT( vel )
PATCH_PARAM_SET_AMOUNT( key )



/**************************************************************************/
/********************** MODULATION GETTERS ********************************/
/**************************************************************************/

int patch_param_get_mod_src(int patch_id, PatchParamType param, int slot)
{
    PATCH_PARAM_CHECKS
    assert(slot >=0 && slot <= MAX_MOD_SLOTS);
    return p->mod_id[slot];
}


float patch_param_get_mod_amt(int patch_id, PatchParamType param, int slot)
{
    PATCH_PARAM_CHECKS
    assert(slot >=0 && slot <= MAX_MOD_SLOTS);
    return p->mod_amt[slot];
}


float patch_param_get_vel_amount(int patch_id, PatchParamType param)
{
    PATCH_PARAM_CHECKS
    return p->vel_amt;
}


float patch_param_get_key_amount(int patch_id, PatchParamType param)
{
    PATCH_PARAM_CHECKS
    return p->key_amt;
}


#define PATCH_BOOL_GET                      \
    PatchBool* b;                           \
    assert(patchok(patch_id));                 \
    b = get_patch_bool(patch_id, booltype); \


/* PatchBool set/get */
void patch_bool_set_active(int patch_id, PatchBoolType booltype, bool val)
{
    PATCH_BOOL_GET
    b->active = val;
}


void patch_bool_set_thresh(int patch_id, PatchBoolType booltype, float val)
{
    PATCH_BOOL_GET
    b->thresh = val;
}


void patch_bool_set_mod_src(int patch_id, PatchBoolType booltype,int mod_id)
{
    PATCH_BOOL_GET
    b->mod_id = mod_id;
}


void patch_bool_get_all(int patch_id, PatchBoolType booltype,
                        bool* active, float* thresh, int* mod_id)
{
    PATCH_BOOL_GET
    *active = b->active;
    *thresh = b->thresh;
    *mod_id = b->mod_id;
}


bool patch_bool_get_active(int patch_id, PatchBoolType booltype)
{
    PATCH_BOOL_GET
    return b->active;
}


float patch_bool_get_thresh(int patch_id, PatchBoolType booltype)
{
    PATCH_BOOL_GET
    return b->thresh;
}


int patch_bool_get_mod_src(int patch_id, PatchBoolType booltype)
{
    PATCH_BOOL_GET
    return b->mod_id;
}

#define PATCH_FLOAT_GET                     \
    PatchFloat* f;                          \
    assert(patchok(patch_id));                 \
    f = get_patch_float(patch_id, floattype);


/* PatchFloat set/get */
void
patch_float_set_value(int patch_id, PatchFloatType floattype, float val)
{
    PATCH_FLOAT_GET
    f->val = val;
}


void
patch_float_set_mod_amt(int patch_id, PatchFloatType floattype, float val)
{
    PATCH_FLOAT_GET
    f->mod_amt = val;
}


void
patch_float_set_mod_src(int patch_id, PatchFloatType floattype, int mod_id)
{
    PATCH_FLOAT_GET
    f->mod_id = mod_id;
}


void patch_float_get_all(int patch_id, PatchFloatType floattype,
                        float* value, float* mod_amt, int* mod_id)
{
    PATCH_FLOAT_GET
    *value =    f->val;
    *mod_amt =  f->mod_amt;
    *mod_id =   f->mod_id;
}


float patch_float_get_value(int patch_id, PatchFloatType floattype)
{
    PATCH_FLOAT_GET
    return f->val;
}


float patch_float_get_mod_amt(int patch_id, PatchFloatType floattype)
{
    PATCH_FLOAT_GET
    return f->mod_amt;
}


int patch_float_get_mod_src(int patch_id, PatchFloatType floattype)
{
    PATCH_FLOAT_GET
    return f->mod_id;
}



/**************************************************************************/
/****************** LFO FREQ MODULATION SETTERS ***************************/
/**************************************************************************/

#define PATCH_LFO_CHECKS                                    \
    LFO* lfo;                                               \
    LFOParams* lfopar;                                      \
    if (!(lfopar = lfopar_from_id(patch_id, lfo_id, &lfo))) \
        return -1;

#define PATCH_NULL_LFO_CHECKS                               \
    LFOParams* lfopar;                                      \
    if (!(lfopar = lfopar_from_id(patch_id, lfo_id, NULL))) \
        return -1;


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

int patch_get_lfo_fm1_src(int patch_id, int lfo_id)
{
    PATCH_NULL_LFO_CHECKS
    return lfopar->fm1_id;
}

int patch_get_lfo_fm2_src(int patch_id, int lfo_id)
{
    PATCH_NULL_LFO_CHECKS
    return lfopar->fm2_id;
}

float patch_get_lfo_fm1_amt(int patch_id, int lfo_id)
{
    PATCH_NULL_LFO_CHECKS
    return lfopar->fm1_amt;
}

float patch_get_lfo_fm2_amt(int patch_id, int lfo_id)
{
    PATCH_NULL_LFO_CHECKS
    return lfopar->fm2_amt;
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

int patch_get_lfo_am1_src(int patch_id, int lfo_id)
{
    PATCH_NULL_LFO_CHECKS
    return lfopar->am1_id;
}

int patch_get_lfo_am2_src(int patch_id, int lfo_id)
{
    PATCH_NULL_LFO_CHECKS
    return lfopar->am2_id;
}

float patch_get_lfo_am1_amt(int patch_id, int lfo_id)
{
    PATCH_NULL_LFO_CHECKS
    return lfopar->am1_amt;
}

float patch_get_lfo_am2_amt(int patch_id, int lfo_id)
{
    PATCH_NULL_LFO_CHECKS
    return lfopar->am2_amt;
}


