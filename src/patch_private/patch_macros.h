#ifndef PATCH_MACROS_H
#define PATCH_MACROS_H


#define INLINE_ISOK_DEF                                             \
inline static int isok(int id)                                      \
{                                                                   \
    if (id < 0 || id >= PATCH_COUNT                                 \
     || !patches[id] || !patches[id]->active)                       \
        return 0;                                                   \
    return 1;                                                       \
}


#define INLINE_PATCH_TRIGGER_GLOBAL_LFO_DEF                             \
inline static void                                                      \
patch_trigger_global_lfo(int patch_id, LFO* lfo, LFOParams* lfopar)     \
{                                                                       \
    Patch* p = patches[patch_id];                                       \
    float const* src;                                                   \
    src = patch_mod_id_to_pointer(lfopar->mod1_id, p, NULL);            \
    lfo_set_freq_mod1(lfo, src);                                        \
    src = patch_mod_id_to_pointer(lfopar->mod2_id, p, NULL);            \
    lfo_set_freq_mod2(lfo, src);                                        \
}
/*
    lfo->freq_mod1 = patch_mod_id_to_pointer(lfopar->mod1_id, p, NULL); \
    lfo->freq_mod2 = patch_mod_id_to_pointer(lfopar->mod2_id, p, NULL); \
    lfo_rigger(lfo, lfopar);                                            \
}
*/

#define INLINE_PATCH_LOCK_DEF                                       \
/* locks a patch so that it will be ignored by patch_render() */    \
inline static void patch_lock (int id)                              \
{                                                                   \
/*    debug("locking %d\n",id);                                   */    \
    pthread_mutex_lock(&patches[id]->mutex);                        \
}


#define INLINE_PATCH_TRYLOCK_DEF                                    \
/*  same as above, but returns immediately with EBUSY if mutex      \
 *  is already held */                                              \
inline static int patch_trylock (int id)                            \
{                                                                   \
    return pthread_mutex_trylock(&patches[id]->mutex);              \
}


#define INLINE_PATCH_UNLOCK_DEF                                     \
/* unlocks a patch after use */                                     \
inline static void patch_unlock (int id)                            \
{                                                                   \
 /*   debug("unlocking %d\n",id);                                 */    \
    pthread_mutex_unlock(&patches[id]->mutex);                      \
}


#endif
