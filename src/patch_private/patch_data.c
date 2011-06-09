#include "patch_data.h"


#include "patch_defs.h"
#include "mixer.h"

#include <stdlib.h>
#include <string.h>


static int      start_frame = 0;
static float    one = 1.0;


Patch* patch_new(const char* name)
{
    Patch* p;
    ADSRParams  defadsr;
    LFOParams   deflfo;
    bool default_patch = (strcmp("Default", name) == 0);
    int i;

    p = malloc(sizeof(*p));

    if (!p)
        return 0;

    /* name */
    strncpy(p->name, name, PATCH_MAX_NAME);
    p->name[PATCH_MAX_NAME - 1] = '\0';

    /* default values */
    p->active =         true;
    p->sample =         sample_new();
    p->display_index =  -1;
    p->channel =        0;
    p->note =           60;
    p->lower_note =     60;
    p->upper_note =     60;

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
    p->porta =          false;
    p->porta_secs =     0.05;
    p->pitch_steps =    2;
    p->pitch_bend =     0;
    p->mono =           false;
    p->legato =         false;

    p->play_mode = (default_patch   ? PATCH_PLAY_LOOP
                                    : PATCH_PLAY_SINGLESHOT)
                                    | PATCH_PLAY_FORWARD;
    p->vol.val =        DEFAULT_AMPLITUDE;
    p->vol.mod1_id =    MOD_SRC_NONE;
    p->vol.mod2_id =    MOD_SRC_NONE;
    p->vol.mod1_amt =   0;
    p->vol.mod2_amt =   0;
    p->vol.direct_mod_id = (default_patch
                                ? MOD_SRC_EG
                                : MOD_SRC_NONE);
    p->vol.vel_amt =    1.0;
    p->vol.key_amt =    0.0;

    p->pan.val =        0.0;
    p->pan.mod1_id =    MOD_SRC_NONE;
    p->pan.mod2_id =    MOD_SRC_NONE;
    p->pan.mod1_amt =   0;
    p->pan.mod2_amt =   0;
    p->pan.vel_amt =    0;
    p->pan.key_amt =    0.0;

    p->ffreq.val =      1.0;
    p->ffreq.mod1_id =  MOD_SRC_NONE;
    p->ffreq.mod2_id =  MOD_SRC_NONE;
    p->ffreq.mod1_amt = 0;
    p->ffreq.mod2_amt = 0;
    p->ffreq.vel_amt =  0;
    p->ffreq.key_amt =  0;

    p->freso.val =      0.0;
    p->freso.mod1_id =  MOD_SRC_NONE;
    p->freso.mod2_id =  MOD_SRC_NONE;
    p->freso.mod1_amt = 0;
    p->freso.mod2_amt = 0;
    p->freso.vel_amt =  0;
    p->freso.key_amt =  0;

    p->pitch.val =      0.0;
    p->pitch.mod1_id =  MOD_SRC_NONE;
    p->pitch.mod2_id =  MOD_SRC_NONE;
    p->pitch.mod1_amt = 0;
    p->pitch.mod2_amt = 0;
    p->pitch.vel_amt =  0;
    p->pitch.key_amt =  1.0;

    p->mod1_pitch_max = 1.0;
    p->mod1_pitch_min = 1.0;
    p->mod2_pitch_max = 1.0;
    p->mod2_pitch_min = 1.0;

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

    defadsr.env_on =    false;
    defadsr.delay =     0.0;
    defadsr.attack =    0.005;
    defadsr.hold =      0.0;
    defadsr.decay =     0.0;
    defadsr.sustain =   1.0;
    defadsr.release =   0.025;

    for (i = 0; i < VOICE_MAX_ENVS; i++)
        p->env_params[i] = defadsr;

    for (i = 0; i < PATCH_VOICE_COUNT; ++i)
    {
        int j;

        p->voices[i] = patch_voice_new();

        for (j = 0; j < VOICE_MAX_ENVS; ++j)
            adsr_init(p->voices[i]->env[j]);
    }

    p->last_note = -1;

    pthread_mutex_init(&p->mutex, NULL);

    debug("crreated patch:%s [%p]\n", p->name, p);

    return p;
}


void patch_free(Patch* p)
{
    int i;

    if (!p)
        return;

    debug("freeing patch:'%s'\n", p->name);

    sample_free_data(p->sample);

    for (i = 0; i < PATCH_VOICE_COUNT; ++i)
        patch_voice_free(p->voices[i]);

    for (i = 0; i < PATCH_MAX_LFOS; ++i)
        free(p->glfo_table[i]);

    pthread_mutex_destroy(&p->mutex);

    free(p);
}


float const* patch_mod_id_to_pointer(int id, Patch* p, PatchVoice* v)
{
    switch(id)
    {
    case MOD_SRC_NONE:      return 0;
    case MOD_SRC_ONE:       return &one;
    case MOD_SRC_VELOCITY:  return &v->vel;
    case MOD_SRC_KEY:       return &v->key_track;
    case MOD_SRC_PITCH_WHEEL:
        return mixer_get_control_output(p->channel, MIXER_CC_PITCH_BEND);
    }

    if (id & MOD_SRC_EG)
    {
        id &= ~MOD_SRC_EG;

        if (id < VOICE_MAX_ENVS)
            return adsr_output(v->env[id]);
    }

    if (id & MOD_SRC_VLFO)
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

        return mixer_get_control_output(p->channel, id);
    }

    debug("unknown modulation source:%d\n", id);

    return 0;
}


void patch_set_global_lfo_buffers(Patch* p, int buffersize)
{
    int i;

    debug("setting global LFO buffers (patch:%p bufsize:%d)\n",
                                                p, buffersize);

    if (buffersize <= 0)
        return;

    for (i = 0; i < PATCH_MAX_LFOS; ++i)
    {
        free(p->glfo_table[i]);
        p->glfo_table[i] =
            malloc(sizeof(*p->glfo_table[i]) * patch_buffersize);
    }
}
