/*  Petri-Foo is a fork of the Specimen audio sampler.

    Copyright 2011 Brendan S. Jones

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

#include <stdlib.h>
#include <string.h>
#include <glib.h>
#include <libxml/parser.h>
#include <sys/stat.h>
#include <unistd.h>
#include <errno.h>


#include "phin.h"
#include "global_settings.h"
#include "petri-foo.h"
#include "msg_log.h"


#define SETTINGS_BASENAME "rc.xml"


static global_settings* gbl_settings = 0;
/*
    init and read settings
*/
void settings_init()
{
    if (gbl_settings) settings_free();

    gbl_settings  = malloc(sizeof(global_settings));
    gbl_settings->last_sample_dir = strdup(getenv("HOME"));
    gbl_settings->last_bank_dir = strdup(getenv("HOME"));

    gbl_settings->sample_file_filter = strdup("All Audio files");
    gbl_settings->sample_auto_preview = true;
    gbl_settings->sample_preview_length = 10.0f;

    gbl_settings->filename = (char*) g_build_filename(
                             g_get_user_config_dir(),
                             g_get_prgname(),
                             SETTINGS_BASENAME,
                             NULL);

    gbl_settings->log_lines =           DEFAULT_LOG_LINES;
/*
    gbl_settings->abs_max_sample_size = DEFAULT_ABS_MAX_SAMPLE;
    gbl_settings->max_sample_size =     DEFAULT_MAX_SAMPLE;
 */
    settings_read((char*) gbl_settings->filename);
}


static gboolean xmlstr_to_gboolean(xmlChar* str)
{
    if (xmlStrcasecmp(str, BAD_CAST "true") == 0
     || xmlStrcasecmp(str, BAD_CAST "on") == 0
     || xmlStrcasecmp(str, BAD_CAST "yes") == 0)
    {
        return TRUE;
    }

    return FALSE;
}


int settings_read(const char* path)
{
    xmlDocPtr   doc;
    xmlNodePtr  noderoot;
    xmlNodePtr  node1;
    xmlNodePtr  node2;
    xmlChar*    prop;

    msg_log(MSG_MESSAGE, "Reading global settings from: %s\n",path);

    doc = xmlParseFile (path);

    if (doc == NULL)
    {
        msg_log(MSG_ERROR, "Failed to parse %s\n", path);
        return -1;
    }

    noderoot = xmlDocGetRootElement(doc);

    if (noderoot == NULL)
    {
        msg_log(MSG_WARNING, "%s is empty\n", path);
        xmlFreeDoc(doc);
        return -1;
    }

    if (xmlStrcmp(noderoot->name, BAD_CAST "Petri-Foo-Settings") != 0)
    {
        msg_log(MSG_ERROR,
                "%s is not a valid 'Petri-Foo-Settings' file\n", path);
        xmlFreeDoc(doc);
        return -1;
    }

    for (node1 = noderoot->children;
         node1 != NULL;
         node1 = node1->next)
    {
        if (node1->type != XML_ELEMENT_NODE)
            continue;

        for ( node2 = node1->children;
            node2 != NULL;
            node2 = node2->next)
        {
            int n;

            if (xmlStrcmp(node2->name, BAD_CAST "property") == 0)
            {
                prop = BAD_CAST xmlGetProp(node2, BAD_CAST "name");

                if (xmlStrcmp(prop, BAD_CAST "last-sample-directory") == 0)
                {
                    free(gbl_settings->last_sample_dir);
                    gbl_settings->last_sample_dir =
                        (char*) xmlGetProp(node2, BAD_CAST "value");
                }

                if (xmlStrcmp(prop, BAD_CAST "last-bank-directory") == 0)
                {
                    free(gbl_settings->last_bank_dir);
                    gbl_settings->last_bank_dir =
                        (char*) xmlGetProp(node2, BAD_CAST "value");
                }
/*
                if (xmlStrcmp(prop, BAD_CAST "sliders-use-fans") == 0)
                {
                    phin_fan_slider_set_fans_active(
                        xmlstr_to_gboolean(xmlGetProp(node2,
                                                    BAD_CAST "value")));
                }
 */
                if (xmlStrcmp(prop, BAD_CAST "log-lines") == 0)
                {
                    xmlChar* vprop = xmlGetProp(node2, BAD_CAST "value");

                    if (sscanf((const char*)vprop, "%d", &n) == 1)
                    {   /* arbitrary value alert */
                        if (n > 0 && n < 65536)
                            gbl_settings->log_lines = n;
                    }
                }

                if (xmlStrcmp(prop, BAD_CAST "sample-file-filter") == 0)
                {
                    free(gbl_settings->sample_file_filter);
                    gbl_settings->sample_file_filter =
                        (char*) xmlGetProp(node2, BAD_CAST "value");
                }

                if (xmlStrcmp(prop, BAD_CAST "sample-auto-preview") == 0)
                {
                    gbl_settings->sample_auto_preview =
                        xmlstr_to_gboolean(xmlGetProp(node2,
                                                        BAD_CAST "value"));
                }

                if (xmlStrcmp(prop, BAD_CAST "sample-preview-length") == 0)
                {
                    float f;
                    xmlChar* vprop = xmlGetProp(node2, BAD_CAST "value");

                    if (sscanf((const char*)vprop, "%f", &f) == 1)
                    {
                        if (f > 0.0f && f < 60.0f)
                            gbl_settings->sample_preview_length = f;
                    }
                }
            }
        }
    }

    return 0;
}


