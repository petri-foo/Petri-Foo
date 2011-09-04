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


#ifndef __PATCH_SET_AND_GET_H__
#define __PATCH_SET_AND_GET_H__


#include "patch.h"


/*  **ALL** IDs passed to these functions **MUST** be VALID.
 *  in debugging mode, this is ensured with assertions.
 *
 *  patch_id:   0,1,2,3... < 
 *  env_id:     MOD_SRC_EG, +0, +1, +2, +3...
 *  lfo_id:     MOD_SRC_LFO, +0, +1, +2, +3...
 *  slot:       0,1,2,3... < MAX_MOD_SLOTS
 *
 *  ALL setter functions return 0 on success (value set was in
 *  range), and -1 on failure (value set was out of range) and
 *  additionally set an error code (retrievable via pf_error_get).
 */





/* envelope setters */
int patch_set_env_active  (int patch_id, int env_id, bool state);
int patch_set_env_delay   (int patch_id, int env_id, float secs);
int patch_set_env_attack  (int patch_id, int env_id, float secs);
int patch_set_env_hold    (int patch_id, int env_id, float secs);
int patch_set_env_decay   (int patch_id, int env_id, float secs);
int patch_set_env_sustain (int patch_id, int env_id, float level);
int patch_set_env_release (int patch_id, int env_id, float secs);
int patch_set_env_key_amt (int patch_id, int env_id, float val);
int patch_set_env_vel_amt (int patch_id, int env_id, float val);

/* envelope getters */
bool    patch_get_env_active  (int patch_id, int env_id);
float   patch_get_env_delay   (int patch_id, int env_id);
float   patch_get_env_attack  (int patch_id, int env_id);
float   patch_get_env_hold    (int patch_id, int env_id);
float   patch_get_env_decay   (int patch_id, int env_id);
float   patch_get_env_sustain (int patch_id, int env_id);
float   patch_get_env_release (int patch_id, int env_id);
float   patch_get_env_key_amt (int patch_id, int env_id);
float   patch_get_env_vel_amt (int patch_id, int env_id);

/* lfo setters */
int patch_set_lfo_active    (int patch_id, int lfo_id, bool state);
int patch_set_lfo_attack    (int patch_id, int lfo_id, float secs);
int patch_set_lfo_sync_beats(int patch_id, int lfo_id, float beats);
int patch_set_lfo_delay     (int patch_id, int lfo_id, float secs);
int patch_set_lfo_freq      (int patch_id, int lfo_id, float freq);
int patch_set_lfo_positive  (int patch_id, int lfo_id, bool state);
int patch_set_lfo_shape     (int patch_id, int lfo_id, LFOShape shape);
int patch_set_lfo_sync      (int patch_id, int lfo_id, bool state);

/* lfo getters */
bool        patch_get_lfo_active    (int patch_id, int lfo_id);
float       patch_get_lfo_attack    (int patch_id, int lfo_id);
float       patch_get_lfo_sync_beats(int patch_id, int lfo_id);
float       patch_get_lfo_delay     (int patch_id, int lfo_id);
float       patch_get_lfo_freq      (int patch_id, int lfo_id);
bool        patch_get_lfo_positive  (int patch_id, int lfo_id);
LFOShape    patch_get_lfo_shape     (int patch_id, int lfo_id);
bool        patch_get_lfo_sync      (int patch_id, int lfo_id);

/* parameter setters */
int patch_set_channel   (int patch_id, int channel);
int patch_set_cut       (int patch_id, int cut);
int patch_set_cut_by    (int patch_id, int cut_by);
int patch_set_cutoff    (int patch_id, float freq);
int patch_set_legato    (int patch_id, bool val);
int patch_set_lower_note(int patch_id, int note);
int patch_set_lower_vel (int patch_id, int vel);
int patch_set_upper_vel (int patch_id, int vel);

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
int patch_set_root_note    (int id, int note);
int patch_set_panning      (int id, float pan);
int patch_set_pitch        (int id, float pitch);
int patch_set_pitch_steps  (int id, int steps);
int patch_set_play_mode    (int id, PatchPlayMode mode);
int patch_set_portamento   (int id, bool val);
int patch_set_portamento_time(int id, float secs);
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
int     patch_get_lower_vel     (int id);
int     patch_get_upper_vel     (int id);

int     patch_get_mark_frame        (int patch_id, int mark_id);
int     patch_get_mark_frame_range  (int patch_id, int mark_id,
                                                   int* frame_min,
                                                   int* frame_max);

