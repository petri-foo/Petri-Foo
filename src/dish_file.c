#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <libxml/parser.h>
#include <glib.h>

#include "mixer.h"
#include "petri-foo.h"
#include "patch_set_and_get.h"

#include "gui/mod_src.h"


#define BUFSIZE 256

static const char* dish_file_ext = ".petri-foo";

const char* dish_file_extension(void)
{
    return dish_file_ext;
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

    if (patch_param_get_value(patch_id, param, &val1)
                                                == PATCH_PARAM_INVALID)
        return -1;

    param_names = mod_src_param_names();
    mod_names = mod_src_get_names();

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
    const char** adsr_names = mod_src_adsr_names();

    debug("writing xml for env %d\n", eg_id);

    if (patch_get_env_on(patch_id, eg_id, &active) == -1)
        return -1;

    node1 = xmlNewTextChild(nodeparent, NULL,
                            BAD_CAST adsr_names[eg_id], NULL);

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
    const char** shapes = lfo_get_shape_names();
    const char** lfo_names = mod_src_lfo_names();
    int     mod1src;
    int     mod2src;
    float   mod1amt;
    float   mod2amt;
    LFOShape shape;

    debug("writing xml for lfo %d\n", lfo_id);

    if (patch_get_lfo_on(patch_id, lfo_id, &state) == -1)
        return -1;

    mod_names = mod_src_get_names();

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

    patch_get_lfo_mod1_src(patch_id, lfo_id, &mod1src);
    patch_get_lfo_mod1_amt(patch_id, lfo_id, &mod1amt);
    patch_get_lfo_mod2_src(patch_id, lfo_id, &mod2src);
    patch_get_lfo_mod2_amt(patch_id, lfo_id, &mod2amt);

    printf("mod1src:%d mod2src:%d\n",mod1src,mod2src);
    printf("mod1amt:%f mod2amt:%f\n",mod1amt,mod2amt);

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

        snprintf(buf, BUFSIZE, "%d", patch_get_play_mode(patch_id[i]));
        xmlNewProp(node1,   BAD_CAST "play_mode", BAD_CAST buf);

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
        dish_file_write_param(nodepatch, patch_id[i],
                                            PATCH_PARAM_AMPLITUDE);

        /*  ------------------------
            pitch
         */
        dish_file_write_param(nodepatch, patch_id[i],
                                            PATCH_PARAM_PITCH);

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
        for (j = 0; j < VOICE_MAX_LFOS; ++j)
            dish_file_write_lfo(nodepatch, patch_id[i], PATCH_MAX_LFOS + j);
    }

    rc = xmlSaveFormatFile(name, doc, 1);
    xmlFreeDoc(doc);

    return rc;

#ifdef WE_WANT_MORE_BEEF

    char tmp[256];
    gboolean tmpb;
    LFOShape tmpLS;
    char tmpName[256];
    char *ptmp;
    float tmpf;
    int *id;
    int i, j, count;
    xmlDocPtr doc;
    xmlNodePtr rootnode;
    xmlNodePtr xmlpatch;

