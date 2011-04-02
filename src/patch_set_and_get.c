#include "patch_set_and_get.h"


#include <string.h>
#include <math.h>


#include "sample.h"
#include "adsr.h"
#include "lfo.h"


#include "patch_private/patch_data.h"
#include "patch_private/patch_defs.h"


static float one = 1.0;


/* verifies that a given id refers to a valid patch */
inline static int isok (int id)
{
    if (id < 0 || id >= PATCH_COUNT || !patches[id].active)
	return 0;

    return 1;
}


inline static int bad_env(int patch_id, int env_id)
{
    if (!isok(patch_id))
        return PATCH_ID_INVALID;

    if (env_id < 0 || env_id >= VOICE_MAX_ENVS)
        return PATCH_ENV_ID_INVALID;

    return 0;
}


inline static int bad_lfo(int patch_id, int lfo_id)
{
    if (!isok(patch_id))
        return PATCH_ID_INVALID;

    if (lfo_id < 0 || lfo_id >= TOTAL_LFOS)
        return PATCH_LFO_ID_INVALID;

    return 0;
}


inline static int mod_src_ok(int id)
{
    if (id < MOD_SRC_NONE || id >= MOD_SRC_LAST)
        return PATCH_MOD_SRC_INVALID;
    return 0;
}


inline static gboolean mark_ok(int id)
{
    if (id < WF_MARK_START || id > WF_MARK_STOP)
        return FALSE;
    return TRUE;
}

inline static gboolean mark_settable(int id)
{
    if (id < WF_MARK_PLAY_START || id > WF_MARK_PLAY_STOP)
        return FALSE;
    return TRUE;
}


static inline void set_mark_frame(int patch, int mark, int frame)
{
    *(patches[patch].marks[mark]) = frame;
}


static inline int get_mark_frame(int patch, int mark)
{
    return *(patches[patch].marks[mark]);
}


static int get_mark_frame_range(int patch, int mark, int* min, int* max)
{
    int xfade = patches[patch].xfade_samples;

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
    case PATCH_PARAM_AMPLITUDE: *p = &patches[patch_id].vol;      break;
    case PATCH_PARAM_PANNING:   *p = &patches[patch_id].pan;      break;
    case PATCH_PARAM_CUTOFF:    *p = &patches[patch_id].ffreq;    break;
    case PATCH_PARAM_RESONANCE: *p = &patches[patch_id].freso;    break;
    case PATCH_PARAM_PITCH:     *p = &patches[patch_id].pitch;    break;
    default:
        debug ("Invalid request for address of param\n");
        return PATCH_PARAM_INVALID;
    }

    return 0;
}


gboolean patch_lfo_is_global(int lfo_id)
{
    if (lfo_id >= 0 && lfo_id < MOD_SRC_LAST_GLFO - MOD_SRC_FIRST_GLFO)
    {
        debug("lfo_is_global:input:%d TRUE\n", lfo_id)
        return TRUE;
    }

    debug("lfo_is_global:input:%d FALSE\n", lfo_id)
    return FALSE;
}


/* inline static function def macro, see private/patch_data.h */
INLINE_PATCH_TRIGGER_GLOBAL_LFO_DEF


float* mod_id_to_pointer(int id, Patch* p, PatchVoice* v)
{
    if (!id)
        return 0;


    if (id == MOD_SRC_ONE)
        return &one;

    if (id >= MOD_SRC_FIRST_EG
     && id <  MOD_SRC_LAST_EG)
    {
        int env_id = id - MOD_SRC_FIRST_EG;
        return &v->env[env_id].val;
    }

    if (id >= MOD_SRC_FIRST_GLFO
     && id <  MOD_SRC_LAST_GLFO)
    {
        int lfo_id = id - MOD_SRC_FIRST_GLFO;
        return &p->glfo[lfo_id].val;
    }

    if (id >= MOD_SRC_FIRST_VLFO
     && id <  MOD_SRC_LAST_VLFO)
    {
        int lfo_id = id - MOD_SRC_FIRST_VLFO;
        return &v->lfo[lfo_id].val;
    }

    debug("unknown modulation source:%d\n", id);

    return 0;
}


/*****************************************************************************/
/*************************** ENVELOPE SETTERS ********************************/
/*****************************************************************************/


int patch_set_env_on (int patch_id, int env_id, gboolean state)
{
    int err;

    if ((err = bad_env(patch_id, env_id)))
        return err;

    patches[patch_id].env_params[env_id].env_on = state;
    return 0;
}


