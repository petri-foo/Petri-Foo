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


#include "dish_file.h"

#include <libxml/parser.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <errno.h>
#include <unistd.h>
#include <locale.h>

#include "config.h"
#include "file_ops.h"
#include "mixer.h"
#include "mod_src.h"
#include "msg_log.h"
#include "patch.h"
#include "patch_util.h"
#include "patch_set_and_get.h"
#include "petri-foo.h"
#include "pf_error.h"
#include "sample.h"


typedef struct _dish_file_data
{
    int     samplerate;
    bool    full_save;
    char*   bank_dir;   /* directory where dish file is located */
    char*   parent_dir; /* parent directory (of full-save) */
    char*   bank_name;  /* name of bank (without path or extension) */
    char*   file_path;  /* full path to dish file */
} dish_file_data;

dish_file_data* dish_data = 0;



const char* dish_file_extension(void)
{
    return ".petri-foo";
}


static int sanitize_sample_points(  int* play_start, int* play_stop,
                                    int* loop_start, int* loop_stop,
    /* oh this is fun... */         int* fade_samples,
                                    int* xfade_samples,
                                    int sample_stop)
{
    /*  the sanity check procedure goes like this:

            1) check start above zero
            2) check end below sample_stop (ie length - 1)
            3) check start before end
            4) check fade < end - start*

        this is done first for the play marks
        and secondly for the loop marks.

        precedence goes to the marks themselves before
        the fades so that the fades are adjusted to
        accomadate the mark positions.

        *note:  the fade check is different to this
                for both play points and loop points.
     */

    if (*play_start < 0)
    {
        msg_log(MSG_WARNING, "Sanitizing play start mark\n");
        *play_start = 0;
    }

    if (*play_stop < *play_start)
    {
        msg_log(MSG_WARNING, "Sanitizing play stop mark\n");
        *play_stop = *play_start + 1;
    }

    if (*play_stop > sample_stop)
    {
        msg_log(MSG_WARNING, "Sanitizing play stop mark\n");

        if ((*play_stop = sample_stop) < 0)
        {
            msg_log(MSG_ERROR, "Can't sanitize sample points\n");
            return -1;
        }
    }

    if (*fade_samples < 0)
    {
        msg_log(MSG_WARNING, "Sanitizing fade samples\n");
        *fade_samples = 0;
    }

    if (*play_start + *fade_samples * 2 > *play_stop)
    {
        msg_log(MSG_WARNING, "Sanitizing fade samples\n");
        *fade_samples = (*play_stop - *play_start) / 2;
    }

    /* LOOP POINT SANITY */

    if (*loop_start < *play_start)
    {
        msg_log(MSG_WARNING, "Sanitizing loop start mark\n");
        *loop_start = *play_start;
    }

    if (*loop_stop > *play_stop)
    {
        msg_log(MSG_WARNING, "Sanitizing loop stop mark\n");
        *loop_stop = *play_stop;
    }

    if (*play_start + *xfade_samples > *loop_start)
    {
        msg_log(MSG_WARNING, "Sanitizing xfade samples\n");
        *xfade_samples = *loop_start - *play_start;
    }

    if (*loop_start + *xfade_samples > *loop_stop)
    {
        msg_log(MSG_WARNING, "Sanitizing xfade samples\n");
        *xfade_samples = *loop_stop - *loop_start;
    }

    if (*loop_stop + *xfade_samples > *play_stop)
    {
        msg_log(MSG_WARNING, "Sanitizing xfade samples\n");
        *xfade_samples = *play_stop - *loop_stop;
    }

    return 0;
}


static int dish_file_write_sample_mode_props(xmlNodePtr node, int patch_id)
{
    const char* mode = 0;
    int playmode = patch_get_play_mode(patch_id);

    if (playmode & PATCH_PLAY_TRIM)
        mode = "trim";
    else if (playmode & PATCH_PLAY_PINGPONG)
        mode = "pingpong";
    else if (playmode & PATCH_PLAY_LOOP)
        mode = "loop";
    else
        mode = "singleshot";

    xmlNewProp(node,    BAD_CAST "mode", BAD_CAST mode);
    xmlNewProp(node,    BAD_CAST "reverse",
                        BAD_CAST (playmode & PATCH_PLAY_REVERSE ? "true"
                                                                : "false"));
    xmlNewProp(node,    BAD_CAST "to_end",
                        BAD_CAST (playmode & PATCH_PLAY_TO_END  ? "true"
                                                                : "false"));
    return 0;
}


static int dish_file_write_sample_raw(xmlNodePtr nodeparent, int patch_id)
{
    xmlNodePtr node;
    const Sample* s = patch_sample_data(patch_id);
    char buf[CHARBUFSIZE];

    if (!(s->raw_samplerate || s->raw_channels || s->sndfile_format))
        return 0;

    node = xmlNewTextChild(nodeparent, NULL, BAD_CAST "Raw", NULL);

    snprintf(buf, CHARBUFSIZE, "%d", s->raw_samplerate);
    xmlNewProp(node, BAD_CAST "samplerate", BAD_CAST buf);

    snprintf(buf, CHARBUFSIZE, "%d", s->raw_channels);
    xmlNewProp(node, BAD_CAST "channels", BAD_CAST buf);

    snprintf(buf, CHARBUFSIZE, "%d", s->sndfile_format);
    xmlNewProp(node, BAD_CAST "sndfile_format", BAD_CAST buf);

    return 0;
}


static int
dish_file_write_param(xmlNodePtr nodeparent, int patch_id,
                                                PatchParamType param)
{
    xmlNodePtr  node1;
    xmlNodePtr  node2;
    char buf[CHARBUFSIZE];
    const char** param_names;

    const char* prop1 = 0;
    const char* prop2 = 0;

    float   val1 = 0;
    float   val2 = 0;
    float   vel_amt;
    float   key_trk;
    int     modsrc;
    float   modamt;

    int     last_mod_slot = MAX_MOD_SLOTS;
    int     i;

    val1 = patch_param_get_value(patch_id, param);

    param_names = names_params_get();

    switch(param)
    {
    case PATCH_PARAM_AMPLITUDE: prop1 = "level";    prop2 = 0;
                                --last_mod_slot;                break;
    case PATCH_PARAM_PANNING:   prop1 = "position"; prop2 = 0;  break;
    case PATCH_PARAM_CUTOFF:    prop1 = "value";    prop2 = 0;  break;
    case PATCH_PARAM_RESONANCE: prop1 = "amount";   prop2 = 0;  break;
    case PATCH_PARAM_PITCH:     val2 = patch_get_pitch_steps(patch_id);
                                prop1 = "tuning";
                                prop2 = "tuning_range";         break;
    default:    /* probably won't ever get here if this is the case ;-} */
        return -1;
    }

    node1 = xmlNewTextChild(nodeparent, NULL,
                            BAD_CAST param_names[param], NULL);

    snprintf(buf, CHARBUFSIZE, "%f", val1);
    xmlNewProp(node1, BAD_CAST prop1, BAD_CAST buf);

    if (prop2)
    {
        snprintf(buf, CHARBUFSIZE, "%f", val2);
        xmlNewProp(node1, BAD_CAST prop2, BAD_CAST buf);
    }

    /* velocity sensing */
    vel_amt = patch_param_get_vel_amount(patch_id, param);
    snprintf(buf, CHARBUFSIZE, "%f", vel_amt);
    xmlNewProp(node1, BAD_CAST "velocity_sensing", BAD_CAST buf);

    /* keyboard tracking */
    key_trk = patch_param_get_key_amount(patch_id, param);
    snprintf(buf, CHARBUFSIZE, "%f", key_trk);
    xmlNewProp(node1, BAD_CAST "key_tracking", BAD_CAST buf);

    for (i = 0; i < last_mod_slot; ++i)
    {
        snprintf(buf, CHARBUFSIZE, "Mod%d", i + 1);

        modsrc = patch_param_get_mod_src(patch_id, param, i);
        modamt = patch_param_get_mod_amt(patch_id, param, i);

        node2 = xmlNewTextChild(node1, NULL, BAD_CAST buf, NULL);
        xmlNewProp(node2, BAD_CAST "source", BAD_CAST mod_src_name(modsrc));
        snprintf(buf, CHARBUFSIZE, "%f", modamt);
        xmlNewProp(node2, BAD_CAST "amount", BAD_CAST buf);
    }

    if (param == PATCH_PARAM_AMPLITUDE)
    {
        modsrc = patch_param_get_mod_src(patch_id, PATCH_PARAM_AMPLITUDE,
                                                   EG_MOD_SLOT);
        node2 = xmlNewTextChild(node1, NULL, BAD_CAST "Env", NULL);
        xmlNewProp(node2, BAD_CAST "source", BAD_CAST mod_src_name(modsrc));
    }

    return 0;
}