bool            patch_get_monophonic        (int id);
char*           patch_get_name              (int id);
int             patch_get_root_note         (int id);
float           patch_get_panning           (int id);
float           patch_get_pitch             (int id);
int             patch_get_pitch_steps       (int id);
PatchPlayMode   patch_get_play_mode         (int id);
bool            patch_get_portamento        (int id);
float           patch_get_portamento_time   (int id);


float           patch_get_resonance         (int id);
const float*    patch_get_sample            (int id);
const char*     patch_get_sample_name       (int id);
int             patch_get_upper_note        (int id);
float           patch_get_amplitude         (int id);
int             patch_get_fade_samples      (int id);
int             patch_get_xfade_samples     (int id);
int             patch_get_max_fade_samples  (int id);
int             patch_get_max_xfade_samples (int id);


/* returns 0 if non-raw sample loaded */
int patch_get_raw_samplerate(int id);
int patch_get_raw_channels(int id);
int patch_get_raw_sndfile_format(int id);


/* param */
float   patch_param_get_value(int patch_id, PatchParamType);
void    patch_param_set_value(int patch_id, PatchParamType, float  val);

/* modulation setters */
int patch_param_set_mod_src(int patch_id, PatchParamType,   int slot,
                                                            int src_id);
int patch_param_set_mod_amt(int patch_id, PatchParamType,   int slot,
                                                            float amt);
int patch_param_set_vel_amount(int id, PatchParamType param,float amt);
int patch_param_set_key_amount(int id, PatchParamType param,float amt);

/* modulation getters */
int     patch_param_get_mod_src(int patch_id, PatchParamType,   int slot);
float   patch_param_get_mod_amt(int patch_id, PatchParamType,   int slot);
float   patch_param_get_vel_amount(int id, PatchParamType param);
float   patch_param_get_key_amount(int id, PatchParamType param);


/* PatchBool set/get */
void    patch_bool_set_active(  int patch_id, PatchBoolType, bool);
void    patch_bool_set_thresh(  int patch_id, PatchBoolType, float);
void    patch_bool_set_mod_src( int patch_id, PatchBoolType, int mod_id);


bool    patch_bool_get_active(  int patch_id, PatchBoolType);
float   patch_bool_get_thresh(  int patch_id, PatchBoolType);
int     patch_bool_get_mod_src( int patch_id, PatchBoolType);
void    patch_bool_get_all(     int patch_id, PatchBoolType,
                                              bool*  active,
                                              float* thresh,
                                              int*   mod_id);

/* PatchFloat set/get */
void    patch_float_set_value(  int patch_id, PatchFloatType, float);
void    patch_float_set_mod_src(int patch_id, PatchFloatType, int mod_id);
void    patch_float_set_mod_amt(int patch_id, PatchFloatType, float);

float   patch_float_get_value(  int patch_id, PatchFloatType);
int     patch_float_get_mod_src(int patch_id, PatchFloatType);
float   patch_float_get_mod_amt(int patch_id, PatchFloatType);
void    patch_float_get_all(    int patch_id, PatchFloatType,
                                              float* value,
                                              float* mod_amt,
                                              int*   mod_id);

/* lfo freq modulation setters */
int patch_set_lfo_fm1_src(int patch_id, int lfo_id, int modsrc_id);
int patch_set_lfo_fm2_src(int patch_id, int lfo_id, int modsrc_id);
int patch_set_lfo_fm1_amt(int patch_id, int lfo_id, float amount);
int patch_set_lfo_fm2_amt(int patch_id, int lfo_id, float amount);

/* lfo amp modulation setters */
int patch_set_lfo_am1_src(int patch_id, int lfo_id, int modsrc_id);
int patch_set_lfo_am2_src(int patch_id, int lfo_id, int modsrc_id);
int patch_set_lfo_am1_amt(int patch_id, int lfo_id, float amount);
int patch_set_lfo_am2_amt(int patch_id, int lfo_id, float amount);

/* lfo freq modulation getters */
int     patch_get_lfo_fm1_src(int patch_id, int lfo_id);
int     patch_get_lfo_fm2_src(int patch_id, int lfo_id);
float   patch_get_lfo_fm1_amt(int patch_id, int lfo_id);
float   patch_get_lfo_fm2_amt(int patch_id, int lfo_id);

/* lfo amp modulation getters */
int     patch_get_lfo_am1_src(int patch_id, int lfo_id);
int     patch_get_lfo_am2_src(int patch_id, int lfo_id);
float   patch_get_lfo_am1_amt(int patch_id, int lfo_id);
float   patch_get_lfo_am2_amt(int patch_id, int lfo_id);



#endif /* __PATCH_SET_AND_GET_H__ */