/* sets the delay length in seconds */
int patch_set_env_delay (int patch_id, int env_id, float secs)
{
    int err;

    if ((err = bad_env(patch_id, env_id)))
        return err;

    if (secs < 0.0)
        return PATCH_PARAM_INVALID;

    patches[patch_id].env_params[env_id].delay = secs;
    return 0;
}

/* sets the attack length in seconds */
int patch_set_env_attack (int patch_id, int env_id, float secs)
{
    int err;

    if ((err = bad_env(patch_id, env_id)))
        return err;

    if (secs < 0.0)
        return PATCH_PARAM_INVALID;

    patches[patch_id].env_params[env_id].attack = secs;
    return 0;
}

/* sets the hold length in seconds */
int patch_set_env_hold (int patch_id, int env_id, float secs)
{
    int err;

    if ((err = bad_env(patch_id, env_id)))
        return err;

    if (secs < 0.0)
        return PATCH_PARAM_INVALID;

    patches[patch_id].env_params[env_id].hold = secs;
    return 0;
}

/* sets the decay length in seconds */
int patch_set_env_decay (int patch_id, int env_id, float secs)
{
    int err;

    if ((err = bad_env(patch_id, env_id)))
        return err;

    if (secs < 0.0)
        return PATCH_PARAM_INVALID;

    patches[patch_id].env_params[env_id].decay = secs;
    return 0;
}

/* sets the sustain level */
int patch_set_env_sustain (int patch_id, int env_id, float level)
{
    int err;

    if ((err = bad_env(patch_id, env_id)))
        return err;

    if (level < 0.0 || level > 1.0)
        return PATCH_PARAM_INVALID;

    patches[patch_id].env_params[env_id].sustain = level;
    return 0;
}

/* sets the release length in seconds */
int patch_set_env_release (int patch_id, int env_id, float secs)
{
    int err;

    if ((err = bad_env(patch_id, env_id)))
        return err;

    if (secs < 0.0)
        return PATCH_PARAM_INVALID;

    if (secs < PATCH_MIN_RELEASE)
        secs = PATCH_MIN_RELEASE;

    patches[patch_id].env_params[env_id].release = secs;
    return 0;
}

/*****************************************************************************/
/*************************** ENVELOPE GETTERS ********************************/
/*****************************************************************************/

/* places the delay length in seconds into val */
int patch_get_env_on(int patch_id, int env_id, gboolean* val)
{
    int err;
    if ((err = bad_env(patch_id, env_id)))
        return err;
    *val = patches[patch_id].env_params[env_id].env_on;
    return 0;
}



/* places the delay length in seconds into val */
int patch_get_env_delay (int patch_id, int env_id, float* val)
{
    int err;
    if ((err = bad_env(patch_id, env_id)))
        return err;
    *val = patches[patch_id].env_params[env_id].delay;
    return 0;
}


/* places the attack length in seconds into val */
int patch_get_env_attack (int patch_id, int env_id, float* val)
{
    int err;
    if ((err = bad_env(patch_id, env_id)))
        return err;
    *val = patches[patch_id].env_params[env_id].attack;
    return 0;
}


/* places the hold length in seconds into val */
int patch_get_env_hold (int patch_id, int env_id, float* val)
{
    int err;
    if ((err = bad_env(patch_id, env_id)))
        return err;
    *val = patches[patch_id].env_params[env_id].hold;
    return 0;
}


/* places the decay length in seconds into val */
int patch_get_env_decay (int patch_id, int env_id, float* val)
{
    int err;
    if ((err = bad_env(patch_id, env_id)))
        return err;
    *val = patches[patch_id].env_params[env_id].decay;
    return 0;
}


/* places the sustain level into val */
int patch_get_env_sustain (int patch_id, int env_id, float* val)
{
    int err;
    if ((err = bad_env(patch_id, env_id)))
        return err;
    *val = patches[patch_id].env_params[env_id].sustain;
    return 0;
}

/* places the release length in seconds into val */
int patch_get_env_release (int patch_id, int env_id, float* val)
{
    int err;
    if ((err = bad_env(patch_id, env_id)))
        return err;
    *val = patches[patch_id].env_params[env_id].release;

    /* we hide the fact that we have a min release value from the
     * outside world (they're on a need-to-know basis) */
    if (*val <= PATCH_MIN_RELEASE)
        *val = 0;
    return 0;
}

/************************************************************************/
/*************************** LFO SETTERS ********************************/
/************************************************************************/