static int
dish_file_write_bool(xmlNodePtr nodeparent, int patch_id,
                                                PatchBoolType bool_type)
{
    xmlNodePtr  node1;
    xmlNodePtr  node2;
    char buf[CHARBUFSIZE];
    const char* nodestr;

    bool    set;
    float   thresh;
    int     modsrc;

    switch(bool_type)
    {
    case PATCH_BOOL_PORTAMENTO:
        nodestr = "Portamento";
        break;
    case PATCH_BOOL_MONO:
        nodestr = "Mono";
        break;
    case PATCH_BOOL_LEGATO:
        nodestr = "Legato";
        break;
    default:
        return -1;
    }

    patch_bool_get_all(patch_id, bool_type, &set, &thresh, &modsrc);

    node1 = xmlNewTextChild(nodeparent, NULL, BAD_CAST nodestr, NULL);

    xmlNewProp(node1, BAD_CAST "active",
                        BAD_CAST ((set) ? "true" : "false"));

    node2 = xmlNewTextChild(node1, NULL, BAD_CAST "Mod", NULL);

    xmlNewProp(node2, BAD_CAST "source", BAD_CAST mod_src_name(modsrc));

    snprintf(buf, CHARBUFSIZE, "%f", thresh);
    xmlNewProp(node2, BAD_CAST "threshold", BAD_CAST buf);

    return 0;
}


static int
dish_file_write_float(xmlNodePtr nodeparent, int patch_id,
                                                PatchFloatType float_type)
{
    xmlNodePtr  node1;
    xmlNodePtr  node2;
    char buf[CHARBUFSIZE];
    const char* nodestr;

    float   val;
    float   modamt;
    int     modsrc;

    switch(float_type)
    {
    case PATCH_FLOAT_PORTAMENTO_TIME:
        nodestr = "Portamento_time";
        break;
    default:
        return -1;
    }

    patch_float_get_all(patch_id, float_type, &val, &modamt, &modsrc);

    node1 = xmlNewTextChild(nodeparent, NULL, BAD_CAST nodestr, NULL);

    snprintf(buf, CHARBUFSIZE, "%f", val);
    xmlNewProp(node1,   BAD_CAST "value",   BAD_CAST buf);

    node2 = xmlNewTextChild(node1, NULL, BAD_CAST "Mod", NULL);

    xmlNewProp(node2, BAD_CAST "source", BAD_CAST mod_src_name(modsrc));

    snprintf(buf, CHARBUFSIZE, "%f", modamt);
    xmlNewProp(node2, BAD_CAST "amount", BAD_CAST buf);

    return 0;
}


static int
dish_file_write_eg(xmlNodePtr nodeparent, int patch_id, int eg_id)
{
    xmlNodePtr  node1;
    char buf[CHARBUFSIZE];
    bool active;
    float val;

    active = patch_get_env_active(patch_id, eg_id);

    node1 = xmlNewTextChild(nodeparent, NULL,
                            BAD_CAST mod_src_name(eg_id), NULL);

    xmlNewProp(node1,   BAD_CAST "active",
                        BAD_CAST (active ? "true" : "false"));

    val = patch_get_env_delay(patch_id, eg_id);
    snprintf(buf, CHARBUFSIZE, "%f", val);
    xmlNewProp(node1,   BAD_CAST "delay",   BAD_CAST buf);

    val = patch_get_env_attack(patch_id, eg_id);
    snprintf(buf, CHARBUFSIZE, "%f", val);
    xmlNewProp(node1,   BAD_CAST "attack",   BAD_CAST buf);

    val = patch_get_env_hold(patch_id, eg_id);
    snprintf(buf, CHARBUFSIZE, "%f", val);
    xmlNewProp(node1,   BAD_CAST "hold",   BAD_CAST buf);

    val = patch_get_env_decay(patch_id, eg_id);
    snprintf(buf, CHARBUFSIZE, "%f", val);
    xmlNewProp(node1,   BAD_CAST "decay",   BAD_CAST buf);

    val = patch_get_env_sustain(patch_id, eg_id);
    snprintf(buf, CHARBUFSIZE, "%f", val);
    xmlNewProp(node1,   BAD_CAST "sustain",   BAD_CAST buf);

    val = patch_get_env_release(patch_id, eg_id);
    snprintf(buf, CHARBUFSIZE, "%f", val);
    xmlNewProp(node1,   BAD_CAST "release",   BAD_CAST buf);

    val = patch_get_env_key_amt(patch_id, eg_id);
    snprintf(buf, CHARBUFSIZE, "%f", val);
    xmlNewProp(node1,   BAD_CAST "key_tracking",   BAD_CAST buf);

    return 0;
}


