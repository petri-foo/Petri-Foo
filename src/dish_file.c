#include "dish_file.h"

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <libxml/parser.h>


#include "mixer.h"
#include "mod_src.h"
#include "petri-foo.h"
#include "patch.h"
#include "patch_util.h"
#include "patch_set_and_get.h"
#include "sample.h"


#define BUFSIZE 256

static const char* dish_file_ext = ".petri-foo";


const char* dish_file_extension(void)
{
    return dish_file_ext;
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
    char buf[BUFSIZE];

    if (!(s->raw_samplerate || s->raw_channels || s->sndfile_format))
        return 0;

    node = xmlNewTextChild(nodeparent, NULL, BAD_CAST "Raw", NULL);

    snprintf(buf, BUFSIZE, "%d", s->raw_samplerate);
    xmlNewProp(node, BAD_CAST "samplerate", BAD_CAST buf);

    snprintf(buf, BUFSIZE, "%d", s->raw_channels);
    xmlNewProp(node, BAD_CAST "channels", BAD_CAST buf);

    snprintf(buf, BUFSIZE, "%d", s->sndfile_format);
    xmlNewProp(node, BAD_CAST "sndfile_format", BAD_CAST buf);

    return 0;
}


static int
dish_file_write_param(xmlNodePtr nodeparent, int patch_id,
                                                PatchParamType param)
{
    xmlNodePtr  node1;
    xmlNodePtr  node2;
    char buf[BUFSIZE];
    const char** param_names;

    const char* prop1 = 0;
    const char* prop2 = 0;

    float   val1;
    float   val2;
    float   vel_amt;
    int     modsrc;
    float   modamt;
    float   velsens;
    float   keytrack;

    int     last_mod_slot = MAX_MOD_SLOTS;
    int     i;

    if (patch_param_get_value(patch_id, param, &val1)
                                                == PATCH_PARAM_INVALID)
        return -1;

    param_names = names_params_get();

    patch_get_vel_amount(patch_id, param, &vel_amt);

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

    snprintf(buf, BUFSIZE, "%f", val1);
    xmlNewProp(node1, BAD_CAST prop1, BAD_CAST buf);

    if (prop2)
    {
        snprintf(buf, BUFSIZE, "%f", val2);
        xmlNewProp(node1, BAD_CAST prop2, BAD_CAST buf);
    }

    /* velocity sensing */
    patch_get_vel_amount(patch_id, param, &velsens);
    snprintf(buf, BUFSIZE, "%f", velsens);
    xmlNewProp(node1, BAD_CAST "velocity_sensing", BAD_CAST buf);

    /* keyboard tracking */
    patch_get_key_amount(patch_id, param, &keytrack);
    snprintf(buf, BUFSIZE, "%f", keytrack);
    xmlNewProp(node1, BAD_CAST "key_tracking", BAD_CAST buf);

    for (i = 0; i < last_mod_slot; ++i)
    {
        snprintf(buf, BUFSIZE, "Mod%d", i + 1);

        patch_get_mod_src(patch_id, param, i, &modsrc);
        patch_get_mod_amt(patch_id, param, i, &modamt);

        node2 = xmlNewTextChild(node1, NULL, BAD_CAST buf, NULL);
        xmlNewProp(node2, BAD_CAST "source", BAD_CAST mod_src_name(modsrc));
        snprintf(buf, BUFSIZE, "%f", modamt);
        xmlNewProp(node2, BAD_CAST "amount", BAD_CAST buf);
    }

    if (param == PATCH_PARAM_AMPLITUDE)
    {
        patch_get_mod_src(patch_id, PATCH_PARAM_AMPLITUDE,
                                    EG_MOD_SLOT, &modsrc);
        node2 = xmlNewTextChild(node1, NULL, BAD_CAST "Env", NULL);
        xmlNewProp(node2, BAD_CAST "source", BAD_CAST mod_src_name(modsrc));
    }

    return 0;
}