int lfo_from_id(int patch_id, int lfo_id, LFO** lfo, LFOParams** lfopar)
{
    if (lfo)
        *lfo = 0;

    *lfopar = 0;

    if (!isok(patch_id))
        return PATCH_ID_INVALID;

    if (lfo_id < 0 || lfo_id >= TOTAL_LFOS)
        return PATCH_LFO_ID_INVALID;

    if (lfo_id < PATCH_MAX_LFOS)
    {
        if (lfo)
            *lfo = &patches[patch_id].glfo[lfo_id];
        *lfopar = &patches[patch_id].glfo_params[lfo_id];
    }
    else
    {
        /* we don't know voice information so can't set *lfo */
        *lfopar = &patches[patch_id].vlfo_params[lfo_id - PATCH_MAX_LFOS];
    }

    return 0;
}

int patch_set_lfo_on (int patch_id, int lfo_id, gboolean state)
{
    LFO*        lfo;
    LFOParams*  lfopar;
    int         err;

    if ((err = lfo_from_id(patch_id, lfo_id, &lfo, &lfopar)))
        return err;

    lfopar->lfo_on = state;

    if (lfo)
        lfo_trigger(lfo, lfopar);

    return 0;
}

/* set the attack time of the param's LFO */
int patch_set_lfo_attack (int patch_id, int lfo_id, float secs)
{
    LFO*        lfo;
    LFOParams*  lfopar;
    int         err;

    if ((err = lfo_from_id(patch_id, lfo_id, &lfo, &lfopar)))
        return err;

    if (secs < 0.0)
        return PATCH_PARAM_INVALID;

    lfopar->attack = secs;

    if (lfo)
        lfo_trigger(lfo, lfopar);

    return 0;
}

/* set the period length of the param's lfo in beats */
int patch_set_lfo_beats (int patch_id, int lfo_id, float beats)
{
    LFO*        lfo;
    LFOParams*  lfopar;
    int         err;

    if ((err = lfo_from_id(patch_id, lfo_id, &lfo, &lfopar)))
        return err;

    if (beats < 0.0)
        return PATCH_PARAM_INVALID;

    lfopar->sync_beats = beats;

    if (lfo)
        lfo_trigger(lfo, lfopar);

    return 0;
}

/* set the delay time of the param's LFO */
int patch_set_lfo_delay (int patch_id, int lfo_id, float secs)
{
    LFO*        lfo;
    LFOParams*  lfopar;
    int         err;

    if ((err = lfo_from_id(patch_id, lfo_id, &lfo, &lfopar)))
        return err;

    if (secs < 0.0)
        return PATCH_PARAM_INVALID;

    lfopar->delay = secs;

    if (lfo)
        lfo_trigger(lfo, lfopar);

    return 0;
}

/* set the frequency of the param's lfo */
int patch_set_lfo_freq (int patch_id, int lfo_id, float freq)
{
    LFO*        lfo;
    LFOParams*  lfopar;
    int         err;

    if ((err = lfo_from_id(patch_id, lfo_id, &lfo, &lfopar)))
        return err;

    if (freq < 0.0)
        return PATCH_PARAM_INVALID;

    lfopar->freq = freq;

    if (lfo)
        lfo_trigger(lfo, lfopar);

    return 0;
}


/* set whether to constrain the param's LFOs to positive values or not */
int patch_set_lfo_positive (int patch_id, int lfo_id, gboolean state)
{
    LFO*        lfo;
    LFOParams*  lfopar;
    int         err;

    if ((err = lfo_from_id(patch_id, lfo_id, &lfo, &lfopar)))
        return err;

    lfopar->positive = state;

    if (lfo)
        lfo_trigger(lfo, lfopar);

    return 0;
}


/* set the param's lfo shape */
int patch_set_lfo_shape (int patch_id, int lfo_id, LFOShape shape)
{
    LFO*        lfo;
    LFOParams*  lfopar;
    int         err;

    if ((err = lfo_from_id(patch_id, lfo_id, &lfo, &lfopar)))
        return err;

    lfopar->shape = shape;

    if (lfo)
        lfo_trigger(lfo, lfopar);

    return 0;
}

/* set whether to the param's lfo should sync to tempo or not */
int patch_set_lfo_sync (int patch_id, int lfo_id, gboolean state)
{
    LFO*        lfo;
    LFOParams*  lfopar;
    int         err;

    if ((err = lfo_from_id(patch_id, lfo_id, &lfo, &lfopar)))
        return err;

    lfopar->sync = state;

    if (lfo)
        lfo_trigger(lfo, lfopar);

    return 0;
}

/************************************************************************/
/*************************** LFO GETTERS ********************************/
/************************************************************************/

int patch_get_lfo_on(int patch_id, int lfo_id, gboolean* val)
{
    LFOParams*  lfopar;
    int         err;
    if ((err = lfo_from_id(patch_id, lfo_id, NULL, &lfopar)))
        return err;
    *val = lfopar->lfo_on;
    return 0;
}