static int
dish_file_write_lfo(xmlNodePtr nodeparent, int patch_id, int lfo_id)
{
    xmlNodePtr  node1;
    xmlNodePtr  node2;
    xmlNodePtr  node3;
    char buf[CHARBUFSIZE];
    bool state;
    float val;
    const char** shapes = names_lfo_shapes_get();
    int     mod1src;
    int     mod2src;
    float   mod1amt;
    float   mod2amt;
    LFOShape shape;

    state = patch_get_lfo_active(patch_id, lfo_id);

    node1 = xmlNewTextChild(nodeparent, NULL,
                            BAD_CAST mod_src_name(lfo_id), NULL);

    xmlNewProp(node1,   BAD_CAST "active",
                        BAD_CAST (state ? "true" : "false"));

    node2 = xmlNewTextChild(node1, NULL, BAD_CAST "Frequency", NULL);
    val = patch_get_lfo_freq(patch_id, lfo_id);
    snprintf(buf, CHARBUFSIZE, "%f", val);
    xmlNewProp(node2,   BAD_CAST "hrtz", BAD_CAST buf);

    val = patch_get_lfo_sync_beats(patch_id, lfo_id);
    snprintf(buf, CHARBUFSIZE, "%f", val);
    xmlNewProp(node2,   BAD_CAST "beats", BAD_CAST buf);

    state = patch_get_lfo_sync(patch_id, lfo_id);
    xmlNewProp(node2,   BAD_CAST "sync",
                        BAD_CAST (state ? "true" : "false"));

    mod1src = patch_get_lfo_fm1_src(patch_id, lfo_id);
    mod1amt = patch_get_lfo_fm1_amt(patch_id, lfo_id);
    mod2src = patch_get_lfo_fm2_src(patch_id, lfo_id);
    mod2amt = patch_get_lfo_fm2_amt(patch_id, lfo_id);

    node3 = xmlNewTextChild(node2, NULL, BAD_CAST "Mod1", NULL);
    xmlNewProp(node3, BAD_CAST "source", BAD_CAST mod_src_name(mod1src));
    snprintf(buf, CHARBUFSIZE, "%f", mod1amt);
    xmlNewProp(node3, BAD_CAST "amount", BAD_CAST buf);

    node3 = xmlNewTextChild(node2, NULL, BAD_CAST "Mod2", NULL);
    xmlNewProp(node3, BAD_CAST "source", BAD_CAST mod_src_name(mod2src));
    snprintf(buf, CHARBUFSIZE, "%f", mod2amt);
    xmlNewProp(node3, BAD_CAST "amount", BAD_CAST buf);


    node2 = xmlNewTextChild(node1, NULL, BAD_CAST "Amplitude", NULL);

    shape = patch_get_lfo_shape(patch_id, lfo_id);
    xmlNewProp(node2,   BAD_CAST "shape", BAD_CAST shapes[shape]);

    state = patch_get_lfo_positive(patch_id, lfo_id);
    xmlNewProp(node2,   BAD_CAST "positive",
                        BAD_CAST (state ? "true" : "false"));

    /* assured by caller that lfo_id IS an lfo_id */
    if (!mod_src_is_global(lfo_id))
    {
        val = patch_get_lfo_delay(patch_id, lfo_id);
        snprintf(buf, CHARBUFSIZE, "%f", val);
        xmlNewProp(node2,   BAD_CAST "delay",   BAD_CAST buf);

        val = patch_get_lfo_attack(patch_id, lfo_id);
        snprintf(buf, CHARBUFSIZE, "%f", val);
        xmlNewProp(node2,   BAD_CAST "attack",  BAD_CAST buf);
    }

    mod1src = patch_get_lfo_am1_src(patch_id, lfo_id);
    mod1amt = patch_get_lfo_am1_amt(patch_id, lfo_id);
    mod2src = patch_get_lfo_am2_src(patch_id, lfo_id);
    mod2amt = patch_get_lfo_am2_amt(patch_id, lfo_id);

    node3 = xmlNewTextChild(node2, NULL, BAD_CAST "Mod1", NULL);
    xmlNewProp(node3, BAD_CAST "source", BAD_CAST mod_src_name(mod1src));
    snprintf(buf, CHARBUFSIZE, "%f", mod1amt);
    xmlNewProp(node3, BAD_CAST "amount", BAD_CAST buf);

    node3 = xmlNewTextChild(node2, NULL, BAD_CAST "Mod2", NULL);
    xmlNewProp(node3, BAD_CAST "source", BAD_CAST mod_src_name(mod2src));
    snprintf(buf, CHARBUFSIZE, "%f", mod2amt);
    xmlNewProp(node3, BAD_CAST "amount", BAD_CAST buf);

    return 0;
}


