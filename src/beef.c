
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <libxml/parser.h>
#include <glib.h>
#include "petri-foo.h"
#include "patch.h"

int beef_write (char *name)
{

    int val = 0;

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
    rootnode = xmlNewDocNode (doc, NULL, (const xmlChar *) "beef", NULL);
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
    return val;
}

int beef_read (char *path)
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