int settings_write()
{
    int rc;
    char* config_dir;
    char buf[CHARBUFSIZE];

    xmlDocPtr   doc;
    xmlNodePtr  noderoot;
    xmlNodePtr  node1;
    xmlNodePtr  node2;

    msg_log(MSG_MESSAGE, "Writing global settings to: %s\n",
                         gbl_settings->filename);

    doc = xmlNewDoc(BAD_CAST "1.0");

    if (!doc)
    {
        msg_log(MSG_ERROR, "XML error!\n");
        return -1; 
    }

    config_dir = (char*) g_build_filename(g_get_user_config_dir(),
                                          g_get_prgname(), NULL);

    if (mkdir(config_dir, S_IRWXU) != 0)
    {
         if (errno != EEXIST)
         {
             msg_log(MSG_ERROR,
                    "Could not create config directory: %s.\n",
                    config_dir);
             free(config_dir);
             return -1;
         }
    }

    free(gbl_settings->filename);
    gbl_settings->filename = (char*) g_build_filename(config_dir, 
                                                      SETTINGS_BASENAME,
                                                      NULL);
    free(config_dir);

    noderoot = xmlNewDocNode(doc, NULL, BAD_CAST "Petri-Foo-Settings",NULL);

    if (!noderoot)
    {
        msg_log(MSG_ERROR, "XML error!\n");
        return -1;
    }

    xmlDocSetRootElement(doc, noderoot);

    node1 = xmlNewTextChild(noderoot, NULL, BAD_CAST "global", NULL);

    node2 = xmlNewTextChild(node1, NULL, BAD_CAST "property", NULL);
    xmlNewProp(node2, BAD_CAST "name", BAD_CAST "last-sample-directory");
    xmlNewProp(node2, BAD_CAST "type", BAD_CAST "string");
    xmlNewProp(node2, BAD_CAST "value",
                      BAD_CAST gbl_settings->last_sample_dir);

    node2 = xmlNewTextChild(node1, NULL, BAD_CAST "property", NULL);
    xmlNewProp(node2, BAD_CAST "name", BAD_CAST "last-bank-directory");
    xmlNewProp(node2, BAD_CAST "type", BAD_CAST "string");
    xmlNewProp(node2, BAD_CAST "value",
                      BAD_CAST gbl_settings->last_bank_dir);

/*
    node2 = xmlNewTextChild(node1, NULL, BAD_CAST "property", NULL);
    xmlNewProp(node2, BAD_CAST "name", BAD_CAST "sliders-use-fans");
    xmlNewProp(node2, BAD_CAST "type", BAD_CAST "boolean");
    xmlNewProp(node2, BAD_CAST "value",
                      BAD_CAST (phin_fan_slider_get_fans_active()
                                    ? "true"
                                    : "false"));
*/
    node2 = xmlNewTextChild(node1, NULL, BAD_CAST "property", NULL);
    xmlNewProp(node2, BAD_CAST "name", BAD_CAST "last-bank-directory");
    xmlNewProp(node2, BAD_CAST "type", BAD_CAST "string");
    xmlNewProp(node2, BAD_CAST "value",
                      BAD_CAST gbl_settings->last_bank_dir);

    node2 = xmlNewTextChild(node1, NULL, BAD_CAST "property", NULL);
    xmlNewProp(node2, BAD_CAST "name", BAD_CAST "log-lines");
    xmlNewProp(node2, BAD_CAST "type", BAD_CAST "int");
    snprintf(buf, CHARBUFSIZE, "%d", gbl_settings->log_lines);
    xmlNewProp(node2, BAD_CAST "value", BAD_CAST buf);


    node2 = xmlNewTextChild(node1, NULL, BAD_CAST "property", NULL);
    xmlNewProp(node2, BAD_CAST "name", BAD_CAST "sample-file-filter");
    xmlNewProp(node2, BAD_CAST "type", BAD_CAST "string");
    xmlNewProp(node2, BAD_CAST "value",
                      BAD_CAST gbl_settings->sample_file_filter);

    node2 = xmlNewTextChild(node1, NULL, BAD_CAST "property", NULL);
    xmlNewProp(node2, BAD_CAST "name", BAD_CAST "sample-auto-preview");
    xmlNewProp(node2, BAD_CAST "type", BAD_CAST "boolean");
    xmlNewProp(node2, BAD_CAST "value",
                      BAD_CAST (gbl_settings->sample_auto_preview
                                    ? "true"
                                    : "false"));

    node2 = xmlNewTextChild(node1, NULL, BAD_CAST "property", NULL);
    xmlNewProp(node2, BAD_CAST "name", BAD_CAST "sample-preview-length");
    xmlNewProp(node2, BAD_CAST "type", BAD_CAST "float");
    snprintf(buf, CHARBUFSIZE, "%0.3f",
                      gbl_settings->sample_preview_length);
    xmlNewProp(node2, BAD_CAST "value", BAD_CAST buf);

    debug("attempting to write file:%s\n",gbl_settings->filename);

    rc = xmlSaveFormatFile(gbl_settings->filename, doc, 1);
    xmlFreeDoc(doc);

    return rc;
}


global_settings* settings_get(void)
{
    return gbl_settings;
}


void settings_free(void)
{
    if (gbl_settings == NULL)
        return;

    if (gbl_settings->filename) 
        free(gbl_settings->filename);

    if (gbl_settings->last_sample_dir) 
        free(gbl_settings->last_sample_dir);

    if (gbl_settings->last_bank_dir) 
        free(gbl_settings->last_bank_dir);

    if (gbl_settings->sample_file_filter)
        free(gbl_settings->sample_file_filter);

    free(gbl_settings);

    return;
}