static int dish_write(void)
{
    int rc;

    xmlDocPtr   doc;
    xmlNodePtr  noderoot;
    xmlNodePtr  nodepatch;
    xmlNodePtr  node1;
    xmlNodePtr  node2;

    char    buf[CHARBUFSIZE];
    int     i, j;
    int*    patch_id;
    int     patch_count;
    char*   samples_dir = 0;

    setlocale(LC_NUMERIC, "C");

    assert(dish_data != 0);
    assert(dish_data->file_path != 0);

    debug("attempting to write file:%s\n", dish_data->file_path);

    doc = xmlNewDoc(BAD_CAST "1.0");

    if (!doc)
    {
        msg_log(MSG_ERROR, "XML error!\n");
        setlocale(LC_NUMERIC, "");
        return -1;
    }

    noderoot = xmlNewDocNode(doc, NULL, BAD_CAST "Petri-Foo-Dish", NULL);

    if (!noderoot)
    {
        msg_log(MSG_ERROR, "XML error!\n");
        setlocale(LC_NUMERIC, "");
        return -1;
    }

    xmlNewProp(noderoot, BAD_CAST "save-type",
                         BAD_CAST (dish_data->full_save ? "full"
                                                        : "basic"));
    if (dish_data->full_save)
    {
        free(file_ops_mkdir(0, dish_data->bank_dir));
        samples_dir = file_ops_mkdir("samples", dish_data->bank_dir);

        if (!samples_dir)
        {
            setlocale(LC_NUMERIC, "");
            return -1;
        }
    }

    xmlDocSetRootElement(doc, noderoot);

    /*  ------------------------
        master
     */
    node1 = xmlNewTextChild(noderoot, NULL, BAD_CAST "Master", NULL);

    snprintf(buf, CHARBUFSIZE, "%f", mixer_get_amplitude());
    xmlNewProp(node1, BAD_CAST "level", BAD_CAST buf);

    snprintf(buf, CHARBUFSIZE, "%d", patch_get_samplerate());
    xmlNewProp(node1, BAD_CAST "samplerate", BAD_CAST buf);


    /*  ------------------------
        patches
     */
    patch_count = patch_dump(&patch_id);

    for (i = 0; i < patch_count; ++i)
    {
        const char* samplepath = patch_get_sample_name(patch_id[i]);
        debug("writing patch:%d\n", patch_id[i]);

        nodepatch = xmlNewTextChild(noderoot, NULL,
                                BAD_CAST "Patch", NULL);

        xmlNewProp(nodepatch,   BAD_CAST "name",
                                BAD_CAST patch_get_name(patch_id[i]));

        snprintf(buf, CHARBUFSIZE, "%d", patch_get_channel(patch_id[i]));
        xmlNewProp(nodepatch,   BAD_CAST "channel", BAD_CAST buf);

        /*  ------------------------
            sample
         */
        node1 = xmlNewTextChild(nodepatch, NULL, BAD_CAST "Sample", NULL);

        if (samplepath && dish_data->full_save
         && strcmp(samplepath, "Default") != 0)
        {
            char* path = file_ops_sample_path_mangle(samplepath,
                                                     dish_data->bank_dir,
                                                     samples_dir);

            xmlNewProp(node1, BAD_CAST "file", BAD_CAST path);
            msg_log(MSG_MESSAGE, "patch %d symlink %s to %s\n",
                                                    i, path, samplepath);
            free(path);
        }
        else
            xmlNewProp(node1, BAD_CAST "file", BAD_CAST samplepath);

        /* play mode, reverse, to_end */
        dish_file_write_sample_mode_props(node1, patch_id[i]);

        /* raw samplerate, raw channels, sndfile format */
        dish_file_write_sample_raw(node1, patch_id[i]);

        if (patch_get_sample(patch_id[i]))
        {
            /* sample play */
            node2 = xmlNewTextChild(node1, NULL, BAD_CAST "Play", NULL);

            snprintf(buf, CHARBUFSIZE, "%d",
                    patch_get_mark_frame(patch_id[i], WF_MARK_PLAY_START));
            xmlNewProp(node2,   BAD_CAST "start", BAD_CAST buf);

            snprintf(buf, CHARBUFSIZE, "%d",
                    patch_get_mark_frame(patch_id[i], WF_MARK_PLAY_STOP));
            xmlNewProp(node2,   BAD_CAST "stop", BAD_CAST buf);

            snprintf(buf, CHARBUFSIZE, "%d",
                                patch_get_fade_samples(patch_id[i]));
            xmlNewProp(node2,   BAD_CAST "fade_samples", BAD_CAST buf);

            /* sample loop */
            node2 = xmlNewTextChild(node1, NULL, BAD_CAST "Loop", NULL);

            snprintf(buf, CHARBUFSIZE, "%d",
                    patch_get_mark_frame(patch_id[i], WF_MARK_LOOP_START));
            xmlNewProp(node2,   BAD_CAST "start", BAD_CAST buf);

            snprintf(buf, CHARBUFSIZE, "%d",
                    patch_get_mark_frame(patch_id[i], WF_MARK_LOOP_STOP));
            xmlNewProp(node2,   BAD_CAST "stop", BAD_CAST buf);

            snprintf(buf, CHARBUFSIZE, "%d",
                    patch_get_xfade_samples(patch_id[i]));
            xmlNewProp(node2,   BAD_CAST "xfade_samples", BAD_CAST buf);
        }

        /* sample note */
        node2 = xmlNewTextChild(node1, NULL, BAD_CAST "Note", NULL);

        snprintf(buf, CHARBUFSIZE, "%d", patch_get_root_note(patch_id[i]));
        xmlNewProp(node2,   BAD_CAST "root", BAD_CAST buf);

        snprintf(buf, CHARBUFSIZE, "%d", patch_get_lower_note(patch_id[i]));
        xmlNewProp(node2,   BAD_CAST "lower", BAD_CAST buf);

        snprintf(buf, CHARBUFSIZE, "%d", patch_get_upper_note(patch_id[i]));
        xmlNewProp(node2,   BAD_CAST "upper", BAD_CAST buf);

        snprintf(buf, CHARBUFSIZE, "%d", patch_get_lower_vel(patch_id[i]));
        xmlNewProp(node2,   BAD_CAST "velocity_lower", BAD_CAST buf);

        snprintf(buf, CHARBUFSIZE, "%d", patch_get_upper_vel(patch_id[i]));
        xmlNewProp(node2,   BAD_CAST "velocity_upper", BAD_CAST buf);


        /*  ------------------------
            amplitude
         */
        dish_file_write_param(nodepatch, patch_id[i],PATCH_PARAM_AMPLITUDE);

        /*  ------------------------
            panning
         */
        dish_file_write_param(nodepatch, patch_id[i], PATCH_PARAM_PANNING);

        /*  ------------------------
            pitch
         */
        dish_file_write_param(nodepatch, patch_id[i], PATCH_PARAM_PITCH);

        /*  ------------------------
            lowpass
         */
        node1 = xmlNewTextChild(nodepatch, NULL, BAD_CAST "Lowpass", NULL);
        dish_file_write_param(node1, patch_id[i], PATCH_PARAM_CUTOFF);
        dish_file_write_param(node1, patch_id[i], PATCH_PARAM_RESONANCE);

        /*  ------------------------
            voice
         */
        node1 = xmlNewTextChild(nodepatch, NULL, BAD_CAST "Voice", NULL);

        /* voice cut */
        snprintf(buf, CHARBUFSIZE, "%d", patch_get_cut(patch_id[i]));
        xmlNewProp(node1,   BAD_CAST "cut", BAD_CAST buf);

        /* voice cut by */
        snprintf(buf, CHARBUFSIZE, "%d", patch_get_cut_by(patch_id[i]));
        xmlNewProp(node1,   BAD_CAST "cut_by", BAD_CAST buf);

        /* voice portamento */
        dish_file_write_bool(node1, patch_id[i], PATCH_BOOL_PORTAMENTO);

        /* voice portamento_time */
        dish_file_write_float(node1, patch_id[i],
                                            PATCH_FLOAT_PORTAMENTO_TIME);

        /* voice monophonic */
        xmlNewProp(node1,   BAD_CAST "monophonic",
                            BAD_CAST (patch_get_monophonic(patch_id[i])
                                        ? "true"
                                        : "false"));

        /* voice legato */
        dish_file_write_bool(node1, patch_id[i], PATCH_BOOL_LEGATO);

        /*  ------------------------
            envelopes
         */
        for (j = 0; j < VOICE_MAX_ENVS; ++j)
            dish_file_write_eg(nodepatch,   patch_id[i],
                                            MOD_SRC_EG + j);

        /*  ------------------------
            lfos
         */
        for (j = 0; j < VOICE_MAX_LFOS; ++j)
            dish_file_write_lfo(nodepatch,  patch_id[i],
                                            MOD_SRC_VLFO + j);

        for (j = 0; j < PATCH_MAX_LFOS; ++j)
            dish_file_write_lfo(nodepatch,  patch_id[i],
                                            MOD_SRC_GLFO + j);
    }

    rc = xmlSaveFormatFile(dish_data->file_path, doc, 1);
    xmlFreeDoc(doc);

    msg_log(MSG_MESSAGE, "Successfully wrote dish file '%s'\n",
                                                dish_data->file_path);

    free(samples_dir);
    free(patch_id);

    setlocale(LC_NUMERIC, "");
    return rc;
}


static bool xmlstr_to_bool(xmlChar* str)
{
    if (xmlStrcasecmp(str, BAD_CAST "true") == 0
     || xmlStrcasecmp(str, BAD_CAST "on") == 0
     || xmlStrcasecmp(str, BAD_CAST "yes") == 0)
    {
        xmlFree(str);
        return true;
    }

    xmlFree(str);
    return false;
}


int get_prop_int(xmlNodePtr node, const char* name, int* value)
{
    int ret = 0;
    xmlChar* prop = xmlGetProp(node, BAD_CAST name);

    if (prop)
    {
        ret = sscanf((const char*)prop, "%d", value);
        xmlFree(prop);
    }

    return ret;
}

int get_prop_float(xmlNodePtr node, const char* name, float* value)
{
    int ret = 0;
    int int_val;
    xmlChar* prop = xmlGetProp(node, BAD_CAST name);

    if (prop)
    {
        ret = sscanf((const char*)prop, "%f", value);
        sscanf((const char*)prop, "%d", &int_val);

        if ((float)int_val == *value)
        {
            setlocale(LC_NUMERIC, "");
            ret = sscanf((const char*)prop, "%f", value);
            setlocale(LC_NUMERIC, "C");
        }
        xmlFree(prop);
    }

    return ret;
}


int get_prop_mod_src(xmlNodePtr node, const char* name, int* mod_id)
{
    xmlChar* prop = xmlGetProp(node, BAD_CAST name);

    if (!prop)
        return 0;

    *mod_id = mod_src_id((const char*)prop, MOD_SRC_ALL);
    xmlFree(prop);
    return 1;
}


