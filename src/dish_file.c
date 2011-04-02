#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <libxml/parser.h>
#include <glib.h>

#include "mixer.h"
#include "names.h"
#include "petri-foo.h"
#include "patch.h"
#include "patch_set_and_get.h"


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


static int
dish_file_write_param(xmlNodePtr nodeparent, int patch_id,
                                                PatchParamType param)
{
    xmlNodePtr  node1;
    xmlNodePtr  node2;
    char buf[BUFSIZE];
    const char** param_names;
    char** mod_names;
    const char* prop1 = 0;
    const char* prop2 = 0;

    float   val1;
    float   val2;
    float   vel_amt;
    int     envsrc;
    int     mod1src;
    int     mod2src;
    float   mod1amt;
    float   mod2amt;
    float   velsens;

    if (patch_param_get_value(patch_id, param, &val1)
                                                == PATCH_PARAM_INVALID)
        return -1;

    param_names = names_params_get();
    mod_names = names_mod_srcs_get();

    patch_get_vel_amount(patch_id, param, &vel_amt);

    switch(param)
    {
    case PATCH_PARAM_AMPLITUDE: prop1 = "level";     prop2 = 0; break;
    case PATCH_PARAM_PANNING:   prop1 = "position";  prop2 = 0; break;
    case PATCH_PARAM_CUTOFF:    prop1 = "value";     prop2 = 0; break;
    case PATCH_PARAM_RESONANCE: prop1 = "amount";    prop2 = 0; break;
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

    if (param == PATCH_PARAM_AMPLITUDE)
    {
        patch_get_amp_env(patch_id, &envsrc);
        node2 = xmlNewTextChild(node1, NULL, BAD_CAST "Env", NULL);
        xmlNewProp(node2, BAD_CAST "source", BAD_CAST mod_names[envsrc]);
    }

    patch_get_mod1_src(patch_id, param, &mod1src);
    patch_get_mod1_amt(patch_id, param, &mod1amt);
    patch_get_mod2_src(patch_id, param, &mod2src);
    patch_get_mod2_amt(patch_id, param, &mod2amt);

    node2 = xmlNewTextChild(node1, NULL, BAD_CAST "Mod1", NULL);
    xmlNewProp(node2, BAD_CAST "source", BAD_CAST mod_names[mod1src]);
    snprintf(buf, BUFSIZE, "%f", mod1amt);
    xmlNewProp(node2, BAD_CAST "amount", BAD_CAST buf);

    node2 = xmlNewTextChild(node1, NULL, BAD_CAST "Mod2", NULL);
    xmlNewProp(node2, BAD_CAST "source", BAD_CAST mod_names[mod2src]);
    snprintf(buf, BUFSIZE, "%f", mod2amt);
    xmlNewProp(node2, BAD_CAST "amount", BAD_CAST buf);

    return 0;
}


static int
dish_file_write_eg(xmlNodePtr nodeparent, int patch_id, int eg_id)
{
    xmlNodePtr  node1;
    char buf[BUFSIZE];
    gboolean active;
    float val;

    if (patch_get_env_on(patch_id, eg_id, &active) == -1)
        return -1;

    node1 = xmlNewTextChild(nodeparent, NULL,
                            BAD_CAST names_egs_get()[eg_id], NULL);

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

    return 0;
}


static int
dish_file_write_lfo(xmlNodePtr nodeparent, int patch_id, int lfo_id)
{
    xmlNodePtr  node1;
    xmlNodePtr  node2;
    xmlNodePtr  node3;
    char buf[BUFSIZE];
    gboolean state;
    float val;
    char** mod_names;
    const char** shapes = names_lfo_shapes_get();
    const char** lfo_names = names_lfos_get();
    int     mod1src;
    int     mod2src;
    float   mod1amt;
    float   mod2amt;
    LFOShape shape;

    if (patch_get_lfo_on(patch_id, lfo_id, &state) == -1)
        return -1;

    mod_names = names_mod_srcs_get();

    node1 = xmlNewTextChild(nodeparent, NULL,
                            BAD_CAST lfo_names[lfo_id], NULL);

    xmlNewProp(node1,   BAD_CAST "active",
                        BAD_CAST (state ? "true" : "false"));

    patch_get_lfo_shape(patch_id, lfo_id, &shape);
    xmlNewProp(node1,   BAD_CAST "shape", BAD_CAST shapes[shape]);

    patch_get_lfo_positive(patch_id, lfo_id, &state);
    xmlNewProp(node1,   BAD_CAST "positive",
                        BAD_CAST (state ? "true" : "false"));

    patch_get_lfo_delay(patch_id, lfo_id, &val);
    snprintf(buf, BUFSIZE, "%f", val);
    xmlNewProp(node1,   BAD_CAST "delay",   BAD_CAST buf);

    patch_get_lfo_attack(patch_id, lfo_id, &val);
    snprintf(buf, BUFSIZE, "%f", val);
    xmlNewProp(node1,   BAD_CAST "attack",  BAD_CAST buf);

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

    patch_get_lfo_mod1_src(patch_id, lfo_id, &mod1src);
    patch_get_lfo_mod1_amt(patch_id, lfo_id, &mod1amt);
    patch_get_lfo_mod2_src(patch_id, lfo_id, &mod2src);
    patch_get_lfo_mod2_amt(patch_id, lfo_id, &mod2amt);

    node3 = xmlNewTextChild(node2, NULL, BAD_CAST "Mod1", NULL);
    xmlNewProp(node3, BAD_CAST "source", BAD_CAST mod_names[mod1src]);
    snprintf(buf, BUFSIZE, "%f", mod1amt);
    xmlNewProp(node3, BAD_CAST "amount", BAD_CAST buf);

    node3 = xmlNewTextChild(node2, NULL, BAD_CAST "Mod2", NULL);
    xmlNewProp(node3, BAD_CAST "source", BAD_CAST mod_names[mod2src]);
    snprintf(buf, BUFSIZE, "%f", mod2amt);
    xmlNewProp(node3, BAD_CAST "amount", BAD_CAST buf);

    return 0;
}


int dish_file_write(char *name)
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
            dish_file_write_eg(nodepatch, patch_id[i], j);

        /*  ------------------------
            lfos
         */
        for (j = 0; j < TOTAL_LFOS; ++j)
            dish_file_write_lfo(nodepatch, patch_id[i], j);
    }

    rc = xmlSaveFormatFile(name, doc, 1);
    xmlFreeDoc(doc);

    return rc;
}