static int
dish_file_write_eg(xmlNodePtr nodeparent, int patch_id, int eg_id)
{
    xmlNodePtr  node1;
    char buf[BUFSIZE];
    bool active;
    float val;

    if (patch_get_env_on(patch_id, eg_id, &active) == -1)
        return -1;

    node1 = xmlNewTextChild(nodeparent, NULL,
                            BAD_CAST mod_src_name(eg_id), NULL);

    xmlNewProp(node1,   BAD_CAST "active",
                        BAD_CAST (active ? "true" : "false"));

    patch_get_env_delay(patch_id, eg_id, &val);
    snprintf(buf, BUFSIZE, "%f", val);
    xmlNewProp(node1,   BAD_CAST "delay",   BAD_CAST buf);

    patch_get_env_attack(patch_id, eg_id, &val);
    snprintf(buf, BUFSIZE, "%f", val);
    xmlNewProp(node1,   BAD_CAST "attack",   BAD_CAST buf);

    patch_get_env_hold(patch_id, eg_id, &val);
    snprintf(buf, BUFSIZE, "%f", val);
    xmlNewProp(node1,   BAD_CAST "hold",   BAD_CAST buf);

    patch_get_env_decay(patch_id, eg_id, &val);
    snprintf(buf, BUFSIZE, "%f", val);
    xmlNewProp(node1,   BAD_CAST "decay",   BAD_CAST buf);

    patch_get_env_sustain(patch_id, eg_id, &val);
    snprintf(buf, BUFSIZE, "%f", val);
    xmlNewProp(node1,   BAD_CAST "sustain",   BAD_CAST buf);

    patch_get_env_release(patch_id, eg_id, &val);
    snprintf(buf, BUFSIZE, "%f", val);
    xmlNewProp(node1,   BAD_CAST "release",   BAD_CAST buf);

    patch_get_env_key_amt(patch_id, eg_id, &val);
    snprintf(buf, BUFSIZE, "%f", val);
    xmlNewProp(node1,   BAD_CAST "key_tracking",   BAD_CAST buf);

    return 0;
}


static int
dish_file_write_lfo(xmlNodePtr nodeparent, int patch_id, int lfo_id)
{
    xmlNodePtr  node1;
    xmlNodePtr  node2;
    xmlNodePtr  node3;
    char buf[BUFSIZE];
    bool state;
    float val;
    const char** shapes = names_lfo_shapes_get();
    int     mod1src;
    int     mod2src;
    float   mod1amt;
    float   mod2amt;
    LFOShape shape;

    if (patch_get_lfo_on(patch_id, lfo_id, &state) == -1)
        return -1;

    node1 = xmlNewTextChild(nodeparent, NULL,
                            BAD_CAST mod_src_name(lfo_id), NULL);

    xmlNewProp(node1,   BAD_CAST "active",
                        BAD_CAST (state ? "true" : "false"));

    node2 = xmlNewTextChild(node1, NULL, BAD_CAST "Frequency", NULL);
    patch_get_lfo_freq(patch_id, lfo_id, &val);
    snprintf(buf, BUFSIZE, "%f", val);
    xmlNewProp(node2,   BAD_CAST "hrtz", BAD_CAST buf);

    patch_get_lfo_beats(patch_id, lfo_id, &val);
    snprintf(buf, BUFSIZE, "%f", val);
    xmlNewProp(node2,   BAD_CAST "beats", BAD_CAST buf);

    patch_get_lfo_sync(patch_id, lfo_id, &state);
    xmlNewProp(node2,   BAD_CAST "sync",
                        BAD_CAST (state ? "true" : "false"));

    patch_get_lfo_fm1_src(patch_id, lfo_id, &mod1src);
    patch_get_lfo_fm1_amt(patch_id, lfo_id, &mod1amt);
    patch_get_lfo_fm2_src(patch_id, lfo_id, &mod2src);
    patch_get_lfo_fm2_amt(patch_id, lfo_id, &mod2amt);

    node3 = xmlNewTextChild(node2, NULL, BAD_CAST "Mod1", NULL);
    xmlNewProp(node3, BAD_CAST "source", BAD_CAST mod_src_name(mod1src));
    snprintf(buf, BUFSIZE, "%f", mod1amt);
    xmlNewProp(node3, BAD_CAST "amount", BAD_CAST buf);

    node3 = xmlNewTextChild(node2, NULL, BAD_CAST "Mod2", NULL);
    xmlNewProp(node3, BAD_CAST "source", BAD_CAST mod_src_name(mod2src));
    snprintf(buf, BUFSIZE, "%f", mod2amt);
    xmlNewProp(node3, BAD_CAST "amount", BAD_CAST buf);


    node2 = xmlNewTextChild(node1, NULL, BAD_CAST "Amplitude", NULL);

    patch_get_lfo_shape(patch_id, lfo_id, &shape);
    xmlNewProp(node2,   BAD_CAST "shape", BAD_CAST shapes[shape]);

    patch_get_lfo_positive(patch_id, lfo_id, &state);
    xmlNewProp(node2,   BAD_CAST "positive",
                        BAD_CAST (state ? "true" : "false"));

    /* assured by caller that lfo_id IS an lfo_id */
    if (!mod_src_is_global(lfo_id))
    {
        patch_get_lfo_delay(patch_id, lfo_id, &val);
        snprintf(buf, BUFSIZE, "%f", val);
        xmlNewProp(node2,   BAD_CAST "delay",   BAD_CAST buf);

        patch_get_lfo_attack(patch_id, lfo_id, &val);
        snprintf(buf, BUFSIZE, "%f", val);
        xmlNewProp(node2,   BAD_CAST "attack",  BAD_CAST buf);
    }

    patch_get_lfo_am1_src(patch_id, lfo_id, &mod1src);
    patch_get_lfo_am1_amt(patch_id, lfo_id, &mod1amt);
    patch_get_lfo_am2_src(patch_id, lfo_id, &mod2src);
    patch_get_lfo_am2_amt(patch_id, lfo_id, &mod2amt);

    node3 = xmlNewTextChild(node2, NULL, BAD_CAST "Mod1", NULL);
    xmlNewProp(node3, BAD_CAST "source", BAD_CAST mod_src_name(mod1src));
    snprintf(buf, BUFSIZE, "%f", mod1amt);
    xmlNewProp(node3, BAD_CAST "amount", BAD_CAST buf);

    node3 = xmlNewTextChild(node2, NULL, BAD_CAST "Mod2", NULL);
    xmlNewProp(node3, BAD_CAST "source", BAD_CAST mod_src_name(mod2src));
    snprintf(buf, BUFSIZE, "%f", mod2amt);
    xmlNewProp(node3, BAD_CAST "amount", BAD_CAST buf);

    return 0;
}


