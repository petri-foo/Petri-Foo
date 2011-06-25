#ifndef __PATCH_SET_AND_GET_H__
#define __PATCH_SET_AND_GET_H__


#include "patch.h"


/*  values to use as env_id and lfo_id are found in patch.h
    under _MOD_SRC_ID_BITMASK.

    to set the third envelope: env_id = MOD_SRC_EG + 3
 */

/* envelope setters */
int patch_set_env_on      (int patch_id, int env_id, bool state);
int patch_set_env_delay   (int patch_id, int env_id, float secs);
int patch_set_env_attack  (int patch_id, int env_id, float secs);
int patch_set_env_hold    (int patch_id, int env_id, float secs);
int patch_set_env_decay   (int patch_id, int env_id, float secs);
int patch_set_env_sustain (int patch_id, int env_id, float level);
int patch_set_env_release (int patch_id, int env_id, float secs);
int patch_set_env_key_amt (int patch_id, int env_id, float val);
int patch_set_env_vel_amt (int patch_id, int env_id, float val);

/* envelope getters */
int patch_get_env_on      (int patch_id, int env_id, bool* val);
int patch_get_env_delay   (int patch_id, int env_id, float* val);
int patch_get_env_attack  (int patch_id, int env_id, float* val);
int patch_get_env_hold    (int patch_id, int env_id, float* val);
int patch_get_env_decay   (int patch_id, int env_id, float* val);
int patch_get_env_sustain (int patch_id, int env_id, float* val);
int patch_get_env_release (int patch_id, int env_id, float* val);
int patch_get_env_key_amt (int patch_id, int env_id, float* val);
int patch_get_env_vel_amt (int patch_id, int env_id, float* val);

/* lfo setters */
int patch_set_lfo_on       (int patch_id, int lfo_id, bool state);
int patch_set_lfo_attack   (int patch_id, int lfo_id, float secs);
int patch_set_lfo_beats    (int patch_id, int lfo_id, float beats);
int patch_set_lfo_delay    (int patch_id, int lfo_id, float secs);
int patch_set_lfo_freq     (int patch_id, int lfo_id, float freq);
int patch_set_lfo_positive (int patch_id, int lfo_id, bool state);
int patch_set_lfo_shape    (int patch_id, int lfo_id, LFOShape shape);
int patch_set_lfo_sync     (int patch_id, int lfo_id, bool state);

/* lfo getters */
int patch_get_lfo_on       (int patch_id, int lfo_id, bool* val);
int patch_get_lfo_attack   (int patch_id, int lfo_id, float* secs);
int patch_get_lfo_beats    (int patch_id, int lfo_id, float* val);
int patch_get_lfo_delay    (int patch_id, int lfo_id, float* secs);
int patch_get_lfo_freq     (int patch_id, int lfo_id, float* val);
int patch_get_lfo_positive (int patch_id, int lfo_id, bool* val);
int patch_get_lfo_shape    (int patch_id, int lfo_id, LFOShape* val);
int patch_get_lfo_sync     (int patch_id, int lfo_id, bool* val);

/* parameter setters */
int patch_set_channel      (int id, int channel);
int patch_set_cut          (int id, int cut);
int patch_set_cut_by       (int id, int cut_by);
int patch_set_cutoff       (int id, float freq);
int patch_set_legato       (int id, bool val);
int patch_set_lower_note   (int id, int note);

/* both of these return mark_id on success */
int patch_set_mark_frame   (int patch_id, int mark_id, int frame);

/* returns mark_id on sucessful setting, if another mark required
    moving to accomodate the set mark, the id of the moved mark will
    be set via also_changed which is otherwise -1.
 */
int patch_set_mark_frame_expand(int patch_id, int mark_id, int frame,
                                                    int* also_changed);

int patch_set_monophonic   (int id, bool val);
int patch_set_name         (int id, const char* name);
int patch_set_note         (int id, int note);
int patch_set_panning      (int id, float pan);
int patch_set_pitch        (int id, float pitch);
int patch_set_pitch_steps  (int id, int steps);
int patch_set_play_mode    (int id, PatchPlayMode mode);
int patch_set_portamento   (int id, bool val);
int patch_set_portamento_time(int id, float secs);
int patch_set_range        (int id, bool range);
int patch_set_resonance     (int id, float reso);

