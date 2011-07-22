/*  Petri-Foo is a fork of the Specimen audio sampler.

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
*/


#include "mod_src.h"


#include "midi_control.h"
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
static id_name* mod_src_midi_cc = 0;


static void make_midi_cc(id_name* ids, int id, const char* name)
{
    char buf[80];

    if (snprintf(buf, 80, "CC %d - %s", id, name) > 80)
        buf[79] = '\0';

    id_name_init(ids, MOD_SRC_MIDI_CC + id, buf);
}

static id_name* get_midi_cc(id_name* start)
{
    id_name* ids = start;
    const char* undefined = "Undefined";
    const char* unimplemented = "Unimplemented";
    int id = 0;

    make_midi_cc(ids++, id++,   "Bank Select");
    make_midi_cc(ids++, id++,   "Mod Wheel");
    make_midi_cc(ids++, id++,   "Breath");

    make_midi_cc(ids++, id++,   undefined);

    make_midi_cc(ids++, id++,   "Foot");
    make_midi_cc(ids++, id++,   "Portamento Time");
    make_midi_cc(ids++, id++,   "Data Entry MSB");
    make_midi_cc(ids++, id++,   "Channel Volume");
    make_midi_cc(ids++, id++,   "Balance");

    make_midi_cc(ids++, id++,   undefined);

    make_midi_cc(ids++, id++,   "Pan");
    make_midi_cc(ids++, id++,   "Expression");
    make_midi_cc(ids++, id++,   "Effect Control 1");
    make_midi_cc(ids++, id++,   "Effect Control 2");

    make_midi_cc(ids++, id++,   undefined);
    make_midi_cc(ids++, id++,   undefined);

    make_midi_cc(ids++, id++,   "General Purpose 1");
    make_midi_cc(ids++, id++,   "General Purpose 2");
    make_midi_cc(ids++, id++,   "General Purpose 3");
    make_midi_cc(ids++, id++,   "General Purpose 4");

    for (; id < 32;)
        make_midi_cc(ids++, id++, undefined);

    for (; id < 64;)
        make_midi_cc(ids++, id++, unimplemented);

    make_midi_cc(ids++, id++,   "Sustain On/Off");
    make_midi_cc(ids++, id++,   "Portamento On/Off");
    make_midi_cc(ids++, id++,   "Sostenuto On/Off");
    make_midi_cc(ids++, id++,   "Soft On/Off");
    make_midi_cc(ids++, id++,   "Legato On/Off");
    make_midi_cc(ids++, id++,   "Hold 2 On/Off");

    make_midi_cc(ids++, id++,   "Variation");
    make_midi_cc(ids++, id++,   "Timbre");
    make_midi_cc(ids++, id++,   "Release Time");
    make_midi_cc(ids++, id++,   "Attack Time");
    make_midi_cc(ids++, id++,   "Brightness");
    make_midi_cc(ids++, id++,   "Decay Time");
    make_midi_cc(ids++, id++,   "Vibrato Rate");
    make_midi_cc(ids++, id++,   "Vibrato Depth");
    make_midi_cc(ids++, id++,   "Vibrato Delay");
    make_midi_cc(ids++, id++,   "Sound Controller 10");

    make_midi_cc(ids++, id++,   "General Purpose 5");
    make_midi_cc(ids++, id++,   "General Purpose 6");
    make_midi_cc(ids++, id++,   "General Purpose 7");
    make_midi_cc(ids++, id++,   "General Purpose 8");

    make_midi_cc(ids++, id++,   "Portamento Control");

    make_midi_cc(ids++, id++,   undefined);
    make_midi_cc(ids++, id++,   undefined);
    make_midi_cc(ids++, id++,   undefined);

    make_midi_cc(ids++, id++,   "Hi-Res Velocity Prefix");

    make_midi_cc(ids++, id++,   undefined);
    make_midi_cc(ids++, id++,   undefined);

    make_midi_cc(ids++, id++,   "Effects 1 Depth");
    make_midi_cc(ids++, id++,   "Effects 2 Depth");
    make_midi_cc(ids++, id++,   "Effects 3 Depth");
    make_midi_cc(ids++, id++,   "Effects 4 Depth");
    make_midi_cc(ids++, id++,   "Effects 5 Depth");

    make_midi_cc(ids++, id++,   "Data Increment");
    make_midi_cc(ids++, id++,   "Data Decrement");

    make_midi_cc(ids++, id++,   "NRPN - LSB");
    make_midi_cc(ids++, id++,   "NRPN - MSB");
    make_midi_cc(ids++, id++,   "RPN - LSB");
    make_midi_cc(ids++, id++,   "RPN - MSB");

    for (; id < CC___MSG___LAST;)
        make_midi_cc(ids++, id++, undefined);

    return ids;
}

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

    for(; !(ids->id & MOD_SRC_MIDI_CC); ++ ids);
    mod_src_midi_cc = ids;
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

    if (bitmask & MOD_SRC_MIDI_CC)
        count += CC___MSG___LAST;

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
        id_name_init(ids++, MOD_SRC_PITCH_WHEEL,"Pitch Wheel");
    }

    if (bitmask & MOD_SRC_EG)
        ids = id_name_sequence(ids, MOD_SRC_EG,   VOICE_MAX_ENVS, "EG%d");

    if (bitmask & MOD_SRC_VLFO)
        ids = id_name_sequence(ids, MOD_SRC_VLFO, VOICE_MAX_LFOS, "VLFO%d");

    if (bitmask & MOD_SRC_GLFO)
        ids = id_name_sequence(ids, MOD_SRC_GLFO, PATCH_MAX_LFOS, "GLFO%d");

    if (bitmask & MOD_SRC_MIDI_CC)
    {
        ids = get_midi_cc(ids);
    }

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

    if (mask & MOD_SRC_MIDI_CC)
    {
        for (ids = mod_src_midi_cc; (ids->id & MOD_SRC_MIDI_CC); ++ids)
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

