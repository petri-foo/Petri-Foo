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


#ifndef PRIVATE_PATCH_DATA_H
#define PRIVATE_PATCH_DATA_H


#include "midi_control.h"
#include "patch.h"
#include "patch_voice.h"
#include "sample.h"


/*  PatchParam
        a structure used for sound parameters which can be modulated
        continually as the voice plays.
 */
typedef struct _PatchParam
{
    float   val;

    /* modulation sources */
    int     mod_id[MAX_MOD_SLOTS];
    float   mod_amt[MAX_MOD_SLOTS];

    /* velocity sensitivity and keyboard tracking */
    float   vel_amt;
    float   key_amt;

} PatchParam;


/*  PatchBool
        a structure used for storing boolean settings which can be turned
        on and off by a modulation source.

        *usually* these settings will not be continually modified by the
        modulation source; their value will usually only be taken each
        time a voice is triggered.

        modulation values below the threshold turn the feature off, while
        those above turn it on.
 */
 typedef struct _PatchBool
{
    bool    on;     /* set value */
    bool    active; /* actual value after modulation */
    int     mod_id;
    float   thresh;

} PatchBool;


typedef struct _PatchFloat
{
    float   assign; /* set value */
    float   value;  /* actual value after modulation */
    int     mod_id;
    float   mod_amt;

} PatchFloat;


/* type for array of instruments (called patches) */
struct _Patch
{
    bool     active;        /* whether patch is in use or not */
    Sample*  sample;        /* sample data */
    int      display_index; /* order in which this Patch to be displayed */

    char     name[PATCH_MAX_NAME];

    int      channel;       /* midi channel to listen on */
    int      note;          /* midi note to listen on */
    int      lower_note;    /* lowest note in range */
    int      upper_note;    /* highest note in range */
    int      cut;           /* cut signal this patch emits */
    int      cut_by;        /* what cut signals stop this patch */
    int      play_start;    /* the first frame to play */
    int      play_stop;     /* the last frame to play */
    int      loop_start;    /* the first frame to loop at */
    int      loop_stop;     /* the last frame to loop at */
    int      lower_vel;     /* lower velocity trigger */
    int      upper_vel;     /* upper velocity trigger */

    int     sample_stop;    /* very last frame in sample */

    int*    marks[WF_MARK_STOP + 1];

    int     fade_samples;
    int     xfade_samples;

    PatchBool   porta;
    PatchFloat  porta_secs;

    int         pitch_steps;    /* range of pitch.val in halfsteps */
    float       pitch_bend;     /* pitch bending factor */
    bool        mono;           /* whether patch is monophonic or not */
    PatchBool   legato;         /* whether patch is played legato or not */

    PatchPlayMode   play_mode;  /* how this patch is to be played */
    PatchParam      vol;        /* volume:                  [0.0, 1.0] */
    PatchParam      pan;        /* panning:                [-1.0, 1.0] */
    PatchParam      ffreq;      /* filter cutoff frequency: [0.0, 1.0] */
    PatchParam      freso;      /* filter resonance:        [0.0, 1.0] */
    PatchParam      pitch;      /* pitch scaling:           [0.0, 1.0] */

    double mod_pitch_min[MAX_MOD_SLOTS];
    double mod_pitch_max[MAX_MOD_SLOTS];

    LFO*        glfo[PATCH_MAX_LFOS];
    LFOParams   glfo_params[PATCH_MAX_LFOS];
    LFOParams   vlfo_params[VOICE_MAX_LFOS];

    /*  use tables to store output values of global LFOs
    */
    float*      glfo_table[PATCH_MAX_LFOS];

    ADSRParams  env_params[VOICE_MAX_ENVS];

    /* each patch is responsible for its own voices */
    PatchVoice* voices[PATCH_VOICE_COUNT];
    int         last_note;	/* the last MIDI note value that played us */
     
    /* used exclusively by patch_lock functions to ensure that
     * patch_render ignores this patch */
    pthread_mutex_t mutex;

};


Patch*          patch_new(void);
void            patch_free(Patch*);
void            patch_copy(Patch* dest, Patch* src);

void            patch_set_control_array(float (*ccs)[16][CC_ARR_SIZE]);

void            patch_set_global_lfo_buffers(Patch*, int buffersize);

float const*    patch_mod_id_to_pointer(int mod_src_id, Patch*,
                                                        PatchVoice*);

#endif
