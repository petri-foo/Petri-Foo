#include "patch_voice.h"

#include <stdlib.h>

PatchVoice* patch_voice_new(void)
{
    int i;
    PatchVoice* pv = malloc(sizeof(*pv));

    if (!pv)
        return 0;

    pv->active =        false;
    pv->ticks =         0;
    pv->relset =        0;
    pv->relmode =       RELEASE_NONE;
    pv->released =      false;
    pv->to_end =        false;
    pv->dir =           0;
    pv->note =          0;
    pv->pitch =         0;
    pv->pitch_step =    0;
    pv->porta_ticks =   0;
    pv->posi =          0;
    pv->posf =          0;
    pv->stepi =         0;
    pv->stepf =         0;
    pv->vel =           0;
    pv->key_track =     0;
    pv->vol_mod1 =      0;
    pv->vol_mod2 =      0;
    pv->vol_direct =    0;
    pv->pan_mod1 =      0;
    pv->pan_mod2 =      0;
    pv->ffreq_mod1 =    0;
    pv->ffreq_mod2 =    0;
    pv->freso_mod1 =    0;
    pv->freso_mod2 =    0;
    pv->pitch_mod1 =    0;
    pv->pitch_mod2 =    0;

    for (i = 0; i < VOICE_MAX_ENVS; i++)
        pv->env[i] = adsr_new();

    for (i = 0; i < VOICE_MAX_LFOS; i++)
        pv->lfo[i] = lfo_new();

    pv->fll =           0;
    pv->fbl =           0;
    pv->flr =           0;
    pv->fbr =           0;
    pv->playstate =     PLAYSTATE_OFF;
    pv->xfade =         false;
    pv->loop =          false;
    pv->fade_posi =     -1;
    pv->fade_posf =     0;

    pv->fade_out_start_pos =    0;
    pv->fade_declick =          0;
    pv->xfade_point_posi =      0;
    pv->xfade_point_posf =      0;

    pv->xfade_posi =    -1;
    pv->xfade_posf =    0;
    pv->xfade_dir =     0;
    pv->xfade_declick = 0;

    return pv;
}


void patch_voice_free(PatchVoice* pv)
{
    if (!pv)
        return;

    int i;

    for (i = 0; i < VOICE_MAX_ENVS; ++i)
        adsr_free(pv->env[i]); 

    for (i = 0; i < VOICE_MAX_LFOS; ++i)
        lfo_free(pv->lfo[i]);
}
