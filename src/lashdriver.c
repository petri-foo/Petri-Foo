#include <gtk/gtk.h>            /* for idle callback */
#include <string.h>

#include "instance.h"
#include "lashdriver.h"
#include "dish_file.h"
#include "gui/gui.h"


#include <stdlib.h> /* for calloc */

lash_client_t* lash_client;


void lashdriver_init (lash_args_t *lash_args)
{
    const char* instancename = get_instance_name();

    lash_client = lash_init(lash_args, instancename, LASH_Config_File, LASH_PROTOCOL(2, 0));

    if (!lash_client)
        fprintf(stderr, "Could not initialise LASH\n");

    lash_event_t* event = lash_event_new_with_type(LASH_Client_Name);
    lash_event_set_string(event, instancename);
    lash_send_event(lash_client, event);
}


gboolean lashdriver_process (void* absolutely_nothing)
{
    lash_event_t*  event  = NULL;
    lash_config_t* config = NULL;
    const char*    str    = NULL;
    char*          path   = NULL;

    while ((event = lash_get_event(lash_client)))
    {
        if (lash_event_get_type(event) == LASH_Quit)
        {
            gtk_main_quit();
            return FALSE;
        }
        else if (lash_event_get_type(event) == LASH_Save_File)
        {
            str = lash_event_get_string(event); /* our restore directory */
            path = (char*)calloc(strlen(str)+11, sizeof(char));
            strcpy(path, str);         /* path = dir */
            strcat(path, "/");         /* path += "/" */
            strcat(path, "bank.dish"); /* path += "bank.beef" */
            fprintf(stderr, "Saving to file %s\n", path);
            dish_file_write(path);
            lash_send_event(lash_client,
                        lash_event_new_with_type(LASH_Save_File));
        }
        else if (lash_event_get_type(event) == LASH_Restore_File)
        {
            str = lash_event_get_string(event); /* our restore directory */
            path = (char*)calloc(strlen(str)+11, sizeof(char));
            strcpy(path, str);         /* path = dir */
            strcat(path, "/");         /* path += "/" */
            strcat(path, "bank.dish"); /* path += "bank.beef" */
            fprintf(stderr, "Restoring from file %s\n", path);
            dish_file_read(path);
            gui_refresh();
            lash_send_event(lash_client, lash_event_new_with_type(LASH_Restore_File));
        }
        else 
        {
            fprintf(stderr, "Unhandled LASH event: type %d, '%s''\n",
                    lash_event_get_type(event),
                    lash_event_get_string(event));
        }
    }

    while ((config = lash_get_config(lash_client)))
    {
        fprintf(stderr, "Unexpected LASH config: %s\n",
                lash_config_get_key(config));
    }

    return TRUE;
}


void lashdriver_start ( )
{
    g_timeout_add(250, lashdriver_process, NULL);
}

void lashdriver_set_jack_name (char *name)
{
    if (name)
        lash_jack_client_name(lash_client, name);
}

void lashdriver_set_alsa_id (int id)
{
    if (id >= 0 && id < 256)
        lash_alsa_client_id(lash_client, (unsigned char)id);
}