/* get the attack time of the param's LFO */
int patch_get_lfo_attack (int patch_id, int lfo_id, float* val)
{
    LFOParams*  lfopar;
    int         err;
    if ((err = lfo_from_id(patch_id, lfo_id, NULL, &lfopar)))
        return err;
    *val = lfopar->attack;
    return 0;
}

/* get the param's lfo period length in beats */
int patch_get_lfo_beats (int patch_id, int lfo_id, float* val)
{
    LFOParams*  lfopar;
    int         err;
    if ((err = lfo_from_id(patch_id, lfo_id, NULL, &lfopar)))
        return err;
    *val = lfopar->sync_beats;
    return 0;
}

/* get the delay time of the param's LFO */
int patch_get_lfo_delay (int patch_id, int lfo_id, float* val)
{
    LFOParams*  lfopar;
    int         err;
    if ((err = lfo_from_id(patch_id, lfo_id, NULL, &lfopar)))
        return err;
    *val = lfopar->delay;
    return 0;
}

/* get the param's lfo frequency */
int patch_get_lfo_freq (int patch_id, int lfo_id, float* val)
{
    LFOParams*  lfopar;
    int         err;
    if ((err = lfo_from_id(patch_id, lfo_id, NULL, &lfopar)))
        return err;
    *val = lfopar->freq;
    return 0;
}

int patch_get_lfo_positive (int patch_id, int lfo_id, gboolean* val)
{
    LFOParams*  lfopar;
    int         err;
    if ((err = lfo_from_id(patch_id, lfo_id, NULL, &lfopar)))
        return err;
    *val = lfopar->positive;
    return 0;
}

/* get param's lfo shape */
int patch_get_lfo_shape (int patch_id, int lfo_id, LFOShape* val)
{
    LFOParams*  lfopar;
    int         err;
    if ((err = lfo_from_id(patch_id, lfo_id, NULL, &lfopar)))
        return err;
    *val = lfopar->shape;
    return 0;
}

/* get whether param's lfo is tempo synced or not */
int patch_get_lfo_sync (int patch_id, int lfo_id, gboolean* val)
{
    LFOParams*  lfopar;
    int         err;
    if ((err = lfo_from_id(patch_id, lfo_id, NULL, &lfopar)))
        return err;
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

    patches[id].channel = channel;
    return 0;
}

/* sets the cut signal this patch emits when activated */
int patch_set_cut (int id, int cut)
{
    if (!isok (id))
        return PATCH_ID_INVALID;
    patches[id].cut = cut;
    return 0;
}

/* sets the cut signal that terminates this patch if active */
int patch_set_cut_by (int id, int cut_by)
{
    if (!isok (id))
        return PATCH_ID_INVALID;
    patches[id].cut_by = cut_by;
    return 0;
}

/* sets filter cutoff frequency */
int patch_set_cutoff (int id, float freq)
{
    if (!isok (id))
        return PATCH_ID_INVALID;

    if (freq < 0.0 || freq > 1.0)
	return PATCH_PARAM_INVALID;

    patches[id].ffreq.val = freq;
    return 0;
}

/* set whether this patch should be played legato or not */
int patch_set_legato(int id, gboolean val)
{
    if (!isok (id))
        return PATCH_ID_INVALID;
    patches[id].legato = val;
    return 0;
}


int patch_set_fade_samples(int id, int samples)
{
    if (!isok (id))
        return PATCH_ID_INVALID;

    if (patches[id].sample->sp == NULL)
        return 0;

    if (samples < 0)
    {
        debug ("refusing to set negative fade length\n");
        return PATCH_PARAM_INVALID;
    }

    if (patches[id].play_start + samples * 2 >= patches[id].play_stop)
    {
        debug ("refusing to set fade length greater than half the "
               " number of samples between play start and play stop\n");
        return PATCH_PARAM_INVALID;
    }

    patches[id].fade_samples = samples;
    return 0;
}

int patch_set_xfade_samples(int id, int samples)
{
    if (!isok (id))
        return PATCH_ID_INVALID;

    if (patches[id].sample->sp == NULL)
        return 0;

    if (samples < 0)
    {
        debug ("refusing to set negative xfade length\n");
        return PATCH_PARAM_INVALID;
    }

    if (patches[id].loop_start + samples >= patches[id].loop_stop)
    {
        debug ("refusing to set xfade length greater than samples"
               " between play start and play stop\n");
        return PATCH_PARAM_INVALID;
    }

    if (patches[id].loop_stop + samples > patches[id].play_stop)
    {
        debug ("refusing to set xfade length greater than samples"
               " between loop stop and play stop\n");
        return PATCH_PARAM_INVALID;
    }

debug("setting xfade to %d\n",samples);
    patches[id].xfade_samples = samples;
    return 0;
}


