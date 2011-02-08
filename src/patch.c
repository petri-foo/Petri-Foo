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


#include "private/patch_data.h" /* the structs that were here */


static const char* adsr_names[] = {
    "EG1", "EG2", "EG3", "EG4", "EG5", 0
};

static const char* lfo_names[] = {
    "GLFO1", "GLFO2", "GLFO3", "GLFO4", "GLFO5",
    "VLFO1", "VLFO2", "VLFO3", "VLFO4", "VLFO5"
};

static const char* param_names[] = {
    "Amplitude",
    "Pan",
    "Cutoff",
    "Resonance",
    "Pitch",
    "Frequency Modulation"
};

static char** mod_source_names = 0;

static float one = 1.0;

static void create_mod_source_names(void)
{
    int i;
    int id;

    /* check for mismatched counts etc: */
    if (!patch_adsr_names() || !patch_lfo_names())
    {
        fprintf(stderr, "*** PROBLEM OF DEATH IS FORECAST ***\n");
        return;
    }

    mod_source_names = malloc(sizeof(*mod_source_names) * MOD_SRC_LAST);

    const char none[] = "OFF";
    const char one[] = "1.0";

    mod_source_names[MOD_SRC_NONE] = malloc(strlen(none) + 1);
    strcpy(mod_source_names[MOD_SRC_NONE], none);
    mod_source_names[MOD_SRC_ONE] = malloc(strlen(one) + 1);
    strcpy(mod_source_names[MOD_SRC_ONE], one);

    for (i = MOD_SRC_FIRST_EG; i < MOD_SRC_LAST_EG; ++i)
    {
        id = i - MOD_SRC_FIRST_EG;
        if (adsr_names[id])
        {
            mod_source_names[i] = malloc(strlen(adsr_names[id]) + 1);
            strcpy(mod_source_names[i], adsr_names[id]);
        }
        else
        {
            debug("adsr_names mismatch adsr count\n");
            break;
        }
    }

    for (i = MOD_SRC_FIRST_GLFO; i < MOD_SRC_LAST_GLFO; ++i)
    {
        id = i - MOD_SRC_FIRST_GLFO;
        mod_source_names[i] = malloc(strlen(lfo_names[id]) + 1);
        strcpy(mod_source_names[i], lfo_names[id]);
    }

    for (i = MOD_SRC_FIRST_VLFO; i < MOD_SRC_LAST_VLFO; ++i)
    {
        id = (MOD_SRC_LAST_GLFO - MOD_SRC_FIRST_GLFO)
             + (i - MOD_SRC_FIRST_VLFO);
        mod_source_names[i] = malloc(strlen(lfo_names[id]) + 1);
        strcpy(mod_source_names[i], lfo_names[id]);
    }

    for (i = 0; i < MOD_SRC_LAST; ++i)
        printf("mod_source:%12s\tid:%4d\n", mod_source_names[i], i);

}


char** patch_mod_source_names(void)
{
    return mod_source_names;
}


const char** patch_adsr_names(void)
{
    int i;

    for (i = 0; adsr_names[i] != 0; ++i);

    if (i != VOICE_MAX_ENVS)
    {
        fprintf(stderr,
                "Friendly warning to the programmer:\n"
                "You've either changed the enum value for VOICE_MAX_ENVS\n"
                "Or you've changed the list of ADSR names\n"
                "In either case it's broken now. Please fix!\n");
        return 0;
    }

    return adsr_names;
}

const char** patch_lfo_names(void)
{
    int i;

    for (i = 0; lfo_names[i] != 0; ++i);

    if (i != TOTAL_LFOS)
    {
        fprintf(stderr,
                "Friendly warning to the programmer:\n"
                "You've either changed the enum value for PATCH_MAX_LFOS\n"
                "and/or ther enum value VOICE_MAX_LFOS\n"
                "Or you've changed the list of LFO names\n"
                "In either case it's broken now. Please fix!\n");
        return 0;
    }

    return lfo_names;
}

const char** patch_param_names(void)
{
    return param_names;
}

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
static Patch patches[PATCH_COUNT];

/*****************************************************************************/
/************************ PRIVATE GENERAL HELPER FUNCTIONS********************/
/*****************************************************************************/

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


/* a utility function which finds the PatchParam* associated with
 * a particular PatchParamType */
