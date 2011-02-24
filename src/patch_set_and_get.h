#ifndef __PATCH_SET_AND_GET_H__
#define __PATCH_SET_AND_GET_H__


#include "patch.h"

gboolean    patch_lfo_is_global(int lfo_id);


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
int patch_set_amplitude    (int id, float vol);
int patch_set_sample_xfade (int id, int samples);
int patch_set_sample_fade_in (int id, int samples);
int patch_set_sample_fade_out(int id, int samples);

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
float         patch_get_amplitude     (int id);
int           patch_get_sample_xfade  (int id);
int           patch_get_sample_fade_in (int id);
int           patch_get_sample_fade_out(int id);

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




#endif /* __PATCH_SET_AND_GET_H__ */
