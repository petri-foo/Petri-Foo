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


#include "patch_data.h"


#include "patch_defs.h"
#include "midi_control.h"
#include "mixer.h"

#include <stdlib.h>
#include <string.h>


static int      start_frame = 0;
static float    one = 1.0;
static float    (*cc_arr)[16][CC_ARR_SIZE];


Patch* patch_new(void)
{
    int i;
    Patch* p;

    p = malloc(sizeof(*p));

    if (!p)
        return 0;

    p->name[0] = '\0';

    p->active =         false;
    p->sample =         sample_new();
    p->display_index =  -1;

    p->name[0] = '\0';

    p->channel =        0;
    p->root_note =      60;
    p->lower_note =     60;
    p->upper_note =     60;
    p->lower_vel =      0;
    p->upper_vel =      127;

    p->cut =            0;
    p->cut_by =         0;

    p->play_start =     0;
    p->play_stop =      0;
    p->loop_start =     0;
    p->loop_stop =      0;
    p->sample_stop =    0;

    p->marks[WF_MARK_START] =      &start_frame;
    p->marks[WF_MARK_STOP] =       &p->sample_stop;
    p->marks[WF_MARK_PLAY_START] = &p->play_start;
    p->marks[WF_MARK_PLAY_STOP] =  &p->play_stop;
    p->marks[WF_MARK_LOOP_START] = &p->loop_start;
    p->marks[WF_MARK_LOOP_STOP] =  &p->loop_stop;

    p->fade_samples =   0;
    p->xfade_samples =  0;

    p->porta.active =   true;   /* but only if PORTAMENTO   */
    p->porta.thresh =   0.5;    /* controller says so...    */
    p->porta.mod_id =   MOD_SRC_MIDI_CC + CC_PORTAMENTO;

    p->porta_secs.val =     0.05;
    p->porta_secs.mod_amt = 1.0;
    p->porta_secs.mod_id =  MOD_SRC_MIDI_CC + CC_PORTAMENTO_TIME;

    p->pitch_steps =    2;
    p->pitch_bend =     0;

    p->mono = false;

    p->legato.active =  true;   /* but only if mono is on, *AND*    */
    p->legato.thresh =  0.5;    /* LEGATO controller says so...     */
    p->legato.mod_id =  MOD_SRC_MIDI_CC + CC_LEGATO;

    p->play_mode =      PATCH_PLAY_SINGLESHOT;

    for (i = 0; i < MAX_MOD_SLOTS; ++i)
    {
        p->amp.mod_id[i] = MOD_SRC_NONE;
        p->amp.mod_amt[i] = 0.0;

        p->pan.mod_id[i] = MOD_SRC_NONE;
        p->pan.mod_amt[i] = 0.0;

        p->ffreq.mod_id[i] = MOD_SRC_NONE;
        p->ffreq.mod_amt[i] = 0.0;

        p->freso.mod_id[i] = MOD_SRC_NONE;
        p->freso.mod_amt[i] = 0.0;

        p->pitch.mod_id[i] = MOD_SRC_NONE;
        p->pitch.mod_amt[i] = 0.0;

        p->mod_pitch_min[i] = 1.0;
        p->mod_pitch_max[i] = 1.0;
    }

    p->amp.val =        DEFAULT_AMPLITUDE;
    p->amp.vel_amt =    1.0;
    p->amp.key_amt =    0.0;

    p->pan.val =        0.0;
    p->pan.vel_amt =    0;
    p->pan.key_amt =    0.0;

    p->ffreq.val =      1.0;
    p->ffreq.vel_amt =  0;
    p->ffreq.key_amt =  0;

    p->freso.val =      0.0;
    p->freso.vel_amt =  0;
    p->freso.key_amt =  0;

    p->pitch.val =      0.0;
    p->pitch.vel_amt =  0;
    p->pitch.key_amt =  1.0;

    for (i = 0; i < PATCH_MAX_LFOS; ++i)
    {
        lfo_params_init(&p->glfo_params[i], 1.0, LFO_SHAPE_SINE);
        p->glfo[i] = lfo_new();
        /* init tables to NULL */
        p->glfo_table[i] = 0;
    }

    patch_set_global_lfo_buffers(p, patch_buffersize);

    /* only the params for the voice lfo can be set at this stage */
    for (i = 0; i < VOICE_MAX_LFOS; ++i)
        lfo_params_init(&p->vlfo_params[i], 1.0, LFO_SHAPE_SINE);

    for (i = 0; i < VOICE_MAX_ENVS; i++)
        adsr_params_init(&p->env_params[i], 0.005, 0.025);

    for (i = 0; i < PATCH_VOICE_COUNT; ++i)
    {
        int j;

        p->voices[i] = patch_voice_new();

        for (j = 0; j < VOICE_MAX_ENVS; ++j)
            adsr_init(p->voices[i]->env[j]);
    }

    p->last_note = -1;

    pthread_mutex_init(&p->mutex, NULL);

    debug("********************************\n");
    debug("created patch:%s [%p]\n", p->name, p);
    debug("********************************\n");

    return p;
}