static int patch_get_param (PatchParam** p, int id, PatchParamType param)
{
    if (!isok (id))
        return PATCH_ID_INVALID;

    switch (param)
    {
    case PATCH_PARAM_AMPLITUDE:    *p = &patches[id].vol;      break;
    case PATCH_PARAM_PANNING:   *p = &patches[id].pan;      break;
    case PATCH_PARAM_CUTOFF:    *p = &patches[id].ffreq;    break;
    case PATCH_PARAM_RESONANCE: *p = &patches[id].freso;    break;
    case PATCH_PARAM_PITCH:     *p = &patches[id].pitch;    break;
    default:
        debug ("Invalid request for address of param\n");
        return PATCH_PARAM_INVALID;
    }

    return 0;
}

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



static float* mod_id_to_pointer(int id, Patch* p, PatchVoice* v)
{
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
    return 0;
}




/* triggers all global LFOs if they are used with amounts greater than 0 */
inline static void patch_trigger_global_lfos ( )
{
    int i,j;

    debug ("retriggering...\n");
    for (i = 0; i < PATCH_COUNT; i++)
    {
        Patch* p = &patches[i];
        for (j = 0; j < PATCH_MAX_LFOS; j++)
        {
            p->glfo[j].freq_mod1 =
                mod_id_to_pointer(p->glfo_params[j].mod1_id, p, NULL);

            p->glfo[j].freq_mod2 =
                mod_id_to_pointer(p->glfo_params[j].mod2_id, p, NULL);

            p->glfo[j].mod1_amt = p->glfo_params[j].mod1_amt;
            p->glfo[j].mod2_amt = p->glfo_params[j].mod2_amt;

            lfo_trigger(&patches[i].glfo[j], &patches[i].glfo_params[j]);
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

    create_mod_source_names();

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

    if (mod_source_names)
    {
        for (i = 0; i < MOD_SRC_LAST; ++i)
            if (mod_source_names[i])
                free(mod_source_names[i]);

        free(mod_source_names);
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
    v->declick_vol = 1.0;
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
        ffreq += *v->ffreq_mod1 * p->ffreq.mod2_amt;

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
    else if (v->released
            && !((p->play_mode & PATCH_PLAY_SINGLESHOT)
            && (v->relmode == RELEASE_NOTEOFF)))
    {
        /* if the patch is singleshot and it doesn't have a amplitude
         * envelope, we want to let it finish playing in toto
         */
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
         * applies when handling the envelopes below) */
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
    if (p->play_mode & PATCH_PLAY_LOOP)
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

    int id = (lfo_id < PATCH_MAX_LFOS)
                ? lfo_id
                : lfo_id - PATCH_MAX_LFOS;

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
/************************ VELOCITY FUNCTIONS ******************************/
/**************************************************************************/

int patch_set_vel_amount (int id, PatchParamType param, float amt)
{
    PatchParam* p;
    int err;
     
    if (!isok (id))
	return PATCH_ID_INVALID;

    if ((err = patch_get_param (&p, id, param)) < 0)
	return err;

    if (amt < 0.0 || amt > 1.0)
	return PATCH_PARAM_INVALID;

    p->vel_amt = amt;
    return 0;
}
     
int patch_get_vel_amount (int id, PatchParamType param, float* val)
{
    PatchParam* p;
    int err;
     
    if (!isok (id))
	return PATCH_ID_INVALID;

    if ((err = patch_get_param (&p, id, param)) < 0)
	return err;

    *val = p->vel_amt;
    return 0;
}

/*****************************************************************************/
/*************************** PARAMETER SETTERS *******************************/
/*****************************************************************************/

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
    

/* sets the start loop point */
int patch_set_loop_start (int id, int start)
{
    if (!isok (id))
	return PATCH_ID_INVALID;

    if (patches[id].sample->sp == NULL)
	return 0;

    if (start < patches[id].sample_start)
	start = patches[id].sample_start;
    else if (start > patches[id].sample_stop)
	start = patches[id].sample_stop;

    patches[id].loop_start = start;
    if (start > patches[id].loop_stop)
	patches[id].loop_stop = start;

    return 0;
}

/* sets the stopping loop point */
int patch_set_loop_stop (int id, int stop)
{
    if (!isok (id))
	return PATCH_ID_INVALID;

    if (patches[id].sample->sp == NULL)
	return 0;

    if (stop > patches[id].sample_stop)
	stop = patches[id].sample_stop;
    else if (stop < patches[id].sample_start)
	stop = patches[id].sample_start;

    patches[id].loop_stop = stop;
    if (stop < patches[id].loop_start)
	patches[id].loop_start = stop;

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
int patch_set_range (int id, int range)
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

/* set the point within the sample to begin playing */
int patch_set_sample_start (int id, int start)
{
    if (!isok (id))
	return PATCH_ID_INVALID;

    if (patches[id].sample->sp == NULL)
	return 0;

    if (start < 0)
    {
	debug ("refusing to set negative sample start point\n");
	return PATCH_PARAM_INVALID;
    }

    if (start > patches[id].sample_stop)
    {
	debug ("refusing to set incongruous sample start point\n");
	return PATCH_PARAM_INVALID;
    }

    patches[id].sample_start = start;
    return 0;
}

/* set the point within the sample to stop playing */
int patch_set_sample_stop (int id, int stop)
{
    if (!isok (id))
	return PATCH_ID_INVALID;

    if (patches[id].sample->sp == NULL)
	return 0;

    if (stop >= patches[id].sample->frames)
    {
	debug ("refusing to set sample stop greater than sample frame count\n");
	return PATCH_PARAM_INVALID;
    }

    if (stop < patches[id].sample_start)
    {
	debug ("refusing to set incongruous sample stop point\n");
	return PATCH_PARAM_INVALID;
    }

    patches[id].sample_stop = stop;
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

/* get the starting loop point */
int patch_get_loop_start (int id)
{
    if (!isok (id))
	return PATCH_ID_INVALID;
    return patches[id].loop_start;
}

/* get the stopping loop point */
int patch_get_loop_stop (int id)
{
    if (!isok (id))
	return PATCH_ID_INVALID;
    return patches[id].loop_stop;
}

/* get the lower note */
int patch_get_lower_note (int id)
{
    if (!isok (id))
	return PATCH_ID_INVALID;
    return patches[id].lower_note;
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
int patch_get_range (int id)
{
    if (!isok (id))
	return PATCH_ID_INVALID;
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

/* get the starting playback point */
int patch_get_sample_start (int id)
{
    if (!isok (id))
	return PATCH_ID_INVALID;
    return patches[id].sample_start;
}

/* get the stopping playback point */
int patch_get_sample_stop (int id)
{
    if (!isok (id))
	return PATCH_ID_INVALID;
    return patches[id].sample_stop;
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


/*****************************************************************************/
/************************* MODULATION HELPERS ********************************/
/*****************************************************************************/

static int get_patch_param(int patch_id, PatchParamType param, PatchParam** p)
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
        return PATCH_PARAM_INVALID;
    }

    return 0;
}

static int mod_src_ok(int id)
{
    if (id < MOD_SRC_NONE || id >= MOD_SRC_LAST)
        return PATCH_MOD_SRC_INVALID;
    return 0;
}


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
    int err;

    if (!isok(patch_id))
        return PATCH_ID_INVALID;

    *modsrc_id = patches[patch_id].vol.direct_mod_id;
    return 0;
}



/**************************************************************************/
/********************** MODULATION SETTERS ********************************/
/**************************************************************************/

int patch_set_lfo_mod1_src(int patch_id, int lfo_id, int modsrc_id)
{
    LFOParams* lfopar;
    int err; 
    if ((err = lfo_from_id(patch_id, lfo_id, NULL, &lfopar)))
        return err;
    lfopar->mod1_id = modsrc_id;
    return 0;
}

int patch_set_lfo_mod2_src(int patch_id, int lfo_id, int modsrc_id)
{
    LFOParams* lfopar;
    int err; 
    if ((err = lfo_from_id(patch_id, lfo_id, NULL, &lfopar)))
        return err;
    lfopar->mod2_id = modsrc_id;
    return 0;
}

int patch_set_lfo_mod1_amt(int patch_id, int lfo_id, float amount)
{
    LFOParams* lfopar;
    LFO* lfo;
    int err; 
    if ((err = lfo_from_id(patch_id, lfo_id, &lfo, &lfopar)))
        return err;
    lfopar->mod1_amt = amount;
    return 0;
}

int patch_set_lfo_mod2_amt(int patch_id, int lfo_id, float amount)
{
    LFOParams* lfopar;
    int err; 
    if ((err = lfo_from_id(patch_id, lfo_id, NULL, &lfopar)))
        return err;
    lfopar->mod2_amt = amount;
    return 0;
}



/**************************************************************************/
/********************** MODULATION GETTERS ********************************/
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