/* sets the lower note of a patch's range */
int patch_set_lower_note (int id, int note)
{
    if (!isok (id))
	return PATCH_ID_INVALID;

    if (note < 0 || note > 127)
	return PATCH_NOTE_INVALID;

    patches[id].lower_note = note;
    return 0;
}


int patch_set_mark_frame(int patch, int mark, int frame)
{
    if (!isok(patch))
        return -1;

    if (patches[patch].sample->sp == NULL)
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
    int xfade = patches[patch].xfade_samples;
    int fade = patches[patch].fade_samples;

    if (!isok(patch))
        return -1;

    if (patches[patch].sample->sp == NULL)
        return -1;

    /*  if callee wishes not to be informed about which marks get changed
        as a result of changing this one, also_changed will be NULL. It
        needs to be a valid pointer.
     */
    if (!also_changed)
        also_changed = &also;

    *also_changed = -1;
    int also_frame = -1;

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
int patch_set_monophonic(int id, gboolean val)
{
    if (!isok (id))
	return PATCH_ID_INVALID;

    patches[id].mono = val;
    return 0;
}

/* sets the name */
int patch_set_name (int id, const char *name)
{
    if (!isok (id))
	return PATCH_ID_INVALID;
    strncpy (patches[id].name, name, PATCH_MAX_NAME);
    return 0;
}

/* sets the root note */
int patch_set_note (int id, int note)
{
    if (!isok (id))
	return PATCH_ID_INVALID;

    if (note < 0 || note > 127)
	return PATCH_NOTE_INVALID;

    patches[id].note = note;
    return 0;
}

/* sets the panorama */
int patch_set_panning (int id, float pan)
{
    if (!isok (id))
	return PATCH_ID_INVALID;

    if (pan < -1.0 || pan > 1.0)
	return PATCH_PAN_INVALID;

    patches[id].pan.val = pan;
    return 0;
}

/* set the pitch */
int patch_set_pitch (int id, float pitch)
{
    if (!isok (id))
	return PATCH_ID_INVALID;

    if (pitch < -1.0 || pitch > 1.0)
	return PATCH_PARAM_INVALID;

    patches[id].pitch.val = pitch;
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

    patches[id].pitch_steps = steps;
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

    patches[id].play_mode = mode;
    return 0;
}

/* set whether portamento is being used or not */
int patch_set_portamento (int id, gboolean val)
{
    if (!isok (id))
        return PATCH_ID_INVALID;

    patches[id].porta = val;
    return 0;
}

/* set length of portamento slides in seconds */
int patch_set_portamento_time (int id, float secs)
{
    if (!isok (id))
	return PATCH_ID_INVALID;

    if (secs < 0.0)
	return PATCH_PARAM_INVALID;

    patches[id].porta_secs = secs;
    return 0;
}

/* set patch to listen to a range of notes if non-zero */
int patch_set_range (int id, gboolean range)
{
    if (!isok (id))
	return PATCH_ID_INVALID;
     
    patches[id].range = range;
    return 0;
}

/* set the filter's resonance */
int patch_set_resonance (int id, float reso)
{
    if (!isok (id))
	return PATCH_ID_INVALID;

    if (reso < 0.0 || reso > 1.0)
	return PATCH_PARAM_INVALID;

    patches[id].freso.val = reso;
    return 0;
}


/* set the upper note of a patch's range */
int patch_set_upper_note (int id, int note)
{
    if (!isok (id))
	return PATCH_ID_INVALID;

    if (note < 0 || note > 127)
	return PATCH_NOTE_INVALID;

    patches[id].upper_note = note;
    return 0;
}

/* set the amplitude */
int patch_set_amplitude (int id, float vol)
{

    if (!isok (id))
	return PATCH_ID_INVALID;

    if (vol < 0 || vol > 1.0)
	return PATCH_VOL_INVALID;

    patches[id].vol.val = vol;
    return 0;
}

/*****************************************************************************/
/*************************** PARAMETER GETTERS********************************/
/*****************************************************************************/

/* get the channel the patch listens on */
int patch_get_channel (int id)
{
    if (!isok (id))
	return PATCH_ID_INVALID;
    return patches[id].channel;
}

/* get the cut signal */
int patch_get_cut (int id)
{
    if (!isok (id))
	return PATCH_ID_INVALID;
    return patches[id].cut;
}

/* get the cut-by signal */
int patch_get_cut_by (int id)
{
    if (!isok (id))
	return PATCH_ID_INVALID;
    return patches[id].cut_by;
}

/* get the filter cutoff value */
float patch_get_cutoff (int id)
{
    if (!isok (id))
	return PATCH_ID_INVALID;
    return patches[id].ffreq.val;
}

/* get the display index */
int patch_get_display_index (int id)
{
    if (!isok (id))
	return PATCH_ID_INVALID;
    return patches[id].display_index;
}

/* get the number of frame in the sample */
int patch_get_frames (int id)
{
    if (!isok (id))
	return PATCH_ID_INVALID;

    if (patches[id].sample->sp == NULL)
	return 0;

    return patches[id].sample->frames;
}

/* get whether this patch is played legato or not */
gboolean patch_get_legato(int id)
{
    if (!isok (id))
	return PATCH_ID_INVALID;
    return patches[id].legato;
}


/* get the lower note */
int patch_get_lower_note (int id)
{
    if (!isok (id))
	return PATCH_ID_INVALID;
    return patches[id].lower_note;
}


int patch_get_mark_frame(int patch, int mark)
{
    if (!isok(patch))
        return -1;

    if (patches[patch].sample->sp == NULL)
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
gboolean patch_get_monophonic(int id)
{
    if (!isok (id))
	return PATCH_ID_INVALID;
    return patches[id].mono;
}

/* get the name */
char *patch_get_name (int id)
{
    char *name;

    if (id < 0 || id >= PATCH_COUNT || !patches[id].active)
	name = strdup ("\0");
    else
	name = strdup (patches[id].name);

    return name;
}

/* get the root note */
int patch_get_note (int id)
{
    if (!isok (id))
	return PATCH_ID_INVALID;
    return patches[id].note;
}

/* get the panorama */
float patch_get_panning (int id)
{
    if (!isok (id))
	return PATCH_ID_INVALID;
    return patches[id].pan.val;
}

/* get the pitch */
float patch_get_pitch (int id)
{
    if (!isok (id))
	return PATCH_ID_INVALID;

    return patches[id].pitch.val;
}

/* get the pitch range */
int patch_get_pitch_steps (int id)
{
    if (!isok (id))
	return PATCH_ID_INVALID;

    return patches[id].pitch_steps;
}

/* get the play mode */
PatchPlayMode patch_get_play_mode (int id)
{
    if (!isok (id))
	return PATCH_ID_INVALID;
    return patches[id].play_mode;
}

/* get whether portamento is used or not */
gboolean patch_get_portamento (int id)
{
    if (!isok (id))
	return PATCH_ID_INVALID;

    return patches[id].porta;
}

/* get length of portamento slides in seconds */
float patch_get_portamento_time (int id)
{
    if (!isok (id))
	return PATCH_ID_INVALID;

    return patches[id].porta_secs;
}

/* get whether a range of notes is used or not */
gboolean patch_get_range (int id)
{
    return patches[id].range;
}

/* get the filter's resonance amount */
float patch_get_resonance (int id)
{
    if (!isok (id))
	return PATCH_ID_INVALID;
    return patches[id].freso.val;
}

/* get a pointer to the sample data */
const float *patch_get_sample (int id)
{
    if (id < 0 || id >= PATCH_COUNT || !patches[id].active)
	return NULL;

    return patches[id].sample->sp;
}

/* get the name of the sample file */
char *patch_get_sample_name (int id)
{
    char *name;

    if (id < 0 || id >= PATCH_COUNT || !patches[id].active)
	name = strdup ("\0");
    else
	name = strdup (sample_get_file (patches[id].sample));
    return name;
}

/* get the upper note */
int patch_get_upper_note (int id)
{
    if (!isok (id))
	return PATCH_ID_INVALID;
    return patches[id].upper_note;
}

/* get the amplitude */
float patch_get_amplitude (int id)
{
    if (!isok (id))
	return PATCH_ID_INVALID;
    return patches[id].vol.val;
}


int patch_get_fade_samples(int id)
{
    if (!isok (id))
        return PATCH_ID_INVALID;
    return patches[id].fade_samples;
}

int patch_get_xfade_samples(int id)
{
    if (!isok (id))
        return PATCH_ID_INVALID;
    return patches[id].xfade_samples;
}


int patch_get_max_fade_samples(int id)
{
    return (patches[id].play_stop - patches[id].play_start) / 2;
}


int patch_get_max_xfade_samples(int id)
{
    int min = patches[id].sample->frames;
    int tmp;
    
    tmp = patches[id].loop_stop - patches[id].loop_start;
    min = (tmp < min) ? tmp : min;

    tmp = patches[id].play_stop - patches[id].loop_stop;
    min = (tmp < min) ? tmp : min;

    tmp = patches[id].loop_start - patches[id].play_start;
    min = (tmp < min) ? tmp : min;

    return tmp;
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
    case PATCH_PARAM_AMPLITUDE: *v = patches[patch_id].vol.val;     break;
    case PATCH_PARAM_PANNING:   *v = patches[patch_id].pan.val;     break;
    case PATCH_PARAM_CUTOFF:    *v = patches[patch_id].ffreq.val;   break;
    case PATCH_PARAM_RESONANCE: *v = patches[patch_id].freso.val;   break;
    case PATCH_PARAM_PITCH:     *v = patches[patch_id].pitch.val;   break;
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
    case PATCH_PARAM_AMPLITUDE: patches[patch_id].vol.val = v;      break;
    case PATCH_PARAM_PANNING:   patches[patch_id].pan.val = v;      break;
    case PATCH_PARAM_CUTOFF:    patches[patch_id].ffreq.val = v;    break;
    case PATCH_PARAM_RESONANCE: patches[patch_id].freso.val = v;    break;
    case PATCH_PARAM_PITCH:     patches[patch_id].pitch.val = v;    break;
    default:
        return PATCH_PARAM_INVALID;
    }

    return 0;
}


/*****************************************************************************/
/************************* MODULATION SETTERS ********************************/
/*****************************************************************************/

int patch_set_mod1_src(int patch_id, PatchParamType param, int modsrc_id)
{
    PatchParam* p;
    int err; 

    if ((err = get_patch_param(patch_id, param, &p)) != 0)
        return err;

    if ((err = mod_src_ok(modsrc_id)) != 0)
        return err;

    p->mod1_id = modsrc_id;
    return 0;
}

int patch_set_mod2_src(int patch_id, PatchParamType param, int modsrc_id)
{
    PatchParam* p;
    int err; 

    if ((err = get_patch_param(patch_id, param, &p)) != 0)
        return err;

    if ((err = mod_src_ok(modsrc_id)) != 0)
        return err;

    p->mod2_id = modsrc_id;
    return 0;
}

int patch_set_mod1_amt(int patch_id, PatchParamType param, float amt)
{
    PatchParam* p;
    int err; 

    if ((err = get_patch_param(patch_id, param, &p)) != 0)
        return err;

    if (amt < -1.0 || amt > 1.0)
        return PATCH_MOD_AMOUNT_INVALID;

    p->mod1_amt = amt;

    if (param == PATCH_PARAM_PITCH)
    {
        patches[patch_id].mod1_pitch_max =
                        pow (2, (amt * PATCH_MAX_PITCH_STEPS) / 12.0);
        patches[patch_id].mod1_pitch_min =
                        pow (2, -(amt * PATCH_MAX_PITCH_STEPS) / 12.0);
    }

    return 0;
}

int patch_set_mod2_amt(int patch_id, PatchParamType param, float amt)
{
    PatchParam* p;
    int err; 

    if ((err = get_patch_param(patch_id, param, &p)) != 0)
        return err;

    if (amt < -1.0 || amt > 1.0)
        return PATCH_MOD_AMOUNT_INVALID;

    p->mod2_amt = amt;

    if (param == PATCH_PARAM_PITCH)
    {
        patches[patch_id].mod2_pitch_max =
                        pow (2, (amt * PATCH_MAX_PITCH_STEPS) / 12.0);
        patches[patch_id].mod2_pitch_min =
                        pow (2, -(amt * PATCH_MAX_PITCH_STEPS) / 12.0);
    }

    return 0;
}

int patch_set_amp_env(int patch_id, int modsrc_id)
{
    int err;

    if (!isok(patch_id))
        return PATCH_ID_INVALID;

    if ((err = mod_src_ok(modsrc_id)) != 0)
        return err;

    patches[patch_id].vol.direct_mod_id = modsrc_id;
    return 0;
}


int patch_set_vel_amount (int patch_id, PatchParamType param, float amt)
{
    PatchParam* p;
    int err;
     debug("set vel amount:%d %d %f\n",patch_id, param, amt);
    if (!isok (patch_id))
	return PATCH_ID_INVALID;

    if ((err = get_patch_param(patch_id, param, &p)) < 0)
	return err;

    if (amt < 0.0 || amt > 1.0)
	return PATCH_PARAM_INVALID;

    p->vel_amt = amt;
    return 0;
}


/**************************************************************************/
/********************** MODULATION GETTERS ********************************/
/**************************************************************************/

int patch_get_mod1_src(int patch_id, PatchParamType param, int* modsrc_id)
{
    PatchParam* p;
    int err; 

    if ((err = get_patch_param(patch_id, param, &p)) != 0)
        return err;

    *modsrc_id = p->mod1_id;
    return 0;
}

int patch_get_mod2_src(int patch_id, PatchParamType param, int* modsrc_id)
{
    PatchParam* p;
    int err; 

    if ((err = get_patch_param(patch_id, param, &p)) != 0)
        return err;

    *modsrc_id = p->mod2_id;
    return 0;
}

int patch_get_mod1_amt(int patch_id, PatchParamType param, float* amount)
{
    PatchParam* p;
    int err; 

    if ((err = get_patch_param(patch_id, param, &p)) != 0)
        return err;

    *amount = p->mod1_amt;
    return 0;
}

int patch_get_mod2_amt(int patch_id, PatchParamType param, float* amount)
{
    PatchParam* p;
    int err; 

    if ((err = get_patch_param(patch_id, param, &p)) != 0)
        return err;

    *amount = p->mod2_amt;
    return 0;
}

int patch_get_amp_env(int patch_id, int* modsrc_id)
{
    if (!isok(patch_id))
        return PATCH_ID_INVALID;

    *modsrc_id = patches[patch_id].vol.direct_mod_id;
    return 0;
}


int patch_get_vel_amount (int patch_id, PatchParamType param, float* val)
{
    PatchParam* p;
    int err;
     
    if (!isok (patch_id))
	return PATCH_ID_INVALID;

    if ((err = get_patch_param(patch_id, param, &p)) < 0)
	return err;

    *val = p->vel_amt;
    return 0;
}


/**************************************************************************/
/******************** LFO MODULATION SETTERS ******************************/
/**************************************************************************/

int patch_set_lfo_mod1_src(int patch_id, int lfo_id, int modsrc_id)
{
    LFO* lfo;
    LFOParams* lfopar;
    int err; 
    if ((err = lfo_from_id(patch_id, lfo_id, &lfo, &lfopar)))
        return err;
    lfopar->mod1_id = modsrc_id;

    if (lfo)
        patch_trigger_global_lfo(patch_id, lfo, lfopar);

    return 0;
}

int patch_set_lfo_mod2_src(int patch_id, int lfo_id, int modsrc_id)
{
    LFO* lfo;
    LFOParams* lfopar;
    int err; 
    if ((err = lfo_from_id(patch_id, lfo_id, &lfo, &lfopar)))
        return err;
    lfopar->mod2_id = modsrc_id;

    if (lfo)
        patch_trigger_global_lfo(patch_id, lfo, lfopar);

    return 0;
}

int patch_set_lfo_mod1_amt(int patch_id, int lfo_id, float amount)
{
    LFO* lfo;
    LFOParams* lfopar;
    int err; 
    if ((err = lfo_from_id(patch_id, lfo_id, &lfo, &lfopar)))
        return err;
    lfopar->mod1_amt = amount;

    if (lfo)
        patch_trigger_global_lfo(patch_id, lfo, lfopar);

    return 0;
}

int patch_set_lfo_mod2_amt(int patch_id, int lfo_id, float amount)
{
    LFO* lfo;
    LFOParams* lfopar;
    int err; 
    if ((err = lfo_from_id(patch_id, lfo_id, &lfo, &lfopar)))
        return err;
    lfopar->mod2_amt = amount;

    if (lfo)
        patch_trigger_global_lfo(patch_id, lfo, lfopar);

    return 0;
}



/**************************************************************************/
/******************** LFO MODULATION GETTERS ******************************/
/**************************************************************************/
int patch_get_lfo_mod1_src(int patch_id, int lfo_id, int* modsrc_id)
{
    LFOParams* lfopar;
    int err; 
    if ((err = lfo_from_id(patch_id, lfo_id, NULL, &lfopar)))
        return err;
    *modsrc_id = lfopar->mod1_id;
    return 0;
}

int patch_get_lfo_mod2_src(int patch_id, int lfo_id, int* modsrc_id)
{
    LFOParams* lfopar;
    int err; 
    if ((err = lfo_from_id(patch_id, lfo_id, NULL, &lfopar)))
        return err;
    *modsrc_id = lfopar->mod2_id;
    return 0;
}

int patch_get_lfo_mod1_amt(int patch_id, int lfo_id, float* amount)
{
    LFOParams* lfopar;
    int err; 
    if ((err = lfo_from_id(patch_id, lfo_id, NULL, &lfopar)))
        return err;
    *amount = lfopar->mod1_amt;
    return 0;
}

int patch_get_lfo_mod2_amt(int patch_id, int lfo_id, float* amount)
{
    LFOParams* lfopar;
    int err; 
    if ((err = lfo_from_id(patch_id, lfo_id, NULL, &lfopar)))
        return err;
    *amount = lfopar->mod2_amt;
    return 0;
}