#define NR_OF_PPT 5
    PatchParamType ppt_lst[NR_OF_PPT] = { PATCH_PARAM_AMPLITUDE,
					  PATCH_PARAM_PANNING,
					  PATCH_PARAM_CUTOFF,
					  PATCH_PARAM_RESONANCE,
					  PATCH_PARAM_PITCH };
    char* ppt_names[NR_OF_PPT] = { "amplitude_",
				   "panning_",
				   "cutoff_",
				   "resonance_",
				   "pitch_" };

    debug ("Writing current bank to file %s\n", name);

    /* create the guts of the doc */
    doc = xmlNewDoc ((const xmlChar *)"1.0");
    rootnode = xmlNewDocNode(doc, NULL, (const xmlChar *)"petri-foo-dish",
                                                                    NULL);
    xmlDocSetRootElement (doc, rootnode);

    /* walk the walk */
    count = patch_dump (&id);
    for (i = 0; i < count; i++)
    {
	xmlpatch =
	    xmlNewTextChild (rootnode, NULL, (const xmlChar *) "patch",
			     NULL);

	ptmp = patch_get_name (id[i]);
	xmlNewTextChild (xmlpatch, NULL, (const xmlChar *) "name",
			 (const xmlChar *) ptmp);
	g_free (ptmp);

	ptmp = patch_get_sample_name (id[i]);
	if (ptmp[0] != '\0')
	    xmlNewTextChild (xmlpatch, NULL, (const xmlChar *) "file",
			     (const xmlChar *) ptmp);
	g_free (ptmp);

	sprintf (tmp, "%d", patch_get_channel (id[i]));
	xmlNewTextChild (xmlpatch, NULL, (const xmlChar *) "channel",
			 (const xmlChar *) tmp);

	sprintf (tmp, "%d", patch_get_note (id[i]));
	xmlNewTextChild (xmlpatch, NULL, (const xmlChar *) "note",
			 (const xmlChar *) tmp);

	sprintf (tmp, "%f", patch_get_amplitude (id[i]));
	xmlNewTextChild (xmlpatch, NULL, (const xmlChar *) "amplitude",
			 (const xmlChar *) tmp);

	sprintf (tmp, "%f", patch_get_panning (id[i]));
	xmlNewTextChild (xmlpatch, NULL, (const xmlChar *) "pan",
			 (const xmlChar *) tmp);

	sprintf (tmp, "%d", patch_get_play_mode (id[i]));
	xmlNewTextChild (xmlpatch, NULL, (const xmlChar *) "play_mode",
			 (const xmlChar *) tmp);

	sprintf (tmp, "%d", patch_get_cut (id[i]));
	xmlNewTextChild (xmlpatch, NULL, (const xmlChar *) "cut",
			 (const xmlChar *) tmp);

	sprintf (tmp, "%d", patch_get_cut_by (id[i]));
	xmlNewTextChild (xmlpatch, NULL, (const xmlChar *) "cut_by",
			 (const xmlChar *) tmp);

	sprintf (tmp, "%d", patch_get_range (id[i]));
	xmlNewTextChild (xmlpatch, NULL, (const xmlChar *) "range",
			 (const xmlChar *) tmp);

	sprintf (tmp, "%d", patch_get_lower_note (id[i]));
	xmlNewTextChild (xmlpatch, NULL, (const xmlChar *) "lower_note",
			 (const xmlChar *) tmp);

	sprintf (tmp, "%d", patch_get_upper_note (id[i]));
	xmlNewTextChild (xmlpatch, NULL, (const xmlChar *) "upper_note",
			 (const xmlChar *) tmp);

	sprintf (tmp, "%d", patch_get_sample_start (id[i]));
	xmlNewTextChild (xmlpatch, NULL, (const xmlChar *) "sample_start",
			 (const xmlChar *) tmp);

	sprintf (tmp, "%d", patch_get_sample_stop (id[i]));
	xmlNewTextChild (xmlpatch, NULL, (const xmlChar *) "sample_stop",
			 (const xmlChar *) tmp);

	sprintf (tmp, "%d", patch_get_loop_start (id[i]));
	xmlNewTextChild (xmlpatch, NULL, (const xmlChar *) "loop_start",
			 (const xmlChar *) tmp);

	sprintf (tmp, "%d", patch_get_loop_stop (id[i]));
	xmlNewTextChild (xmlpatch, NULL, (const xmlChar *) "loop_stop",
			 (const xmlChar *) tmp);

	sprintf (tmp, "%f", patch_get_cutoff (id[i]));
	xmlNewTextChild (xmlpatch, NULL, (const xmlChar *) "cutoff",
			 (const xmlChar *) tmp);

	sprintf (tmp, "%f", patch_get_resonance (id[i]));
	xmlNewTextChild (xmlpatch, NULL, (const xmlChar *) "resonance",
			 (const xmlChar *) tmp);

	sprintf (tmp, "%f", patch_get_pitch (id[i]));
	xmlNewTextChild (xmlpatch, NULL, (const xmlChar *) "pitch",
			 (const xmlChar *) tmp);

	sprintf (tmp, "%d", patch_get_pitch_steps (id[i]));
	xmlNewTextChild (xmlpatch, NULL, (const xmlChar *) "pitch_steps",
			 (const xmlChar *) tmp);

	tmpb = patch_get_portamento (id[i]);
	if ( tmpb )
	    sprintf (tmp, "%s", "yes");
	else
	    sprintf (tmp, "%s", "no");
	xmlNewTextChild (xmlpatch, NULL, (const xmlChar *) "portamento",
			 (const xmlChar *) tmp);

	sprintf (tmp, "%f", patch_get_portamento_time (id[i]));
	xmlNewTextChild (xmlpatch, NULL, (const xmlChar *) "portamento_time",
			 (const xmlChar *) tmp);

	tmpb = patch_get_monophonic (id[i]);
	if ( tmpb )
	    sprintf (tmp, "%s", "yes");
	else
	    sprintf (tmp, "%s", "no");
	xmlNewTextChild (xmlpatch, NULL, (const xmlChar *) "monophonic",
			 (const xmlChar *) tmp);

	tmpb = patch_get_legato (id[i]);
	if ( tmpb )
	    sprintf (tmp, "%s", "yes");
	else
	    sprintf (tmp, "%s", "no");
	xmlNewTextChild (xmlpatch, NULL, (const xmlChar *) "legato",
			 (const xmlChar *) tmp);

	/* params */
	for (j=0; j<NR_OF_PPT; j++)
	{
	    strcpy(tmpName,ppt_names[j]);
	    strcat(tmpName,"a");
	    patch_get_env_attack (id[i], ppt_lst[j], &tmpf);
	    sprintf (tmp, "%f", tmpf);
	    xmlNewTextChild (xmlpatch, NULL, (const xmlChar *) tmpName,
			     (const xmlChar *) tmp);

	    strcpy(tmpName,ppt_names[j]);
	    strcat(tmpName,"env_on");
	    patch_get_env_on (id[i], ppt_lst[j], &tmpb);
	    if ( tmpb )
		sprintf (tmp, "%s", "yes");
	    else
		sprintf (tmp, "%s", "no");
	    xmlNewTextChild (xmlpatch, NULL, (const xmlChar *) tmpName,
			     (const xmlChar *) tmp);

	    strcpy(tmpName,ppt_names[j]);
	    strcat(tmpName,"amt");
	    patch_get_env_amount (id[i], ppt_lst[j], &tmpf);
	    sprintf (tmp, "%f", tmpf);
	    xmlNewTextChild (xmlpatch, NULL, (const xmlChar *) tmpName,
			     (const xmlChar *) tmp);

	    strcpy(tmpName,ppt_names[j]);
	    strcat(tmpName,"d");
	    patch_get_env_decay (id[i], ppt_lst[j], &tmpf);
	    sprintf (tmp, "%f", tmpf);
	    xmlNewTextChild (xmlpatch, NULL, (const xmlChar *) tmpName,
			     (const xmlChar *) tmp);

	    strcpy(tmpName,ppt_names[j]);
	    strcat(tmpName,"s");
	    patch_get_env_sustain (id[i], ppt_lst[j], &tmpf);
	    sprintf (tmp, "%f", tmpf);
	    xmlNewTextChild (xmlpatch, NULL, (const xmlChar *) tmpName,
			     (const xmlChar *) tmp);

	    strcpy(tmpName,ppt_names[j]);
	    strcat(tmpName,"r");
	    patch_get_env_release (id[i], ppt_lst[j], &tmpf);
	    sprintf (tmp, "%f", tmpf);
	    xmlNewTextChild (xmlpatch, NULL, (const xmlChar *) tmpName,
			     (const xmlChar *) tmp);
  
	    strcpy(tmpName,ppt_names[j]);
	    strcat(tmpName,"delay");
	    patch_get_env_delay (id[i], ppt_lst[j], &tmpf);
	    sprintf (tmp, "%f", tmpf);
	    xmlNewTextChild (xmlpatch, NULL, (const xmlChar *) tmpName,
			     (const xmlChar *) tmp);
  
	    strcpy(tmpName,ppt_names[j]);
	    strcat(tmpName,"hold");
	    patch_get_env_hold (id[i], ppt_lst[j], &tmpf);
	    sprintf (tmp, "%f", tmpf);
	    xmlNewTextChild (xmlpatch, NULL, (const xmlChar *) tmpName,
			     (const xmlChar *) tmp);
  
	    strcpy(tmpName,ppt_names[j]);
	    strcat(tmpName,"lfo_amt");
	    patch_get_lfo_amount (id[i], ppt_lst[j], &tmpf);
	    sprintf (tmp, "%f", tmpf);
	    xmlNewTextChild (xmlpatch, NULL, (const xmlChar *) tmpName,
			     (const xmlChar *) tmp);

	    strcpy(tmpName,ppt_names[j]);
	    strcat(tmpName,"lfo_on");
	    patch_get_lfo_on (id[i], ppt_lst[j], &tmpb);
	    if ( tmpb )
		sprintf (tmp, "%s", "yes");
	    else
		sprintf (tmp, "%s", "no");
	    xmlNewTextChild (xmlpatch, NULL, (const xmlChar *) tmpName,
			     (const xmlChar *) tmp);

	    strcpy(tmpName,ppt_names[j]);
	    strcat(tmpName,"vel_amt");
	    patch_get_vel_amount (id[i], ppt_lst[j], &tmpf);
	    sprintf (tmp, "%f", tmpf);
	    xmlNewTextChild (xmlpatch, NULL, (const xmlChar *) tmpName,
			     (const xmlChar *) tmp);

	    strcpy(tmpName,ppt_names[j]);
	    strcat(tmpName,"lfo_a");
	    patch_get_lfo_attack (id[i], ppt_lst[j], &tmpf);
	    sprintf (tmp, "%f", tmpf);
	    xmlNewTextChild (xmlpatch, NULL, (const xmlChar *) tmpName,
			     (const xmlChar *) tmp);

	    strcpy(tmpName,ppt_names[j]);
	    strcat(tmpName,"lfo_delay");
	    patch_get_lfo_delay (id[i], ppt_lst[j], &tmpf);
	    sprintf (tmp, "%f", tmpf);
	    xmlNewTextChild (xmlpatch, NULL, (const xmlChar *) tmpName,
			     (const xmlChar *) tmp);

	    strcpy(tmpName,ppt_names[j]);
	    strcat(tmpName,"lfo_beats");
	    patch_get_lfo_beats (id[i], ppt_lst[j], &tmpf);
	    sprintf (tmp, "%f", tmpf);
	    xmlNewTextChild (xmlpatch, NULL, (const xmlChar *) tmpName,
			     (const xmlChar *) tmp);

	    strcpy(tmpName,ppt_names[j]);
	    strcat(tmpName,"lfo_freq");
	    patch_get_lfo_freq (id[i], ppt_lst[j], &tmpf);
	    sprintf (tmp, "%f", tmpf);
	    xmlNewTextChild (xmlpatch, NULL, (const xmlChar *) tmpName,
			     (const xmlChar *) tmp);

	    strcpy(tmpName,ppt_names[j]);
	    strcat(tmpName,"lfo_global");
	    patch_get_lfo_global (id[i], ppt_lst[j], &tmpb);
	    if ( tmpb )
		sprintf (tmp, "%s", "yes");
	    else
		sprintf (tmp, "%s", "no");
	    xmlNewTextChild (xmlpatch, NULL, (const xmlChar *) tmpName,
			     (const xmlChar *) tmp);

	    strcpy(tmpName,ppt_names[j]);
	    strcat(tmpName,"lfo_sync");
	    patch_get_lfo_sync (id[i], ppt_lst[j], &tmpb);
	    if ( tmpb )
		sprintf (tmp, "%s", "yes");
	    else
		sprintf (tmp, "%s", "no");
	    xmlNewTextChild (xmlpatch, NULL, (const xmlChar *) tmpName,
			     (const xmlChar *) tmp);

	    strcpy(tmpName,ppt_names[j]);
	    strcat(tmpName,"lfo_positive");
	    patch_get_lfo_positive (id[i], ppt_lst[j], &tmpb);
	    if ( tmpb )
		sprintf (tmp, "%s", "yes");
	    else
		sprintf (tmp, "%s", "no");
	    xmlNewTextChild (xmlpatch, NULL, (const xmlChar *) tmpName,
			     (const xmlChar *) tmp);

	    strcpy(tmpName,ppt_names[j]);
	    strcat(tmpName,"lfo_shape");
	    patch_get_lfo_shape (id[i], ppt_lst[j], &tmpLS);
	    switch(tmpLS)
	    {
	    case LFO_SHAPE_SINE :
		sprintf (tmp, "%s", "sine");
		break;
	    case LFO_SHAPE_TRIANGLE :
		sprintf (tmp, "%s", "triangle");
		break;
	    case LFO_SHAPE_SAW :
		sprintf (tmp, "%s", "saw");
		break;
	    case LFO_SHAPE_SQUARE :
		sprintf (tmp, "%s", "square");
		break;
	    default:
		break;
	    }
	    xmlNewTextChild (xmlpatch, NULL, (const xmlChar *) tmpName,
			     (const xmlChar *) tmp);
	} /* end of param loop */
    } /* end of "walk the walk" */


    /* write the file to disk */
    val = xmlSaveFormatFile (name, doc, 1);
    xmlFreeDoc (doc);
