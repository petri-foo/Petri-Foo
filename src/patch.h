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
    PATCH_VOICE_COUNT = 8,      /* maximum active voices per patch */
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
     PATCH_PLAY_FORWARD = 1,
     PATCH_PLAY_REVERSE = 1 << 1,
     /************/

     /* duration */
     PATCH_PLAY_SINGLESHOT = 1 << 2,
     PATCH_PLAY_TRIM = 1 << 3,
     PATCH_PLAY_LOOP = 1 << 4,
     /***********/

     /* ping pong mode can be set independently of all the other
      * params, but it should only be tested for if PATCH_PLAY_LOOP is set */
     PATCH_PLAY_PINGPONG = 1 << 5
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
}
PatchParamType;


/* utility functions */
int         patch_count           ( );
int         patch_create          (const char* name);
int         patch_destroy         (int id);
void        patch_destroy_all     ( );
int         patch_dump            (int** dump);
int         patch_duplicate       (int id);
int         patch_flush           (int id);
void        patch_flush_all       ( );
void        patch_init            ( );
const char* patch_strerror        (int error);
int         patch_sample_load     (int id, const char* file);
void        patch_sample_unload   (int id);
void        patch_set_buffersize  (int nframes);
void        patch_set_samplerate  (int rate);
void        patch_shutdown        ( );
void        patch_sync            (float bpm);
int         patch_verify          (int id);


/* returns a NULL terminated list of NULL terminated C strings */
const char**    patch_adsr_names(void);
const char**    patch_lfo_names(void);
const char**    patch_param_names(void);
char**    patch_mod_source_names(void);

/* playback and rendering functions  */
void patch_control         (int chan, ControlParamType param, float value);
void patch_release         (int chan, int note);
void patch_release_with_id (int id, int note);
void patch_render          (float* buf, int nframes);
void patch_trigger         (int chan, int note, float vel, Tick ticks);
void patch_trigger_with_id (int id, int note, float vel, Tick ticks);

/* envelope setters */
int patch_set_env_on      (int patch_id, int env_id, gboolean state);
int patch_set_env_delay   (int patch_id, int env_id, float secs);
int patch_set_env_attack  (int patch_id, int env_id, float secs);
int patch_set_env_hold    (int patch_id, int env_id, float secs);
int patch_set_env_decay   (int patch_id, int env_id, float secs);
int patch_set_env_sustain (int patch_id, int env_id, float level);
int patch_set_env_release (int patch_id, int env_id, float secs);

/* envelope getters */
int patch_get_env_on      (int patch_id, int env_id, gboolean* val);
int patch_get_env_delay   (int patch_id, int env_id, float* val);
int patch_get_env_attack  (int patch_id, int env_id, float* val);
int patch_get_env_hold    (int patch_id, int env_id, float* val);
int patch_get_env_decay   (int patch_id, int env_id, float* val);
int patch_get_env_sustain (int patch_id, int env_id, float* val);
int patch_get_env_release (int patch_id, int env_id, float* val);

/* lfo setters */
int patch_set_lfo_on       (int patch_id, int lfo_id, gboolean state);
int patch_set_lfo_attack   (int patch_id, int lfo_id, float secs);
int patch_set_lfo_beats    (int patch_id, int lfo_id, float beats);
int patch_set_lfo_delay    (int patch_id, int lfo_id, float secs);
int patch_set_lfo_freq     (int patch_id, int lfo_id, float freq);
int patch_set_lfo_global   (int patch_id, int lfo_id, gboolean state);
int patch_set_lfo_positive (int patch_id, int lfo_id, gboolean state);
int patch_set_lfo_shape    (int patch_id, int lfo_id, LFOShape shape);
int patch_set_lfo_sync     (int patch_id, int lfo_id, gboolean state);

/* lfo getters */
int patch_get_lfo_on       (int patch_id, int lfo_id, gboolean* val);
int patch_get_lfo_attack   (int patch_id, int lfo_id, float* secs);
int patch_get_lfo_beats    (int patch_id, int lfo_id, float* val);
int patch_get_lfo_delay    (int patch_id, int lfo_id, float* secs);
int patch_get_lfo_freq     (int patch_id, int lfo_id, float* val);
int patch_get_lfo_positive (int patch_id, int lfo_id, gboolean* val);
int patch_get_lfo_shape    (int patch_id, int lfo_id, LFOShape* val);
int patch_get_lfo_sync     (int patch_id, int lfo_id, gboolean* val);

