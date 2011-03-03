#include "patch_set_and_get.h"

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



#include "patch_private/patch_data.h"
#include "patch_private/patch_defs.h"


/**************************************************************************/
/********************** PRIVATE GENERAL HELPER FUNCTIONS*******************/
/**************************************************************************/


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



/**************************************************************************/
/********************** UTILITY FUNCTIONS *********************************/
/**************************************************************************/

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
    defadsr.attack  = 0.005;
    defadsr.hold    = 0.0;
    defadsr.decay   = 0.0;
    defadsr.sustain = 1.0;
    defadsr.release = 0.150;

    /* default lfo params */
    deflfo.lfo_on = FALSE;
    deflfo.positive = FALSE;
    deflfo.shape = LFO_SHAPE_SINE;
    deflfo.freq = 1.0;
    deflfo.sync_beats = 1.0;
    deflfo.sync = FALSE;
    deflfo.delay = 0.0;
    deflfo.attack = 0.0;
    deflfo.mod1_id = MOD_SRC_NONE;
    deflfo.mod2_id = MOD_SRC_NONE;
    deflfo.mod1_amt = 0.0;
    deflfo.mod2_amt = 0.0;

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
    patches[id].vol.direct_mod_id = MOD_SRC_FIRST_EG;
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
        if (i == 0)
        {
            patches[id].env_params[i].env_on = TRUE;
        }
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
    val = sample_load_file (patches[id].sample, name, patch_samplerate);

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
    int oldrate = patch_samplerate;

    patch_samplerate = rate;

    debug ("changing samplerate to %d\n", rate);
    if (patch_samplerate != oldrate)
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

    patch_declick_dec = 1.0 / (PATCH_MIN_RELEASE * rate);
    patch_legato_lag = PATCH_LEGATO_LAG * rate;
    debug("patch_declick_dec = %f\n", patch_declick_dec);
    debug("patch_legato_lag = %d\n", patch_legato_lag);
     
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