#endif
    return -1;
}

int dish_file_read (char *path)
{
#ifdef WE_WANT_MORE_BEEF
    xmlDocPtr doc;
    xmlNodePtr rootnode;
    xmlNodePtr xmlpatch;
    xmlNodePtr cur;
    xmlChar *key;
    int id;

    debug ("Loading bank from file %s\n", path);

    doc = xmlParseFile (path);
    if (doc == NULL)
    {
	errmsg ("Failed to parse %s\n", path);
	return -1;
    }

    rootnode = xmlDocGetRootElement (doc);
    if (rootnode == NULL)
    {
	errmsg ("%s is empty\n", path);
	xmlFreeDoc (doc);
	return -1;
    }

    if (xmlStrcmp (rootnode->name, (const xmlChar *) "beef") != 0)
    {
	errmsg ("%s is not a valid 'beef' file\n", path);
	xmlFreeDoc (doc);
	return -1;
    }


    patch_destroy_all ( );
    for (xmlpatch = rootnode->xmlChildrenNode; xmlpatch != NULL;
	 xmlpatch = xmlpatch->next)
    {

	key = NULL;
	if (!xmlStrcmp (xmlpatch->name, (const xmlChar *) "patch"))
	{
	    id = patch_create ("Loading...");

	    for (cur = xmlpatch->xmlChildrenNode; cur != NULL;
		 cur = cur->next)
	    {
		if (!xmlStrcmp (cur->name, (const xmlChar *) "name"))
		{
		    key =
			xmlNodeListGetString (doc, cur->xmlChildrenNode,
					      1);
		    patch_set_name (id, (char *) key);

		}
		else if (!xmlStrcmp (cur->name, (const xmlChar *) "file"))
		{
		    key =
			xmlNodeListGetString (doc, cur->xmlChildrenNode,
					      1);
		    patch_sample_load (id, (char *) key);

		}
		else if (!xmlStrcmp
			 (cur->name, (const xmlChar *) "channel"))
		{
		    key =
			xmlNodeListGetString (doc, cur->xmlChildrenNode,
					      1);
		    patch_set_channel (id, atoi ((const char *) key));

		}
		else if (!xmlStrcmp (cur->name, (const xmlChar *) "note"))
		{
		    key =
			xmlNodeListGetString (doc, cur->xmlChildrenNode,
					      1);
		    patch_set_note (id, atoi ((const char *) key));

		}
		else if (!xmlStrcmp
			 (cur->name, (const xmlChar *) "amplitude"))
		{
		    key =
			xmlNodeListGetString (doc, cur->xmlChildrenNode,
					      1);
		    patch_set_amplitude (id, atof ((const char *) key));

		}
		else if (!xmlStrcmp (cur->name, (const xmlChar *) "pan"))
		{
		    key =
			xmlNodeListGetString (doc, cur->xmlChildrenNode,
					      1);
		    patch_set_panning (id, atof ((const char *) key));

		}
		else if (!xmlStrcmp
			 (cur->name, (const xmlChar *) "play_mode"))
		{
		    key =
			xmlNodeListGetString (doc, cur->xmlChildrenNode,
					      1);
		    patch_set_play_mode (id, atoi ((const char *) key));

		}
		else if (!xmlStrcmp (cur->name, (const xmlChar *) "cut"))
		{
		    key =
			xmlNodeListGetString (doc, cur->xmlChildrenNode,
					      1);
		    patch_set_cut (id, atoi ((const char *) key));

		}
		else if (!xmlStrcmp
			 (cur->name, (const xmlChar *) "cut_by"))
		{
		    key =
			xmlNodeListGetString (doc, cur->xmlChildrenNode,
					      1);
		    patch_set_cut_by (id, atoi ((const char *) key));

		}
		else if (!xmlStrcmp (cur->name, (const xmlChar *) "range"))
		{
		    key =
			xmlNodeListGetString (doc, cur->xmlChildrenNode,
					      1);
		    patch_set_range (id, atoi ((const char *) key));

		}
		else if (!xmlStrcmp
			 (cur->name, (const xmlChar *) "lower_note"))
		{
		    key =
			xmlNodeListGetString (doc, cur->xmlChildrenNode,
					      1);
		    patch_set_lower_note (id, atoi ((const char *) key));

		}
		else if (!xmlStrcmp
			 (cur->name, (const xmlChar *) "upper_note"))
		{
		    key =
			xmlNodeListGetString (doc, cur->xmlChildrenNode,
					      1);
		    patch_set_upper_note (id, atoi ((const char *) key));

		}
		else if (!xmlStrcmp
			 (cur->name, (const xmlChar *) "sample_start"))
		{
		    key =
			xmlNodeListGetString (doc, cur->xmlChildrenNode,
					      1);
		    patch_set_sample_start (id, atoi ((const char *) key));

		}
		else if (!xmlStrcmp
			 (cur->name, (const xmlChar *) "sample_stop"))
		{
		    key =
			xmlNodeListGetString (doc, cur->xmlChildrenNode,
					      1);
		    patch_set_sample_stop (id, atoi ((const char *) key));

		}
		else if (!xmlStrcmp
			 (cur->name, (const xmlChar *) "loop_start"))
		{
		    key =
			xmlNodeListGetString (doc, cur->xmlChildrenNode,
					      1);
		    patch_set_loop_start (id, atoi ((const char *) key));

		}
		else if (!xmlStrcmp
			 (cur->name, (const xmlChar *) "loop_stop"))
		{
		    key =
			xmlNodeListGetString (doc, cur->xmlChildrenNode,
					      1);
		    patch_set_loop_stop (id, atoi ((const char *) key));

		}
		else if (!xmlStrcmp
			 (cur->name, (const xmlChar *) "cutoff"))
		{
		    key =
			xmlNodeListGetString (doc, cur->xmlChildrenNode,
					      1);
		    patch_set_cutoff (id, atof ((const char *) key));

		}
		else if (!xmlStrcmp
			 (cur->name, (const xmlChar *) "portamento"))
		{
		    key =
			xmlNodeListGetString (doc, cur->xmlChildrenNode,
					      1);
		    patch_set_portamento (id, 
					  xmlStrcmp (key, (const xmlChar *) "yes") == 0);

		}
		else if (!xmlStrcmp
			 (cur->name, (const xmlChar *) "portamento_time"))
		{
		    key =
			xmlNodeListGetString (doc, cur->xmlChildrenNode,
					      1);
		    patch_set_portamento_time (id, atof ((const char *) key));

		}
		else if (!xmlStrcmp
			 (cur->name, (const xmlChar *) "monophonic"))
		{
		    key =
			xmlNodeListGetString (doc, cur->xmlChildrenNode,
					      1);
		    patch_set_monophonic (id, 
					  xmlStrcmp (key, (const xmlChar *) "yes") == 0);

		}
		else if (!xmlStrcmp
			 (cur->name, (const xmlChar *) "legato"))
		{
		    key =
			xmlNodeListGetString (doc, cur->xmlChildrenNode,
					      1);
		    patch_set_legato (id, 
				      xmlStrcmp (key, (const xmlChar *) "yes") == 0);

		}
		else if (!xmlStrcmp
			 (cur->name, (const xmlChar *) "resonance"))
		{
		    key =
			xmlNodeListGetString (doc, cur->xmlChildrenNode,
					      1);
		    patch_set_resonance (id, atof ((const char *) key));

		}
		else if (!xmlStrcmp
			 (cur->name, (const xmlChar *) "resonance_a"))
		{
		    key =
			xmlNodeListGetString (doc, cur->xmlChildrenNode,
					      1);
		    patch_set_env_attack (id, PATCH_PARAM_RESONANCE,
					  atof ((const char *) key));

		}
		else if (!xmlStrcmp
			 (cur->name, (const xmlChar *) "resonance_d"))
		{
		    key =
			xmlNodeListGetString (doc, cur->xmlChildrenNode,
					      1);
		    patch_set_env_decay (id, PATCH_PARAM_RESONANCE,
					 atof ((const char *) key));

		}
		else if (!xmlStrcmp
			 (cur->name, (const xmlChar *) "resonance_delay"))
		{
		    key =
			xmlNodeListGetString (doc, cur->xmlChildrenNode,
					      1);
		    patch_set_env_delay (id, PATCH_PARAM_RESONANCE,
					 atof ((const char *) key));

		}
		else if (!xmlStrcmp
			 (cur->name, (const xmlChar *) "resonance_s"))
		{
		    key =
			xmlNodeListGetString (doc, cur->xmlChildrenNode,
					      1);
		    patch_set_env_sustain (id, PATCH_PARAM_RESONANCE,
					   atof ((const char *) key));

		}
		else if (!xmlStrcmp
			 (cur->name, (const xmlChar *) "resonance_r"))
		{
		    key =
			xmlNodeListGetString (doc, cur->xmlChildrenNode,
					      1);
		    patch_set_env_release (id, PATCH_PARAM_RESONANCE,
					   atof ((const char *) key));

		}
		else if (!xmlStrcmp
			 (cur->name, (const xmlChar *) "resonance_hold"))
		{
		    key =
			xmlNodeListGetString (doc, cur->xmlChildrenNode,
					      1);
		    patch_set_env_hold (id, PATCH_PARAM_RESONANCE,
					atof ((const char *) key));

		}
		else if (!xmlStrcmp
			 (cur->name, (const xmlChar *) "resonance_amt"))
		{
		    key =
			xmlNodeListGetString (doc, cur->xmlChildrenNode,
					      1);
		    patch_set_env_amount (id, PATCH_PARAM_RESONANCE,
					  atof ((const char *) key));

		}
		else if (!xmlStrcmp
			 (cur->name, (const xmlChar *) "resonance_env_on"))
		{
		    key =
			xmlNodeListGetString (doc, cur->xmlChildrenNode,
					      1);
		    patch_set_env_on (id, PATCH_PARAM_RESONANCE,
				      xmlStrcmp (key, (const xmlChar *) "yes") == 0);

		}
		else if (!xmlStrcmp
			 (cur->name, (const xmlChar *) "resonance_lfo_on"))
		{
		    key =
			xmlNodeListGetString (doc, cur->xmlChildrenNode,
					      1);
		    patch_set_lfo_on (id, PATCH_PARAM_RESONANCE,
				      xmlStrcmp (key, (const xmlChar *) "yes") == 0);

		}
		else if (!xmlStrcmp
			 (cur->name, (const xmlChar *) "resonance_vel_amt"))
		{
		    key =
			xmlNodeListGetString (doc, cur->xmlChildrenNode,
					      1);
		    patch_set_vel_amount (id, PATCH_PARAM_RESONANCE,
					  atof ((const char *) key));

		}
		else if (!xmlStrcmp
			 (cur->name, (const xmlChar *) "resonance_lfo_a"))
		{
		    key =
			xmlNodeListGetString (doc, cur->xmlChildrenNode,
					      1);
		    patch_set_lfo_attack (id, PATCH_PARAM_RESONANCE,
					  atof ((const char *) key));

		}
		else if (!xmlStrcmp
			 (cur->name, (const xmlChar *) "resonance_lfo_delay"))
		{
		    key =
			xmlNodeListGetString (doc, cur->xmlChildrenNode,
					      1);
		    patch_set_lfo_delay (id, PATCH_PARAM_RESONANCE,
					 atof ((const char *) key));

		}
		else if (!xmlStrcmp
			 (cur->name, (const xmlChar *) "resonance_lfo_amt"))
		{
		    key =
			xmlNodeListGetString (doc, cur->xmlChildrenNode,
					      1);
		    patch_set_lfo_amount (id, PATCH_PARAM_RESONANCE,
					  atof ((const char *) key));

		}
		else if (!xmlStrcmp
			 (cur->name, (const xmlChar *) "resonance_lfo_beats"))
		{
		    key =
			xmlNodeListGetString (doc, cur->xmlChildrenNode,
					      1);
		    patch_set_lfo_beats (id, PATCH_PARAM_RESONANCE,
					 atof ((const char *) key));

		}
		else if (!xmlStrcmp
			 (cur->name, (const xmlChar *) "resonance_lfo_freq"))
		{
		    key =
			xmlNodeListGetString (doc, cur->xmlChildrenNode,
					      1);
		    patch_set_lfo_freq (id, PATCH_PARAM_RESONANCE,
					atof ((const char *) key));

		}
		else if (!xmlStrcmp
			 (cur->name, (const xmlChar *) "resonance_lfo_positive"))
		{
		    key =
			xmlNodeListGetString (doc, cur->xmlChildrenNode,
					      1);
		    patch_set_lfo_positive (id, PATCH_PARAM_RESONANCE,
					    xmlStrcmp (key, (const xmlChar *) "yes") == 0);

		}
		else if (!xmlStrcmp
			 (cur->name, (const xmlChar *) "resonance_lfo_global"))
		{
		    key =
			xmlNodeListGetString (doc, cur->xmlChildrenNode,
					      1);
		    patch_set_lfo_global (id, PATCH_PARAM_RESONANCE,
					  xmlStrcmp (key, (const xmlChar *) "yes") == 0);

		}
		else if (!xmlStrcmp
			 (cur->name, (const xmlChar *) "resonance_lfo_sync"))
		{
		    key =
			xmlNodeListGetString (doc, cur->xmlChildrenNode,
					      1);
		    patch_set_lfo_sync (id, PATCH_PARAM_RESONANCE,
					xmlStrcmp (key, (const xmlChar *) "yes") == 0);

		}
		else if (!xmlStrcmp
			 (cur->name, (const xmlChar *) "resonance_lfo_shape"))
		{
		    key =
			xmlNodeListGetString (doc, cur->xmlChildrenNode,
					      1);
		    if (xmlStrcmp (key, (const xmlChar *) "sine") == 0)
			patch_set_lfo_shape (id, PATCH_PARAM_RESONANCE,
					     LFO_SHAPE_SINE);
		    else if (xmlStrcmp (key, (const xmlChar *) "triangle") == 0)
			patch_set_lfo_shape (id, PATCH_PARAM_RESONANCE,
					     LFO_SHAPE_TRIANGLE);
		    else if (xmlStrcmp (key, (const xmlChar *) "saw") == 0)
			patch_set_lfo_shape (id, PATCH_PARAM_RESONANCE,
					     LFO_SHAPE_SAW);
		    else if (xmlStrcmp (key, (const xmlChar *) "square") == 0)
			patch_set_lfo_shape (id, PATCH_PARAM_RESONANCE,
					     LFO_SHAPE_SQUARE);

		}
		else if (!xmlStrcmp
			 (cur->name, (const xmlChar *) "amplitude_a"))
		{
		    key =
			xmlNodeListGetString (doc, cur->xmlChildrenNode,
					      1);
		    patch_set_env_attack (id, PATCH_PARAM_AMPLITUDE,
					  atof ((const char *) key));

		}
		else if (!xmlStrcmp
			 (cur->name, (const xmlChar *) "amplitude_d"))
		{
		    key =
			xmlNodeListGetString (doc, cur->xmlChildrenNode,
					      1);
		    patch_set_env_decay (id, PATCH_PARAM_AMPLITUDE,
					 atof ((const char *) key));

		}
		else if (!xmlStrcmp
			 (cur->name, (const xmlChar *) "amplitude_delay"))
		{
		    key =
			xmlNodeListGetString (doc, cur->xmlChildrenNode,
					      1);
		    patch_set_env_delay (id, PATCH_PARAM_AMPLITUDE,
					 atof ((const char *) key));

		}
		else if (!xmlStrcmp
			 (cur->name, (const xmlChar *) "amplitude_s"))
		{
		    key =
			xmlNodeListGetString (doc, cur->xmlChildrenNode,
					      1);
		    patch_set_env_sustain (id, PATCH_PARAM_AMPLITUDE,
					   atof ((const char *) key));

		}
		else if (!xmlStrcmp
			 (cur->name, (const xmlChar *) "amplitude_r"))
		{
		    key =
			xmlNodeListGetString (doc, cur->xmlChildrenNode,
					      1);
		    patch_set_env_release (id, PATCH_PARAM_AMPLITUDE,
					   atof ((const char *) key));

		}
		else if (!xmlStrcmp
			 (cur->name, (const xmlChar *) "amplitude_hold"))
		{
		    key =
			xmlNodeListGetString (doc, cur->xmlChildrenNode,
					      1);
		    patch_set_env_hold (id, PATCH_PARAM_AMPLITUDE,
					atof ((const char *) key));

		}
		else if (!xmlStrcmp
			 (cur->name, (const xmlChar *) "amplitude_amt"))
		{
		    key =
			xmlNodeListGetString (doc, cur->xmlChildrenNode,
					      1);
		    patch_set_env_amount (id, PATCH_PARAM_AMPLITUDE,
					  atof ((const char *) key));

		}
		else if (!xmlStrcmp
			 (cur->name, (const xmlChar *) "amplitude_vel_amt"))
		{
		    key =
			xmlNodeListGetString (doc, cur->xmlChildrenNode,
					      1);
		    patch_set_vel_amount (id, PATCH_PARAM_AMPLITUDE,
					  atof ((const char *) key));

		}
		else if (!xmlStrcmp
			 (cur->name, (const xmlChar *) "amplitude_env_on"))
		{
		    key =
			xmlNodeListGetString (doc, cur->xmlChildrenNode,
					      1);
		    patch_set_env_on (id, PATCH_PARAM_AMPLITUDE,
				      xmlStrcmp (key, (const xmlChar *) "yes") == 0);

		}
		else if (!xmlStrcmp
			 (cur->name, (const xmlChar *) "amplitude_lfo_a"))
		{
		    key =
			xmlNodeListGetString (doc, cur->xmlChildrenNode,
					      1);
		    patch_set_lfo_attack (id, PATCH_PARAM_AMPLITUDE,
					  atof ((const char *) key));

		}
		else if (!xmlStrcmp
			 (cur->name, (const xmlChar *) "amplitude_lfo_delay"))
		{
		    key =
			xmlNodeListGetString (doc, cur->xmlChildrenNode,
					      1);
		    patch_set_lfo_delay (id, PATCH_PARAM_AMPLITUDE,
					 atof ((const char *) key));

		}
		else if (!xmlStrcmp
			 (cur->name, (const xmlChar *) "amplitude_lfo_amt"))
		{
		    key =
			xmlNodeListGetString (doc, cur->xmlChildrenNode,
					      1);
		    patch_set_lfo_amount (id, PATCH_PARAM_AMPLITUDE,
					  atof ((const char *) key));

		}
		else if (!xmlStrcmp
			 (cur->name, (const xmlChar *) "amplitude_lfo_on"))
		{
		    key =
			xmlNodeListGetString (doc, cur->xmlChildrenNode,
					      1);
		    patch_set_lfo_on (id, PATCH_PARAM_AMPLITUDE,
				      xmlStrcmp (key, (const xmlChar *) "yes") == 0);

		}
		else if (!xmlStrcmp
			 (cur->name, (const xmlChar *) "amplitude_lfo_beats"))
		{
		    key =
			xmlNodeListGetString (doc, cur->xmlChildrenNode,
					      1);
		    patch_set_lfo_beats (id, PATCH_PARAM_AMPLITUDE,
					 atof ((const char *) key));

		}
		else if (!xmlStrcmp
			 (cur->name, (const xmlChar *) "amplitude_lfo_freq"))
		{
		    key =
			xmlNodeListGetString (doc, cur->xmlChildrenNode,
					      1);
		    patch_set_lfo_freq (id, PATCH_PARAM_AMPLITUDE,
					atof ((const char *) key));

		}
		else if (!xmlStrcmp
			 (cur->name, (const xmlChar *) "amplitude_lfo_positive"))
		{
		    key =
			xmlNodeListGetString (doc, cur->xmlChildrenNode,
					      1);
		    patch_set_lfo_positive (id, PATCH_PARAM_AMPLITUDE,
					    xmlStrcmp (key, (const xmlChar *) "yes") == 0);

		}
		else if (!xmlStrcmp
			 (cur->name, (const xmlChar *) "amplitude_lfo_global"))
		{
		    key =
			xmlNodeListGetString (doc, cur->xmlChildrenNode,
					      1);
		    patch_set_lfo_global (id, PATCH_PARAM_AMPLITUDE,
					  xmlStrcmp (key, (const xmlChar *) "yes") == 0);

		}
		else if (!xmlStrcmp
			 (cur->name, (const xmlChar *) "amplitude_lfo_sync"))
		{
		    key =
			xmlNodeListGetString (doc, cur->xmlChildrenNode,
					      1);
		    patch_set_lfo_sync (id, PATCH_PARAM_AMPLITUDE,
					xmlStrcmp (key, (const xmlChar *) "yes") == 0);

		}
		else if (!xmlStrcmp
			 (cur->name, (const xmlChar *) "amplitude_lfo_shape"))
		{
		    key =
			xmlNodeListGetString (doc, cur->xmlChildrenNode,
					      1);
		    if (xmlStrcmp (key, (const xmlChar *) "sine") == 0)
			patch_set_lfo_shape (id, PATCH_PARAM_AMPLITUDE,
					     LFO_SHAPE_SINE);
		    else if (xmlStrcmp (key, (const xmlChar *) "triangle") == 0)
			patch_set_lfo_shape (id, PATCH_PARAM_AMPLITUDE,
					     LFO_SHAPE_TRIANGLE);
		    else if (xmlStrcmp (key, (const xmlChar *) "saw") == 0)
			patch_set_lfo_shape (id, PATCH_PARAM_AMPLITUDE,
					     LFO_SHAPE_SAW);
		    else if (xmlStrcmp (key, (const xmlChar *) "square") == 0)
			patch_set_lfo_shape (id, PATCH_PARAM_AMPLITUDE,
					     LFO_SHAPE_SQUARE);

		}
		else if (!xmlStrcmp
			 (cur->name, (const xmlChar *) "pitch"))
		{
		    key =
			xmlNodeListGetString (doc, cur->xmlChildrenNode,
					      1);
		    patch_set_pitch (id, atof ((const char *) key));

		}
		else if (!xmlStrcmp
			 (cur->name, (const xmlChar *) "pitch_steps"))
		{
		    key = xmlNodeListGetString (doc, cur->xmlChildrenNode, 1);
		    patch_set_pitch_steps (id, atoi ((const char *) key));

		}
		else if (!xmlStrcmp
			 (cur->name, (const xmlChar *) "pitch_a"))
		{
		    key =
			xmlNodeListGetString (doc, cur->xmlChildrenNode,
					      1);
		    patch_set_env_attack (id, PATCH_PARAM_PITCH,
					  atof ((const char *) key));

		}
		else if (!xmlStrcmp
			 (cur->name, (const xmlChar *) "pitch_d"))
		{
		    key =
			xmlNodeListGetString (doc, cur->xmlChildrenNode,
					      1);
		    patch_set_env_decay (id, PATCH_PARAM_PITCH,
					 atof ((const char *) key));

		}
		else if (!xmlStrcmp
			 (cur->name, (const xmlChar *) "pitch_delay"))
		{
		    key =
			xmlNodeListGetString (doc, cur->xmlChildrenNode,
					      1);
		    patch_set_env_delay (id, PATCH_PARAM_PITCH,
					 atof ((const char *) key));

		}
		else if (!xmlStrcmp
			 (cur->name, (const xmlChar *) "pitch_s"))
		{
		    key =
			xmlNodeListGetString (doc, cur->xmlChildrenNode,
					      1);
		    patch_set_env_sustain (id, PATCH_PARAM_PITCH,
					   atof ((const char *) key));

		}
		else if (!xmlStrcmp
			 (cur->name, (const xmlChar *) "pitch_r"))
		{
		    key =
			xmlNodeListGetString (doc, cur->xmlChildrenNode,
					      1);
		    patch_set_env_release (id, PATCH_PARAM_PITCH,
					   atof ((const char *) key));

		}
		else if (!xmlStrcmp
			 (cur->name, (const xmlChar *) "pitch_hold"))
		{
		    key =
			xmlNodeListGetString (doc, cur->xmlChildrenNode,
					      1);
		    patch_set_env_hold (id, PATCH_PARAM_PITCH,
					atof ((const char *) key));

		}
		else if (!xmlStrcmp
			 (cur->name, (const xmlChar *) "pitch_amt"))
		{
		    key =
			xmlNodeListGetString (doc, cur->xmlChildrenNode,
					      1);
		    patch_set_env_amount (id, PATCH_PARAM_PITCH,
					  atof ((const char *) key));

		}
		else if (!xmlStrcmp
			 (cur->name, (const xmlChar *) "pitch_env_on"))
		{
		    key =
			xmlNodeListGetString (doc, cur->xmlChildrenNode,
					      1);
		    patch_set_env_on (id, PATCH_PARAM_PITCH,
				      xmlStrcmp (key, (const xmlChar *) "yes") == 0);

		}
		else if (!xmlStrcmp
			 (cur->name, (const xmlChar *) "pitch_lfo_a"))
		{
		    key =
			xmlNodeListGetString (doc, cur->xmlChildrenNode,
					      1);
		    patch_set_lfo_attack (id, PATCH_PARAM_PITCH,
					  atof ((const char *) key));

		}
		else if (!xmlStrcmp
			 (cur->name, (const xmlChar *) "pitch_vel_amt"))
		{
		    key =
			xmlNodeListGetString (doc, cur->xmlChildrenNode,
					      1);
		    patch_set_vel_amount (id, PATCH_PARAM_PITCH,
					  atof ((const char *) key));

		}
		else if (!xmlStrcmp
			 (cur->name, (const xmlChar *) "pitch_lfo_delay"))
		{
		    key =
			xmlNodeListGetString (doc, cur->xmlChildrenNode,
					      1);
		    patch_set_lfo_delay (id, PATCH_PARAM_PITCH,
					 atof ((const char *) key));

		}
		else if (!xmlStrcmp
			 (cur->name, (const xmlChar *) "pitch_lfo_amt"))
		{
		    key =
			xmlNodeListGetString (doc, cur->xmlChildrenNode,
					      1);
		    patch_set_lfo_amount (id, PATCH_PARAM_PITCH,
					  atof ((const char *) key));

		}
		else if (!xmlStrcmp
			 (cur->name, (const xmlChar *) "pitch_lfo_on"))
		{
		    key =
			xmlNodeListGetString (doc, cur->xmlChildrenNode,
					      1);
		    patch_set_lfo_on (id, PATCH_PARAM_PITCH,
				      xmlStrcmp (key, (const xmlChar *) "yes") == 0);

		}
		else if (!xmlStrcmp
			 (cur->name, (const xmlChar *) "pitch_lfo_beats"))
		{
		    key =
			xmlNodeListGetString (doc, cur->xmlChildrenNode,
					      1);
		    patch_set_lfo_beats (id, PATCH_PARAM_PITCH,
					 atof ((const char *) key));

		}
		else if (!xmlStrcmp
			 (cur->name, (const xmlChar *) "pitch_lfo_freq"))
		{
		    key =
			xmlNodeListGetString (doc, cur->xmlChildrenNode,
					      1);
		    patch_set_lfo_freq (id, PATCH_PARAM_PITCH,
					atof ((const char *) key));

		}
		else if (!xmlStrcmp
			 (cur->name, (const xmlChar *) "pitch_lfo_positive"))
		{
		    key =
			xmlNodeListGetString (doc, cur->xmlChildrenNode,
					      1);
		    patch_set_lfo_positive (id, PATCH_PARAM_PITCH,
					    xmlStrcmp (key, (const xmlChar *) "yes") == 0);

		}
		else if (!xmlStrcmp
			 (cur->name, (const xmlChar *) "pitch_lfo_global"))
		{
		    key =
			xmlNodeListGetString (doc, cur->xmlChildrenNode,
					      1);
		    patch_set_lfo_global (id, PATCH_PARAM_PITCH,
					  xmlStrcmp (key, (const xmlChar *) "yes") == 0);

		}
		else if (!xmlStrcmp
			 (cur->name, (const xmlChar *) "pitch_lfo_sync"))
		{
		    key =
			xmlNodeListGetString (doc, cur->xmlChildrenNode,
					      1);
		    patch_set_lfo_sync (id, PATCH_PARAM_PITCH,
					xmlStrcmp (key, (const xmlChar *) "yes") == 0);

		}
		else if (!xmlStrcmp
			 (cur->name, (const xmlChar *) "pitch_lfo_shape"))
		{
		    key =
			xmlNodeListGetString (doc, cur->xmlChildrenNode,
					      1);
		    if (xmlStrcmp (key, (const xmlChar *) "sine") == 0)
			patch_set_lfo_shape (id, PATCH_PARAM_PITCH,
					     LFO_SHAPE_SINE);
		    else if (xmlStrcmp (key, (const xmlChar *) "triangle") == 0)
			patch_set_lfo_shape (id, PATCH_PARAM_PITCH,
					     LFO_SHAPE_TRIANGLE);
		    else if (xmlStrcmp (key, (const xmlChar *) "saw") == 0)
			patch_set_lfo_shape (id, PATCH_PARAM_PITCH,
					     LFO_SHAPE_SAW);
		    else if (xmlStrcmp (key, (const xmlChar *) "square") == 0)
			patch_set_lfo_shape (id, PATCH_PARAM_PITCH,
					     LFO_SHAPE_SQUARE);

		}
		else if (!xmlStrcmp
			 (cur->name, (const xmlChar *) "panning_a"))
		{
		    key =
			xmlNodeListGetString (doc, cur->xmlChildrenNode,
					      1);
		    patch_set_env_attack (id, PATCH_PARAM_PANNING,
					  atof ((const char *) key));

		}
		else if (!xmlStrcmp
			 (cur->name, (const xmlChar *) "panning_d"))
		{
		    key =
			xmlNodeListGetString (doc, cur->xmlChildrenNode,
					      1);
		    patch_set_env_decay (id, PATCH_PARAM_PANNING,
					 atof ((const char *) key));

		}
		else if (!xmlStrcmp
			 (cur->name, (const xmlChar *) "panning_delay"))
		{
		    key =
			xmlNodeListGetString (doc, cur->xmlChildrenNode,
					      1);
		    patch_set_env_delay (id, PATCH_PARAM_PANNING,
					 atof ((const char *) key));

		}
		else if (!xmlStrcmp
			 (cur->name, (const xmlChar *) "panning_s"))
		{
		    key =
			xmlNodeListGetString (doc, cur->xmlChildrenNode,
					      1);
		    patch_set_env_sustain (id, PATCH_PARAM_PANNING,
					   atof ((const char *) key));

		}
		else if (!xmlStrcmp
			 (cur->name, (const xmlChar *) "panning_r"))
		{
		    key =
			xmlNodeListGetString (doc, cur->xmlChildrenNode,
					      1);
		    patch_set_env_release (id, PATCH_PARAM_PANNING,
					   atof ((const char *) key));

		}
		else if (!xmlStrcmp
			 (cur->name, (const xmlChar *) "panning_hold"))
		{
		    key =
			xmlNodeListGetString (doc, cur->xmlChildrenNode,
					      1);
		    patch_set_env_hold (id, PATCH_PARAM_PANNING,
					atof ((const char *) key));

		}
		else if (!xmlStrcmp
			 (cur->name, (const xmlChar *) "panning_amt"))
		{
		    key =
			xmlNodeListGetString (doc, cur->xmlChildrenNode,
					      1);
		    patch_set_env_amount (id, PATCH_PARAM_PANNING,
					  atof ((const char *) key));

		}
		else if (!xmlStrcmp
			 (cur->name, (const xmlChar *) "panning_env_on"))
		{
		    key =
			xmlNodeListGetString (doc, cur->xmlChildrenNode,
					      1);
		    patch_set_env_on (id, PATCH_PARAM_PANNING,
				      xmlStrcmp (key, (const xmlChar *) "yes") == 0);

		}
		else if (!xmlStrcmp
			 (cur->name, (const xmlChar *) "panning_vel_amt"))
		{
		    key =
			xmlNodeListGetString (doc, cur->xmlChildrenNode,
					      1);
		    patch_set_vel_amount (id, PATCH_PARAM_PANNING,
					  atof ((const char *) key));

		}
		else if (!xmlStrcmp
			 (cur->name, (const xmlChar *) "panning_lfo_a"))
		{
		    key =
			xmlNodeListGetString (doc, cur->xmlChildrenNode,
					      1);
		    patch_set_lfo_attack (id, PATCH_PARAM_PANNING,
					  atof ((const char *) key));

		}
		else if (!xmlStrcmp
			 (cur->name, (const xmlChar *) "panning_lfo_delay"))
		{
		    key =
			xmlNodeListGetString (doc, cur->xmlChildrenNode,
					      1);
		    patch_set_lfo_delay (id, PATCH_PARAM_PANNING,
					 atof ((const char *) key));

		}
		else if (!xmlStrcmp
			 (cur->name, (const xmlChar *) "panning_lfo_amt"))
		{
		    key =
			xmlNodeListGetString (doc, cur->xmlChildrenNode,
					      1);
		    patch_set_lfo_amount (id, PATCH_PARAM_PANNING,
					  atof ((const char *) key));

		}
		else if (!xmlStrcmp
			 (cur->name, (const xmlChar *) "panning_lfo_on"))
		{
		    key =
			xmlNodeListGetString (doc, cur->xmlChildrenNode,
					      1);
		    patch_set_lfo_on (id, PATCH_PARAM_PANNING,
				      xmlStrcmp (key, (const xmlChar *) "yes") == 0);

		}
		else if (!xmlStrcmp
			 (cur->name, (const xmlChar *) "panning_lfo_beats"))
		{
		    key =
			xmlNodeListGetString (doc, cur->xmlChildrenNode,
					      1);
		    patch_set_lfo_beats (id, PATCH_PARAM_PANNING,
					 atof ((const char *) key));

		}
		else if (!xmlStrcmp
			 (cur->name, (const xmlChar *) "panning_lfo_freq"))
		{
		    key =
			xmlNodeListGetString (doc, cur->xmlChildrenNode,
					      1);
		    patch_set_lfo_freq (id, PATCH_PARAM_PANNING,
					atof ((const char *) key));

		}
		else if (!xmlStrcmp
			 (cur->name, (const xmlChar *) "panning_lfo_positive"))
		{
		    key =
			xmlNodeListGetString (doc, cur->xmlChildrenNode,
					      1);
		    patch_set_lfo_positive (id, PATCH_PARAM_PANNING,
					    xmlStrcmp (key, (const xmlChar *) "yes") == 0);

		}
		else if (!xmlStrcmp
			 (cur->name, (const xmlChar *) "panning_lfo_global"))
		{
		    key =
			xmlNodeListGetString (doc, cur->xmlChildrenNode,
					      1);
		    patch_set_lfo_global (id, PATCH_PARAM_PANNING,
					  xmlStrcmp (key, (const xmlChar *) "yes") == 0);

		}
		else if (!xmlStrcmp
			 (cur->name, (const xmlChar *) "panning_lfo_sync"))
		{
		    key =
			xmlNodeListGetString (doc, cur->xmlChildrenNode,
					      1);
		    patch_set_lfo_sync (id, PATCH_PARAM_PANNING,
					xmlStrcmp (key, (const xmlChar *) "yes") == 0);

		}
		else if (!xmlStrcmp
			 (cur->name, (const xmlChar *) "panning_lfo_shape"))
		{
		    key =
			xmlNodeListGetString (doc, cur->xmlChildrenNode,
					      1);
		    if (xmlStrcmp (key, (const xmlChar *) "sine") == 0)
			patch_set_lfo_shape (id, PATCH_PARAM_PANNING,
					     LFO_SHAPE_SINE);
		    else if (xmlStrcmp (key, (const xmlChar *) "triangle") == 0)
			patch_set_lfo_shape (id, PATCH_PARAM_PANNING,
					     LFO_SHAPE_TRIANGLE);
		    else if (xmlStrcmp (key, (const xmlChar *) "saw") == 0)
			patch_set_lfo_shape (id, PATCH_PARAM_PANNING,
					     LFO_SHAPE_SAW);
		    else if (xmlStrcmp (key, (const xmlChar *) "square") == 0)
			patch_set_lfo_shape (id, PATCH_PARAM_PANNING,
					     LFO_SHAPE_SQUARE);

		}
		else if (!xmlStrcmp
			 (cur->name, (const xmlChar *) "cutoff_a"))
		{
		    key =
			xmlNodeListGetString (doc, cur->xmlChildrenNode,
					      1);
		    patch_set_env_attack (id, PATCH_PARAM_CUTOFF,
					  atof ((const char *) key));

		}
		else if (!xmlStrcmp
			 (cur->name, (const xmlChar *) "cutoff_d"))
		{
		    key =
			xmlNodeListGetString (doc, cur->xmlChildrenNode,
					      1);
		    patch_set_env_decay (id, PATCH_PARAM_CUTOFF,
					 atof ((const char *) key));

		}
		else if (!xmlStrcmp
			 (cur->name, (const xmlChar *) "cutoff_delay"))
		{
		    key =
			xmlNodeListGetString (doc, cur->xmlChildrenNode,
					      1);
		    patch_set_env_delay (id, PATCH_PARAM_CUTOFF,
					 atof ((const char *) key));

		}
		else if (!xmlStrcmp
			 (cur->name, (const xmlChar *) "cutoff_s"))
		{
		    key =
			xmlNodeListGetString (doc, cur->xmlChildrenNode,
					      1);
		    patch_set_env_sustain (id, PATCH_PARAM_CUTOFF,
					   atof ((const char *) key));

		}
		else if (!xmlStrcmp
			 (cur->name, (const xmlChar *) "cutoff_r"))
		{
		    key =
			xmlNodeListGetString (doc, cur->xmlChildrenNode,
					      1);
		    patch_set_env_release (id, PATCH_PARAM_CUTOFF,
					   atof ((const char *) key));

		}
		else if (!xmlStrcmp
			 (cur->name, (const xmlChar *) "cutoff_hold"))
		{
		    key =
			xmlNodeListGetString (doc, cur->xmlChildrenNode,
					      1);
		    patch_set_env_hold (id, PATCH_PARAM_CUTOFF,
					atof ((const char *) key));

		}
		else if (!xmlStrcmp
			 (cur->name, (const xmlChar *) "cutoff_amt"))
		{
		    key =
			xmlNodeListGetString (doc, cur->xmlChildrenNode,
					      1);
		    patch_set_env_amount (id, PATCH_PARAM_CUTOFF,
					  atof ((const char *) key));

		}
		else if (!xmlStrcmp
			 (cur->name, (const xmlChar *) "cutoff_env_on"))
		{
		    key =
			xmlNodeListGetString (doc, cur->xmlChildrenNode,
					      1);
		    patch_set_env_on (id, PATCH_PARAM_CUTOFF,
				      xmlStrcmp (key, (const xmlChar *) "yes") == 0);

		}
		else if (!xmlStrcmp
			 (cur->name, (const xmlChar *) "cutoff_vel_amt"))
		{
		    key =
			xmlNodeListGetString (doc, cur->xmlChildrenNode,
					      1);
		    patch_set_vel_amount (id, PATCH_PARAM_CUTOFF,
					  atof ((const char *) key));

		}
		else if (!xmlStrcmp
			 (cur->name, (const xmlChar *) "cutoff_lfo_a"))
		{
		    key =
			xmlNodeListGetString (doc, cur->xmlChildrenNode,
					      1);
		    patch_set_lfo_attack (id, PATCH_PARAM_CUTOFF,
					  atof ((const char *) key));

		}
		else if (!xmlStrcmp
			 (cur->name, (const xmlChar *) "cutoff_lfo_delay"))
		{
		    key =
			xmlNodeListGetString (doc, cur->xmlChildrenNode,
					      1);
		    patch_set_lfo_delay (id, PATCH_PARAM_CUTOFF,
					 atof ((const char *) key));

		}
		else if (!xmlStrcmp
			 (cur->name, (const xmlChar *) "cutoff_lfo_amt"))
		{
		    key =
			xmlNodeListGetString (doc, cur->xmlChildrenNode,
					      1);
		    patch_set_lfo_amount (id, PATCH_PARAM_CUTOFF,
					  atof ((const char *) key));

		}
		else if (!xmlStrcmp
			 (cur->name, (const xmlChar *) "cutoff_lfo_on"))
		{
		    key =
			xmlNodeListGetString (doc, cur->xmlChildrenNode,
					      1);
		    patch_set_lfo_on (id, PATCH_PARAM_CUTOFF,
				      xmlStrcmp (key, (const xmlChar *) "yes") == 0);

		}
		else if (!xmlStrcmp
			 (cur->name, (const xmlChar *) "cutoff_lfo_beats"))
		{
		    key =
			xmlNodeListGetString (doc, cur->xmlChildrenNode,
					      1);
		    patch_set_lfo_beats (id, PATCH_PARAM_CUTOFF,
					 atof ((const char *) key));

		}
		else if (!xmlStrcmp
			 (cur->name, (const xmlChar *) "cutoff_lfo_freq"))
		{
		    key =
			xmlNodeListGetString (doc, cur->xmlChildrenNode,
					      1);
		    patch_set_lfo_freq (id, PATCH_PARAM_CUTOFF,
					atof ((const char *) key));

		}
		else if (!xmlStrcmp
			 (cur->name, (const xmlChar *) "cutoff_lfo_positive"))
		{
		    key =
			xmlNodeListGetString (doc, cur->xmlChildrenNode,
					      1);
		    patch_set_lfo_positive (id, PATCH_PARAM_CUTOFF,
					    xmlStrcmp (key, (const xmlChar *) "yes") == 0);

		}
		else if (!xmlStrcmp
			 (cur->name, (const xmlChar *) "cutoff_lfo_global"))
		{
		    key =
			xmlNodeListGetString (doc, cur->xmlChildrenNode,
					      1);
		    patch_set_lfo_global (id, PATCH_PARAM_CUTOFF,
					  xmlStrcmp (key, (const xmlChar *) "yes") == 0);

		}
		else if (!xmlStrcmp
			 (cur->name, (const xmlChar *) "cutoff_lfo_sync"))
		{
		    key =
			xmlNodeListGetString (doc, cur->xmlChildrenNode,
					      1);
		    patch_set_lfo_sync (id, PATCH_PARAM_CUTOFF,
					xmlStrcmp (key, (const xmlChar *) "yes") == 0);

		}
		else if (!xmlStrcmp
			 (cur->name, (const xmlChar *) "cutoff_lfo_shape"))
		{
		    key =
			xmlNodeListGetString (doc, cur->xmlChildrenNode,
					      1);
		    if (xmlStrcmp (key, (const xmlChar *) "sine") == 0)
			patch_set_lfo_shape (id, PATCH_PARAM_CUTOFF,
					     LFO_SHAPE_SINE);
		    else if (xmlStrcmp (key, (const xmlChar *) "triangle") == 0)
			patch_set_lfo_shape (id, PATCH_PARAM_CUTOFF,
					     LFO_SHAPE_TRIANGLE);
		    else if (xmlStrcmp (key, (const xmlChar *) "saw") == 0)
			patch_set_lfo_shape (id, PATCH_PARAM_CUTOFF,
					     LFO_SHAPE_SAW);
		    else if (xmlStrcmp (key, (const xmlChar *) "square") == 0)
			patch_set_lfo_shape (id, PATCH_PARAM_CUTOFF,
					     LFO_SHAPE_SQUARE);
		}

		if (key != NULL)
		{
		    xmlFree (key);
		    key = NULL;
		}
	    }
	}
    }

    xmlFreeDoc (doc);
#endif
    return 0;
}