static int dish_file_read_sample(xmlNodePtr node,  int patch_id)
{
    xmlChar* prop;
    xmlNodePtr node1;
    char* filename = 0;
    bool sample_loaded = false;

    int play_start = -1;
    int play_stop = -1;
    int loop_start = -1;
    int loop_stop = -1;
    int fade_samples = -1;
    int xfade_samples = -1;
    int mode = PATCH_PLAY_SINGLESHOT;
    double sr_ratio = 1.0;

    if (dish_data->samplerate
     && dish_data->samplerate != patch_get_samplerate())
        sr_ratio = patch_get_samplerate() / (double)dish_data->samplerate;

    prop = xmlGetProp(node, BAD_CAST "file");

    if (prop && strlen((const char*)prop) > 0)
    {
        const char* p = (const char*)prop;
        bool default_sample = (strcmp(p, "Default") == 0);

        filename = (*p == '/' || default_sample)
                            ? strdup(p)
                            : file_ops_join_path(dish_data->bank_dir, p);

        if (dish_data->bank_dir && !default_sample)
        {
            char* tmp = file_ops_read_link(filename);

            if (tmp)
            {
                free(filename);
                filename = tmp;
            }
        }
    }

    if (prop)
        xmlFree(prop);

    if ((prop = xmlGetProp(node, BAD_CAST "mode")))
    {
        if (xmlStrcmp(prop, BAD_CAST "singleshot") == 0)
            mode = PATCH_PLAY_SINGLESHOT;
        else if (xmlStrcmp(prop, BAD_CAST "trim") == 0)
            mode = PATCH_PLAY_TRIM;
        else if (xmlStrcmp(prop, BAD_CAST "loop") == 0)
            mode = PATCH_PLAY_LOOP;
        else if (xmlStrcmp(prop, BAD_CAST "pingpong") == 0)
            mode = PATCH_PLAY_LOOP | PATCH_PLAY_PINGPONG;
        else
        {
            msg_log(MSG_ERROR, "Invalid sample play mode:%s\n", prop);
        }

        xmlFree(prop);
    }

    if ((prop = xmlGetProp(node, BAD_CAST "reverse"))
     && xmlstr_to_bool(prop))
    {
        mode |= PATCH_PLAY_REVERSE;
    }

    if ((prop = xmlGetProp(node, BAD_CAST "to_end"))
     && xmlstr_to_bool(prop))
    {
        mode |= PATCH_PLAY_TO_END;
    }

    patch_set_play_mode(patch_id, mode);

    for(node1 = node->children;
        node1 != NULL;
        node1 = node1->next)
    {
        int n;

        if (node1->type != XML_ELEMENT_NODE)
            continue;

        if (!sample_loaded && filename)
        {
            int raw_samplerate = 0;
            int raw_channels = 0;
            int sndfile_format = 0;

            if (xmlStrcmp(node1->name, BAD_CAST "Raw") == 0)
            {
                if (get_prop_int(node1, "samplerate", &n))
                        raw_samplerate = n;

                if (get_prop_int(node1, "channels", &n))
                        raw_channels = n;

                if (get_prop_int(node1, "sndfile_format", &n))
                        sndfile_format = n;
            }

            if (patch_sample_load(patch_id, filename,
                                            raw_samplerate,
                                            raw_channels,
                                            sndfile_format,
                                            false) < 0)
            {
                msg_log(MSG_ERROR, "failed to load sample: %s error (%s)\n",
                    filename, pf_error_str(pf_error_get()));
            }
            else
            {
                msg_log(MSG_MESSAGE, "loaded sample %s into patch %d\n",
                                     filename, patch_id);
                sample_loaded = true;
            }

            free(filename);
            filename = 0;
        }

        if (xmlStrcmp(node1->name, BAD_CAST "Play") == 0)
        {
            if (get_prop_int(node1, "start", &n))
                play_start = n * sr_ratio;

            if (get_prop_int(node1, "stop", &n))
                play_stop = n * sr_ratio;

            if (get_prop_int(node1, "fade_samples", &n))
                fade_samples = n * sr_ratio;
        }
        else if (xmlStrcmp(node1->name, BAD_CAST "Loop") == 0)
        {
            if (get_prop_int(node1, "start", &n))
                loop_start = n * sr_ratio;

            if (get_prop_int(node1, "stop", &n))
                loop_stop = n * sr_ratio;

            if (get_prop_int(node1, "xfade_samples", &n))
                xfade_samples = n * sr_ratio;
        }
        else if (xmlStrcmp(node1->name, BAD_CAST "Note") == 0)
        {
            int lower = 60, root = 60, upper = 60;
            int lower_vel = 0;
            int upper_vel = 127;

            get_prop_int(node1, "root", &root);
            get_prop_int(node1, "lower", &lower);
            get_prop_int(node1, "upper", &upper);
            get_prop_int(node1, "velocity_lower", &lower_vel);
            get_prop_int(node1, "velocity_upper", &upper_vel);

            if (lower > upper)
            {
                int tmp = lower;
                lower = upper;
                upper = tmp;
                msg_log(MSG_WARNING, "swapped lower/upper note values\n");
            }

            if (root < lower)
            {
                msg_log(MSG_WARNING,"adjusting root note to lower value\n");
                root = lower;
            }

            if (root > upper)
            {
                msg_log(MSG_WARNING,"adjusting root note to upper value\n");
                root = upper;
            }

            patch_set_root_note(patch_id, root);
            patch_set_lower_note(patch_id, lower);
            patch_set_upper_note(patch_id, upper);
            patch_set_lower_vel(patch_id, lower_vel);
            patch_set_upper_vel(patch_id, upper_vel);
        }
        else
        {
            msg_log(MSG_WARNING, "ignoring XML NODE: %s\n",
                                    (const char*)node1->name);
        }
    }

    if (!sample_loaded)
    {
        free(filename);
        return 0;
    }

    if (sanitize_sample_points( &play_start,    &play_stop,
                                &loop_start,    &loop_stop,
                                &fade_samples,  &xfade_samples,
                    patch_get_mark_frame(patch_id, WF_MARK_STOP)) < 0)
    {
        /* it's all so borked that sanity becomes impossible */
        return -1;
    }

    patch_sample_set_points(patch_id,   play_start,     play_stop,
                                        loop_start,     loop_stop,
                                        fade_samples,   xfade_samples);
    return 0;
}


static int dish_file_read_eg(xmlNodePtr node, int patch_id)
{
    int eg_id = 0;
    xmlChar* prop;
    float n;

    if ((eg_id = mod_src_id((const char*)node->name, MOD_SRC_EG)) < 0)
        return -1;

    if ((prop = xmlGetProp(node, BAD_CAST "active")))
        patch_set_env_active(patch_id, eg_id, xmlstr_to_bool(prop));

    if (get_prop_float(node, "delay", &n))
        patch_set_env_delay(patch_id, eg_id, n);

    if (get_prop_float(node, "attack", &n))
        patch_set_env_attack(patch_id, eg_id, n);

    if (get_prop_float(node, "hold", &n))
        patch_set_env_hold(patch_id, eg_id, n);

    if (get_prop_float(node, "decay", &n))
        patch_set_env_decay(patch_id, eg_id, n);

    if (get_prop_float(node, "sustain", &n))
        patch_set_env_sustain(patch_id, eg_id, n);

    if (get_prop_float(node, "release", &n))
        patch_set_env_release(patch_id, eg_id, n);

    if (get_prop_float(node, "key_tracking", &n))
        patch_set_env_key_amt(patch_id, eg_id, n);

    return 0;
}