int dish_file_write(const char *name)
{
    int rc;

    xmlDocPtr   doc;
    xmlNodePtr  noderoot;
    xmlNodePtr  nodepatch;
    xmlNodePtr  node1;
    xmlNodePtr  node2;

    char    buf[BUFSIZE];
    int     i, j;
    int*    patch_id;
    int     patch_count;

    debug("attempting to write file:%s\n",name);

    doc = xmlNewDoc(BAD_CAST "1.0");
    if (!doc) { errmsg("XML error!\n"); return -1; }

    noderoot = xmlNewDocNode(doc, NULL, BAD_CAST "Petri-Foo-Dish", NULL);
    if (!noderoot) { errmsg("XML error!\n"); return -1; }

    xmlDocSetRootElement(doc, noderoot);

    /*  ------------------------
        master
     */
    node1 = xmlNewTextChild(noderoot, NULL, BAD_CAST "Master", NULL);

    snprintf(buf, BUFSIZE, "%f", mixer_get_amplitude());
    xmlNewProp(node1, BAD_CAST "level", BAD_CAST buf);

    /*  ------------------------
        patches
     */
    patch_count = patch_dump(&patch_id);

    for (i = 0; i < patch_count; ++i)
    {
        debug("writing patch:%d\n", patch_id[i]);

        nodepatch = xmlNewTextChild(noderoot, NULL,
                                BAD_CAST "Patch", NULL);

        xmlNewProp(nodepatch,   BAD_CAST "name",
                                BAD_CAST patch_get_name(patch_id[i]));

        snprintf(buf, BUFSIZE, "%d", patch_get_channel(patch_id[i]));
        xmlNewProp(nodepatch,   BAD_CAST "channel", BAD_CAST buf);

        /*  ------------------------
            sample
         */
        node1 = xmlNewTextChild(nodepatch, NULL, BAD_CAST "Sample", NULL);
        xmlNewProp(node1,   BAD_CAST "file",
                            BAD_CAST patch_get_sample_name(patch_id[i]));

        /* play mode, reverse, to_end */
        dish_file_write_sample_mode_props(node1, patch_id[i]);

        /* raw samplerate, raw channels, sndfile format */
        dish_file_write_sample_raw(node1, patch_id[i]);

        /* sample play */
        node2 = xmlNewTextChild(node1, NULL, BAD_CAST "Play", NULL);

        snprintf(buf, BUFSIZE, "%d",
                    patch_get_mark_frame(patch_id[i], WF_MARK_PLAY_START));
        xmlNewProp(node2,   BAD_CAST "start", BAD_CAST buf);

        snprintf(buf, BUFSIZE, "%d",
                    patch_get_mark_frame(patch_id[i], WF_MARK_PLAY_STOP));
        xmlNewProp(node2,   BAD_CAST "stop", BAD_CAST buf);

        snprintf(buf, BUFSIZE, "%d", patch_get_fade_samples(patch_id[i]));
        xmlNewProp(node2,   BAD_CAST "fade_samples", BAD_CAST buf);

        /* sample loop */
        node2 = xmlNewTextChild(node1, NULL, BAD_CAST "Loop", NULL);

        snprintf(buf, BUFSIZE, "%d",
                    patch_get_mark_frame(patch_id[i], WF_MARK_LOOP_START));
        xmlNewProp(node2,   BAD_CAST "start", BAD_CAST buf);

        snprintf(buf, BUFSIZE, "%d",
                    patch_get_mark_frame(patch_id[i], WF_MARK_LOOP_STOP));
        xmlNewProp(node2,   BAD_CAST "stop", BAD_CAST buf);

        snprintf(buf, BUFSIZE, "%d", patch_get_xfade_samples(patch_id[i]));
        xmlNewProp(node2,   BAD_CAST "xfade_samples", BAD_CAST buf);

        /* sample note */
        node2 = xmlNewTextChild(node1, NULL, BAD_CAST "Note", NULL);

        snprintf(buf, BUFSIZE, "%d", patch_get_note(patch_id[i]));
        xmlNewProp(node2,   BAD_CAST "root", BAD_CAST buf);

        snprintf(buf, BUFSIZE, "%d", patch_get_lower_note(patch_id[i]));
        xmlNewProp(node2,   BAD_CAST "lower", BAD_CAST buf);

        snprintf(buf, BUFSIZE, "%d", patch_get_upper_note(patch_id[i]));
        xmlNewProp(node2,   BAD_CAST "upper", BAD_CAST buf);

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
        snprintf(buf, BUFSIZE, "%d", patch_get_cut(patch_id[i]));
        xmlNewProp(node1,   BAD_CAST "cut", BAD_CAST buf);

        /* voice cut by */
        snprintf(buf, BUFSIZE, "%d", patch_get_cut_by(patch_id[i]));
        xmlNewProp(node1,   BAD_CAST "cut_by", BAD_CAST buf);

        /* voice portamento */
        xmlNewProp(node1,   BAD_CAST "portamento",
                            BAD_CAST (patch_get_portamento(patch_id[i])
                                        ? "true"
                                        : "false"));

        /* voice portamento_time */
        snprintf(buf, BUFSIZE, "%f",
                            patch_get_portamento_time(patch_id[i]));
        xmlNewProp(node1,   BAD_CAST "portamento_time", BAD_CAST buf);

        /* voice monophonic */
        xmlNewProp(node1,   BAD_CAST "monophonic",
                            BAD_CAST (patch_get_monophonic(patch_id[i])
                                        ? "true"
                                        : "false"));

        /* voice legato */
        xmlNewProp(node1,   BAD_CAST "legato",
                            BAD_CAST (patch_get_legato(patch_id[i])
                                        ? "true"
                                        : "false"));

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

    rc = xmlSaveFormatFile(name, doc, 1);
    xmlFreeDoc(doc);

    return rc;
}


static bool xmlstr_to_bool(xmlChar* str)
{
    if (xmlStrcasecmp(str, BAD_CAST "true") == 0
     || xmlStrcasecmp(str, BAD_CAST "on") == 0
     || xmlStrcasecmp(str, BAD_CAST "yes") == 0)
    {
        return true;
    }

    return false;
}


int dish_file_read_sample(xmlNodePtr node, int patch_id)
{
    xmlChar* prop;
    xmlNodePtr node1;
    int s;
    char* filename = 0;
    bool sample_loaded = false;

    int mode = PATCH_PLAY_SINGLESHOT;

    if ((prop = xmlGetProp(node, BAD_CAST "file")))
        filename = strdup((const char*)prop);

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
            errmsg("invalid play mode:%s\n", prop);
        }
    }

    if ((prop = xmlGetProp(node, BAD_CAST "reverse"))
                                                && xmlstr_to_bool(prop))
        mode |= PATCH_PLAY_REVERSE;
    else
        mode |= PATCH_PLAY_FORWARD;

    if ((prop = xmlGetProp(node, BAD_CAST "to_end"))
                                                && xmlstr_to_bool(prop))
        mode |= PATCH_PLAY_TO_END;

    patch_set_play_mode(patch_id, mode);

    for(node1 = node->children;
        node1 != NULL;
        node1 = node1->next)
    {
        if (node1->type != XML_ELEMENT_NODE)
            continue;

        if (!sample_loaded)
        {
            int n;
            int raw_samplerate = 0;
            int raw_channels = 0;
            int sndfile_format = 0;
            int err;

            if (xmlStrcmp(node1->name, BAD_CAST "Raw") == 0)
            {
                if ((prop = xmlGetProp(node1, BAD_CAST "samplerate")))
                    if (sscanf((const char*)prop, "%d", &n) == 1)
                        raw_samplerate = n;

                if ((prop = xmlGetProp(node1, BAD_CAST "channels")))
                    if (sscanf((const char*)prop, "%d", &n) == 1)
                        raw_channels = n;

                if ((prop = xmlGetProp(node1, BAD_CAST "sndfile_format")))
                    if (sscanf((const char*)prop, "%d", &n) == 1)
                        sndfile_format = n;
            }

            err = patch_sample_load(patch_id, filename,
                                              raw_samplerate,
                                              raw_channels,
                                              sndfile_format);
            if (err)
            {
                errmsg("failed to load sample:%s\n", (const char*)filename);
                return 0;
            }

            free(filename);
            filename = 0;
            sample_loaded = true;
        }

        if (xmlStrcmp(node1->name, BAD_CAST "Play") == 0)
        {
            if ((prop = xmlGetProp(node1, BAD_CAST "start")))
            {
                if (sscanf((const char*)prop, "%d", &s) == 1)
                    patch_set_mark_frame_expand(patch_id,
                                                WF_MARK_PLAY_START,
                                                s, NULL);
            }
            if ((prop = xmlGetProp(node1, BAD_CAST "stop")))
                if (sscanf((const char*)prop, "%d", &s) == 1)
                    patch_set_mark_frame_expand(patch_id,
                                                WF_MARK_PLAY_STOP,
                                                s, NULL);

            if ((prop = xmlGetProp(node1, BAD_CAST "fade_samples")))
                if (sscanf((const char*)prop, "%d", &s) == 1)
                    patch_set_fade_samples(patch_id, s);
        }
        else if (xmlStrcmp(node1->name, BAD_CAST "Loop") == 0)
        {
            if ((prop = xmlGetProp(node1, BAD_CAST "start")))
                if (sscanf((const char*)prop, "%d", &s) == 1)
                    patch_set_mark_frame_expand(patch_id,
                                                WF_MARK_LOOP_START,
                                                s, NULL);

            if ((prop = xmlGetProp(node1, BAD_CAST "stop")))
                if (sscanf((const char*)prop, "%d", &s) == 1)
                    patch_set_mark_frame_expand(patch_id,
                                                WF_MARK_LOOP_STOP,
                                                s, NULL);

            if ((prop = xmlGetProp(node1, BAD_CAST "xfade_samples")))
                if (sscanf((const char*)prop, "%d", &s) == 1)
                    patch_set_xfade_samples(patch_id, s);
        }
        else if (xmlStrcmp(node1->name, BAD_CAST "Note") == 0)
        {
            int lower, root, upper;

            if ((prop = xmlGetProp(node1, BAD_CAST "root")))
                if (sscanf((const char*)prop, "%d", &root) == 1)
                    patch_set_note(patch_id,  root);

            if ((prop = xmlGetProp(node1, BAD_CAST "lower")))
                if (sscanf((const char*)prop, "%d", &lower) == 1)
                    patch_set_lower_note(patch_id,  lower);

            if ((prop = xmlGetProp(node1, BAD_CAST "upper")))
                if (sscanf((const char*)prop, "%d", &upper) == 1)
                    patch_set_upper_note(patch_id,  upper);
        }
        else
        {
            errmsg("ignoring:%s\n", node1->name);
        }
    }

    return 0;
}

