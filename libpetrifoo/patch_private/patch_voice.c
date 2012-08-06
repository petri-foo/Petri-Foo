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

    for (i = 0; i < MAX_MOD_SLOTS; ++i)
    {
        pv->amp_mod[i] = NULL;
        pv->pan_mod[i] = NULL;
        pv->ffreq_mod[i] = NULL;
        pv->freso_mod[i] = NULL;
        pv->pitch_mod[i] = NULL;
    }

    for (i = 0; i < VOICE_MAX_ENVS; i++)
    {
        pv->env[i] = adsr_new();
        adsr_init(pv->env[i]);
    }

    for (i = 0; i < VOICE_MAX_LFOS; i++)
    {
        pv->lfo[i] = lfo_new();
        lfo_init(pv->lfo[i]);
    }

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

    free(pv);
}
