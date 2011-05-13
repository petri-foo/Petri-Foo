#include "mod_src.h"


#include "patch.h"
#include "petri-foo.h"


#include <stdlib.h>
#include <string.h>
#include <strings.h>


static id_name* mod_src_names = 0;
static id_name* mod_src_misc = 0;
static id_name* mod_src_egs = 0;
static id_name* mod_src_vlfos = 0;
static id_name* mod_src_glfos = 0;


void mod_src_create(void)
{
    id_name* ids;

    if (mod_src_names)
    {
        debug("mod_src_ids_create called again\n");
        return;
    }

    ids = mod_src_names = mod_src_get(MOD_SRC_ALL);

    for(; !(ids->id & MOD_SRC_MISC); ++ids);
    mod_src_misc = ids;

    for(; !(ids->id & MOD_SRC_EG); ++ids);
    mod_src_egs = ids;

    for(; !(ids->id & MOD_SRC_VLFO); ++ids);
    mod_src_vlfos = ids;

    for(; !(ids->id & MOD_SRC_GLFO); ++ids);
    mod_src_glfos = ids;
}


void mod_src_destroy(void)
{
    mod_src_free(mod_src_names);
    mod_src_names = 0;
}


id_name* mod_src_get(int id_bitmask)
{
    int bitmask = id_bitmask;
    int count;

    id_name* idnames;
    id_name* ids;

    /*  count 'off' mod source if necessary */
    count = (bitmask == MOD_SRC_ALL || bitmask == MOD_SRC_GLOBALS) ? 1 : 0;

    if (bitmask & MOD_SRC_MISC)
        count += MOD_SRC__LAST_MISC__ - MOD_SRC_MISC;

    if (bitmask & MOD_SRC_EG)
        count += VOICE_MAX_ENVS;

    if (bitmask & MOD_SRC_VLFO)
        count += VOICE_MAX_LFOS;

    if (bitmask & MOD_SRC_GLFO)
        count += PATCH_MAX_LFOS;

    ids = idnames = malloc(sizeof(*idnames) * (count + 1));

    if (!idnames)
        return 0;

    if (bitmask == MOD_SRC_ALL || bitmask == MOD_SRC_GLOBALS)
        id_name_init(ids++, MOD_SRC_NONE, "OFF");

    bitmask = id_bitmask;

    if (bitmask & MOD_SRC_MISC)
    {
        id_name_init(ids++, MOD_SRC_ONE,        "1.0");
        id_name_init(ids++, MOD_SRC_KEY,        "Key");
        id_name_init(ids++, MOD_SRC_VELOCITY,   "Velocity");
    }

    if (bitmask & MOD_SRC_EG)
        ids = id_name_sequence(ids, MOD_SRC_EG,   VOICE_MAX_ENVS, "EG%d");

    if (bitmask & MOD_SRC_VLFO)
        ids = id_name_sequence(ids, MOD_SRC_VLFO, VOICE_MAX_LFOS, "VLFO%d");

    if (bitmask & MOD_SRC_GLFO)
        ids = id_name_sequence(ids, MOD_SRC_GLFO, PATCH_MAX_LFOS, "GLFO%d");

    /* terminate */
    id_name_init(ids, 0, 0);

    return idnames;
}


void mod_src_free(id_name* idnames)
{
    id_name* ids = idnames;

    while(ids->name)
    {
        free(ids->name);
        ++ids;
    }

    free(idnames);
}


int mod_src_id(const char* name, int mask)
{
    id_name* ids;

    debug("identifying %s\n", name);

    if (strcmp(name, "OFF") == 0)
        return MOD_SRC_NONE;

    if (mask & MOD_SRC_MISC)
    {
        for (ids = mod_src_misc; (ids->id & MOD_SRC_MISC); ++ids)
            if (strcmp(ids->name, name) == 0)
                return ids->id;
    }

    if (mask & MOD_SRC_EG)
    {
        for (ids = mod_src_egs; (ids->id & MOD_SRC_EG); ++ids)
            if (strcmp(ids->name, name) == 0)
                return ids->id;
    }

    if (mask & MOD_SRC_VLFO)
    {
        for (ids = mod_src_vlfos; (ids->id & MOD_SRC_VLFO); ++ids)
            if (strcmp(ids->name, name) == 0)
                return ids->id;
    }

    if (mask & MOD_SRC_GLFO)
    {
        for (ids = mod_src_glfos; (ids->id & MOD_SRC_GLFO); ++ids)
            if (strcmp(ids->name, name) == 0)
                return ids->id;
    }

    return MOD_SRC_NONE;
}


const char* mod_src_name(int id)
{
    id_name* ids;

    for (ids = mod_src_names; ids->name; ++ids)
        if (ids->id == id)
            return ids->name;

    return 0;
}


bool mod_src_is_global(int id)
{
    return (id == MOD_SRC_NONE
         || id == MOD_SRC_ONE
         || (id & MOD_SRC_GLOBALS))
            ? true
            : false;
}


bool mod_src_maybe_eg(const char* str)
{
    if (strlen(str) != 3)
        return false;

    if (strncmp(str, "EG", 2) == 0)
        return true;

    return false;
}


bool mod_src_maybe_lfo(const char* str)
{
    if (strlen(str) != 5)
        return false;

    if (strncmp(&str[1], "LFO", 3) == 0)
        return true;

    return false;
}