static int
dish_file_read_lfo_freq_data(xmlNodePtr node, int patch_id, int lfo_id)
{
    xmlNodePtr node1;
    xmlChar* prop;
    float n;
    int mod;

    if (get_prop_float(node, "hrtz", &n))
        patch_set_lfo_freq(patch_id, lfo_id, n);

    if (get_prop_float(node, "beats", &n))
        patch_set_lfo_sync_beats(patch_id, lfo_id, n);

    if ((prop = xmlGetProp(node, BAD_CAST "sync")))
        patch_set_lfo_sync(patch_id, lfo_id, xmlstr_to_bool(prop));

    for (   node1 = node->children;
            node1 != NULL;
            node1 = node1->next)
    {
        if (node1->type != XML_ELEMENT_NODE)
            continue;

        if (xmlStrcmp(node1->name, BAD_CAST "Mod1") == 0)
        {
            if (get_prop_mod_src(node1, "source", &mod))
                patch_set_lfo_fm1_src(patch_id, lfo_id, mod);

            if (get_prop_float(node1, "amount", &n))
                patch_set_lfo_fm1_amt(patch_id, lfo_id, n);
        }
        else if (xmlStrcmp(node1->name, BAD_CAST "Mod2") == 0)
        {
            if (get_prop_mod_src(node1, "source", &mod))
                patch_set_lfo_fm2_src(patch_id, lfo_id, mod);

            if (get_prop_float(node1, "amount", &n))
                patch_set_lfo_fm2_amt(patch_id, lfo_id, n);
        }
        else
        {
            msg_log(MSG_WARNING, "ignoring XML NODE: %s\n",
                                    (const char*)node1->name);
        }
    }

    return 0;
}


static int
dish_file_read_lfo_amp_data(xmlNodePtr node, int patch_id, int lfo_id)
{
    xmlNodePtr node1;
    xmlChar* prop;
    float n;
    int mod;

    if ((prop = xmlGetProp(node, BAD_CAST "shape")))
    {
        patch_set_lfo_shape(patch_id, lfo_id,
            (LFOShape)names_lfo_shapes_id_from_str((const char*)prop));
        xmlFree(prop);
    }

    if ((prop = xmlGetProp(node, BAD_CAST "positive")))
        patch_set_lfo_positive(patch_id, lfo_id, xmlstr_to_bool(prop));

    if (!mod_src_is_global(lfo_id)) /* already know it IS an LFO id */
    {
        if (get_prop_float(node, "delay", &n))
            patch_set_lfo_delay(patch_id, lfo_id, n);

        if (get_prop_float(node, "attack", &n))
            patch_set_lfo_attack(patch_id, lfo_id, n);
    }

    for (   node1 = node->children;
            node1 != NULL;
            node1 = node1->next)
    {
        if (node1->type != XML_ELEMENT_NODE)
            continue;

        if (xmlStrcmp(node1->name, BAD_CAST "Mod1") == 0)
        {
            if (get_prop_mod_src(node1, "source", &mod))
                patch_set_lfo_am1_src(patch_id, lfo_id, mod);

            if (get_prop_float(node1, "amount", &n))
                patch_set_lfo_am1_amt(patch_id, lfo_id, n);
        }
        else if (xmlStrcmp(node1->name, BAD_CAST "Mod2") == 0)
        {
            if (get_prop_mod_src(node1, "source", &mod))
                patch_set_lfo_am2_src(patch_id, lfo_id, mod);

            if (get_prop_float(node1, "amount", &n))
                patch_set_lfo_am2_amt(patch_id, lfo_id, n);
        }
        else
        {
            msg_log(MSG_WARNING, "ignoring XML NODE: %s\n",
                                    (const char*)node1->name);
        }
    }

    return 0;
}


static int
dish_file_read_lfo(xmlNodePtr node, int patch_id)
{
    int lfo_id;
    xmlChar* prop;
    xmlNodePtr node1;

    lfo_id = mod_src_id((const char*)node->name, MOD_SRC_LFOS);

    if (lfo_id < 0)
    {
        msg_log(MSG_ERROR, "invalid LFO:%s\n", (const char*)node->name);
        return -1;
    }

    if ((prop = xmlGetProp(node, BAD_CAST "active")))
        patch_set_lfo_active(patch_id, lfo_id, xmlstr_to_bool(prop));

    for (   node1 = node->children;
            node1 != NULL;
            node1 = node1->next)
    {
        if (node1->type != XML_ELEMENT_NODE)
            continue;

        if (xmlStrcmp(node1->name, BAD_CAST "Frequency") == 0)
        {
            dish_file_read_lfo_freq_data(node1, patch_id, lfo_id);
        }
        else if (xmlStrcmp(node1->name, BAD_CAST "Amplitude") == 0)
        {
            dish_file_read_lfo_amp_data(node1, patch_id, lfo_id);
        }
        else
        {
            msg_log(MSG_WARNING, "ignoring XML NODE: %s\n",
                                    (const char*)node1->name);
        }
    }

    return 0;
}

static int
dish_file_read_param(xmlNodePtr node, int patch_id, PatchParamType param)
{
    const char* pname = 0;
    float n;
    int s;
    xmlNodePtr node1;

    switch(param)
    {
    case PATCH_PARAM_AMPLITUDE: pname = "level";    break;
    case PATCH_PARAM_PANNING:   pname = "position"; break;
    case PATCH_PARAM_CUTOFF:    pname = "value";    break;
    case PATCH_PARAM_RESONANCE: pname = "amount";   break;
    case PATCH_PARAM_PITCH:     pname = "tuning";   break;
    default:    /* shouldn't ever get here if this is the case ;-} */
        return -1;
    }

    if (get_prop_float(node, pname, &n))
        patch_param_set_value(patch_id, param, n);

    if (param == PATCH_PARAM_PITCH)
    {
        if (get_prop_int(node, "tuning_range", &s))
            patch_set_pitch_steps(patch_id, s);
    }

    if (get_prop_float(node, "velocity_sensing", &n))
        patch_param_set_vel_amount(patch_id, param, n);

    if (get_prop_float(node, "key_tracking", &n))
        patch_param_set_key_amount(patch_id, param, n);

    for (   node1 = node->children;
            node1 != NULL;
            node1 = node1->next)
    {
        if (node1->type != XML_ELEMENT_NODE)
            continue;

        int slot = -1;
        int mod;

        if (sscanf((const char*)node1->name, "Mod%d", &slot) == 1
            && slot > 0 && slot <= MAX_MOD_SLOTS)
        {
            --slot; /* slot 0 is named as MOD1 */

            if (get_prop_mod_src(node1, "source", &mod))
                patch_param_set_mod_src(patch_id, param, slot, mod);

            if (get_prop_float(node1, "amount", &n))
                patch_param_set_mod_amt(patch_id, param, slot, n);
        }
        else if ((param == PATCH_PARAM_AMPLITUDE
                && xmlStrcmp(node1->name, BAD_CAST "Env") == 0))
        {
            if (get_prop_mod_src(node1, "source", &mod))
                patch_param_set_mod_src(patch_id, param, EG_MOD_SLOT, mod);
        }
        else
        {
            msg_log(MSG_WARNING, "ignoring XML NODE: %s\n",
                                    (const char*)node1->name);
        }
    }

    return 0;
}


static int
dish_file_read_bool(xmlNodePtr node, int patch_id, PatchBoolType bool_type)
{
    float       n;
    xmlChar*    prop;
    xmlNodePtr  node1;

    if ((prop = xmlGetProp(node, BAD_CAST "active")))
        patch_bool_set_active(patch_id, bool_type, xmlstr_to_bool(prop));

    for (   node1 = node->children;
            node1 != NULL;
            node1 = node1->next)
    {
        if (node1->type != XML_ELEMENT_NODE)
            continue;

        if (xmlStrcmp(node1->name, BAD_CAST "Mod") == 0)
        {   /*  don't use get_prop_mod_src as it doesn't do masking */
            if ((prop = xmlGetProp(node1, BAD_CAST "source")))
            {
                patch_bool_set_mod_src(patch_id, bool_type,
                            mod_src_id((const char*)prop, MOD_SRC_GLOBALS));
                xmlFree(prop);
            }

            if (get_prop_float(node1, "threshold", &n))
                patch_bool_set_thresh(patch_id, bool_type, n);
        }
        else
        {
            msg_log(MSG_WARNING, "ignoring XML NODE: %s\n",
                                    (const char*)node1->name);
        }
    }

    return 0;
}