/* parameter setters */
int patch_set_channel      (int id, int channel);
int patch_set_cut          (int id, int cut);
int patch_set_cut_by       (int id, int cut_by);
int patch_set_cutoff       (int id, float freq);
int patch_set_legato       (int id, gboolean val);
int patch_set_loop_start   (int id, int start);
int patch_set_loop_stop    (int id, int stop);
int patch_set_lower_note   (int id, int note);
int patch_set_monophonic   (int id, gboolean val);
int patch_set_name         (int id, const char* name);
int patch_set_note         (int id, int note);
int patch_set_panning      (int id, float pan);
int patch_set_pitch        (int id, float pitch);
int patch_set_pitch_steps  (int id, int steps);
int patch_set_play_mode    (int id, PatchPlayMode mode);
int patch_set_portamento   (int id, gboolean val);
int patch_set_portamento_time(int id, float secs);
int patch_set_range        (int id, int range);
int patch_set_resonance     (int id, float reso);
int patch_set_sample_start (int id, int start);
int patch_set_sample_stop  (int id, int stop);
int patch_set_upper_note   (int id, int note);
int patch_set_amplitude       (int id, float vol);

/* parameter getters */
int           patch_get_channel       (int id);
int           patch_get_cut           (int id);
int           patch_get_cut_by        (int id);
float         patch_get_cutoff        (int id);
int           patch_get_display_index (int id);
int           patch_get_frames        (int id);
gboolean      patch_get_legato        (int id);
int           patch_get_loop_start    (int id);
int           patch_get_loop_stop     (int id);
int           patch_get_lower_note    (int id);
gboolean      patch_get_monophonic    (int id);
char*         patch_get_name          (int id);
int           patch_get_note          (int id);
float         patch_get_panning       (int id);
float         patch_get_pitch         (int id);
int           patch_get_pitch_steps   (int id);
PatchPlayMode patch_get_play_mode     (int id);
gboolean      patch_get_portamento    (int id);
float        patch_get_portamento_time(int id);
int           patch_get_range         (int id);
float         patch_get_resonance     (int id);
const float*  patch_get_sample        (int id);
char*         patch_get_sample_name   (int id);
int           patch_get_sample_start  (int id);
int           patch_get_sample_stop   (int id);
int           patch_get_upper_note    (int id);
float         patch_get_amplitude        (int id);

/* param */

int patch_param_get_value(int patch_id, PatchParamType, float* val);
int patch_param_set_value(int patch_id, PatchParamType, float  val);

/* modulation setters */
int patch_set_mod1_src(int patch_id, PatchParamType, int modsrc_id);
int patch_set_mod2_src(int patch_id, PatchParamType, int modsrc_id);
int patch_set_mod1_amt(int patch_id, PatchParamType, float amount);
int patch_set_mod2_amt(int patch_id, PatchParamType, float amount);
int patch_set_amp_env(int patch_id, int modsrc_id);
int patch_set_vel_amount(int id, PatchParamType param, float amt);

/* modulation getters */
int patch_get_mod1_src(int patch_id, PatchParamType, int* modsrc_id);
int patch_get_mod2_src(int patch_id, PatchParamType, int* modsrc_id);
int patch_get_mod1_amt(int patch_id, PatchParamType, float* amount);
int patch_get_mod2_amt(int patch_id, PatchParamType, float* amount);
int patch_get_amp_env(int patch_id, int* modsrc_id);
int patch_get_vel_amount(int id, PatchParamType param, float* val);

/* lfo modulation setters */
int patch_set_lfo_mod1_src(int patch_id, int lfo_id, int modsrc_id);
int patch_set_lfo_mod2_src(int patch_id, int lfo_id, int modsrc_id);
int patch_set_lfo_mod1_amt(int patch_id, int lfo_id, float amount);
int patch_set_lfo_mod2_amt(int patch_id, int lfo_id, float amount);

/* lfo modulation getters */
int patch_get_lfo_mod1_src(int patch_id, int lfo_id, int* modsrc_id);
int patch_get_lfo_mod2_src(int patch_id, int lfo_id, int* modsrc_id);
int patch_get_lfo_mod1_amt(int patch_id, int lfo_id, float* amount);
int patch_get_lfo_mod2_amt(int patch_id, int lfo_id, float* amount);


#endif /* __PATCH_H__ */
