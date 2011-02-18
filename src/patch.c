#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <pthread.h>
#include <glib.h>
#include "petri-foo.h"
#include "maths.h"
#include "ticks.h"
#include "patch.h"
#include "sample.h"
#include "adsr.h"
#include "lfo.h"
#include "driver.h"		/* for DRIVER_DEFAULT_SAMPLERATE */
#include "midi.h"		/* for MIDI_CHANS */

#include "patch_set_and_get.h"

#include "private/patch_data.h" /* the structs that were here */


/* the minimum envelope release value we'll allow (to prevent clicks) */
const float PATCH_MIN_RELEASE = 0.05;

/* how much time to wait before actually releasing legato patches;
 * this is to make sure that noteons immediately following noteoffs
 * stay "connected" */
const float PATCH_LEGATO_LAG = 0.05;

/* how much to decrease v->declick_vol by each tick; calculated to
 * take PATCH_MIN_RELEASE seconds (the initial value below is
 * bogus) */
static float declick_dec = -0.1;

/* how many ticks legato releases lag; calculated to take
 * PATCH_LEGATO_LAG seconds (init value is bogus) */
static int legato_lag = 20;

/* in certain places, we consider values with an absolute value less
 * than or equal to this to be equivalent to zero */
const float ALMOST_ZERO = 0.000001;

/* what sample rate we think the audio interface is running at */
static float samplerate = DRIVER_DEFAULT_SAMPLERATE;


/* array of all patches  */
Patch patches[PATCH_COUNT];

/*****************************************************************************/
/************************ PRIVATE GENERAL HELPER FUNCTIONS********************/
/*****************************************************************************/


/* inline static int isok(int id) see private/patch_data.h */
INLINE_ISOK_DEF


/* locks a patch so that it will be ignored by patch_render() */
inline static void patch_lock (int id)
{
    g_assert (id >= 0 && id < PATCH_COUNT);
    pthread_mutex_lock (&patches[id].mutex);
}

/*  same as above, but returns immediately with EBUSY if mutex
 *  is already held */
inline static int patch_trylock (int id)
{
    g_assert (id >= 0 && id < PATCH_COUNT);
    return pthread_mutex_trylock (&patches[id].mutex);
}

/* unlocks a patch after use */
inline static void patch_unlock (int id)
{
    g_assert (id >= 0 && id < PATCH_COUNT);
    pthread_mutex_unlock (&patches[id].mutex);
}


/*  inline static void
    patch_trigger_global_lfo(int patch_id, LFO* lfo, LFOParams* lfopar)
    inline static function def macro, see private/patch_data.h */
INLINE_PATCH_TRIGGER_GLOBAL_LFO_DEF


/* triggers all global LFOs if they are used with amounts greater than 0 */
void patch_trigger_global_lfos ( )
{
    int patch_id, lfo_id;

    debug ("retriggering global LFOs...\n");

    for (patch_id = 0; patch_id < PATCH_COUNT; patch_id++)
    {
        for (lfo_id = 0; lfo_id < PATCH_MAX_LFOS; lfo_id++)
        {
            LFO* lfo =          &patches[patch_id].glfo[lfo_id];
            LFOParams* lfopar = &patches[patch_id].glfo_params[lfo_id];
            patch_trigger_global_lfo(patch_id, lfo, lfopar);
        }
    }

    debug ("done\n");
}



/*****************************************************************************/
/************************ UTILITY FUNCTIONS **********************************/
/*****************************************************************************/

/* returns the number of patches currently active */
int patch_count ( )
{
    int id, count;

    for (id = count = 0; id < PATCH_COUNT; id++)
        if (patches[id].active)
            count++;

    return count;
}

