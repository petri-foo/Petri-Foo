#include "names.h"


#include "patch.h"
#include "petri-foo.h"


#include <stdlib.h>
#include <string.h>
#include <strings.h>

static char** mod_src_names = 0;

static const char* shape_names[] = {
    "Sine", "Triangle", "Saw", "Square", 0
};


static const char* eg_names[] = {
    "EG1", "EG2", "EG3", "EG4", "EG5", 0
};


static const char* lfo_names[] = {
    "GLFO1", "GLFO2", "GLFO3", "GLFO4", "GLFO5",
    "VLFO1", "VLFO2", "VLFO3", "VLFO4", "VLFO5", 0
};


static const char* param_names[] = {
    "Amplitude",
    "Pan",
    "Pitch",
    "Cutoff",
    "Resonance",
    "Frequency Modulation", 0
};


void names_create(void)
{
    const char none[] = "OFF";
    const char one[] = "1.0";
    const char key[] = "Key";
    const char velocity[] = "Velocity";

    int i;
    int id;

    /* check for mismatched counts etc: */
    #ifdef DEBUG
    if (!names_egs_get() || !names_lfos_get())
    {
        debug("*** PROBLEM OF DEATH IS FORECAST ***\n");
        return;
    }
    #endif

    mod_src_names = malloc(sizeof(*mod_src_names) * MOD_SRC_LAST);

    for (i = 0; i < MOD_SRC_LAST; ++i)
        mod_src_names[i] = 0;

    mod_src_names[MOD_SRC_NONE] = malloc(strlen(none) + 1);
    strcpy(mod_src_names[MOD_SRC_NONE], none);
    mod_src_names[MOD_SRC_ONE] = malloc(strlen(one) + 1);
    strcpy(mod_src_names[MOD_SRC_ONE], one);
    mod_src_names[MOD_SRC_KEY] = malloc(strlen(key) + 1);
    strcpy(mod_src_names[MOD_SRC_KEY], key);
    mod_src_names[MOD_SRC_VELOCITY] = malloc(strlen(velocity) + 1);
    strcpy(mod_src_names[MOD_SRC_VELOCITY], velocity);

    for (i = MOD_SRC_FIRST_EG; i < MOD_SRC_LAST_EG; ++i)
    {
        id = i - MOD_SRC_FIRST_EG;
        if (eg_names[id])
        {
            mod_src_names[i] = malloc(strlen(eg_names[id]) + 1);
            strcpy(mod_src_names[i], eg_names[id]);
        }
        else
        {
            debug("adsr_names mismatch adsr count\n");
            break;
        }
    }

    for (i = MOD_SRC_FIRST_GLFO; i < MOD_SRC_LAST_GLFO; ++i)
    {
        id = i - MOD_SRC_FIRST_GLFO;
        mod_src_names[i] = malloc(strlen(lfo_names[id]) + 1);
        strcpy(mod_src_names[i], lfo_names[id]);
    }

    for (i = MOD_SRC_FIRST_VLFO; i < MOD_SRC_LAST_VLFO; ++i)
    {
        id = (MOD_SRC_LAST_GLFO - MOD_SRC_FIRST_GLFO)
             + (i - MOD_SRC_FIRST_VLFO);
        mod_src_names[i] = malloc(strlen(lfo_names[id]) + 1);
        strcpy(mod_src_names[i], lfo_names[id]);
    }
}


void names_destroy(void)
{
    if (mod_src_names)
    {
        int i;

        for (i = 0; i < MOD_SRC_LAST; ++i)
            if (mod_src_names[i])
                free(mod_src_names[i]);

        free(mod_src_names);
        mod_src_names = 0;
    }
}


const char** names_lfo_shapes_get(void)
{
    return shape_names;
}


int names_lfo_shapes_id_from_str(const char* str)
{
    int i;

    for (i = 0; shape_names[i]; ++i)
        if (strcasecmp(str, shape_names[i]) == 0)
            return i;

    return -1;
}


char** names_mod_srcs_get(void)
{
    #ifdef DEBUG
    if (!mod_src_names)
    {
        debug("mod_src_names not set\n");
    }
    #endif
    return mod_src_names;
}


int names_mod_srcs_id_from_str(const char* str)
{
    int i;

    for (i = 0; i < MOD_SRC_LAST; ++i)
    {
        if (mod_src_names[i])
        {
            if (strcasecmp(str, mod_src_names[i]) == 0)
                return i;
        }
    }

    return -1;
}


const char** names_egs_get(void)
{
    #ifdef DEBUG
    int i;

    for (i = 0; eg_names[i] != 0; ++i);

    if (i != VOICE_MAX_ENVS)
    {
        debug(  "Friendly warning to the programmer:\n"
                "You've either changed the enum value for VOICE_MAX_ENVS\n"
                "Or you've changed the list of ADSR names\n"
                "In either case it's broken now. Please fix!\n");
        return 0;
    }
    #endif
    return eg_names;
}


bool names_egs_maybe_eg_id(const char* str)
{
    if (strlen(str) != 3)
        return FALSE;

    if (strncmp(str, "EG", 2) == 0)
        return TRUE;

    return FALSE;
}


int names_egs_id_from_str(const char* str)
{
    int id = 0;

    #ifdef DEBUG
    if (strncmp(str, "EG", 2) != 0)
        return -1;
    #endif

    if (sscanf(&str[2], "%d", &id) != 1)
        return -1;

    /* EG1 has id zero */
    --id;

    return (id >= 0 && id < VOICE_MAX_ENVS) ? id : -1;
}


const char** names_lfos_get(void)
{
    #ifdef DEBUG
    int i;

    for (i = 0; lfo_names[i] != 0; ++i);

    if (i != TOTAL_LFOS)
    {
        debug(  "Friendly warning to the programmer:\n"
                "You've either changed the enum value for PATCH_MAX_LFOS\n"
                "and/or ther enum value VOICE_MAX_LFOS\n"
                "Or you've changed the list of LFO names\n"
                "In either case it's broken now. Please fix!\n");
        return 0;
    }
    #endif
    return lfo_names;
}


bool names_lfos_maybe_lfo_id(const char* str)
{
    if (strlen(str) != 5)
        return FALSE;

    if (strncmp(&str[1], "LFO", 3) == 0)
        return TRUE;

    return FALSE;
}


int names_lfos_id_from_str(const char* str)
{
    int id;

    #ifdef DEBUG
    if (strncmp(&str[1], "LFO", 3) != 0)
        return -1;
    #endif

    if (sscanf(&str[4], "%d", &id) != 1)
        return -1;

    --id; /* first LFO has id zero */

    if (str[0] == 'G')
    {
        if (id < 0 || id > PATCH_MAX_LFOS)
            return -1;
    }
    else if (str[0] == 'V')
    {
        if (id < 0 || id > VOICE_MAX_LFOS)
            return -1;

        id += PATCH_MAX_LFOS;
    }
    else
        return -1;

    return id;
}

const char** names_params_get(void)
{
    return param_names;
}

int names_params_id_from_str(const char* str)
{
    int i;

    for (i = 0; param_names[i]; ++i)
        if (strcasecmp(str, param_names[i]) == 0)
            return i;

    return -1;
}