int dish_file_read_eg(xmlNodePtr node, int patch_id)
{
    int eg_id = 0;
    xmlChar* prop;
    float n;

    if ((eg_id = mod_src_id((const char*)node->name, MOD_SRC_EG)) < 0)
        return -1;

debug("loading eg with id:%d\n", eg_id);

    if ((prop = xmlGetProp(node, BAD_CAST "active")))
        patch_set_env_on(patch_id, eg_id, xmlstr_to_bool(prop));

    if ((prop = xmlGetProp(node, BAD_CAST "delay")))
        if (sscanf((const char*)prop, "%f", &n) == 1)
            patch_set_env_delay(patch_id, eg_id, n);

    if ((prop = xmlGetProp(node, BAD_CAST "attack")))
        if (sscanf((const char*)prop, "%f", &n) == 1)
            patch_set_env_attack(patch_id, eg_id, n);

    if ((prop = xmlGetProp(node, BAD_CAST "hold")))
        if (sscanf((const char*)prop, "%f", &n) == 1)
            patch_set_env_hold(patch_id, eg_id, n);

    if ((prop = xmlGetProp(node, BAD_CAST "decay")))
        if (sscanf((const char*)prop, "%f", &n) == 1)
            patch_set_env_decay(patch_id, eg_id, n);

    if ((prop = xmlGetProp(node, BAD_CAST "sustain")))
        if (sscanf((const char*)prop, "%f", &n) == 1)
            patch_set_env_sustain(patch_id, eg_id, n);

    if ((prop = xmlGetProp(node, BAD_CAST "release")))
        if (sscanf((const char*)prop, "%f", &n) == 1)
            patch_set_env_release(patch_id, eg_id, n);

    if ((prop = xmlGetProp(node, BAD_CAST "key_tracking")))
        if (sscanf((const char*)prop, "%f", &n) == 1)
            patch_set_env_key_amt(patch_id, eg_id, n);

    return 0;
}