/* returns assigned patch id on success, negative value on failure */
int patch_create (const char *name)
{
    int id, i;
    ADSRParams defadsr;
    LFOParams deflfo;
    PatchVoice defvoice;

    /* find an unoccupied patch id */
    for (id = 0; patches[id].active; id++)
        if (id == PATCH_COUNT)
            return PATCH_LIMIT;

    patch_lock (id);
    patches[id].active = 1;

    debug ("Creating patch %s (%d).\n", name, id);

    /* name */
    g_strlcpy (patches[id].name, name, PATCH_MAX_NAME);
     
    /* default values */
    patches[id].channel = 0;
    patches[id].note = 60;
    patches[id].range = 0;
    patches[id].lower_note = 60;
    patches[id].upper_note = 60;
    patches[id].play_mode = PATCH_PLAY_FORWARD | PATCH_PLAY_SINGLESHOT;
    patches[id].cut = 0;
    patches[id].cut_by = 0;
    patches[id].sample_start = 0;
    patches[id].sample_stop = 0;

    patches[id].sample_xfade = 0;
    patches[id].sample_fade_in = 0;
    patches[id].sample_fade_out = 0;

    patches[id].loop_start = 0;
    patches[id].loop_stop = 0;
    patches[id].porta = FALSE;
    patches[id].mono = FALSE;
    patches[id].legato = FALSE;
    patches[id].porta_secs = 0.05;
    patches[id].pitch_steps = 2;
    patches[id].pitch_bend = 0;
    patches[id].mod1_pitch_max = 1.0;
    patches[id].mod1_pitch_min = 1.0;
    patches[id].mod2_pitch_max = 1.0;
    patches[id].mod2_pitch_min = 1.0;
     
    /* default adsr params */
    defadsr.env_on  = FALSE;
    defadsr.delay   = 0.0;
    defadsr.attack  = PATCH_MIN_RELEASE;
    defadsr.hold    = 0.0;
    defadsr.decay   = 0.0;
    defadsr.sustain = 1.0;
    defadsr.release = PATCH_MIN_RELEASE;

    /* default lfo params */
    deflfo.lfo_on = FALSE;
    deflfo.positive = FALSE;
    deflfo.shape = LFO_SHAPE_SINE;
    deflfo.freq = 1.0;
    deflfo.sync_beats = 1.0;
    deflfo.sync = FALSE;
    deflfo.delay = 0.0;
    deflfo.attack = 0.0;

    for (i = 0; i < PATCH_MAX_LFOS; i++)
    {
        patches[id].glfo_params[i] = deflfo;
        lfo_prepare (&patches[id].glfo[i]);
    }

    /* amplitude */
    patches[id].vol.val = DEFAULT_AMPLITUDE;
    patches[id].vol.mod1_id = MOD_SRC_NONE;
    patches[id].vol.mod2_id = MOD_SRC_NONE;
    patches[id].vol.mod1_amt = 0;
    patches[id].vol.mod2_amt = 0;
    patches[id].vol.direct_mod_id = MOD_SRC_NONE;
    patches[id].vol.vel_amt = 1.0;

    /* panning */
    patches[id].pan.val = 0.0;
    patches[id].pan.mod1_id = MOD_SRC_NONE;
    patches[id].pan.mod2_id = MOD_SRC_NONE;
    patches[id].pan.mod1_amt = 0;
    patches[id].pan.mod2_amt = 0;
    patches[id].pan.vel_amt = 0;

    /* cutoff */
    patches[id].ffreq.val = 1.0;
    patches[id].ffreq.mod1_id = MOD_SRC_NONE;
    patches[id].ffreq.mod2_id = MOD_SRC_NONE;
    patches[id].ffreq.mod1_amt = 0;
    patches[id].ffreq.mod2_amt = 0;
    patches[id].ffreq.vel_amt = 0;

    /* resonance */
    patches[id].freso.val = 0.0;
    patches[id].freso.mod1_id = MOD_SRC_NONE;
    patches[id].freso.mod2_id = MOD_SRC_NONE;
    patches[id].freso.mod1_amt = 0;
    patches[id].freso.mod2_amt = 0;
    patches[id].freso.vel_amt = 0;

    /* pitch */
    patches[id].pitch.val = 0.0;
    patches[id].pitch.mod1_id = MOD_SRC_NONE;
    patches[id].pitch.mod2_id = MOD_SRC_NONE;
    patches[id].pitch.mod1_amt = 0;
    patches[id].pitch.mod2_amt = 0;
    patches[id].pitch.vel_amt = 0;

    /* default voice */
    defvoice.active = FALSE;
    defvoice.note = 0;
    defvoice.posi = 0;
    defvoice.posf = 0;
    defvoice.stepi = 0;
    defvoice.stepf = 0;
    defvoice.vel = 0;

    defvoice.vol_mod1 = 0;
    defvoice.vol_mod2 = 0;
    defvoice.vol_direct = 0;

    defvoice.pan_mod1 = 0;
    defvoice.pan_mod2 = 0;

    defvoice.ffreq_mod1 = 0;
    defvoice.ffreq_mod2 = 0;

    defvoice.freso_mod1 = 0;
    defvoice.freso_mod2 = 0;

    defvoice.pitch_mod1 = 0;
    defvoice.pitch_mod2 = 0;

    for (i = 0; i < VOICE_MAX_ENVS; i++)
    {
        patches[id].env_params[i] = defadsr;
        adsr_init(&defvoice.env[i]);
    }

    for (i = 0; i < VOICE_MAX_LFOS; i++)
        lfo_prepare(&defvoice.lfo[i]);

    defvoice.fll = 0;
    defvoice.flr = 0;
    defvoice.fbl = 0;
    defvoice.fbr = 0;

     /* initialize voices */
    for (i = 0; i < PATCH_VOICE_COUNT; i++)
    {
        patches[id].voices[i] = defvoice;
        patches[id].last_note = 60;
    }

    /* set display_index to next unique value */
    patches[id].display_index = 0;
    for (i = 0; i < PATCH_COUNT; i++)
    {
        if (i == id)
            continue;
        if (patches[i].active
            && patches[i].display_index >= patches[id].display_index)
        {
            patches[id].display_index = patches[i].display_index + 1;
        }
    }

    patch_unlock (id);
    return id;
}

/* destroy a single patch with given id */
int patch_destroy (int id)
{
    int index;

    if (!isok (id))
	return PATCH_ID_INVALID;
    debug ("Removing patch: %d\n", id);

    patch_lock (id);

    patches[id].active = 0;
    sample_free_file (patches[id].sample);

    patch_unlock (id);

    /* every active patch with a display_index greater than this
     * patch's needs to have it's value decremented so that we
     * preservere continuity; no locking necessary because the
     * display_index is not thread-shared data */
    index = patches[id].display_index;
    for (id = 0; id < PATCH_COUNT; id++)
    {
        if (patches[id].active && patches[id].display_index > index)
            patches[id].display_index--;
    }

    return 0;
}

/* destroy all patches */
void patch_destroy_all ( )
{
    int id;

    for (id = 0; id < PATCH_COUNT; id++)
	patch_destroy (id);

    return;
}

/* place all patch ids, sorted in ascending order by channels and then
   notes, into array 'id' and return number of patches */