void patch_free(Patch* p)
{
    int i;

    if (!p)
        return;

    debug("********************************\n");
    debug("freeing patch:'%s'\n", p->name);
    debug("********************************\n");

    sample_free_data(p->sample);

    for (i = 0; i < PATCH_VOICE_COUNT; ++i)
        patch_voice_free(p->voices[i]);

    for (i = 0; i < PATCH_MAX_LFOS; ++i)
        free(p->glfo_table[i]);

    pthread_mutex_destroy(&p->mutex);

    free(p);
}


void patch_set_control_array(float (*ccs)[16][CC_ARR_SIZE])
{
    cc_arr = ccs;
}


void patch_set_global_lfo_buffers(Patch* p, int buffersize)
{
    int i;

    if (buffersize <= 0)
        return;

    for (i = 0; i < PATCH_MAX_LFOS; ++i)
    {
        free(p->glfo_table[i]);
        p->glfo_table[i] =
            malloc(sizeof(*p->glfo_table[i]) * patch_buffersize);
    }
}


float const* patch_mod_id_to_pointer(int id, Patch* p, PatchVoice* v)
{
    switch(id)
    {
    case MOD_SRC_NONE:      return NULL;
    case MOD_SRC_ONE:       return &one;
    case MOD_SRC_VELOCITY:  return (v) ? &v->vel :          NULL;
    case MOD_SRC_KEY:       return (v) ? &v->key_track :    NULL;
    case MOD_SRC_PITCH_WHEEL:
        return &((*cc_arr)[p->channel][0]);
    }

    if (id & MOD_SRC_EG && v)
    {
        id &= ~MOD_SRC_EG;

        if (id < VOICE_MAX_ENVS)
            return adsr_output(v->env[id]);
    }

    if (id & MOD_SRC_VLFO && v)
    {
        id &= ~MOD_SRC_VLFO;

        if (id < VOICE_MAX_LFOS)
            return lfo_output(v->lfo[id]);
    }

    if (id & MOD_SRC_GLFO)
    {
        id &= ~MOD_SRC_GLFO;

        if (id < PATCH_MAX_LFOS)
            return lfo_output(p->glfo[id]);
    }

    if (id & MOD_SRC_MIDI_CC)
    {
        id &= ~MOD_SRC_MIDI_CC;
        return &((*cc_arr)[p->channel][id + 1]);
    }

    debug("unknown modulation source:%d\n", id);

    return 0;
}


void patch_copy(Patch* dest, Patch* src)
{
    int i;

    sample_deep_copy(dest->sample, src->sample);

    strcpy(dest->name, src->name);

    dest->channel =         src->channel;
    dest->root_note =       src->root_note;
    dest->lower_note =      src->lower_note;
    dest->upper_note =      src->upper_note;
    dest->cut =             src->cut;
    dest->cut_by =          src->cut_by;
    dest->cut =             src->cut;
    dest->play_start =      src->play_start;
    dest->play_stop =       src->play_stop;
    dest->loop_start =      src->loop_start;
    dest->loop_stop =       src->loop_stop;
    dest->sample_stop =     src->sample_stop;
    dest->fade_samples =    src->fade_samples;
    dest->xfade_samples =   src->xfade_samples;
    dest->porta =           src->porta;
    dest->porta_secs =      src->porta_secs;
    dest->pitch_steps =     src->pitch_steps;
    dest->pitch_bend =      src->pitch_bend;
    dest->mono =            src->mono;
    dest->legato =          src->legato;
    dest->play_mode =       src->play_mode;

    dest->amp =             src->amp;
    dest->pan =             src->pan;
    dest->ffreq =           src->ffreq;
    dest->freso =           src->freso;
    dest->pitch =           src->pitch;

    for (i = 0; i < MAX_MOD_SLOTS; ++i)
    {
        dest->mod_pitch_min[i] = src->mod_pitch_min[i];
        dest->mod_pitch_max[i] = src->mod_pitch_max[i];
    }

    for (i = 0; i < PATCH_MAX_LFOS; ++i)
    {
        dest->glfo_params[i] = src->glfo_params[i];
        lfo_update_params(dest->glfo[i], &dest->glfo_params[i]);
    }

    for (i = 0; i < VOICE_MAX_LFOS; ++i)
        dest->vlfo_params[i] = src->vlfo_params[i];
        /* VLFO updated when voice triggered */

    for (i = 0; i < VOICE_MAX_ENVS; ++i)
        dest->env_params[i] = src->env_params[i];

    debug("copied patch src %p to patch dest %p\n", src, dest);
}