static int
dish_file_read_float(xmlNodePtr node, int patch_id,
                                      PatchFloatType float_type)
{
    float       n;
    xmlChar*    prop;
    xmlNodePtr  node1;

    if ((prop = xmlGetProp(node, BAD_CAST "active")))
    /* FIXME: needs value bounds testing */
        patch_float_set_value(patch_id, float_type, xmlstr_to_bool(prop));

    for (   node1 = node->children;
            node1 != NULL;
            node1 = node1->next)
    {
        if (node1->type != XML_ELEMENT_NODE)
            continue;

        if (xmlStrcmp(node1->name, BAD_CAST "Mod") == 0)
        {   /*  don't use get_prop_mod_src as it doesn't do masking */
            if ((prop = xmlGetProp(node1, BAD_CAST "source")))
            {
                patch_float_set_mod_src(patch_id, float_type,
                            mod_src_id((const char*)prop, MOD_SRC_GLOBALS));
                xmlFree(prop);
            }

            if (get_prop_float(node1, "amount", &n))
                patch_float_set_mod_amt(patch_id, float_type, n);
        }
        else
        {
            msg_log(MSG_WARNING, "ignoring XML NODE: %s\n",
                                    (const char*)node1->name);
        }
    }

    return 0;
}


static int dish_file_read_voice(xmlNodePtr node, int patch_id)
{
    xmlChar*    prop;
    xmlNodePtr  node1;

    int i;

    if (get_prop_int(node, "cut", &i))
        patch_set_cut(patch_id, i);

    if (get_prop_int(node, "cut_by", &i))
        patch_set_cut_by(patch_id, i);

    if ((prop = xmlGetProp(node, BAD_CAST "monophonic")))
        patch_set_monophonic(patch_id, xmlstr_to_bool(prop));

    for (   node1 = node->children;
            node1 != NULL;
            node1 = node1->next)
    {
        if (node1->type != XML_ELEMENT_NODE)
            continue;

        if (xmlStrcmp(node1->name, BAD_CAST "Portamento") == 0)
        {
            dish_file_read_bool(node1, patch_id, PATCH_BOOL_PORTAMENTO);
        }
        else if (xmlStrcmp(node1->name, BAD_CAST "Portamento_time") == 0)
        {
            dish_file_read_float(node1, patch_id,
                                            PATCH_FLOAT_PORTAMENTO_TIME);
        }
        else if (xmlStrcmp(node1->name, BAD_CAST "Legato") == 0)
        {
            dish_file_read_bool(node1, patch_id, PATCH_BOOL_LEGATO);
        }
        else
        {
            msg_log(MSG_WARNING, "ignoring XML NODE: %s\n",
                                    (const char*)node1->name);
        }
    }

    return 0;
}


static int dish_read(const char *path)
{
    struct stat st;
    xmlDocPtr   doc;
    xmlNodePtr  noderoot;
    xmlNodePtr  nodepatch;
    xmlNodePtr  node1;
    xmlNodePtr  node2;
    xmlChar*    prop;
    float   n;
    int i;
    bool full_save = false;

    setlocale(LC_NUMERIC, "C");

    debug("Loading bank from file %s\n", path);

    if (stat(path, &st) != 0)
    {
        msg_log(MSG_ERROR, "file '%s' does not exist\n", path);
        setlocale(LC_NUMERIC, "");
        return -1;
    }

    doc = xmlParseFile(path);

    if (doc == NULL)
    {
        msg_log(MSG_ERROR, "Failed to parse %s\n", path);
        setlocale(LC_NUMERIC, "");
        return -1;
    }

    noderoot = xmlDocGetRootElement(doc);

    if (noderoot == NULL)
    {
        msg_log(MSG_ERROR, "%s is empty\n", path);
        xmlFreeDoc(doc);
        setlocale(LC_NUMERIC, "");
        return -1;
    }

    if (xmlStrcmp(noderoot->name, BAD_CAST "Petri-Foo-Dish") != 0)
    {
        msg_log(MSG_ERROR, "%s is not a valid 'Petri-Foo-Dish' file\n",
                                                                path);
        xmlFreeDoc(doc);
        setlocale(LC_NUMERIC, "");
        return -1;
    }

    if ((prop = xmlGetProp(noderoot, BAD_CAST "save-type")))
    {
        if (xmlStrcmp(prop, BAD_CAST "full") == 0)
            full_save = true;
        else if (xmlStrcmp(prop, BAD_CAST "basic") != 0)
            msg_log(MSG_WARNING, "Unknown save-type '%s'\n", prop);
        xmlFree(prop);
    }

    if (dish_file_state_set_by_path(path, full_save) == -1)
    {
        setlocale(LC_NUMERIC, "");
        return -1;
    }

    dish_data->samplerate = 0;

    for (node1 = noderoot->children;
         node1 != NULL;
         node1 = node1->next)
    {
        if (node1->type != XML_ELEMENT_NODE)
            continue;

        if (xmlStrcmp(node1->name, BAD_CAST "Master") == 0)
        {
            if (get_prop_float(node1, "level", &n))
                mixer_set_amplitude(n);

            if (get_prop_int(node1, "samplerate", &i))
                dish_data->samplerate = i;
        }
        else if (xmlStrcmp(node1->name, BAD_CAST "Patch") == 0)
        {
            int patch_id = patch_create();

            if (patch_id < 0)
                continue;

            nodepatch = node1;

            /* patch name */
            if ((prop = xmlGetProp(nodepatch, BAD_CAST "name")))
            {
                patch_set_name(patch_id, (const char*)prop);
                xmlFree(prop);
            }

            if (get_prop_int(nodepatch, "channel", &i))
                patch_set_channel(patch_id, i);

            msg_log(MSG_MESSAGE, "Reading data for patch %d '%s'\n",
                                 patch_id, patch_get_name(patch_id));

            for (node2 = nodepatch->children;
                 node2 != NULL;
                 node2 = node2->next)
            {
                if (node2->type != XML_ELEMENT_NODE)
                    continue;

                if (xmlStrcmp(node2->name, BAD_CAST "Sample") == 0)
                {
                    dish_file_read_sample(node2, patch_id);
                }
                else if (xmlStrcmp(node2->name, BAD_CAST "Amplitude") ==0)
                {
                    dish_file_read_param(node2, patch_id,
                                                PATCH_PARAM_AMPLITUDE);
                }
                else if (xmlStrcmp(node2->name, BAD_CAST "Pan") ==0)
                {
                    dish_file_read_param(node2, patch_id,
                                                PATCH_PARAM_PANNING);
                }
                else if (xmlStrcmp(node2->name, BAD_CAST "Pitch") == 0)
                {
                    dish_file_read_param(node2, patch_id,
                                                PATCH_PARAM_PITCH);
                }
                else if (xmlStrcmp(node2->name, BAD_CAST "Lowpass") == 0)
                {
                    xmlNodePtr node3;

                    for (node3 = node2->children;
                         node3 != NULL;
                         node3 = node3->next)
                    {
                        if (node3->type != XML_ELEMENT_NODE)
                            continue;

                        if (xmlStrcmp(node3->name, BAD_CAST "Cutoff") == 0)
                        {
                            dish_file_read_param(node3, patch_id,
                                                    PATCH_PARAM_CUTOFF);
                        }
                        else if (xmlStrcmp(node3->name,
                                           BAD_CAST "Resonance") == 0)
                        {
                            dish_file_read_param(node3, patch_id,
                                                    PATCH_PARAM_RESONANCE);
                        }
                    }
                }
                else if (xmlStrcmp(node2->name, BAD_CAST "Voice") == 0)
                {
                    dish_file_read_voice(node2, patch_id);
                }
                else
                {
                    if (mod_src_maybe_eg((const char*)node2->name))
                        dish_file_read_eg(node2, patch_id);
                    else if (mod_src_maybe_lfo((const char*)node2->name))
                        dish_file_read_lfo(node2, patch_id);
                    else
                    {
                        msg_log(MSG_WARNING, "ignoring XML NODE: %s\n",
                                            (const char*)node2->name);
                    }
                }
            }
        }
        else
        {
            msg_log(MSG_WARNING, "ignoring XML NODE: %s\n",
                                    (const char*)node1->name);
        }
    }

    xmlFreeDoc(doc);

    msg_log(MSG_MESSAGE, "Successfully read dish file '%s'\n", path);

    setlocale(LC_NUMERIC, "");

    return 0;
}