int patch_dump (int **dump)
{
    int i, j, k, id, count, tmp;

    *dump = NULL;

    /* determine number of patches */
    count = patch_count ( );

    if (count == 0)
	return count;

    /* allocate dump */
    *dump = malloc (sizeof (int) * count);
    if (*dump == NULL)
	return PATCH_ALLOC_FAIL;

    /* place active patches into dump array */
    for (id = i = 0; id < PATCH_COUNT; id++)
	if (patches[id].active)
	    (*dump)[i++] = id;

    /* sort dump array by channel in ascending order */
    for (i = 0; i < count; i++)
    {
	for (j = i; j < count; j++)
	{
	    if (patches[(*dump)[j]].channel <
		patches[(*dump)[i]].channel)
	    {
		tmp = (*dump)[i];
		(*dump)[i] = (*dump)[j];
		(*dump)[j] = tmp;
	    }
	}
    }

    /* sort dump array by note in ascending order while preserving
     * existing channel order */
    for (i = 0; i < MIDI_CHANS; i++)
    {
	for (j = 0; j < count; j++)
	{
	    if (patches[(*dump)[j]].channel != i)
		continue;
	    
	    for (k = j; k < count; k++)
	    {
		if (patches[(*dump)[k]].channel != i)
		    continue;

		if (patches[(*dump)[k]].note <
		    patches[(*dump)[j]].note)
		{
		    tmp = (*dump)[j];
		    (*dump)[j] = (*dump)[k];
		    (*dump)[k] = tmp;
		}
	    }
	}
    }

    return count;
}

int patch_duplicate (int target)
{
    /* we'll come back to this... */
    return PATCH_ID_INVALID;

/*
    int id, i;
    Sample* oldsam;
    float* vol;
    float* pan;
    float* cut;
    float* res;
    float* pitch;
     
    if (target < 0 || target > PATCH_COUNT || !patches[target].active)
        return PATCH_ID_INVALID;

    for (id = 0; patches[id].active; id++)
        if (id == PATCH_COUNT)
            return PATCH_LIMIT;

    debug ("Creating patch (%d) from patch %s (%d).\n", id,
           patches[target].name, target);

    patch_lock (id);
*/
    /* we have to store and restore our pointers to allocated memory,
     * because the assignment below will overwrite them </kludge?> */
/* yeah, um, this might now be changed to frequency modulation sources...
    vol = patches[id].vol.lfo_tab;
    pan = patches[id].pan.lfo_tab;
    cut = patches[id].ffreq.lfo_tab;
    res = patches[id].freso.lfo_tab;
    pitch = patches[id].pitch.lfo_tab;
    oldsam = patches[id].sample;
     
    patches[id] = patches[target];

    patches[id].vol.lfo_tab = vol;
    patches[id].pan.lfo_tab = pan;
    patches[id].ffreq.lfo_tab = cut;
    patches[id].freso.lfo_tab = res;
    patches[id].pitch.lfo_tab = pitch;
    patches[id].sample = oldsam;

*/
    /* this is residual paranoia, I think */
/*
    patches[id].sample->sp = NULL;

    if (patches[target].sample->sp != NULL)
    {
	patch_sample_load (id, sample_get_file (patches[target].sample));
    }
*/
    /* set display_index to next unique value */
/*
    patches[id].display_index = 0;
    for (i = 0; i < PATCH_COUNT; i++)
    {
	if (i == id)
	    continue;
	if (patches[i].active
	    && patches[i].display_index >= patches[id].display_index)
	{
	    patches[id].display_index = patches[i].display_index + 1;
	}
    }
    debug ("chosen display: %d\n", patches[id].display_index);

    patch_unlock (id);
    return id;
*/
}

/* stop all currently playing voices in given patch */
int patch_flush (int id)
{
    int i;
     
    if (!isok(id))
        return PATCH_ID_INVALID;

    patch_lock (id);

    if (patches[id].sample->sp == NULL)
    {
        patch_unlock (id);
        return 0;
    }

    for (i = 0; i < PATCH_VOICE_COUNT; i++)
        patches[id].voices[i].active = FALSE;

    patch_unlock (id);
    return 0;
}

/* stop all voices for all patches */
void patch_flush_all ( )
{
    int i;

    for (i = 0; i < PATCH_COUNT; i++)
        patch_flush (i);
}

/* constructor */
void patch_init ( )
{
    int i,j;

    debug ("initializing...\n");
    for (i = 0; i < PATCH_COUNT; i++)
    {
        pthread_mutex_init (&patches[i].mutex, NULL);
        patches[i].sample = sample_new ( );
        patches[i].vol.mod1_id =
        patches[i].vol.mod2_id =    MOD_SRC_NONE;
        patches[i].pan.mod1_id =
        patches[i].pan.mod2_id =    MOD_SRC_NONE;
        patches[i].ffreq.mod1_id =
        patches[i].ffreq.mod2_id =  MOD_SRC_NONE;
        patches[i].freso.mod1_id =
        patches[i].freso.mod2_id =  MOD_SRC_NONE;
        patches[i].pitch.mod1_id =
        patches[i].pitch.mod2_id =  MOD_SRC_NONE;

        for (j = 0; j < PATCH_MAX_LFOS; ++j)
            patches[i].glfo_table[j] = NULL;
    }

    debug ("done\n");
}

/* returns error message associated with error code */
const char *patch_strerror (int error)
{
    switch (error)
    {
    case PATCH_PARAM_INVALID:
	return "patch parameter is invalid";
	break;
    case PATCH_ID_INVALID:
	return "patch id is invalid";
	break;
    case PATCH_ALLOC_FAIL:
	return "failed to allocate space for patch";
	break;
    case PATCH_NOTE_INVALID:
	return "specified note is invalid";
	break;
    case PATCH_PAN_INVALID:
	return "specified panning is invalid";
	break;
    case PATCH_CHANNEL_INVALID:
	return "specified channel is invalid";
	break;
    case PATCH_VOL_INVALID:
	return "specified amplitude is invalid";
	break;
    case PATCH_PLAY_MODE_INVALID:
	return "specified patch play mode is invalid";
	break;
    case PATCH_LIMIT:
	return "maximum patch count reached, can't create another";
	break;
    case PATCH_SAMPLE_INDEX_INVALID:
	return "specified sample is invalid";
	break;
    default:
	return "unknown error";
	break;
    }
}

