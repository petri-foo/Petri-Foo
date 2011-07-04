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


#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <pthread.h>

#include "petri-foo.h"
#include "maths.h"
#include "ticks.h"
#include "patch.h"
#include "sample.h"
#include "adsr.h"
#include "lfo.h"
#include "driver.h" /* for DRIVER_DEFAULT_SAMPLERATE    */
#include "midi.h"   /* for MIDI_CHANS                   */


#include "patch_private/patch_data.h"
#include "patch_private/patch_defs.h"
#include "patch_private/patch_macros.h"


/*  MIDI controller outputs 
 *      pitch wheel =           index 0
 *      CC x =                  index x + 1
 */
static float cc[16][CC_ARR_SIZE];


/**************************************************************************/
/********************** PRIVATE GENERAL HELPER FUNCTIONS*******************/
/**************************************************************************/


/*  inline definitions shared by patch and patch_util:
 *                          (see private/patch_data.h)
 */
INLINE_ISOK_DEF
INLINE_PATCH_LOCK_DEF
INLINE_PATCH_TRYLOCK_DEF
INLINE_PATCH_UNLOCK_DEF


/**************************************************************************/
/****************** PLAYBACK AND RENDERING FUNCTIONS **********************/
/**************************************************************************/


inline static void playstate_init_fade_in(Patch* p, PatchVoice* v)
{
    if (p->fade_samples)
    {
        v->playstate = PLAYSTATE_FADE_IN;
        v->fade_posi = 0;
        v->fade_posf = 0;
        v->fade_declick = 0.0;
    }
    else
    {
        v->playstate = (p->play_mode & PATCH_PLAY_LOOP)
                                     ? PLAYSTATE_LOOP
                                     : PLAYSTATE_PLAY;
        v->fade_declick = 1.0;
    }

    if (p->play_mode & PATCH_PLAY_REVERSE)
    {
        v->posi = p->play_stop;
        v->posf = 0;
        v->dir = -1;
        v->fade_out_start_pos = p->play_start + p->fade_samples;
    }
    else
    {
        v->posi = p->play_start;
        v->posf = 0;
        v->dir = 1;
        v->fade_out_start_pos = p->play_stop - p->fade_samples;
    }
}


inline static void playstate_init_x_fade(Patch* p, PatchVoice* v)
{
    if (!p->xfade_samples)
        return;

    v->xfade = true;
    v->xfade_point_posi = v->posi;
    v->xfade_point_posf = v->posf;
    v->xfade_posi = 0;
    v->xfade_posf = 0;
    v->xfade_dir = v->dir;
    v->xfade_declick = 0.0;
}


inline static void playstate_init_fade_out(Patch* p, PatchVoice* v)
{
    if (v->playstate == PLAYSTATE_FADE_OUT)
        return;

    if (!p->fade_samples)
    {
        v->playstate = PLAYSTATE_OFF;
        v->fade_declick = 0.0;
        return;
    }

    if (v->playstate == PLAYSTATE_FADE_IN)
    {
        v->fade_posi = p->fade_samples - v->fade_posi;
    }
    else
    {
        v->fade_posi = 0;
        v->fade_declick = 1.0;
    }

    v->playstate = PLAYSTATE_FADE_OUT;
}


/* a helper function to release all voices matching a given criteria
 * (if note is a negative value, all active voices will be released) */
inline static void patch_release_patch(Patch* p, int note, release_t mode)
{
    int i;

    for (i = 0; i < PATCH_VOICE_COUNT; i++)
    {
        if (p->voices[i]->active
        && (p->voices[i]->note == note || note < 0))
        {
            /* we don't really release here, that's the job of
             * advance( ); we just tell it *when* to release */
            p->voices[i]->relmode = mode;
            p->voices[i]->relset = (p->mono && p->legato.active)
                                        ? patch_legato_lag
                                        : 0;
        }
    }
}


/* a helper function to cut all patches whose cut_by value matches the
 * cut value for the given patch */
inline static void patch_cut_patch (Patch* p)
{
    int i;

    /* a cut value of zero is ignored so that the user has a way
     * of *not* using cuts */
    if (p->cut == 0)
        return;

    for (i = 0; i < PATCH_COUNT; i++)
    {
        if (patches[i]
         && patches[i]->active
         && patches[i]->cut_by == p->cut)
        {
            patch_release_patch(patches[i], -69, RELEASE_CUTOFF);
        }
    }
}