int dish_file_read_lfo_freq_data(xmlNodePtr node, int patch_id, int lfo_id)
{
    xmlNodePtr node1;
    xmlChar* prop;
    float n;

    if ((prop = xmlGetProp(node, BAD_CAST "hrtz")))
        if (sscanf((const char*)prop, "%f", &n) == 1)
            patch_set_lfo_freq(patch_id, lfo_id, n);

    if ((prop = xmlGetProp(node, BAD_CAST "beats")))
        if (sscanf((const char*)prop, "%f", &n) == 1)
            patch_set_lfo_beats(patch_id, lfo_id, n);

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
            if ((prop = xmlGetProp(node1, BAD_CAST "source")))
                patch_set_lfo_fm1_src(patch_id, lfo_id,
                    mod_src_id((const char*)prop, MOD_SRC_ALL));

            if ((prop = xmlGetProp(node1, BAD_CAST "amount")))
                if (sscanf((const char*)prop, "%f", &n) == 1)
                    patch_set_lfo_fm1_amt(patch_id, lfo_id, n);
        }
        else if (xmlStrcmp(node1->name, BAD_CAST "Mod2") == 0)
        {
            if ((prop = xmlGetProp(node1, BAD_CAST "source")))
                patch_set_lfo_fm2_src(patch_id, lfo_id,
                    mod_src_id((const char*)prop, MOD_SRC_ALL));

            if ((prop = xmlGetProp(node1, BAD_CAST "amount")))
                if (sscanf((const char*)prop, "%f", &n) == 1)
                    patch_set_lfo_fm2_amt(patch_id, lfo_id, n);
        }
        else
        {
            errmsg("ignoring:%s\n", (const char*)node1->name);
        }
    }

    return 0;
}