/* loads a sample file for a patch */
int patch_sample_load (int id, const char *name)
{
    int val;

    if (!isok (id))
	return PATCH_ID_INVALID;

    if (name == NULL)
    {
	debug ("Refusing to load null sample for patch %d\n", id);
	return PATCH_PARAM_INVALID;
    }

    debug ("Loading sample %s for patch %d\n", name, id);
    patch_flush (id);

    /* we lock *after* we call patch_flush because patch_flush does
     * its own locking */
    patch_lock (id);
    val = sample_load_file (patches[id].sample, name, samplerate);

    /* set the sample/loop start/stop point appropriately */
    patches[id].sample_xfade = 0;
    patches[id].sample_fade_in = 0;
    patches[id].sample_fade_out = 0;

    patches[id].sample_start = 0;
    patches[id].sample_stop = patches[id].sample->frames - 1;
    patches[id].loop_start = 0;
    patches[id].loop_stop = patches[id].sample->frames - 1;

    patch_unlock (id);
    return val;
}

/* unloads a patch's sample */
void patch_sample_unload (int id)
{
    if (!isok(id))
	return;
     
    debug ("Unloading sample for patch %d\n", id);
    patch_lock (id);

    sample_free_file (patches[id].sample);

    patches[id].sample_start = 0;
    patches[id].sample_stop = 0;
    patches[id].loop_start = 0;
    patches[id].loop_stop = 0;
    patches[id].sample_xfade = 0;
    patches[id].sample_fade_in = 0;
    patches[id].sample_fade_out = 0;

    patch_unlock (id);
}

/* sets our buffersize and reallocates our lfo_tab; this function
 * doesn't need to do any locking because we have a guarantee that
 * mixing will stop when the buffersize changes */
void patch_set_buffersize (int nframes)
{
    int i,j;

    debug ("setting buffersize to %d\n", nframes);
    for (i = 0; i < PATCH_COUNT; i++)
    {
        Patch* p = &patches[i];

        for (j = 0; j < PATCH_MAX_LFOS; j++)
            p->glfo_table[j] = g_renew (float, p->glfo_table[j], nframes);
    }
}

/* sets our samplerate and resamples if necessary; this function
 * doesn't need to do any locking because we have a guarantee that
 * mixing will stop when the samplerate changes */
void patch_set_samplerate (int rate)
{
    int id;
    char *name;
    int oldrate = samplerate;

    samplerate = rate;

    debug ("changing samplerate to %d\n", rate);
    if (samplerate != oldrate)
    {	 
	for (id = 0; id < PATCH_COUNT; id++)
	{
	    if (!patches[id].active)
		continue;
	       
	    name = patch_get_sample_name (id);
	    patch_sample_load (id, name);
	    g_free (name);
	}
    }

    declick_dec = 1.0 / (PATCH_MIN_RELEASE * rate);
    legato_lag = PATCH_LEGATO_LAG * rate;
    debug("declick_dec = %f\n", declick_dec);
    debug("legato_lag = %d\n", legato_lag);
     
    patch_trigger_global_lfos ( );
}

/* destructor */
void patch_shutdown ( )
{
    int i,j;
     
    debug ("shutting down...\n");

    for (i = 0; i < PATCH_COUNT; i++)
    {
        sample_free (patches[i].sample);
        for (j = 0; j < PATCH_MAX_LFOS; j++)
            g_free (patches[i].glfo_table[j]);
    }

    debug ("done\n");
}

/* re-sync all global lfos to new tempo */
void patch_sync (float bpm)
{
    lfo_set_tempo (bpm);
    patch_trigger_global_lfos ();
}

/*****************************************************************************/
/******************* PLAYBACK AND RENDERING FUNCTIONS ************************/
/*****************************************************************************/

/* a helper function to release all voices matching a given criteria
 * (if note is a negative value, all active voices will be released) */
inline static void patch_release_patch (Patch* p, int note, release_t mode)
{
    int i;
    PatchVoice* v;

    for (i = 0; i < PATCH_VOICE_COUNT; i++)
    {
	if (p->voices[i].active && (p->voices[i].note == note || note < 0))
	{
	    v = &p->voices[i];

	    /* we don't really release here, that's the job of
	     * advance( ); we just tell it *when* to release */
	    v->relset = 0;
	    v->relmode = mode;

	    if (p->mono && p->legato)
		v->relset += legato_lag;
	}
    }
}

/* a helper function to cut all patches whose cut_by value matches the
 * cut value for the given patch */
inline static void patch_cut_patch (Patch* p)
{
    int i;

    /* a cut value of zero is ignored so that the user has a way of *not* using
     * cuts */
    if (p->cut == 0)
	return;

    for (i = 0; i < PATCH_COUNT; i++)
    {
	if (patches[i].active && patches[i].cut_by == p->cut)
	{
	    patch_release_patch (&patches[i], -69, RELEASE_CUTOFF);
	}
    }
}

/* a helper function to prepare a voice's pitch information */
inline static void prepare_pitch(Patch* p, PatchVoice* v, int note)
{
    double scale;		/* base pitch scaling factor */

    /* this applies the tuning factor */
    scale = pow (2, (p->pitch.val * p->pitch_steps) / 12.0);

    if (p->porta && (p->porta_secs > 0.0)
	&& (p->last_note != note))
    {
	/* we calculate the pitch here because we can't be certain
	 * what the current value of the last voice's pitch is */
	v->pitch = pow (2, (p->last_note - p->note) / 12.0);
	v->pitch *= scale;
	  
	v->porta_ticks = ticks_secs_to_ticks (p->porta_secs);

	/* calculate the value to be added to pitch each tick by
	 * subtracting the target pitch from the initial pitch and
	 * dividing by porta_ticks */
	v->pitch_step = ((pow (2, (note - p->note)
			       / 12.0) * scale)
			 - v->pitch)
	    / v->porta_ticks;
    }
    else
    {
	v->pitch = pow (2, (note - p->note) / 12.0);
	v->pitch *= scale;
	v->porta_ticks = 0;
    }

    /* store the note that called us as the last note that was played
     * (it's important that we do this *after* we use the
     * p->last_note variable, otherwise we'll be comparing ourself
     * against ourself) */
    p->last_note = note;

    /* give our step variables their initial values */
    v->stepi = v->pitch;
    v->stepf = (v->pitch - v->stepi) * (0xFFFFFFFFU); /* maximum value of guint32 */
}



