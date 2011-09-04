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


#ifndef PATCH_MACROS_H
#define PATCH_MACROS_H


#define INLINE_PATCHOK_DEF          \
inline static bool patchok(int id)  \
{                                   \
    return (id >= 0 && id < PATCH_COUNT     \
                    && patches[id] != 0     \
                    && patches[id]->active);\
}


#define INLINE_PATCH_TRIGGER_GLOBAL_LFO_DEF                 \
inline static void                                          \
patch_trigger_global_lfo(int patch_id, LFO* lfo, LFOParams* lfopar) \
{                                                           \
    Patch* p = patches[patch_id];                           \
    float const* src;                                       \
    src = patch_mod_id_to_pointer(lfopar->fm1_id, p, NULL); \
    lfo_set_fm1(lfo, src);                                  \
    src = patch_mod_id_to_pointer(lfopar->fm2_id, p, NULL); \
    lfo_set_fm2(lfo, src);                                  \
    src = patch_mod_id_to_pointer(lfopar->am1_id, p, NULL); \
    lfo_set_am1(lfo, src);                                  \
    src = patch_mod_id_to_pointer(lfopar->am2_id, p, NULL); \
    lfo_set_am2(lfo, src);                                  \
    lfo_update_params(lfo, lfopar);                         \
}


#define INLINE_PATCH_LOCK_DEF                                       \
/* locks a patch so that it will be ignored by patch_render() */    \
inline static void patch_lock (int id)                              \
{                                                                   \
/*    debug("locking %d\n",id);                               */    \
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
 /*   debug("unlocking %d\n",id);                             */    \
    pthread_mutex_unlock(&patches[id]->mutex);                      \
}


#endif
