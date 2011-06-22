#ifndef PRIVATE_PATCH_DATA_H
#define PRIVATE_PATCH_DATA_H


#include "midi_control.h"
#include "patch.h"

#include "patch_voice.h"

#include "sample.h"


typedef struct _PatchParam
{
    float   val;        /* value of this parameter */

    /* general purpose modulation sources */

    int     mod1_id;    /* ID of modulation source */
    float   mod1_amt;   /* amount of modulation we add [-1.0, 1.0]  */

    int     mod2_id;
    float   mod2_amt;

    /* direct effect modulation source */
    int     direct_mod_id;

    /* velocity sensitivity and keyboard tracking */
    float   vel_amt;
    float   key_amt;

} PatchParam;


/* type for array of instruments (called patches) */
typedef struct _Patch
{
    int      active;        /* whether patch is in use or not */
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

    int     sample_stop;    /* very last frame in sample */

    int*    marks[WF_MARK_STOP + 1];

    int     fade_samples;
    int     xfade_samples;

    bool    porta;          /* whether portamento is being used or not */
    float   porta_secs;     /* length of portamento slides in seconds */
    int     pitch_steps;    /* range of pitch.val in halfsteps */
    float   pitch_bend;     /* pitch bending factor */
    bool    mono;           /* whether patch is monophonic or not */
    bool    legato;         /* whether patch is played legato or not */

    PatchPlayMode   play_mode;  /* how this patch is to be played */
    PatchParam      vol;        /* volume:                  [0.0, 1.0] */
    PatchParam      pan;        /* panning:                [-1.0, 1.0] */
    PatchParam      ffreq;      /* filter cutoff frequency: [0.0, 1.0] */
    PatchParam      freso;      /* filter resonance:        [0.0, 1.0] */
    PatchParam      pitch;      /* pitch scaling:           [0.0, 1.0] */

    double mod1_pitch_max;
    double mod1_pitch_min;
    double mod2_pitch_max;
    double mod2_pitch_min;

    LFO*        glfo[PATCH_MAX_LFOS];
    LFOParams   glfo_params[PATCH_MAX_LFOS];
    LFOParams   vlfo_params[VOICE_MAX_LFOS];

    /*  we need tables to store output values of global LFOs. there are
        good reasons for this, it's a necessity and, it's false to think 
        this places any limitations on the modulation of the global LFOs
        (think: how would modulating a global LFO by a source from one of
        the (many) voices work? (we pretend it's impossible for simplicity's
        sake and therefor the right answer is it cannot work, and 
        consequently there are no problems :-) ).
    */
    float*      glfo_table[PATCH_MAX_LFOS];

    /* NOTE: FIXME-ISH?
        above statement falls apart if the patch is monophonic - there
        would only ever be one voice and this makes it perfectly reasonable
        to allow modulation of the global LFOs by a source within the
        (one and only) voice.

        the over-arching logic would be something along the lines of

        if monophonic
        then
            don't use global lfo tables
        endif
    */

    ADSRParams  env_params[VOICE_MAX_ENVS];


    /* each patch is responsible for its own voices */
    PatchVoice* voices[PATCH_VOICE_COUNT];
    int         last_note;	/* the last MIDI note value that played us */
     
    /* used exclusively by patch_lock functions to ensure that
     * patch_render ignores this patch */
    pthread_mutex_t mutex;

} Patch;


Patch*          patch_new(const char* name);
void            patch_free(Patch*);

void            patch_set_control_array(float (*ccs)[16][CC_ARR_SIZE]);

void            patch_set_global_lfo_buffers(Patch*, int buffersize);

float const*    patch_mod_id_to_pointer(int mod_src_id, Patch*,
                                                        PatchVoice*);

#endif