/* a helper function to activate a voice for a patch */
inline static void
patch_trigger_patch (Patch* p, int note, float vel, Tick ticks)
{
    int i,j;
    PatchVoice* v;
    int index;			/* the index we ended up settling on */
    int gzr = 0;		/* index of the oldest running patch */
    int empty = -1;		/* index of the first empty patch we encountered */
    Tick oldest = ticks;	/* time of the oldest running patch */

    /* we don't activate voices if we don't have a sample */
    if (p->sample->sp == NULL)
        return;

    if (p->mono && p->legato)
    {
        /* find a voice to retrigger */
        for (i = 0; i < PATCH_VOICE_COUNT; ++i)
        {
            if (p->voices[i].ticks <= oldest)
            {
                oldest = p->voices[i].ticks;
                gzr = i;
            }

            if (!p->voices[i].active)
                empty = i;

        /* we need to make sure that we don't grab a voice that is
         * busy trying to release itself, otherwise we'll take it
         * over, only to be promptly reduced to zero amplitude and
         * shut down... */
            else if (!p->voices[i].released)
                break;

        /* ...however, if this is a singleshot patch being
         * released by a noteoff, the amplitude won't be dropping, and we can 
         * proceed normally */
            else if (p->play_mode == PATCH_PLAY_SINGLESHOT
                     && p->voices[i].relmode == RELEASE_NOTEOFF)
                break;
        }

    /* if we couldn't find a voice to take over, use the first
     * empty one we encountered; failing that, we'll take the
     * oldest running */
        if (i == PATCH_VOICE_COUNT)
        {
            if (empty < 0)
                index = gzr;
            else
                index = empty;

        /* ...apparently, I don't think very highly of the above logic... */
            index = 0;
        }
        else
        {
            v = &p->voices[i];
            v->ticks = ticks;
            v->note = note;
            v->vel = vel;
            v->relset = -1;	/* cancel any pending release */
            v->relmode = RELEASE_NONE;
            prepare_pitch(p, v, note);

            debug ("the rest of this function is irrelevant now...\n");

            return;     /* the rest of this function is
                         * irrelevant now, fuck it */
        }
    }
    else
    {
        /* find a free voice slot and determine the oldest running voice */
        for (i = 0; i < PATCH_VOICE_COUNT; ++i)
        {
            if (p->voices[i].ticks <= oldest)
            {
                oldest = p->voices[i].ticks;
                gzr = i;
            }

            if (!p->voices[i].active)
                break;
        }

        /* take the oldest running voice's slot if we couldn't find an
         * empty one */
        if (i == PATCH_VOICE_COUNT)
            index = gzr;
        else
            index = i;
    }

    v = &p->voices[index];
    
    /* shutdown any running voices if monophonic */
    if (p->mono)
        patch_release_patch(p, -69, RELEASE_CUTOFF);

    /* fill in our voice */
    v->ticks = ticks;
    v->relset = -1;		/* N/A at this time */
    v->relmode = RELEASE_NONE;
    v->released = FALSE;
    v->to_end = FALSE;

/*
    v->declick_vol = 0.0;
    debug("***forcing declick vol to 1.0 while not implemented****\n");
*/

    v->declick_vol = 1.0;

/* fades not implemented: */
    v->xfade_step =     1.0f / p->sample_xfade;
    v->fade_in_step =   1.0f / p->sample_fade_in;
    v->fade_out_step =  1.0f / p->sample_fade_out;

    v->note = note;
    v->fll = 0;
    v->fbl = 0;
    v->flr = 0;
    v->fbr = 0;
    v->vel = vel;

    v->vol_mod1 =   mod_id_to_pointer(p->vol.mod1_id, p, v);
    v->vol_mod2 =   mod_id_to_pointer(p->vol.mod2_id, p, v);
    v->vol_direct = mod_id_to_pointer(p->vol.direct_mod_id, p, v);
    v->pan_mod1 =   mod_id_to_pointer(p->pan.mod1_id, p, v);
    v->pan_mod2 =   mod_id_to_pointer(p->pan.mod2_id, p, v);
    v->ffreq_mod1 = mod_id_to_pointer(p->ffreq.mod1_id, p, v);
    v->ffreq_mod2 = mod_id_to_pointer(p->ffreq.mod2_id, p, v);
    v->freso_mod1 = mod_id_to_pointer(p->freso.mod1_id, p, v);
    v->freso_mod2 = mod_id_to_pointer(p->freso.mod2_id, p, v);
    v->pitch_mod1 = mod_id_to_pointer(p->pitch.mod1_id, p, v);
    v->pitch_mod2 = mod_id_to_pointer(p->pitch.mod2_id, p, v);


    /* setup direction */
    if (!(p->mono && p->legato && v->active))
    {
        if (p->play_mode & PATCH_PLAY_REVERSE)
        {
            v->posi = p->sample_stop;
            v->posf = 0;
            v->dir = -1;
        }
        else
        {
            v->posi = p->sample_start;
            v->posf = 0;
            v->dir = 1;
        }
    }

    prepare_pitch(p, v, note);

    for (j = 0; j < VOICE_MAX_ENVS; j++)
    {
        adsr_set_params (&v->env[j], &p->env_params[j]);
        adsr_trigger (&v->env[j]);
    }

    for (j = 0; j < VOICE_MAX_LFOS; j++)
    {
        v->lfo[j].freq_mod1 =
            mod_id_to_pointer(p->vlfo_params[j].mod1_id, p, v);

        v->lfo[j].freq_mod2 =
            mod_id_to_pointer(p->vlfo_params[j].mod2_id, p, v);

        v->lfo[j].mod1_amt = p->vlfo_params[j].mod1_amt;
        v->lfo[j].mod2_amt = p->vlfo_params[j].mod2_amt;

/*
        debug("lfo:%d fm1:%p fm2:%p am1:%f am2:%f\n",
                j,  v->lfo[j].freq_mod1,    v->lfo[j].freq_mod2,
                    v->lfo[j].mod1_amt,     v->lfo[j].mod2_amt );
                
*/
        lfo_trigger (&v->lfo[j], &p->vlfo_params[j]);
    }

    /* mark our territory */
    v->active = TRUE;
}