/* a helper function to prepare a voice's pitch information */
inline static void prepare_pitch(Patch* p, PatchVoice* v, int note)
{
    double scale; /* base pitch scaling factor */

    /* this applies the tuning factor */
    scale = pow(2, (p->pitch.val * p->pitch_steps) / 12.0);

    if (p->porta.active
    && (p->porta_secs.value > 0.0)
    && (p->last_note != note))
    {
        /* we calculate the pitch here because we can't be certain
         * what the current value of the last voice's pitch is */
        v->pitch =
            pow(2, (p->last_note - p->note) * p->pitch.key_amt / 12.0);
        v->pitch *= scale;
        v->porta_ticks = ticks_secs_to_ticks (p->porta_secs.value);

        /* calculate the value to be added to pitch each tick by
         * subtracting the target pitch from the initial pitch and
         * dividing by porta_ticks */
        v->pitch_step =
            ((pow(2, (note - p->note) * p->pitch.key_amt / 12.0)
                             * scale) - v->pitch) / v->porta_ticks;
    }
    else
    {
        v->pitch = pow(2, (note - p->note) * p->pitch.key_amt / 12.0);
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
    v->stepf = (v->pitch - v->stepi) * (0xFFFFFFFFU); /* max of guint32 */
}


/*
inline static void patch_bool_mod(PatchBool* pb, Patch* p)
{
    pb->val = (pb->set && pb->mod_id)
        ? (*(patch_mod_id_to_pointer(pb->mod_id, p, NULL)) > pb->thresh)
        : pb->set;
}

inline static void patch_float_mod(PatchFloat* pp, Patch* p)
{
    pp->val = pp->set;

    if (pp->mod_id)
    {
        const float* mod = patch_mod_id_to_pointer(pp->mod_id, p, NULL);
        pp->val += *mod * pp->mod_amt;
    }
}
*/

inline static void patch_bool_mod(PatchBool* pb, Patch* p)
{
    const float* mod = patch_mod_id_to_pointer(pb->mod_id, p, NULL);
    pb->active = (pb->on && pb->mod_id)
                        ? (*mod > pb->thresh)
                        : pb->on;
}

inline static void patch_float_mod(PatchFloat* pf, Patch* p)
{
    pf->value = pf->assign;

    if (pf->mod_id)
    {
        const float* mod = patch_mod_id_to_pointer(pf->mod_id, p, NULL);
        pf->value += *mod * pf->mod_amt;
    }
}


/**************************************************************************
 *********************     PATCH TRIGGER PATCH      ***********************
 **************************************************************************/

/* a helper function to activate a voice for a patch */
inline static void
patch_trigger_patch (Patch* p, int note, float vel, Tick ticks)
{
    int i;
    PatchVoice* v;
    int index;          /* the index we ended up settling on */
    float key_track;

    if (p->sample->sp == NULL)
        return;

    if (p->upper_note == p->lower_note)
        key_track = 1.0;
    else
        key_track = (float)(note - p->lower_note)
                                / (p->upper_note - p->lower_note);

    /* boolean modulation */
    patch_bool_mod(&p->porta, p);
    patch_bool_mod(&p->legato, p);
    patch_float_mod(&p->porta_secs, p);

    if (p->mono && p->legato.active)
    {
        /*  half of the previous logic operating here was ignored.
         *  removing it left only logic which could be simplified
         *  to the following:
         */

        v = p->voices[0];
        index = 0;

        if (!v->active || v->released)
            v->vel = vel;
        else
        {
            v->ticks =      ticks;
            v->note =       note;
            v->vel =        vel;
            v->relset =     -1;	/* cancel any pending release */
            v->relmode =    RELEASE_NONE;
            v->released =   false;
            v->to_end =     false;
            v->xfade =      false;
            v->loop =       p->play_mode & PATCH_PLAY_LOOP;
            v->key_track =  key_track;

            prepare_pitch(p, v, note);

            return;
        }
    }
    else /* mono w/o legato, or poly */
    {    
        int oldest;
        Tick oldestticks = ticks;

        /* find a free voice slot and determine the oldest running voice */
        for (i = 0; i < PATCH_VOICE_COUNT; ++i)
        {
            if (p->voices[i]->ticks <= oldestticks)
            {
                oldestticks = p->voices[i]->ticks;
                oldest = i;
            }

            if (!p->voices[i]->active)
                break;
        }

        /* take the oldest running voice's slot if we couldn't find an
         * empty one */
        index = (i == PATCH_VOICE_COUNT) ? oldest : i;
    }

    v = p->voices[index];

    /* shutdown any running voices if monophonic */
    if (p->mono)
        patch_release_patch(p, -69, RELEASE_CUTOFF);

    /* fill in our voice */
    v->ticks =      ticks;
    v->relset =     -1; /* N/A at this time */
    v->relmode =    RELEASE_NONE;
    v->released =   false;
    v->to_end =     false; /* TRUE after loop */
    v->xfade =      false;
    v->loop =       p->play_mode & PATCH_PLAY_LOOP;
    v->note =       note;
    v->key_track =  key_track;

    if (!(p->mono && p->legato.active))
        v->vel = vel;

    if (!p->mono)
        v->fll = v->fbl = v->flr = v->fbr = 0;

    for (i = 0; i < MAX_MOD_SLOTS; ++i)
    {
        v->vol_mod[i] = patch_mod_id_to_pointer(p->vol.mod_id[i], p, v);
        v->pan_mod[i] = patch_mod_id_to_pointer(p->pan.mod_id[i], p, v);
        v->ffreq_mod[i] = patch_mod_id_to_pointer(p->ffreq.mod_id[i], p, v);
        v->freso_mod[i] = patch_mod_id_to_pointer(p->freso.mod_id[i], p, v);
        v->pitch_mod[i] = patch_mod_id_to_pointer(p->pitch.mod_id[i], p, v);
    }

    if (!(p->mono && p->legato.active && v->active))
        playstate_init_fade_in(p, v);

    prepare_pitch(p, v, note);

    for (i = 0; i < VOICE_MAX_ENVS; i++)
    {
        if (p->env_params[i].env_on)
        {
            adsr_set_params(v->env[i], &p->env_params[i]);
            adsr_trigger(v->env[i], key_track, vel);
        }
    }

    for (i = 0; i < VOICE_MAX_LFOS; i++)
    {
        if (p->vlfo_params[i].lfo_on)
        {
            float const* src;

            src = patch_mod_id_to_pointer(p->vlfo_params[i].fm1_id, p, v);
            lfo_set_fm1(v->lfo[i], src);
            src = patch_mod_id_to_pointer(p->vlfo_params[i].fm2_id, p, v);
            lfo_set_fm2(v->lfo[i], src);

            src = patch_mod_id_to_pointer(p->vlfo_params[i].am1_id, p, v);
            lfo_set_am1(v->lfo[i], src);
            src = patch_mod_id_to_pointer(p->vlfo_params[i].am2_id, p, v);
            lfo_set_am2(v->lfo[i], src);
            lfo_trigger(v->lfo[i], &p->vlfo_params[i]);
        }
    }

    /* mark our territory */
    v->active = true;
}


/*  a helper routine to determine the pitch-scaled sample values to use
 *  for a frame
 */
inline static void
pitchscale (Patch * p, PatchVoice * v, float *l, float *r)
{
    int y0, y1, y2, y3;

    /* determine sample indices */
    y0 = (v->posi - 1 * v->dir) * 2;
    y1 = (v->posi + 0 * v->dir) * 2;
    y2 = (v->posi + 1 * v->dir) * 2;
    y3 = (v->posi + 2 * v->dir) * 2;

    if (y0 < 0 || y0 >= p->sample->frames * 2)
        y0 = 0;

    if (y2 < 0 || y2 >= p->sample->frames * 2)
        y2 = 0;

    if (y3 < 0 || y3 >= p->sample->frames * 2)
        y3 = 0;

    /* interpolate */
    *l = cerp(  p->sample->sp[y0],
                p->sample->sp[y1],
                p->sample->sp[y2],
                p->sample->sp[y3],      v->posf >> 24);

    *r = cerp(  p->sample->sp[y0 + 1],
                p->sample->sp[y1 + 1],
                p->sample->sp[y2 + 1],
                p->sample->sp[y3 + 1],  v->posf >> 24);

    if (v->xfade)
    {
        /* apply fade-in of x-fade */
        *l *= v->xfade_declick;
        *r *= v->xfade_declick;

        /* determine sample indices */
        y0 = (v->xfade_point_posi - 1 * v->xfade_dir) * 2;
        y1 = (v->xfade_point_posi + 0 * v->xfade_dir) * 2;
        y2 = (v->xfade_point_posi + 1 * v->xfade_dir) * 2;
        y3 = (v->xfade_point_posi + 2 * v->xfade_dir) * 2;

        if (y0 < 0 || y0 >= p->sample->frames * 2)
            y0 = 0;

        if (y1 < 0 || y1 >= p->sample->frames * 2)
        {
            debug("xfade:%s xfade_point_posi out of range:%d frames:%d\n",
                    (v->xfade ? "YES" : "NO"),
                    v->xfade_point_posi, p->sample->frames);
            debug("xfade_samples:%d xfade_posi:%d\n",
                    p->xfade_samples, v->xfade_posi);
            y1 = 0;
        }

        if (y2 < 0 || y2 >= p->sample->frames * 2)
            y2 = 0;

        if (y3 < 0 || y3 >= p->sample->frames * 2)
            y3 = 0;

        /* interpolate */
        *l += cerp( p->sample->sp[y0],
                    p->sample->sp[y1],
                    p->sample->sp[y2],
                    p->sample->sp[y3],      v->xfade_point_posf >> 24)
                                            * (1.0 - v->xfade_declick);

        *r += cerp( p->sample->sp[y0 + 1],
                    p->sample->sp[y1 + 1],
                    p->sample->sp[y2 + 1],
                    p->sample->sp[y3 + 1],  v->xfade_point_posf >> 24)
                                            * (1.0 - v->xfade_declick);
    }
}


/* a helper routine to setup panning of two samples in a frame  */
inline static void
pan (Patch * p, PatchVoice * v, int index, float *l, float *r)
{
    int i;
    float pan;

    /* get pan value */
    pan = p->pan.val;

    for (i = 0; i < MAX_MOD_SLOTS; ++i)
        if (v->pan_mod[i] != NULL)
            pan += *v->pan_mod[i] * p->pan.mod_amt[i];

    /* scale for velocity tracking */
    pan = lerp(pan, pan * v->vel, p->pan.vel_amt);

    /* scale for key tracking */
    if (p->pan.key_amt < 0)
        pan = lerp(pan, pan * (1.0 - v->key_track), p->pan.key_amt * -1);
    else
        pan = lerp(pan, pan * v->key_track, p->pan.key_amt);

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
inline static void
filter (Patch* p, PatchVoice* v, int index,  float* l, float* r)
{
    int i;
    float ffreq, freso, logreso;

    /* get filter cutoff frequency */
    ffreq = p->ffreq.val;

    for (i = 0; i < MAX_MOD_SLOTS; ++i)
        if (v->ffreq_mod[i] != NULL)
            ffreq += *v->ffreq_mod[i] * p->ffreq.mod_amt[i];

    /* scale to velocity */
    ffreq = lerp (ffreq, ffreq * v->vel, p->ffreq.vel_amt);

    /* scale for key tracking */
    if (p->ffreq.key_amt < 0)
        ffreq = lerp(   ffreq,
                        ffreq * (1.0 - v->key_track),
                        p->ffreq.key_amt * -1);
    else
        ffreq = lerp(   ffreq, ffreq * v->key_track, p->ffreq.key_amt);

    /* clip */
    if (ffreq > 1.0)
        ffreq = 1.0;
    else if (ffreq < 0.0)
        ffreq = 0.0;

    /* get filter resonant frequency */
    freso = p->freso.val;

    for (i = 0; i < MAX_MOD_SLOTS; ++i)
        if (v->freso_mod[i] != NULL)
            freso += *v->freso_mod[i] * p->freso.mod_amt[i];

    /* scale to velocity */
    freso = lerp(freso, freso * v->vel, p->freso.vel_amt);

    /* scale for key tracking */
    if (p->freso.key_amt < 0)
        freso = lerp(   freso,
                        freso * (1.0 - v->key_track),
                        p->freso.key_amt * -1);
    else
        freso = lerp(   freso, freso * v->key_track, p->freso.key_amt);

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
inline static int
gain (Patch* p, PatchVoice* v, int index, float* l, float* r)
{
    int i;
    float vol = 0.0;
    float logvol = 0.0;

    /* first, we use our set value as a base */
    vol = p->vol.val;

    for (i = 0; i < EG_MOD_SLOT; ++i)
        if (v->vol_mod[i] != NULL)
            vol += *v->vol_mod[i] * p->vol.mod_amt[i];

    /* direct modulation source (ie no amount) */
    if (v->vol_mod[EG_MOD_SLOT])
        vol *= *v->vol_mod[EG_MOD_SLOT];

    /* scale for key tracking */
    if (p->vol.key_amt < 0)
        vol = lerp(vol, vol * (1.0 - v->key_track), p->vol.key_amt * -1);
    else
        vol = lerp(vol, vol * v->key_track, p->vol.key_amt);

    /* velocity should be the last parameter considered because it
     * has the most "importance" */
    vol = lerp(vol, vol * v->vel, p->vol.vel_amt);

    /* apply fade in/out */
    vol *= v->fade_declick;

    /* clip */
    if (vol > 1.0)
        vol = 1.0;
    else if (vol < 0.0)
        vol = 0.0;

    /* as a last step, make logarithmic */
/*   logvol = log_amplitude(vol);
*/

    /* adjust amplitude */

    *l *= vol;
    *r *= vol;

/*
    *l *= logvol;
    *r *= logvol;
*/
    /* check to see if we've finished a release */
    if (v->released && (v->fade_declick == 0.0f //< ALMOST_ZERO
                    || (v->vol_mod[EG_MOD_SLOT] 
                    && *v->vol_mod[EG_MOD_SLOT] < ALMOST_ZERO)))
    {
        return -1;
    }

    return 0;
}


inline static void advance_pos(int dir, int* posi,  uint32_t* posf,
                                        int stepi,  uint32_t stepf)
{
    uint32_t next_posf = *posf + stepf;

    if (dir > 0)
    {
        if (next_posf < *posf) /* unsigned int wraps around */
            ++(*posi);

        *posi += stepi;
    }
    else
    {
        if (next_posf < *posf) /* unsigned int wraps around */
            --(*posi);

        *posi -= stepi;
    }

    *posf = next_posf;
}


inline static void advance_fwd(int* posi,  uint32_t* posf,
                               int stepi,  uint32_t stepf)
{
    uint32_t next_posf = *posf + stepf;

    if (next_posf < *posf) /* unsigned int wraps around */
        ++(*posi);

    *posi += stepi;
    *posf = next_posf;
}


/* a ;-| helper |-; routine to advance to the next frame while properly
 * accounting for the different possible play modes (negative value
 * returned if we are out of samples after doing our work) */
inline static int advance (Patch* p, PatchVoice* v, int index)
{
    int i,j;
    double pitch;
    double scale;
    bool recalc = false;
        /* whether we need to recalculate our pos/step vars */

    /* portamento */
    if (p->porta.active && v->porta_ticks)
    {
        recalc = true;
        v->pitch += v->pitch_step;
        --(v->porta_ticks);
    }

    /* base pitch value */
    pitch = v->pitch;

    if (p->pitch_bend)
    {
        recalc = true;
        pitch *= p->pitch_bend;
    }

    for (i = 0; i < MAX_MOD_SLOTS; ++i)
    {
        if (v->pitch_mod[i] != NULL)
        {
            recalc = true;
            scale = *v->pitch_mod[i];

            /*  don't multiply against p->pitch.lfo_amount because the
             *  "amount" variable has already been expressed in the
             *  values of lfo_pitch_max and lfo_pitch_min (the same logic
             *  applies when handling the envelopes below)
             */

            if (scale >= 0.0)
                pitch *= lerp(1.0, p->mod_pitch_max[i], scale);
            else
                pitch *= lerp(1.0, p->mod_pitch_min[i], -scale);
        }
    }

    /* scale to velocity */
    if (p->pitch.vel_amt > ALMOST_ZERO)
    {
        recalc = true;
        pitch = lerp (pitch, pitch * v->vel, p->pitch.vel_amt);
    }

    if (recalc)
    {
        v->stepi = pitch;
        v->stepf = (pitch - v->stepi) * (0xFFFFFFFFU);
    }

    /* advance our position indices */

    advance_pos(v->dir, &v->posi,  &v->posf, v->stepi,  v->stepf);

    if (v->playstate == PLAYSTATE_FADE_IN)
    {
        advance_fwd(&v->fade_posi, &v->fade_posf, v->stepi, v->stepf);

        if (v->fade_posi >= p->fade_samples)
        {
           v->playstate = PLAYSTATE_PLAY;
            v->fade_declick = 1.0;
        }
        else
            v->fade_declick = ((float)v->fade_posi / p->fade_samples);
    }

    if (v->loop)
    {
        /* adjust our indices according to our play mode */
        if (p->play_mode & PATCH_PLAY_PINGPONG)
        {
            if ((v->dir > 0) && (v->posi >= p->loop_stop))
            {
                playstate_init_x_fade(p, v);
                v->posi = p->loop_stop;
                v->dir = -1;
            }
            else if ((v->dir < 0) && (v->posi <= p->loop_start))
            {
                playstate_init_x_fade(p, v);
                v->posi = p->loop_start;
                v->dir = 1;
            }
        }
        else
        {
            if ((v->dir > 0) && (v->posi >= p->loop_stop))
            {
                playstate_init_x_fade(p, v);
                v->posi = p->loop_start;
            }
            else if ((v->dir < 0) && (v->posi <= p->loop_start))
            {
                playstate_init_x_fade(p, v);
                v->posi = p->loop_stop;
            }
        }
    }

    if (v->playstate == PLAYSTATE_PLAY)
    {
        /* The potential for a fade out is limited firstly by:
         *  a) we're using a non-looping playmode.
         *  b) we're looping but the note has been released
         */
        if (!(p->play_mode & PATCH_PLAY_LOOP)
         || ((p->play_mode & PATCH_PLAY_LOOP) && v->released))
        {
            if (   ((v->dir > 0) && (v->posi > v->fade_out_start_pos))
                || ((v->dir < 0) && (v->posi < v->fade_out_start_pos)))
            {
                playstate_init_fade_out(p, v);

                if (v->playstate == PLAYSTATE_OFF)
                    return -1;
            }
        }
    }
    
    if (v->playstate == PLAYSTATE_FADE_OUT)
    {
        advance_fwd(&v->fade_posi, &v->fade_posf, v->stepi, v->stepf);

        if (v->fade_posi >= p->fade_samples)
        {
            v->playstate = PLAYSTATE_OFF;
            debug("fadeout end at %f\n", v->fade_declick);
            v->fade_declick = 0.0;
            return -1;
        }

        v->fade_declick = 1.0 - ((float)v->fade_posi / p->fade_samples);
    }

    if (v->xfade)
    {
        advance_pos(v->xfade_dir,   &v->xfade_point_posi,
                                    &v->xfade_point_posf,
                                                    v->stepi, v->stepf);

        advance_fwd(&v->xfade_posi, &v->xfade_posf, v->stepi, v->stepf);

        if (v->xfade_posi >= p->xfade_samples)
        {
            v->xfade = false;
            v->xfade_declick = 1.0;
        }
        else
            v->xfade_declick = ((float)v->xfade_posi / p->xfade_samples);
    }

    /* check to see if it's time to release
        nb: relset == -1 between note-on and note-off.
     */

    if (v->relset >= 0)
    {
        switch(v->relmode)
        {
        case RELEASE_NOTEOFF:
            if (v->released)
                break;

            if (v->relset == 0)
            {
                /*debug("release mode: NOTEOFF\n");*/

                for (j = 0; j < VOICE_MAX_ENVS; j++)
                    adsr_release(v->env[j]);

                v->released = true;

                if (!(p->play_mode & PATCH_PLAY_SINGLESHOT))
                {
                    if (!v->vol_mod[EG_MOD_SLOT]) /* direct mod source */
                    {
                        playstate_init_fade_out(p, v);
                    }
                    else if (p->play_mode & PATCH_PLAY_TO_END)
                    {
                        v->playstate =  PLAYSTATE_PLAY;
                        v->loop =       false;
                        v->to_end =     true;

                        if (p->play_mode & PATCH_PLAY_PINGPONG)
                        {
                            if (v->dir == -1)
                                v->fade_out_start_pos =
                                        p->play_start + p->fade_samples;
                            else
                                v->fade_out_start_pos =
                                        p->play_stop - p->fade_samples;
                        }
                    }
                }
            }

            --v->relset;
            break;

        case RELEASE_CUTOFF:
        default:
            if (v->relset == 0)
            {
                debug("release mode: CUTOFF\n");
                playstate_init_fade_out(p, v);
            }

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
    bool done;
    register int k;


    /*  calculate global LFO output tables first: */
    for (i = 0; i < nframes; ++i)
    {
        for (j = 0; j < PATCH_MAX_LFOS; ++j)
        {
            if (p->glfo_params[j].lfo_on)
                p->glfo_table[j][i] = lfo_tick(p->glfo[j]);
        }
    }

    /*  right then, let's do the voices now... */
    for (i = 0; i < PATCH_VOICE_COUNT; i++)
    {
        if (p->voices[i]->active == false)
            continue;

        /* sanity check */
        if (p->voices[i]->posi < p->sample->frames)
            v = p->voices[i];
        else
        {
            p->voices[i]->active = false;
            continue;
        }

        done = false;

        for (j = 0; j < nframes && !done; j++)
        {
            /* ok so we've calculated the global LFO tables already,
                we just need to set the output of each global LFO to
                the correct value for the frame.
            */
            for (k = 0; k < PATCH_MAX_LFOS; ++k)
                if (p->glfo_params[k].lfo_on)
                    lfo_set_output(p->glfo[k], p->glfo_table[k][j]);

            for (k = 0; k < VOICE_MAX_ENVS; ++k)
                if (p->env_params[k].env_on)
                    adsr_tick(v->env[k]);

            for (k = 0; k < VOICE_MAX_LFOS; ++k)
                if (p->vlfo_params[k].lfo_on)
                    lfo_tick(v->lfo[k]);

            /* process samples */
            pitchscale (p, v,    &l, &r);
            pan        (p, v, j, &l, &r);
            filter     (p, v, j, &l, &r);

            /* adjust amplitude and stop rendering if we finished
             * a release */
            if (gain   (p, v, j, &l, &r) < 0)
                done = true;

            buf[j * 2] += l;
            buf[j * 2 + 1] += r;

            /* advance our position and stop rendering if we
             * run out of samples */
            if (advance (p, v, j) < 0)
                done = true;
        }

        /* check to see if it's time to stop rendering */
        if (done)
            v->active = false;

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
        if (patches[i]
         && patches[i]->active
         && patches[i]->channel == chan
         && (note >= patches[i]->lower_note
         &&  note <= patches[i]->upper_note))
        {
            patch_release_patch(patches[i], note, RELEASE_NOTEOFF);
        }
    }

    return;
}


/* deactivate a single patch with a given id */
void patch_release_with_id (int id, int note)
{
    if (id < 0 || id >= PATCH_COUNT)
        return;

    if (!patches[id]->active)
        return;

    patch_release_patch(patches[id], note, RELEASE_NOTEOFF);
    return;
}


/* render nframes of all active patches into buf */
void patch_render (float *buf, int nframes)
{
    int i;

    /* render potatos */
    for (i = 0; i < PATCH_COUNT; i++)
    {
        if (patches[i] && patches[i]->active)
        {
            if (patch_trylock (i) != 0)
            {
                continue;
            }

            if (patches[i]->sample->sp == NULL)
            {
                patch_unlock(i);
                continue;
            }

            patch_render_patch(patches[i], buf, nframes);
            patch_unlock(i);
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
        if (patches[i]
         && patches[i]->active
         && patches[i]->channel == chan
         && (note >= patches[i]->lower_note
         &&  note <= patches[i]->upper_note))
        {
            idp[j++] = i;
        }
    }

    /* do cuts */
    for (i = 0; i < j; i++)
        patch_cut_patch(patches[idp[i]]);

    /* do triggers */
    for (i = 0; i < j; i++)
        patch_trigger_patch(patches[idp[i]], note, vel, ticks);
}


/* activate a single patch with given id */
void patch_trigger_with_id (int id, int note, float vel, Tick ticks)
{
    if (id < 0 || id >= PATCH_COUNT)
        return;

    if (!patches[id]->active)
        return;

    if (note < patches[id]->lower_note || note > patches[id]->upper_note)
        return;

    patch_cut_patch(patches[id]);
    patch_trigger_patch(patches[id], note, vel, ticks);
    return;
}


void patch_control_init(void)
{
    int c, p;

    debug("initializing control change array\n");

    patch_set_control_array(&cc);

    for (c = 0; c < 16; ++c)
    {
        for (p = 0; p < CC_ARR_SIZE + 1; ++p)
            cc[c][p] = 0.0f;
    }

    debug("done\n");
}


void patch_control(int chan, int param, float value)
{
    /* FIXME: this could probably be put back into mixer and
                a function call could be saved ?
     */
    cc[chan][1 + param] = value;
}

