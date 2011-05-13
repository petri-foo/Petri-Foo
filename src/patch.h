#ifndef __PATCH_H__
#define __PATCH_H__


#include <stdint.h>


#include "ticks.h"
#include "lfo.h"
#include "control.h"


typedef struct _Patch Patch;


enum
{
    /* magic numbers */
    VOICE_MAX_LFOS =        5,
    VOICE_MAX_ENVS =        5,
    PATCH_COUNT =           64, /* maximum patches */
    PATCH_MAX_NAME =        32, /* maximum length of patch name */
    PATCH_VOICE_COUNT =     16, /* maximum active voices per patch */
    PATCH_MAX_PITCH_STEPS = 48, /* maximum val allowable for pitch_steps */
    PATCH_MAX_LFOS =        5,
    TOTAL_LFOS =            VOICE_MAX_LFOS + PATCH_MAX_LFOS
};


/*  typedef enum _MOD_SRC_ID_BITMASK mod_src_id_bitmask

    these are IDs for modulation sources passed to functions such as
    patch_set_mod1_src (see patch_set_and_get.h).

    additionally, these are how EGs, and LFOs are identified with
    functions such as patch_set_eg*.

    the third envelope therefor has an ID of (MOD_SRC_EG + 3).
 */

typedef enum _MOD_SRC_ID_BITMASK
{
    MOD_SRC_NONE,

    MOD_SRC_MISC =          0x10,

    /* specific enumerations for the miscellaneous mod sources */

    MOD_SRC_ONE =           0x10,
    MOD_SRC_KEY,
    MOD_SRC_VELOCITY,
/*  MOD_SRC_NOISE, */

    MOD_SRC__LAST_MISC__,   /* used to gain a count of misc mod srcs */

    /* no enumerations are created for specific EGs or specific LFOs */

    MOD_SRC_EG =            0x20,

    MOD_SRC_VLFO =          0x40, /* glfos not immediately following
                                   * vlfos will cause breakage! -ru sure? */
    MOD_SRC_GLFO =          0x80,

    /* counts of EGs and LFOs are provided top of this file */

    /* helpful bitmasks: */
    MOD_SRC_GLOBALS =       MOD_SRC_GLFO,

    MOD_SRC_LFOS =          MOD_SRC_VLFO | MOD_SRC_GLFO,

    MOD_SRC_ALL =           MOD_SRC_MISC
                          | MOD_SRC_EG
                          | MOD_SRC_VLFO
                          | MOD_SRC_GLFO
} mod_src_id_bitmask;


/* error codes */
enum
{
/*   PATCH_PARAM_INVALID =          -1,  */
     PATCH_ID_INVALID =             -2,
     PATCH_ALLOC_FAIL =             -3,
     PATCH_NOTE_INVALID =           -4,
     PATCH_PAN_INVALID =            -5,
     PATCH_CHANNEL_INVALID =        -6,
     PATCH_VOL_INVALID =            -7,
     PATCH_PLAY_MODE_INVALID =      -9,
     PATCH_LIMIT =                  -10,
     PATCH_SAMPLE_INDEX_INVALID =   -11,
     PATCH_ENV_ID_INVALID =         -12,
     PATCH_LFO_ID_INVALID =         -13,
     PATCH_MOD_SRC_INVALID =        -14,
     PATCH_MOD_AMOUNT_INVALID =     -15
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
typedef uint8_t PatchPlayMode;


/* code names for modulatable parameters */
typedef enum
{
    PATCH_PARAM_INVALID =       -1,
    PATCH_PARAM_AMPLITUDE =    0,
    PATCH_PARAM_PANNING,
    PATCH_PARAM_PITCH,
    PATCH_PARAM_CUTOFF,
    PATCH_PARAM_RESONANCE,

} PatchParamType;


/* playback and rendering functions  */
void patch_control         (int chan, ControlParamType param, float value);
void patch_release         (int chan, int note);
void patch_release_with_id (int id, int note);
void patch_render          (float* buf, int nframes);
void patch_trigger         (int chan, int note, float vel, Tick ticks);
void patch_trigger_with_id (int id, int note, float vel, Tick ticks);


#endif /* __PATCH_H__ */
