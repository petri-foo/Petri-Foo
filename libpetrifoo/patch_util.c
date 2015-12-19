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


#include "patch_util.h"

#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <pthread.h>


#include "petri-foo.h"
#include "maths.h"
#include "ticks.h"
#include "patch.h"
#include "pf_error.h"
#include "sample.h"
#include "adsr.h"
#include "lfo.h"
#include "driver.h"     /* for DRIVER_DEFAULT_SAMPLERATE */
#include "midi.h"       /* for MIDI_CHANS */
#include "patch_set_and_get.h"
#include "midi_control.h"


#include "patch_private/patch_data.h"
#include "patch_private/patch_defs.h"
#include "patch_private/patch_macros.h"


/**************************************************************************/
/********************** PRIVATE GENERAL HELPER FUNCTIONS*******************/
/**************************************************************************/


/*  inline definitions shared by patch and patch_util:
 *                          (see private/patch_macros.h)
 */
INLINE_PATCHOK_DEF
INLINE_PATCH_LOCK_DEF
INLINE_PATCH_TRYLOCK_DEF
INLINE_PATCH_UNLOCK_DEF
INLINE_PATCH_TRIGGER_GLOBAL_LFO_DEF


/* triggers all global LFOs if they are used with amounts greater than 0 */
void patch_trigger_global_lfos ( )
{
    int patch_id, lfo_id;

    debug ("retriggering global LFOs...\n");

    for (patch_id = 0; patch_id < PATCH_COUNT; patch_id++)
    {
        if (!patches[patch_id] || !patches[patch_id]->active)
            continue;

        for (lfo_id = 0; lfo_id < PATCH_MAX_LFOS; lfo_id++)
        {
            LFO* lfo =          patches[patch_id]->glfo[lfo_id];
            LFOParams* lfopar = &patches[patch_id]->glfo_params[lfo_id];
            patch_trigger_global_lfo(patch_id, lfo, lfopar);
        }
    }

    debug ("done\n");
}


void patch_do_display_index(int id)
{
    int i;

    debug("calculating display index for patch id:%d\n", id);

    assert(patches[id]->display_index == -1);

    for (i = 0; i < PATCH_COUNT; i++)
    {
        if (i == id)
            continue;

        if (patches[i] && patches[i]->active
         && patches[i]->display_index >= patches[id]->display_index)
        {
            patches[id]->display_index = patches[i]->display_index + 1;
        }
    }

    if (patches[id]->display_index == -1)
        patches[id]->display_index = 0;

    debug("chosen display: %d\n", patches[id]->display_index);
}

/**************************************************************************/
/********************** UTILITY FUNCTIONS *********************************/
/**************************************************************************/

/* returns the number of patches currently active */
int patch_count()
{
    int id, count;

    for (id = count = 0; id < PATCH_COUNT; id++)
        if (patches[id] && patches[id]->active)
            count++;

    return count;
}


int patch_create(void)
{
    Patch* p;
    int id;

    /* find first unused patch */
    for (id = 0; patches[id] && patches[id]->active; ++id)
    {
        if (id == PATCH_COUNT)
        {
            pf_error(PF_ERR_PATCH_COUNT);
            return -1;
        }
    }

    if (!(p = patch_new()))
    {
        pf_error(PF_ERR_PATCH_ALLOC);
        return -1;
    }

    debug("creating patch id:%d (%p)\n", id, p);

    patches[id] = p;
    patch_lock(id);
    p->active = true;
    patch_do_display_index(id);
    patch_unlock(id);

    return id;
}


int patch_duplicate(int src_id)
{
    int id;

    assert(patchok(src_id));
    assert(patches[src_id]->active);

    debug("\n\nDuplicating patch %s id:%d...\n",
                patches[src_id]->name, src_id);

    if ((id = patch_create()) < 0)
        return -1;

    patch_lock(id);
    patch_copy(patches[id], patches[src_id]);
    patch_unlock(id);

    return id;
}