#define DF_STATE_FREE(dfdata)   \
    if ((dfdata)->bank_dir)  free((dfdata)->bank_dir);   \
    if ((dfdata)->parent_dir)free((dfdata)->parent_dir); \
    if ((dfdata)->bank_name) free((dfdata)->bank_name);  \
    if ((dfdata)->file_path) free((dfdata)->file_path)

#define DF_STATE_ZERO(dfdata)   \
    (dfdata)->samplerate = 0;   \
    (dfdata)->full_save = false;\
    (dfdata)->bank_dir = 0;     \
    (dfdata)->parent_dir = 0;   \
    (dfdata)->bank_name = 0;    \
    (dfdata)->file_path = 0


void dish_file_state_init(void)
{
    if (dish_data)
        return;

    dish_data = malloc(sizeof(*dish_data));

    DF_STATE_ZERO(dish_data);
}


void dish_file_state_reset(void)
{
    DF_STATE_FREE(dish_data);
    DF_STATE_ZERO(dish_data);
}


void dish_file_state_cleanup(void)
{
    debug("cleanup\n");

    if (!dish_data)
        return;

    DF_STATE_FREE(dish_data);
    free(dish_data);
    dish_data = 0;
}


int dish_file_state_set_by_path(const char* file_path, bool full_save)
{
    if (dish_data->full_save != full_save
     || !dish_data->file_path
     || strcmp(file_path, dish_data->file_path) != 0)
    {
        DF_STATE_FREE(dish_data);
        DF_STATE_ZERO(dish_data);

        char* name = 0;
        char* filename;
        char* bank_dir;
        char* parent_dir = 0;

        debug("path:        '%s'\n", file_path);

        if (file_ops_split_path(file_path, &bank_dir, &filename) == -1)
        {
            debug("failed bank_dir/filename split\n");
            goto fail;
        }

        debug("bank_dir:    '%s'\n", bank_dir);
        debug("filename:    '%s'\n", filename);

        if (!(parent_dir = file_ops_parent_dir(bank_dir)))
        {
            debug("failed parent_dir split\n");
            goto fail;
        }

        debug("parent_dir:  '%s'\n", parent_dir);

        if (file_ops_split_file(filename, &name, 0) == -1)
        {
            /* no extension so filename and name the same */
            name = filename;
        }
        else
        {
            free(filename);
        }

        debug("name:        '%s'\n", name);

        dish_data->bank_dir = bank_dir;
        dish_data->parent_dir = parent_dir;
        dish_data->bank_name = name;
        dish_data->file_path = strdup(file_path);
        dish_data->full_save = full_save;

        return 0;

    fail:
        free(filename);
        free(name);
        free(parent_dir);
        free(bank_dir);
        msg_log(MSG_ERROR, "bad dish file path '%s'\n", file_path);

        return -1;
    }

    return 0;
}


bool dish_file_has_state(void)
{
    return dish_data->file_path != 0;
}

bool dish_file_state_is_full(void)
{
    debug("dish_data->full_save == '%s'\n",
        dish_data->full_save ? "true" : "false");
    return dish_data->full_save;
}

bool dish_file_state_is_basic(void)
{
    debug("dish_data->basic_save == '%s'\n",
        !dish_data->full_save ? "true" : "false");
    return !dish_data->full_save;
}


const char* dish_file_state_parent_dir(void)
{
    return dish_data->parent_dir;
}

const char* dish_file_state_bank_dir(void)
{
    return dish_data->bank_dir;
}


const char* dish_file_state_bank_name(void)
{
    return dish_data->bank_name;
}


const char* dish_file_state_path(void)
{
    return dish_data->file_path;
}


int dish_file_read(const char* path)
{
    patch_destroy_all();
    return dish_read(path);
}


int dish_file_import(const char* path)
{
    int ret;
    dish_file_data* tmp = dish_data;
    dish_data = 0;
    dish_file_state_init();
    ret = dish_read(path);
    dish_file_state_cleanup();
    dish_data = tmp;
    return ret;
}


int dish_file_export(const char* path)
{
    int ret;
    dish_file_data* tmp = dish_data;
    dish_data = 0;
    dish_file_state_init();
    dish_file_state_set_by_path(path, false);
    ret = dish_write();
    dish_file_state_cleanup();
    dish_data = tmp;
    return ret;
}


int dish_file_write_basic(const char *path)
{
    dish_file_state_set_by_path(path, false);
    return dish_write();
}


int dish_file_write_full(const char* parent, const char* name)
{
    char* bank_dir = 0;
    char* filename = 0;
    char* bank_path = 0;

    if (!(bank_dir = file_ops_join_path(parent, name)))
    {
        msg_log(MSG_ERROR, "full-save path error\n");
        return -1;
    }

    if (mkdir(bank_dir, 0777) != 0 && errno != EEXIST)
    {
        msg_log(MSG_ERROR, "failed to create bank dir:'%s'\n",
                bank_dir);
        free(bank_dir);
        return -1;
    }

    if (!(filename = file_ops_join_ext(name, dish_file_extension())))
    {
        free(bank_dir);
        msg_log(MSG_ERROR, "full-save filename error\n");
        return -1;
    }

    if (!(bank_path = file_ops_join_path(bank_dir, filename)))
    {
        free(bank_dir);
        free(filename);
        msg_log(MSG_ERROR, "full-save bank error\n");
        return -1;
    }

    if (!dish_data->full_save
     || strcmp(dish_data->bank_dir, bank_dir) != 0)
    {
        /* either the last save was basic, or the bank_dir has changed */
        DF_STATE_FREE(dish_data);
        dish_data->bank_dir = strdup(bank_dir);
        dish_data->parent_dir = strdup(parent);
        dish_data->bank_name = strdup(name);
        dish_data->file_path = strdup(bank_path);
    }

    dish_data->full_save = true;
    free(bank_path);
    free(bank_dir);
    free(filename);

    return dish_write();
}


int dish_file_write(void)
{
    return dish_write();
}

