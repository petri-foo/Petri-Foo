#ifndef PRIVATE_PATCH_DATA_H
#define PRIVATE_PATCH_DATA_H


/* Don't add #includes in here. */


/* release modes */
typedef enum
{
    RELEASE_NONE,       /* release no voices */
    RELEASE_NOTEOFF,    /* release is a result of a midi noteoff */
    RELEASE_CUTOFF,     /* release is a result of a cut */
    RELEASE_ALL         /* release all voices for a given patch */
} release_t;


/* type for currently playing notes (voices) */
typedef struct _PatchVoice
{
    gboolean  active;       /* whether this voice is playing or not */
    Tick      ticks;        /* at what time this voice was activated */
    int       relset;       /* how many ticks should pass before we release
                             * this voice (negative if N/A) */
    release_t relmode;      /* how release activated (noteoff or cutoff) */
    gboolean  released;     /* whether we've been released or not */
    gboolean  to_end;       /* whether we're to go to end of sample after
                               loop or not */
    float     declick_vol;  /* current volume for declick phase */
    int       dir;          /* current direction
                             * (forward == 1, reverse == -1) */
    int       note;         /* the note that activated us */
    double    pitch;        /* what pitch ratio to play at */
    double    pitch_step;   /* how much to increment pitch by each
                             * porta_tick */
    int       porta_ticks;  /* how many ticks to increment pitch for */
    int       posi;         /* integer sample index */
    guint32   posf;         /* fractional sample index */
    int       stepi;        /* integer step amount */
    guint32   stepf;        /* fractional step amount */
    float     vel;          /* velocity; volume of this voice */

    float*  vol_mod1;
    float*  vol_mod2;
    float*  vol_direct;

    float*  pan_mod1;
    float*  pan_mod2;

    float*  ffreq_mod1;
    float*  ffreq_mod2;

    float*  freso_mod1;
    float*  freso_mod2;

    float*  pitch_mod1;
    float*  pitch_mod2;

    ADSR      env[VOICE_MAX_ENVS];
    LFO       lfo[VOICE_MAX_LFOS];

    float     fll;		/* lowpass filter buffer, left */
    float     flr;		/* lowpass filter buffer, right */
    float     fbl;		/* bandpass filter buffer, left */
    float     fbr;		/* bandpass filter buffer, right */

    float   xfl;    /* xfade buffer left */
    float   xfr;    /* xfade buffer right */


    float   xfade_step;
    float   fade_in_step;
    float   fade_out_step;

    int xfade_smp;
    int fadein_smp;
    int fadeout_smp;
    
    

} PatchVoice;


typedef struct _PatchParam
{
    float   val;        /* value of this parameter */

    /* general purpose modulation sources */

    int     mod1_id;    /* ID of modulation source */
    float   mod1_amt;   /* amount of modulation we add [-1.0, 1.0]  */

    int     mod2_id;
    float   mod2_amt;

    /* direct effect modulation source */

    gboolean has_direct;     /* might not make sense for all params */
    int     direct_mod_id;

    float   vel_amt;

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
    int      range;         /* whether to listen to range of notes or not */
    int      lower_note;	/* lowest note in range */
    int      upper_note;	/* highest note in range */
    int      cut;		/* cut signal this patch emits */
    int      cut_by;		/* what cut signals stop this patch */
    int      sample_start;	/* the first frame to play */
    int      sample_stop;	/* the last frame to play */
    int      loop_start;	/* the first frame to loop at */
    int      loop_stop;		/* the last frame to loop at */

    int     sample_xfade;
    int     sample_fade_in;
    int     sample_fade_out;

    gboolean porta;		/* whether portamento is being used or not */
    float    porta_secs;	/* length of portamento slides in seconds */
    int      pitch_steps;	/* range of pitch.val in halfsteps */
    float    pitch_bend;	/* pitch bending factor */
    gboolean mono;		/* whether patch is monophonic or not */
    gboolean legato;		/* whether patch is played legato or not */

    PatchPlayMode play_mode;	/* how this patch is to be played */
    PatchParam    vol;		/* volume:                  [0.0, 1.0] */
    PatchParam    pan;		/* panning:                [-1.0, 1.0] */
    PatchParam    ffreq;	/* filter cutoff frequency: [0.0, 1.0] */
    PatchParam    freso;	/* filter resonance:        [0.0, 1.0] */
    PatchParam    pitch;	/* pitch scaling:           [0.0, 1.0] */

    double mod1_pitch_max;
    double mod1_pitch_min;
    double mod2_pitch_max;
    double mod2_pitch_min;

    LFO         glfo[PATCH_MAX_LFOS];
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
    PatchVoice voices[PATCH_VOICE_COUNT];
    int        last_note;	/* the last MIDI note value that played us */
     
    /* used exclusively by patch_lock functions to ensure that
     * patch_render ignores this patch */
    pthread_mutex_t mutex;
} Patch;



#define INLINE_ISOK_DEF                                             \
inline static int isok (int id)                                     \
{                                                                   \
    if (id < 0 || id >= PATCH_COUNT || !patches[id].active)         \
        return 0;                                                   \
    return 1;                                                       \
}


#define INLINE_PATCH_TRIGGER_GLOBAL_LFO_DEF                         \
inline static void                                                  \
patch_trigger_global_lfo(int patch_id, LFO* lfo, LFOParams* lfopar) \
{                                                                   \
    Patch* p = &patches[patch_id];                                  \
    lfo->freq_mod1 = mod_id_to_pointer(lfopar->mod1_id, p, NULL);   \
    lfo->freq_mod2 = mod_id_to_pointer(lfopar->mod2_id, p, NULL);   \
    lfo_trigger(lfo, lfopar);                                       \
}


#endif