int patch_set_upper_note   (int id, int note);
int patch_set_amplitude    (int id, float vol);

int patch_set_fade_samples (int id, int samples);
int patch_set_xfade_samples(int id, int samples);

/* parameter getters */
int     patch_get_channel       (int id);
int     patch_get_cut           (int id);
int     patch_get_cut_by        (int id);
float   patch_get_cutoff        (int id);
int     patch_get_display_index (int id);
int     patch_get_frames        (int id);
bool    patch_get_legato        (int id);
int     patch_get_lower_note    (int id);

int     patch_get_mark_frame      (int patch_id, int mark_id);
int     patch_get_mark_frame_range(int patch_id, int mark_id,
                                                 int* frame_min,
                                                 int* frame_max);

bool            patch_get_monophonic    (int id);
char*           patch_get_name          (int id);
int             patch_get_note          (int id);
float           patch_get_panning       (int id);
float           patch_get_pitch         (int id);
int             patch_get_pitch_steps   (int id);
PatchPlayMode   patch_get_play_mode     (int id);
bool            patch_get_portamento    (int id);
float           patch_get_portamento_time(int id);

/* still don't get the point of this:
bool      patch_get_range         (int id);
  seems to be TRUE when lower != upper.
 */

float           patch_get_resonance     (int id);
const float*    patch_get_sample        (int id);
const char*     patch_get_sample_name   (int id);
int             patch_get_upper_note    (int id);
float           patch_get_amplitude     (int id);
int             patch_get_fade_samples  (int id);
int             patch_get_xfade_samples (int id);
int             patch_get_max_fade_samples(int id);
int             patch_get_max_xfade_samples(int id);


/* returns 0 if non-raw sample loaded */
int patch_get_raw_samplerate(int id);
int patch_get_raw_channels(int id);
int patch_get_raw_sndfile_format(int id);


/* param */
int patch_param_get_value(int patch_id, PatchParamType, float* val);
int patch_param_set_value(int patch_id, PatchParamType, float  val);

/* modulation setters */
int patch_set_mod_src(int patch_id, PatchParamType, int slot, int src_id);
int patch_set_mod_amt(int patch_id, PatchParamType, int slot, float amt);
int patch_set_vel_amount(int id, PatchParamType param, float amt);
int patch_set_key_amount(int id, PatchParamType param, float amt);

/* modulation getters */
int patch_get_mod_src(int patch_id, PatchParamType, int slot, int* src_id);
int patch_get_mod_amt(int patch_id, PatchParamType, int slot, float* amt);
int patch_get_vel_amount(int id, PatchParamType param, float* val);
int patch_get_key_amount(int id, PatchParamType param, float* val);

/* lfo freq modulation setters */
int patch_set_lfo_fm1_src(int patch_id, int lfo_id, int modsrc_id);
int patch_set_lfo_fm2_src(int patch_id, int lfo_id, int modsrc_id);
int patch_set_lfo_fm1_amt(int patch_id, int lfo_id, float amount);
int patch_set_lfo_fm2_amt(int patch_id, int lfo_id, float amount);

/* lfo freq modulation getters */
int patch_get_lfo_fm1_src(int patch_id, int lfo_id, int* modsrc_id);
int patch_get_lfo_fm2_src(int patch_id, int lfo_id, int* modsrc_id);
int patch_get_lfo_fm1_amt(int patch_id, int lfo_id, float* amount);
int patch_get_lfo_fm2_amt(int patch_id, int lfo_id, float* amount);

/* lfo amp modulation setters */
int patch_set_lfo_am1_src(int patch_id, int lfo_id, int modsrc_id);
int patch_set_lfo_am2_src(int patch_id, int lfo_id, int modsrc_id);
int patch_set_lfo_am1_amt(int patch_id, int lfo_id, float amount);
int patch_set_lfo_am2_amt(int patch_id, int lfo_id, float amount);

/* lfo amp modulation getters */
int patch_get_lfo_am1_src(int patch_id, int lfo_id, int* modsrc_id);
int patch_get_lfo_am2_src(int patch_id, int lfo_id, int* modsrc_id);
int patch_get_lfo_am1_amt(int patch_id, int lfo_id, float* amount);
int patch_get_lfo_am2_amt(int patch_id, int lfo_id, float* amount);



#endif /* __PATCH_SET_AND_GET_H__ */