/* a helper routine to determine the pitch-scaled sample values to use for a frame */
inline static void pitchscale (Patch * p, PatchVoice * v, float *l,
			       float *r)
{
    int y0, y1, y2, y3;

    /* determine sample indices */
    if ((y0 = (v->posi - 1) * 2) < 0)
        y0 = 0;

    y1 = v->posi * 2;

    if ((y2 = (v->posi + 1) * 2) >= p->sample->frames * 2)
        y2 = 0;

    if ((y3 = (v->posi + 2) * 2) >= p->sample->frames * 2)
        y3 = 0;

    /* interpolate */
    *l = cerp (p->sample->sp[y0],
           p->sample->sp[y1],
           p->sample->sp[y2], p->sample->sp[y3], v->posf >> 24);
    *r = cerp (p->sample->sp[y0 + 1],
           p->sample->sp[y1 + 1],
           p->sample->sp[y2 + 1], p->sample->sp[y3 + 1], v->posf >> 24);
}

/* a helper routine to setup panning of two samples in a frame  */
inline
static void pan (Patch * p, PatchVoice * v, int index, float *l, float *r)
{
    float pan;

    /* get pan value */
    pan = p->pan.val;

    if (v->pan_mod1 != NULL)
        pan += *v->pan_mod1 * p->pan.mod1_amt;

    if (v->pan_mod2 != NULL)
        pan += *v->pan_mod2 * p->pan.mod2_amt;

    /* scale to velocity */
    pan = lerp (pan, pan * v->vel, p->pan.vel_amt);

    if (pan > 1.0)
        pan = 1.0;
    else if (pan < -1.0)
        pan = -1.0;

    if (pan < 0.0)
    {   /* panned left */
        *l += *r * -pan;
        *r *= 1 + pan;
    }
    else if (pan > 0.0)
    {   /* panned right */
        *r += *l * pan;
        *l *= 1 - pan;
    }
}

/* a helper routine to apply filters to a frame */
inline
static void filter (Patch* p, PatchVoice* v, int index,  float* l, float* r)
{
    float ffreq, freso, logreso;

    /* get filter cutoff frequency */
    ffreq = p->ffreq.val;

    if (v->ffreq_mod1 != NULL)
        ffreq += *v->ffreq_mod1 * p->ffreq.mod1_amt;

    if (v->ffreq_mod2 != NULL)
        ffreq += *v->ffreq_mod2 * p->ffreq.mod2_amt;

    /* scale to velocity */
    ffreq = lerp (ffreq, ffreq * v->vel, p->ffreq.vel_amt);

    /* clip */
    if (ffreq > 1.0)
        ffreq = 1.0;
    else if (ffreq < 0.0)
        ffreq = 0.0;

    /* get filter resonant frequency */
    freso = p->freso.val;

    if (v->freso_mod1 != NULL)
        freso += *v->freso_mod1 * p->freso.mod1_amt;

    if (v->freso_mod2 != NULL)
        freso += *v->freso_mod2 * p->freso.mod2_amt;

    /* scale to velocity */
    freso = lerp (freso, freso * v->vel, p->freso.vel_amt);

    /* clip */
    if (freso > 1.0)
	freso = 1.0;
    else if (freso < 0.0)
	freso = 0.0;

    /* logify */
    logreso = log_amplitude(freso);

    /* left */
    v->fbl = logreso * v->fbl + ffreq * (*l - v->fll);
    v->fll += ffreq * v->fbl;
    *l = v->fll;

    /* right */
    v->fbr = logreso * v->fbr + ffreq * (*r - v->flr);
    v->flr += ffreq * v->fbr;
    *r = v->flr;
}

/* a helper routine to adjust the amplitude of a frame */
inline static int gain (Patch* p, PatchVoice* v, int index, float* l, float* r)
{
    float vol = 0.0;
    float logvol = 0.0;

    /* first, we use our set value as a base */
    vol = p->vol.val;

    if (v->vol_mod1 != NULL)
        vol += *v->vol_mod1 * p->vol.mod1_amt;

    if (v->vol_mod2 != NULL)
        vol += *v->vol_mod2 * p->vol.mod2_amt;

    if (v->vol_direct != NULL)
        vol *= *v->vol_direct;

    /*  declick_vol will be 1.0 at all times other than during
        a crossfade (how to handle that?) and sample fades in/out
        but that's not implemented.
    vol *= v->declick_vol;
    */
    else if (v->released
            && !((p->play_mode & PATCH_PLAY_SINGLESHOT)
            && (v->relmode == RELEASE_NOTEOFF)))
    {
        // if the patch is singleshot and it doesn't have a amplitude
         // envelope, we want to let it finish playing in toto
         //
        vol *= v->declick_vol;
        v->declick_vol -= declick_dec;
    }

    /* velocity should be the last parameter considered because it
     * has the most "importance" */
    vol = lerp (vol, vol * v->vel, p->vol.vel_amt);

    /* clip */
    if (vol > 1.0)
        vol = 1.0;
    else if (vol < 0.0)
        vol = 0.0;

    /* as a last step, make logarithmic */
   logvol = log_amplitude(vol);

    /* adjust amplitude */

    *l *= vol;
    *r *= vol;

/*
    *l *= logvol;
    *r *= logvol;
*/
    /* check to see if we've finished a release */
    if (v->released && v->vol_direct && *v->vol_direct < ALMOST_ZERO)
        return -1;

    return 0;
}