int patch_create_default(void)
{
    int id;
    Patch* p;
    ADSRParams* eg1;

    if ((id = patch_create()) < 0)
        return id;

    p = patches[id];

    patch_lock(id);

    p->play_mode = PATCH_PLAY_LOOP;
    p->fade_samples =  DEFAULT_FADE_SAMPLES;
    p->xfade_samples = DEFAULT_FADE_SAMPLES;

    /* adsr */
    eg1 = &p->env_params[0];
    eg1->active  = true;
    eg1->attack  = 0.005;
    eg1->release = 0.375;
    eg1->key_amt = -0.99;
    eg1->exp     = false;

    /* controllers... */

    /* pitch */
    patch_param_set_mod_src(id, PATCH_PARAM_PITCH, 0, MOD_SRC_PITCH_WHEEL);
    patch_param_set_mod_amt(id, PATCH_PARAM_PITCH, 0,
                                        6.0f / PATCH_MAX_PITCH_STEPS);

    patch_param_set_mod_src(id, PATCH_PARAM_PITCH, 1, MOD_SRC_VLFO);
    patch_param_set_mod_amt(id, PATCH_PARAM_PITCH, 1,
                                        2.0f / PATCH_MAX_PITCH_STEPS);

    /* AMPLITUDE EG_MOD_SLOT has full effect, no need to set amount */
    patch_param_set_mod_src(id, PATCH_PARAM_AMPLITUDE,  EG_MOD_SLOT,
                                                        MOD_SRC_EG);
    patch_param_set_mod_src(id, PATCH_PARAM_AMPLITUDE, 0,
                                MOD_SRC_MIDI_CC | CC_CHANNEL_VOLUME);
    patch_param_set_mod_amt(id, PATCH_PARAM_AMPLITUDE, 0, 1.0f);

    /* pan */
    patch_param_set_mod_src(id, PATCH_PARAM_PANNING, 0,
                                MOD_SRC_MIDI_CC | CC_PAN);
    patch_param_set_mod_amt(id, PATCH_PARAM_PANNING, 0, 1.0f);

    /* filter cutoff */
    patch_param_set_value(  id, PATCH_PARAM_CUTOFF, 0.5f);
    patch_param_set_mod_src(id, PATCH_PARAM_CUTOFF, 0,
                                MOD_SRC_MIDI_CC | CC_SNDCTRL5_BRIGHTNESS);
    patch_param_set_mod_amt(id, PATCH_PARAM_CUTOFF, 0, 1.0f);

    /* filter resonance */
    patch_param_set_value(  id, PATCH_PARAM_RESONANCE, 0.0f);
    patch_param_set_mod_src(id, PATCH_PARAM_RESONANCE, 0,
                                MOD_SRC_MIDI_CC | CC_SNDCTRL2_TIMBRE);
    patch_param_set_mod_amt(id, PATCH_PARAM_RESONANCE, 0, 0.975f);


    /* setup VLFO0 to provide the pitch modulation */
    patch_set_lfo_active(   id, MOD_SRC_VLFO, true);
    patch_set_lfo_freq(     id, MOD_SRC_VLFO, 9);

    /* and the MOD WHEEL to modulate VLFO0 amplitude */
    patch_set_lfo_am1_src(  id, MOD_SRC_VLFO,
                                MOD_SRC_MIDI_CC | CC_MOD_WHEEL);
    patch_set_lfo_am1_amt(  id, MOD_SRC_VLFO, 1.0);

    patch_unlock(id);

    patch_sample_load(id, "Default", 0, 0, 0, false);
    p->lower_note = 36;
    p->upper_note = 83;
    p->lower_vel  = 0;
    p->upper_vel  = 127;

    patch_set_name(id, "Default");

    return id;
}


void patch_destroy(int id)
{
    int index;
    Patch* p;

    assert(patchok(id));

    debug ("Removing patch: %d\n", id);

    index = patches[id]->display_index;

    patch_lock(id);
    p = patches[id];
    patches[id]->active = false;
    patch_unlock(id);

    patches[id] = 0;
    patch_free(p);

    /* every active patch with a display_index greater than this
     * patch's needs to have it's value decremented so that we
     * preservere continuity; no locking necessary because the
     * display_index is not thread-shared data */

    for (id = 0; id < PATCH_COUNT; id++)
    {
        if (patches[id] && patches[id]->active
                        && patches[id]->display_index > index)
        {
            --patches[id]->display_index;
        }
    }
}



/* destroy all patches */
void patch_destroy_all(void)
{
    int id;

    for (id = 0; id < PATCH_COUNT; id++)
        if (patches[id])
            patch_destroy (id);

    return;
}


/* place all patch ids, sorted in ascending order by channels and then
   notes, into array 'id' and return number of patches */
