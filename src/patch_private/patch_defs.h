#ifndef PRIVATE_PATCH_DEFS_H
#define PRIVATE_PATCH_DEFS_H


/* Don't add #includes in here. */


/* the minimum envelope release value we'll allow (to prevent clicks) 
 *  FIXME: this may become deprecated, we'll see... */
extern const float  PATCH_MIN_RELEASE;


/* how much time to wait before actually releasing legato patches;
 * this is to make sure that noteons immediately following noteoffs
 * stay "connected" */
extern const float  PATCH_LEGATO_LAG;


/* in certain places, we consider values with an absolute value less
 * than or equal to this to be equivalent to zero  - standard stuff */
extern const float  ALMOST_ZERO;


/* what sample rate we think the audio interface is running at */
extern float        patch_samplerate;


/* how many ticks legato releases lag; calculated to take
 * PATCH_LEGATO_LAG seconds */
extern int          patch_legato_lag;


/* how much to decrease v->declick_vol by each tick; calculated to
 * take PATCH_MIN_RELEASE seconds */
extern float        patch_declick_dec;  /* FIXME: declick_dec will become
                                         * deprecated */


/* the patches */
extern Patch        patches[PATCH_COUNT];


extern float* mod_id_to_pointer(int id, Patch* p, PatchVoice* v);


#endif