/* a helper routine to advance to the next frame while properly
 * accounting for the different possible play modes (negative value
 * returned if we are out of samples after doing our work) */
inline static int advance (Patch* p, PatchVoice* v, int index)
{
    int j;
    double pitch;
    double scale;
    guint32 next_posf;
    gboolean recalc = FALSE;
        /* whether we need to recalculate our pos/step vars */

    /* portamento */
    if (p->porta && v->porta_ticks)
    {
        recalc = TRUE;
        v->pitch += v->pitch_step;
        --(v->porta_ticks);
    }

    /* base pitch value */
    pitch = v->pitch;

    if (p->pitch_bend)
    {
        recalc = TRUE;
        pitch *= p->pitch_bend;
    }

    /* pitch lfo value */
    if (v->pitch_mod1 != NULL)
    {
        recalc = TRUE;
        scale = *v->pitch_mod1;

        /* we don't multiply against p->pitch.lfo_amount because the
         * "amount" variable has already been expressed in the
         * values of lfo_pitch_max and lfo_pitch_min (the same logic
         * applies when handling the envelopes below)
         */
        if (scale >= 0.0)
        {
            pitch *= lerp (1.0, p->mod1_pitch_max, scale);
        }
        else
        {
            pitch *= lerp (1.0, p->mod1_pitch_min, -scale);
        }
    }

    if (v->pitch_mod2 != NULL)
    {
        recalc = TRUE;
        scale = *v->pitch_mod2;

        if (scale >= 0.0)
        {
            pitch *= lerp (1.0, p->mod2_pitch_max, scale);
        }
        else
        {
            pitch *= lerp (1.0, p->mod2_pitch_min, -scale);
        }
    }

    /* scale to velocity */
    if (p->pitch.vel_amt > ALMOST_ZERO)
    {
        recalc = TRUE;
        pitch = lerp (pitch, pitch * v->vel, p->pitch.vel_amt);
    }

    /*  pitch == rate for stepping through original sample,
        therefor    1.0 == natural rate,
                    0.5 == octave below,
                    2.0 == octave above.
     */

    if (recalc)
    {
        v->stepi = pitch;
        v->stepf = (pitch - v->stepi) * (0xFFFFFFFFU);
    }

    /* advance our position indices */
    if (v->dir > 0)
    {
        next_posf = v->posf + v->stepf;

        if (next_posf < v->posf)	/* unsigned int wraps around */
            v->posi++;

        v->posf = next_posf;
        v->posi += v->stepi;
    }
    else
    {
        next_posf = v->posf + v->stepf;

        if (next_posf < v->posf) /* unsigned int wraps around */
            v->posi--;

        v->posf = next_posf;
        v->posi -= v->stepi;
    }

    /* adjust our indices according to our play mode */
    if (p->play_mode & PATCH_PLAY_LOOP && !v->to_end)
    {
        if (p->play_mode & PATCH_PLAY_PINGPONG)
        {
            if ((v->dir > 0) && (v->posi > p->loop_stop))
            {
                v->posi = p->loop_stop;
                v->dir = -1;
            }
            else if ((v->dir < 0) && (v->posi < p->loop_start))
            {
                v->posi = p->loop_start;
                v->dir = 1;
            }
        }
        else
        {
            if ((v->dir > 0) && (v->posi > p->loop_stop))
            {
                v->posi = p->loop_start;
            }
            else if ((v->dir < 0) && (v->posi < p->loop_start))
            {
                v->posi = p->loop_stop;
            }
        }
    }
    else
    {
        if (((v->dir > 0) && (v->posi > p->sample_stop))
            || ((v->dir < 0) && (v->posi < p->sample_start)))
        {
            /* we need to let our caller know that they are out of
             * samples */
            return -1;
        }
    }

    /* check to see if it's time to release */
    if (v->relset >= 0 && !v->released)
    {
        if (v->relset == 0)
        {
            for (j = 0; j < VOICE_MAX_ENVS; j++)
            {
                adsr_release (&v->env[j]);
            }
            v->released = TRUE;

            if (p->play_mode & PATCH_PLAY_TO_END)
                v->to_end = TRUE;
        }
        else
        {
            --v->relset;
        }
    }

    return 0;
}


