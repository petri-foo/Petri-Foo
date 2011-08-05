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


#include "names.h"


#include "patch.h"
#include "petri-foo.h"


#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <sndfile.h> /* for format enumerations */


static const char* shape_names[] = {
    "Sine", "Triangle", "Saw", "Square", 0
};


static const char* param_names[] = {
    "Amplitude",
    "Pan",
    "Pitch",
    "Cutoff",
    "Resonance",
    "Frequency Modulation", 0
};


id_name* id_name_new(int id, const char* name)
{
    id_name* idname = malloc(sizeof(*idname));

    if (!idname)
        return 0;

    debug("creating id_name %p %d %s\n", idname, id, name);
    id_name_init(idname, id, name);

    return idname;
}


void id_name_init(id_name* idname, int id, const char* name)
{
    idname->id = id;
    idname->name = 0;

    if (name)
    {
        idname->name = malloc(strlen(name) + 1);
        strcpy(idname->name, name);
    }
/*
    debug("initialized id_name %p %d %s\n", idname, id, name);
 */
}


id_name* id_name_dup(const id_name* src)
{
    return id_name_new(src->id, src->name);
}


void id_name_free(id_name* idname)
{
    if (!idname)
        return;

    debug("freeing %p %d %s\n", idname, idname->id, idname->name);

    if (idname->name)
        free(idname->name);

    free(idname);
}


id_name* id_name_sequence(id_name* start, int first_id, int count,
                                                    const char* name_fmt)
{
    int i;
    int n;
    const int blen = 40;
    char buf[blen];

    for (i = 0; i < count; ++i)
    {
        if ((n = snprintf(buf, blen, name_fmt, i + 1)))
        {
            if (n == blen)
                buf[blen - 1] = '\0';

            id_name_init(start++, first_id + i, buf);
        }
    }

    return start;
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


typedef struct _sample_raw_format
{
    const int id;
    const char* name;

} sample_raw_format;


/*  i previously simply returned the static array below, but now because
    struct id_name has aquired more dynamic use cases, the simple method
    can no longer be used so simply :-(
 */
id_name* names_sample_raw_format_get(void)
{
    static sample_raw_format raw_formats[] = {
        { SF_FORMAT_RAW | SF_FORMAT_PCM_U8,   "Unsigned 8 bit data" },
        { SF_FORMAT_RAW | SF_FORMAT_PCM_S8,   "Signed 8 bit data"   },
        { SF_FORMAT_RAW | SF_FORMAT_PCM_16,   "Signed 16 bit data"  },
        { SF_FORMAT_RAW | SF_FORMAT_PCM_24,   "Signed 24 bit data"  },
        { SF_FORMAT_RAW | SF_FORMAT_PCM_32,   "Signed 32 bit data"  },
        { SF_FORMAT_RAW | SF_FORMAT_FLOAT,    "32 bit float data"   },
        { SF_FORMAT_RAW | SF_FORMAT_DOUBLE,   "64 bit float data"   },
        { SF_FORMAT_RAW | SF_FORMAT_ULAW,     "U-Law encoded"       },
        { SF_FORMAT_RAW | SF_FORMAT_ALAW,     "A-Law encoded"       },
        { SF_FORMAT_RAW | SF_FORMAT_GSM610,   "GSM 6.10 encoding"   },
        { SF_FORMAT_RAW | SF_FORMAT_DWVW_12,
                        "12 bit Delta Width Variable Word encoding" },
        { SF_FORMAT_RAW | SF_FORMAT_DWVW_16,
                        "16 bit Delta Width Variable Word encoding" },
        { SF_FORMAT_RAW | SF_FORMAT_DWVW_24,
                        "24 bit Delta Width Variable Word encoding" },
        { SF_FORMAT_RAW | SF_FORMAT_VOX_ADPCM,
                        "Oki Dialogic ADPCM encoding"               },
        { 0, 0                                                      }
    };

    id_name* ids = 0;
    int count = 0;

    for (count = 0; raw_formats[count].name != 0; ++count);

    ids = malloc(sizeof(*ids) * count);

    if (!ids)
        return 0;

    for (count = 0; raw_formats[count].name != 0; ++count)
        id_name_init(&ids[count], raw_formats[count].id,
                                  raw_formats[count].name);

    id_name_init(&ids[count - 1], 0, 0);

    return ids;
}