int dish_file_read_lfo_amp_data(xmlNodePtr node, int patch_id, int lfo_id)
{
    xmlNodePtr node1;
    xmlChar* prop;
    float n;

    if ((prop = xmlGetProp(node, BAD_CAST "shape")))
        patch_set_lfo_shape(patch_id, lfo_id,
            (LFOShape)names_lfo_shapes_id_from_str((const char*)prop));

    if ((prop = xmlGetProp(node, BAD_CAST "positive")))
        patch_set_lfo_positive(patch_id, lfo_id, xmlstr_to_bool(prop));

    if (!mod_src_is_global(lfo_id)) /* already know it IS an LFO id */
    {
        if ((prop = xmlGetProp(node, BAD_CAST "delay")))
            if (sscanf((const char*)prop, "%f", &n) == 1)
                patch_set_lfo_delay(patch_id, lfo_id, n);

        if ((prop = xmlGetProp(node, BAD_CAST "attack")))
            if (sscanf((const char*)prop, "%f", &n) == 1)
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
            if ((prop = xmlGetProp(node1, BAD_CAST "source")))
                patch_set_lfo_am1_src(patch_id, lfo_id,
                    mod_src_id((const char*)prop, MOD_SRC_ALL));

            if ((prop = xmlGetProp(node1, BAD_CAST "amount")))
                if (sscanf((const char*)prop, "%f", &n) == 1)
                    patch_set_lfo_am1_amt(patch_id, lfo_id, n);
        }
        else if (xmlStrcmp(node1->name, BAD_CAST "Mod2") == 0)
        {
            if ((prop = xmlGetProp(node1, BAD_CAST "source")))
                patch_set_lfo_am2_src(patch_id, lfo_id,
                    mod_src_id((const char*)prop, MOD_SRC_ALL));

            if ((prop = xmlGetProp(node1, BAD_CAST "amount")))
                if (sscanf((const char*)prop, "%f", &n) == 1)
                    patch_set_lfo_am2_amt(patch_id, lfo_id, n);
        }
        else
        {
            errmsg("ignoring:%s\n", (const char*)node1->name);
        }
    }

    return 0;
}