/*  a helper rountine to render all active voices of
    a given patch into buf
*/
inline static void patch_render_patch (Patch* p, float* buf, int nframes)
{
    register int i;
    register int j;
    PatchVoice* v;
    float l, r;
    gboolean done;
    register int k;


    /*  calculate global LFO output tables first:
     *  due to modulation we don't calculate each buffer one by one,
     *  we do it all simultaneously. this probably is bad for cache hits
     *  or whatever, but fuck it, modulation is more important.
    */

    for (i = 0; i < nframes; ++i)
    {
        for (j = 0; j < PATCH_MAX_LFOS; ++j)
        {
            if (p->glfo_params[j].lfo_on)
                p->glfo_table[j][i] = lfo_tick(&p->glfo[j]);
        }
    }

    /*  right then, let's do the voices now...
    */

    for (i = 0; i < PATCH_VOICE_COUNT; i++)
    {
        if (p->voices[i].active == FALSE)
            continue;

        /* sanity check */
        if (p->voices[i].posi < p->sample->frames)
            v = &p->voices[i];
        else
        {
            p->voices[i].active = FALSE;
            continue;
        }

        done = FALSE;

        for (j = 0; j < nframes && done == FALSE; j++)
        {

            /* ok so we've calculated the global LFO tables already,
                we just need to set the output of each global LFO to
                the correct value for the frame.
            */

            for (k = 0; k < PATCH_MAX_LFOS; ++k)
                if (p->glfo_params[k].lfo_on)
                    p->glfo[k].val = p->glfo_table[k][j];

            for (k = 0; k < VOICE_MAX_ENVS; ++k)
                if (p->env_params[k].env_on)
                    adsr_tick(&v->env[k]);

            for (k = 0; k < VOICE_MAX_LFOS; ++k)
                if (p->vlfo_params[k].lfo_on)
                    lfo_tick(&v->lfo[k]);

            /* process samples */
            pitchscale (p, v,    &l, &r);
            pan        (p, v, j, &l, &r);
            filter     (p, v, j, &l, &r);

            /* adjust amplitude and stop rendering if we finished
             * a release */
            if (gain (p, v, j, &l, &r) < 0)
                done = TRUE;

            buf[j * 2] += l;
            buf[j * 2 + 1] += r;

            /* advance our position and stop rendering if we
             * run out of samples */
            if (advance (p, v, j) < 0)
                done = TRUE;
        }

        /* check to see if it's time to stop rendering */
        if (done)
            v->active = FALSE;

        /* overflows bad, OVERFLOWS BAD! */
        if (v->active && (v->posi < 0 || v->posi >= p->sample->frames))
        {
            debug ("overflow! NO! BAD CODE! DIE DIE DIE!\n");
            debug ("v->posi == %d, p->sample.frames == %d\n",
                    v->posi, p->sample->frames);
            v->active = 0;
        }
    }
}

/* deactivate all active patches matching given criteria */
void patch_release (int chan, int note)
{
    int i;

    for (i = 0; i < PATCH_COUNT; i++)
    {
	if (patches[i].active
	    && patches[i].channel == chan
	    && (patches[i].note == note
		|| (patches[i].range > 0
		    && note >= patches[i].lower_note
		    && note <= patches[i].upper_note)))
	{

	    patch_release_patch (&patches[i], note, RELEASE_NOTEOFF);
	}
    }

    return;
}

/* deactivate a single patch with a given id */
void patch_release_with_id (int id, int note)
{
    if (id < 0 || id >= PATCH_COUNT)
	return;
    if (!patches[id].active)
	return;

    patch_release_patch (&patches[id], note, RELEASE_NOTEOFF);
    return;
}

/* render nframes of all active patches into buf */
void patch_render (float *buf, int nframes)
{
    int i;

    /* render patches */
    for (i = 0; i < PATCH_COUNT; i++)
    {
	if (patches[i].active)
	{
	    if (patch_trylock (i) != 0)
	    {
		continue;
	    }
	    if (patches[i].sample->sp == NULL)
	    {
		patch_unlock (i);
		continue;
	    }
	    patch_render_patch (&patches[i], buf, nframes);
	    patch_unlock (i);
	}
    }
}

/* triggers all patches matching criteria */
void patch_trigger (int chan, int note, float vel, Tick ticks)
{
    static int idp[PATCH_COUNT];	/* holds all patches to be activated */
    int i, j;

    /* We gather up all of the patches that need to be activated here
     * so that we can run their cuts and then trigger them without
     * having to find them twice.  We have to make sure that we do
     * cutting before triggering, otherwise patches which listen on
     * the same note and have the same cut/cut_by values will end up
     * stepping over each other before they both are heard.
     */

    for (i = j = 0; i < PATCH_COUNT; i++)
    {
	if (patches[i].active
	    && patches[i].channel == chan
	    && (patches[i].note == note
		|| (patches[i].range
		    && note >= patches[i].lower_note
		    && note <= patches[i].upper_note)))
	{

	    idp[j++] = i;
	}
    }

    /* do cuts */
    for (i = 0; i < j; i++)
	patch_cut_patch (&patches[idp[i]]);

    /* do triggers */
    for (i = 0; i < j; i++)
	patch_trigger_patch (&patches[idp[i]], note, vel, ticks);
}

/* activate a single patch with given id */
void patch_trigger_with_id (int id, int note, float vel, Tick ticks)
{
    if (id < 0 || id >= PATCH_COUNT)
	return;
    if (!patches[id].active)
	return;
    if (note < patches[id].lower_note || note > patches[id].upper_note)
	return;
    if (!patches[id].range && note != patches[id].note)
	return;

    patch_cut_patch (&patches[id]);
    patch_trigger_patch (&patches[id], note, vel, ticks);
    return;
}

static void patch_control_patch(Patch* p, ControlParamType param, float value)
{
    switch( param )
    {
    case CONTROL_PARAM_AMPLITUDE:
	p->vol.val = value;
	break;
    case CONTROL_PARAM_PANNING:
	p->pan.val = value;
	break;
    case CONTROL_PARAM_CUTOFF:
	p->ffreq.val = value;
	break;
    case CONTROL_PARAM_RESONANCE:
	p->freso.val = value;
	break;
    case CONTROL_PARAM_PITCH:
	p->pitch_bend = pow(2, value);
	break;
    case CONTROL_PARAM_PORTAMENTO:
	p->porta = (value < 0.5) ? 0 : 1;
	break;
    case CONTROL_PARAM_PORTAMENTO_TIME:
	p->porta_secs = value;
	break;
    default:
	break;
    }
}

void patch_control(int chan, ControlParamType param, float value)
{
    int i;

    for (i = 0; i < PATCH_COUNT; i++)
    {
	if (patches[i].active && patches[i].channel == chan)
	{
	    patch_control_patch (&patches[i], param, value);
	}
    }

    return;
}


