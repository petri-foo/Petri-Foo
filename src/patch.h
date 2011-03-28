#ifndef __PATCH_H__
#define __PATCH_H__

#include <glib.h>
#include "ticks.h"
#include "lfo.h"
#include "control.h"

/* magic numbers */
enum
{
    PATCH_COUNT = 64,           /* maximum patches */
    PATCH_MAX_NAME = 32,        /* maximum length of patch name */
    PATCH_VOICE_COUNT = 16,     /* maximum active voices per patch */
    PATCH_MAX_PITCH_STEPS = 48, /* maximum val allowable for pitch_steps */

    PATCH_MAX_LFOS = 5, /* LFO's global in scope to patch */
    VOICE_MAX_LFOS = 5, /* LFO's local only to a voice */
    TOTAL_LFOS = PATCH_MAX_LFOS + VOICE_MAX_LFOS,

    /*  LFO id's are in range [0, TOTAL_LFOS - 1], where
     *  range [0, PATCH_MAX_LFOS - 1] are the global patch LFOs
     *  and [PATCH_MAX_LFOS, TOTAL_LFOS - 1] are voice LFOs.
     */

    VOICE_MAX_ENVS = 5
};


enum
{
    MOD_SRC_NONE = 0,
    MOD_SRC_ONE,
    MOD_SRC_PITCH,
    MOD_SRC_NOISE,

    MOD_SRC_FIRST_EG,
    MOD_SRC_LAST_EG = MOD_SRC_FIRST_EG + VOICE_MAX_ENVS,

    MOD_SRC_FIRST_GLFO,
    MOD_SRC_LAST_GLFO = MOD_SRC_FIRST_GLFO + PATCH_MAX_LFOS,
    MOD_SRC_FIRST_VLFO,
    MOD_SRC_LAST_VLFO = MOD_SRC_FIRST_VLFO + VOICE_MAX_LFOS,

    MOD_SRC_LAST
};


/* error codes */
enum
{
/*   PATCH_PARAM_INVALID = -1,  */
     PATCH_ID_INVALID = -2,
     PATCH_ALLOC_FAIL = -3,
     PATCH_NOTE_INVALID = -4,
     PATCH_PAN_INVALID = -5,
     PATCH_CHANNEL_INVALID = -6,
     PATCH_VOL_INVALID = -7,
     PATCH_PLAY_MODE_INVALID = -9,
     PATCH_LIMIT = -10,
     PATCH_SAMPLE_INDEX_INVALID = -11,
     PATCH_ENV_ID_INVALID = -12,
     PATCH_LFO_ID_INVALID = -13,
     PATCH_MOD_SRC_INVALID = -14,
     PATCH_MOD_AMOUNT_INVALID = -15
};

/* These are the bitfield constants for the different ways a patch can
   be played.  I've used comments to indicate mutual exclusion among
   groups. */
enum
{
     /* direction */
     PATCH_PLAY_FORWARD =       1 << 0,
     PATCH_PLAY_REVERSE =       1 << 1,
     /************/

     /* duration */
     PATCH_PLAY_SINGLESHOT =    1 << 2,
     PATCH_PLAY_TRIM =          1 << 3,
     PATCH_PLAY_LOOP =          1 << 4,
     /***********/

     /* ping pong mode can be set independently of all the other
      * params, but it should only be tested for if PATCH_PLAY_LOOP is set */
     PATCH_PLAY_PINGPONG =      1 << 5,

    /*  patch play to end should only be tested for if PATCH_PLAY_LOOP is
        set. if active, after note_off, playback continues past loop end
        toward sample end
     */
     PATCH_PLAY_TO_END =        1 << 6,
};



enum
{
    WF_MARK_START,
    WF_MARK_PLAY_START,
    WF_MARK_LOOP_START,
    WF_MARK_LOOP_STOP,
    WF_MARK_PLAY_STOP,
    WF_MARK_STOP
};



/* type for playmode bitfield */
typedef guint8 PatchPlayMode;

/* code names for modulatable parameters */
typedef enum
{
    PATCH_PARAM_INVALID =       -1,
    PATCH_PARAM_AMPLITUDE =    0,
    PATCH_PARAM_PANNING,
    PATCH_PARAM_CUTOFF,
    PATCH_PARAM_RESONANCE,
    PATCH_PARAM_PITCH,
    PATCH_PARAM_LFO_FREQ

} PatchParamType;


/* utility functions */
int         patch_count           (void);
int         patch_create          (const char* name);
int         patch_destroy         (int id);
void        patch_destroy_all     (void);
int         patch_dump            (int** dump);
int         patch_duplicate       (int id);
int         patch_flush           (int id);
void        patch_flush_all       (void);
void        patch_init            (void);
const char* patch_strerror        (int error);
int         patch_sample_load     (int id, const char* file);
void        patch_sample_unload   (int id);
void        patch_set_buffersize  (int nframes);
void        patch_set_samplerate  (int rate);
void        patch_shutdown        (void);
void        patch_sync            (float bpm);
int         patch_verify          (int id);



/* playback and rendering functions  */
void patch_control         (int chan, ControlParamType param, float value);
void patch_release         (int chan, int note);
void patch_release_with_id (int id, int note);
void patch_render          (float* buf, int nframes);
void patch_trigger         (int chan, int note, float vel, Tick ticks);
void patch_trigger_with_id (int id, int note, float vel, Tick ticks);


#endif /* __PATCH_H__ */