int dish_file_read_lfo(xmlNodePtr node, int patch_id)
{
    int lfo_id;
    xmlChar* prop;
    xmlNodePtr node1;

    lfo_id = mod_src_id((const char*)node->name, MOD_SRC_LFOS);

debug("lfo id:%d\n", lfo_id);

    if (lfo_id < 0)
    {
        errmsg("invalid LFO:%s\n", (const char*)node->name);
        return -1;
    }

    if ((prop = xmlGetProp(node, BAD_CAST "active")))
        patch_set_lfo_on(patch_id, lfo_id, xmlstr_to_bool(prop));

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
            errmsg("ignoring:%s\n", (const char*)node1->name);
        }
    }

    return 0;
}

int dish_file_read_param(xmlNodePtr node,   int patch_id,
                                            PatchParamType param)
{
    const char* pname = 0;
    float   n;
    xmlChar* prop;
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

    if ((prop = xmlGetProp(node, BAD_CAST pname)))
        if (sscanf((const char*)prop, "%f", &n) == 1)
            patch_param_set_value(patch_id, param, n);

    if (param == PATCH_PARAM_PITCH)
    {
        if ((prop = xmlGetProp(node, BAD_CAST "tuning_range")))
        {
            int steps;
            if (sscanf((const char*)prop, "%d", &steps) == 1)
                patch_set_pitch_steps(patch_id, steps);
        }
    }

    if ((prop = xmlGetProp(node, BAD_CAST "velocity_sensing")))
        if (sscanf((const char*)prop, "%f", &n) == 1)
            patch_set_vel_amount(patch_id, param, n);

    if ((prop = xmlGetProp(node, BAD_CAST "key_tracking")))
        if (sscanf((const char*)prop, "%f", &n) == 1)
            patch_set_key_amount(patch_id, param, n);

    for (   node1 = node->children;
            node1 != NULL;
            node1 = node1->next)
    {
        if (node1->type != XML_ELEMENT_NODE)
            continue;

        int slot = -1;

        if (sscanf((const char*)node1->name, "Mod%d", &slot) == 1
            && slot > 0 && slot <= MAX_MOD_SLOTS)
        {
            --slot; /* slot 0 is named as MOD1 */

            if ((prop = xmlGetProp(node1, BAD_CAST "source")))
                patch_set_mod_src(patch_id, param, slot,
                    mod_src_id((const char*)prop, MOD_SRC_ALL));

            if ((prop = xmlGetProp(node1, BAD_CAST "amount")))
                if (sscanf((const char*)prop, "%f", &n) == 1)
                    patch_set_mod_amt(patch_id, param, slot, n);
        }
        else if ((param == PATCH_PARAM_AMPLITUDE
                && xmlStrcmp(node1->name, BAD_CAST "Env") == 0))
        {
            if ((prop = xmlGetProp(node1, BAD_CAST "source")))
                patch_set_mod_src(patch_id, param, EG_MOD_SLOT,
                    mod_src_id((const char*)prop, MOD_SRC_ALL));
        }
        else
        {
            errmsg("ignoring:%s\n", (const char*)node1->name);
        }
    }

    return 0;
}