int patch_dump(int **dump)
{
    int i, j, k, id, count, tmp;

    *dump = NULL;

    /* determine number of patches */
    count = patch_count();

    if (count == 0)
        return count;

    /* allocate dump */
    *dump = malloc(sizeof(int) * count);
    if (*dump == NULL)
    {
        pf_error(PF_ERR_PATCH_DUMP_ALLOC);
        return -1;
    }

    /* place active patches into dump array */
    for (id = i = 0; id < PATCH_COUNT; id++)
        if (patches[id] && patches[id]->active)
            (*dump)[i++] = id;

    /* sort dump array by channel in ascending order */
    for (i = 0; i < count; i++)
    {
        for (j = i; j < count; j++)
        {
            if (patches[(*dump)[j]]->channel <
                patches[(*dump)[i]]->channel)
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
            if (patches[(*dump)[j]]->channel != i)
                continue;

            for (k = j; k < count; k++)
            {
                if (patches[(*dump)[k]]->channel != i)
                    continue;

                if (patches[(*dump)[k]]->root_note <
                    patches[(*dump)[j]]->root_note)
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


/* stop all currently playing voices in given patch */
int patch_flush (int id)
{
    int i;

    assert(patchok(id));

    debug("flusing:%d\n",id);

    patch_lock (id);

    if (patches[id]->sample->sp == NULL)
    {
        patch_unlock (id);
        return 0;
    }

    for (i = 0; i < PATCH_VOICE_COUNT; i++)
    {
        debug("flushing voice:%d\n",i);
        patches[id]->voices[i]->active = false;
    }

    patch_unlock (id);

    debug("done\n");
    return 0;
}

/* stop all voices for all patches */
void patch_flush_all ( )
{
    int i;

    for (i = 0; i < PATCH_COUNT; i++)
        if (patches[i])
            patch_flush (i);
}


/* loads a sample file for a patch */
int patch_sample_load(int id, const char *name,
                                    int raw_samplerate,
                                    int raw_channels,
                                    int sndfile_format,
                                    bool sampleinfo)
{
    int val;
    double ratio = (patch_samplerate == 44100)
                            ? 1
                            : (patch_samplerate / 44100.0f);
    int frames;

    bool defsample = (strcmp(name, "Default") == 0);

    assert(patchok(id));
    assert(name != NULL);

    debug("Loading sample %s for patch %d\n", name, id);
    patch_flush (id);

    /* we lock *after* we call patch_flush because patch_flush does
     * its own locking */
    patch_lock (id);

    if (defsample)
        val = sample_default(patches[id]->sample, patch_samplerate);
    else
        val = sample_load_file(patches[id]->sample, name,
                                            patch_samplerate,
                                            raw_samplerate,
                                            raw_channels,
                                            sndfile_format,
                                            1);

    if (val < 0)
        frames = 0;
    else
        frames = patches[id]->sample->frames - 1;

    patches[id]->sample_stop = frames;

    patches[id]->play_start = 0;
    patches[id]->play_stop = patches[id]->sample_stop;

    if (defsample)
    {
        patches[id]->loop_start = 296 * ratio;
        patches[id]->loop_stop = 5203 * ratio;
        patches[id]->fade_samples = 100 * ratio;
        patches[id]->xfade_samples = 0;
    }
    else
    {
        /* read loop info from the sample file */
        if (sampleinfo && patches[id]->sample->loop_valid)
        {
            debug("Setting loop points from sample l1:%d l2:%d\n",
                                            patches[id]->sample->loop_start,
                                            patches[id]->sample->loop_end);
            patches[id]->fade_samples = 0;
            patches[id]->xfade_samples = 0;
            patches[id]->loop_start = patches[id]->sample->loop_start;
            patches[id]->loop_stop = patches[id]->sample->loop_end;
        }
        else
        {
            patches[id]->fade_samples = (frames / 2 > 100) ? 100 * ratio : 0;
            patches[id]->xfade_samples = (frames / 2 > 100) ? 100 * ratio : 0;
            patches[id]->loop_start = patches[id]->xfade_samples;
            patches[id]->loop_stop = patches[id]->sample_stop -
                                        patches[id]->xfade_samples;
        }
    }

    if (patches[id]->sample_stop < patches[id]->fade_samples)
        patches[id]->fade_samples = patches[id]->xfade_samples = 0;

    patch_unlock (id);
    return val;
}


#ifdef UNUSED
int patch_sample_load_from(int dest_id, int src_id)
{
    int val;
    const char* name;
    bool defsample;

    assert(patchok(dest_id));
    assert(patchok(src_id));

    name = patches[src_id]->sample->filename;
    defsample = (strcmp(name, "Default") == 0);


    debug ("Duplicating sample %s from patch %d to patch %d\n",
            name,   src_id,     dest_id);

    /* lock *after* calling patch_flush because patch_flush
     * does its own locking
     */
    patch_flush(dest_id);
    patch_lock(dest_id);

    sample_shallow_copy(patches[dest_id]->sample, patches[src_id]->sample);

    if (defsample)
        val = sample_default(patches[dest_id]->sample, patch_samplerate);
    else
        val = sample_load_file( patches[dest_id]->sample,
                                name, patch_samplerate,
                                patches[src_id]->sample->raw_samplerate,
                                patches[src_id]->sample->raw_channels,
                                patches[src_id]->sample->sndfile_format,
                                1);
    patch_unlock(dest_id);
    return val;
}
#endif


int patch_sample_set_points(int id, int play_start,     int play_stop,
                                    int loop_start,     int loop_stop,
                                    int fade, int xfade)
{
    assert(patchok(id));
    patches[id]->play_start = play_start;
    patches[id]->play_stop = play_stop;
    patches[id]->loop_start = loop_start;
    patches[id]->loop_stop = loop_stop;
    patches[id]->fade_samples = fade;
    patches[id]->xfade_samples = xfade;
    return 0;
}


const Sample* patch_sample_data(int id)
{
    assert(patchok(id));
    return patches[id]->sample;
}


/* unloads a patch's sample */
void patch_sample_unload (int id)
{
    assert(patchok(id));

    debug ("Unloading sample for patch %d\n", id);
    patch_lock (id);

    sample_free_data(patches[id]->sample);

    patches[id]->play_start = 0;
    patches[id]->play_stop = 0;
    patches[id]->loop_start = 0;
    patches[id]->loop_stop = 0;

    patch_unlock (id);
}

/* sets our buffersize and reallocates our lfo_tab; this function
 * doesn't need to do any locking because we have a guarantee that
 * mixing will stop when the buffersize changes */
void patch_set_buffersize (int nframes)
{
    int i;

    int oldsize = patch_buffersize;

    debug ("setting buffersize to %d\n", nframes);

    patch_buffersize = nframes;

    if (patch_buffersize != oldsize)
    {
        for (i = 0; i < PATCH_COUNT; i++)
        {
            if (patches[i])
                patch_set_global_lfo_buffers(patches[i], patch_buffersize);
        }
    }
}

/* sets our samplerate and resamples if necessary; this function
 * doesn't need to do any locking because we have a guarantee that
 * mixing will stop when the samplerate changes */
void patch_set_samplerate (int rate)
{
    int oldrate = patch_samplerate;

    debug ("changing samplerate to %d\n", rate);

    patch_samplerate = rate;

    if (patch_samplerate != oldrate)
    {
        int id;

        debug("samplerate changed from %d\n", oldrate);

        for (id = 0; id < PATCH_COUNT; id++)
        {
            if (!patches[id] || !patches[id]->active)
                continue;

            if (patches[id]->sample->sp != NULL)
            {
                Sample* s = patches[id]->sample;
                patch_sample_load(id, s->filename,  s->raw_samplerate,
                                                    s->raw_channels,
                                                    s->sndfile_format,
                                                    false);
            }
        }

        patch_legato_lag = PATCH_LEGATO_LAG * rate;
        patch_trigger_global_lfos();
    }
}


int patch_get_samplerate(void)
{
    return patch_samplerate;
}


/* destructor */
void patch_shutdown(void)
{
    int i;

    debug ("shutting down...\n");

    for (i = 0; i < PATCH_COUNT; i++)
        patch_free(patches[i]);

    debug ("done\n");
}

/* re-sync all global lfos to new tempo */
void patch_sync(float bpm)
{
    debug("syncing global LFOs\n");
    lfo_set_tempo(bpm);
    patch_trigger_global_lfos();
}


#if DEBUG
void patch_summary_dump(void)
{
    int i;

    debug("Patch summary\n");
    debug("-------------\n");

    for (i = 0; i < PATCH_COUNT; ++i)
    {
        Patch* p = patches[i];
        if (p)
        {
            debug("patch (%p), id:%d '%s' display index:%d active:%s\n",
                            p, i, p->name, p->display_index,
                            (p->active) ? "Y" : "N");
        }
    }
    debug("End of patch summary\n");
    debug("--------------------\n");
}
#endif