static gboolean xmlstr_to_bool(xmlChar* str)
{
    if (xmlStrcasecmp(str, BAD_CAST "true") == 0
     || xmlStrcasecmp(str, BAD_CAST "on") == 0
     || xmlStrcasecmp(str, BAD_CAST "yes") == 0)
    {
        return TRUE;
    }

    return FALSE;
}


int dish_file_read_sample(xmlNodePtr node, int patch_id)
{
    xmlChar* prop;
    xmlNodePtr node1;
    int s;

    int mode = PATCH_PLAY_SINGLESHOT;

    if ((prop = xmlGetProp(node, BAD_CAST "file")))
    {
        if (patch_sample_load(patch_id, (const char*)prop) != 0)
        {
            errmsg("failed to load sample:%s\n", (const char*)prop);
            return 0;
        }
    }

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

            if (lower != root || upper != root || lower != upper)
                patch_set_range(patch_id, 1);
            else /* still don't have a clue what this does !*/
                patch_set_range(patch_id, 0);
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

    if ((eg_id = names_egs_id_from_str((const char*)node->name)) < 0)
        return -1;

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
                patch_set_lfo_mod1_src(patch_id, lfo_id,
                    names_mod_srcs_id_from_str((const char*)prop));

            if ((prop = xmlGetProp(node1, BAD_CAST "amount")))
                if (sscanf((const char*)prop, "%f", &n) == 1)
                    patch_set_lfo_mod1_amt(patch_id, lfo_id, n);
        }
        else if (xmlStrcmp(node1->name, BAD_CAST "Mod2") == 0)
        {
            if ((prop = xmlGetProp(node1, BAD_CAST "source")))
                patch_set_lfo_mod2_src(patch_id, lfo_id,
                    names_mod_srcs_id_from_str((const char*)prop));

            if ((prop = xmlGetProp(node1, BAD_CAST "amount")))
                if (sscanf((const char*)prop, "%f", &n) == 1)
                    patch_set_lfo_mod2_amt(patch_id, lfo_id, n);
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
    float n;
    xmlNodePtr node1;

    if ((lfo_id = names_lfos_id_from_str((const char*)node->name)) < 0)
    {
        errmsg("invalid LFO:%s\n", (const char*)node->name);
        return -1;
    }

    if ((prop = xmlGetProp(node, BAD_CAST "active")))
        patch_set_lfo_on(patch_id, lfo_id, xmlstr_to_bool(prop));

    if ((prop = xmlGetProp(node, BAD_CAST "shape")))
        patch_set_lfo_shape(patch_id, lfo_id,
            (LFOShape)names_lfo_shapes_id_from_str((const char*)prop));

    if ((prop = xmlGetProp(node, BAD_CAST "positive")))
        patch_set_lfo_positive(patch_id, lfo_id, xmlstr_to_bool(prop));

    if ((prop = xmlGetProp(node, BAD_CAST "delay")))
        if (sscanf((const char*)prop, "%f", &n) == 1)
            patch_set_lfo_delay(patch_id, lfo_id, n);

    if ((prop = xmlGetProp(node, BAD_CAST "attack")))
        if (sscanf((const char*)prop, "%f", &n) == 1)
            patch_set_lfo_attack(patch_id, lfo_id, n);

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
debug("read:%s\n", names_params_get()[param]);
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
    {
        debug("gonna attempt to run scanf on '%s'\n",prop);
        if (sscanf((const char*)prop, "%f", &n) == 1)
        {
            debug("gonna set vel amount now:\n");
            patch_set_vel_amount(patch_id, param, n);
            debug("done\n");
        }
        else
        {
            debug("not setting vel amount, scanf error\n");
        }
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
                patch_set_mod1_src(patch_id, param,
                    names_mod_srcs_id_from_str((const char*)prop));

            if ((prop = xmlGetProp(node1, BAD_CAST "amount")))
                if (sscanf((const char*)prop, "%f", &n) == 1)
                    patch_set_mod1_amt(patch_id, param, n);
        }
        else if (xmlStrcmp(node1->name, BAD_CAST "Mod2") == 0)
        {
            if ((prop = xmlGetProp(node1, BAD_CAST "source")))
                patch_set_mod2_src(patch_id, param,
                    names_mod_srcs_id_from_str((const char*)prop));

            if ((prop = xmlGetProp(node1, BAD_CAST "amount")))
                if (sscanf((const char*)prop, "%f", &n) == 1)
                    patch_set_mod2_amt(patch_id, param, n);
        }
        else if (param == PATCH_PARAM_AMPLITUDE
              && xmlStrcmp(node1->name, BAD_CAST "Env") == 0)
        {
            if ((prop = xmlGetProp(node1, BAD_CAST "source")))
                    patch_set_amp_env(patch_id,
                        names_mod_srcs_id_from_str((const char*)prop));
        }
        else
        {
            errmsg("ignoring:%s\n", (const char*)node1->name);
        }
    }
}


int dish_file_read(char *path)
{
    int rc;

    gboolean sample_loaded = FALSE;

    const char* samplefile = 0;

    xmlDocPtr   doc;
    xmlNodePtr  noderoot;
    xmlNodePtr  nodepatch;
    xmlNodePtr  node1;
    xmlNodePtr  node2;

    xmlChar*    prop;

    char    buf[BUFSIZE];
    int     i, j;
    int*    patch_id;
    int     patch_count;
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
            int patch_id = patch_create("Loading...");

            nodepatch = node1;

            /* patch name */
            if ((prop = xmlGetProp(nodepatch, BAD_CAST "name")))
                patch_set_name(patch_id, (const char*)prop);

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
                    debug("voice xml node\n");
                }
                else
                {
                    if (names_egs_maybe_eg_id((const char*)node2->name))
                    {
                            dish_file_read_eg(node2, patch_id);
                    }
                    else
                    if (names_lfos_maybe_lfo_id((const char*)node2->name))
                    {
                            dish_file_read_lfo(node2, patch_id);
                    }
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