int dish_file_read_voice(xmlNodePtr node, int patch_id)
{
    xmlChar*    prop;
    float n;
    int i;

    if ((prop = xmlGetProp(node, BAD_CAST "cut")))
        if (sscanf((const char*)prop, "%d", &i))
            patch_set_cut(patch_id, i);

    if ((prop = xmlGetProp(node, BAD_CAST "cut_by")))
        if (sscanf((const char*)prop, "%d", &i))
            patch_set_cut_by(patch_id, i);

    if ((prop = xmlGetProp(node, BAD_CAST "portamento")))
        patch_set_portamento(patch_id, xmlstr_to_bool(prop));

    if ((prop = xmlGetProp(node, BAD_CAST "portamento_time")))
        if (sscanf((const char*)prop, "%f", &n))
            patch_set_portamento_time(patch_id, n);

    if ((prop = xmlGetProp(node, BAD_CAST "monophonic")))
        patch_set_monophonic(patch_id, xmlstr_to_bool(prop));

    if ((prop = xmlGetProp(node, BAD_CAST "legato")))
        patch_set_legato(patch_id, xmlstr_to_bool(prop));

    return 0;
}


int dish_file_read(const char *path)
{
    xmlDocPtr   doc;
    xmlNodePtr  noderoot;
    xmlNodePtr  nodepatch;
    xmlNodePtr  node1;
    xmlNodePtr  node2;
    xmlChar*    prop;
    float   n;

    debug("Loading bank from file %s\n", path);

    doc = xmlParseFile (path);

    if (doc == NULL)
    {
        errmsg("Failed to parse %s\n", path);
        return -1;
    }

    noderoot = xmlDocGetRootElement(doc);

    if (noderoot == NULL)
    {
        errmsg("%s is empty\n", path);
        xmlFreeDoc(doc);
        return -1;
    }

    if (xmlStrcmp(noderoot->name, BAD_CAST "Petri-Foo-Dish") != 0)
    {
        errmsg("%s is not a valid 'Petri-Foo-Dish' file\n", path);
        xmlFreeDoc(doc);
        return -1;
    }

    patch_destroy_all();

    for (node1 = noderoot->children;
         node1 != NULL;
         node1 = node1->next)
    {
        if (node1->type != XML_ELEMENT_NODE)
            continue;

        if (xmlStrcmp(node1->name, BAD_CAST "Master") == 0)
        {
            if ((prop = xmlGetProp(node1, BAD_CAST "level")))
                if (sscanf((const char*)prop, "%f", &n) == 1)
                    mixer_set_amplitude(n);
        }
        else if (xmlStrcmp(node1->name, BAD_CAST "Patch") == 0)
        {
            int patch_id = patch_create();

            nodepatch = node1;

            /* patch name */
            if ((prop = xmlGetProp(nodepatch, BAD_CAST "name")))
                patch_set_name(patch_id, (const char*)prop);

            if ((prop = xmlGetProp(nodepatch, BAD_CAST "channel")))
            {
                int c;
                if (sscanf((const char*)prop, "%d", &c))
                    patch_set_channel(patch_id, c);
            }

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
                    else
                    if (mod_src_maybe_lfo((const char*)node2->name))
                            dish_file_read_lfo(node2, patch_id);
                    else
                    {
                        errmsg("ignoring:%s\n", (const char*)node2->name);
                    }
                }
            }
        }
        else
        {
            errmsg("ignoring:%s\n", (const char*)node1->name);
        }
    }

    return 0;
}
