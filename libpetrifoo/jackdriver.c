/*  Petri-Foo is a fork of the Specimen audio sampler.

    Original Specimen author Pete Bessman
    Copyright 2005 Pete Bessman
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

    This file is a derivative of a Specimen original, modified 2011
*/


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <config.h>

#include "jackdriver.h"

#include <jack/midiport.h>
#include <jack/transport.h>

#if HAVE_JACK_SESSION_H
#include <jack/session.h>
#endif /* HAVE_JACK_SESSION_H */

#include <pthread.h>

#include "instance.h"
#include "petri-foo.h"
#include "driver.h"
#include "patch.h"
#include "pf_error.h"
#include "mixer.h"
#include "sync.h"
#include "lfo.h"
#include "midi_control.h"

/* prototypes */
static int start(void);
static int stop(void);


MIDI_CONTROL_H__CC_MAP_DEF


/* file-global variables */
static jack_port_t*     lport;
static jack_port_t*     rport;
static jack_port_t*     midiport;


static jack_client_t*   client;
static float*           buffer;
static int              rate = 44100;
static int              periodsize = 2048;
static int              running = 0;
static pthread_mutex_t  running_mutex = PTHREAD_MUTEX_INITIALIZER;
static char*            session_uuid = NULL;

static bool             autoconnect = false;

#if HAVE_JACK_SESSION_H
static bool             disable_jacksession = false;
#else
static bool             disable_jacksession = true;
#endif

/* working together to stop CTS */
typedef jack_default_audio_sample_t jack_sample_t;


#if HAVE_JACK_SESSION_H
JackSessionCallback session_cb = 0;

void jackdriver_set_session_cb(JackSessionCallback jack_session_cb)
{
    session_cb = jack_session_cb;
}
#endif

void jackdriver_disable_jacksession(void)
{
    disable_jacksession = true;
}


static int process(jack_nframes_t frames, void* arg)
{
    (void)arg;
    size_t i;
    jack_sample_t* l = (jack_sample_t*)jack_port_get_buffer(lport, frames);
    jack_sample_t* r = (jack_sample_t*)jack_port_get_buffer(rport, frames);
    jack_position_t pos;

     /* MIDI data */
    void* midi_buf = jack_port_get_buffer(midiport, frames);
    jack_midi_event_t jack_midi_event;
    jack_nframes_t event_index = 0;
    jack_nframes_t event_count = jack_midi_get_event_count(midi_buf);
    unsigned char* midi_data;

    /* transport state info */
    static jack_transport_state_t last_state = JackTransportStopped;
    jack_transport_state_t new_state;

    /* transport tempo info */
    static float last_tempo = -1;
    float new_tempo;
     
    /* behold: the jack_transport sync code */
    new_state = jack_transport_query (client, &pos);

    if ((new_state == JackTransportRolling)
     && (pos.valid & JackPositionBBT))
    {
        new_tempo = pos.beats_per_minute;

        if ((last_state == JackTransportStopped)
         || (last_state == JackTransportStarting))
        {
            //debug ("got transport start\n");
            sync_start_jack (new_tempo);
        }
        else if (new_tempo != last_tempo)
        {
            //debug ("got tempo change\n");
            sync_start_jack (new_tempo);
        }
        last_tempo = new_tempo;
    }

    last_state = new_state;

    /* send the JACK MIDI events to the mixer */
    while (event_index < event_count)
    {
        jack_midi_event_get(&jack_midi_event, midi_buf,event_index);
        midi_data = jack_midi_event.buffer;

        /* TODO: handle 14-bit controllers and RPNs and NRPNs */

        if (((midi_data[0] & 0xF0) == 0x80) ||
            ((midi_data[0] & 0x90) == 0x90 && midi_data[2] == 0))
        {   /* note-off */
            mixer_direct_note_off(midi_data[0] & 0x0F, midi_data[1],
                                                jack_midi_event.time);
        }
        else if ((midi_data[0] & 0xF0) == 0x90)
        {   /* note-on */
            mixer_direct_note_on(midi_data[0] & 0x0F, midi_data[1],
                                midi_data[2] / 127.0, jack_midi_event.time);
        }
        else if ((midi_data[0] & 0xF0) == 0xB0)
        {   /* controller */
            mixer_direct_control(   midi_data[0] & 0x0F,    /* channel */
                                    midi_data[1],           /* param */
                                    cc_map(midi_data[1], midi_data[2]),
                                    jack_midi_event.time);
        }
        else if ((midi_data[0] & 0xF0) == 0xE0)
        {   /* pitch bend */
            mixer_direct_control(   midi_data[0] & 0x0F,
                                    CC_PITCH_WHEEL,
                -1.0 + ((midi_data[2] << 7) | midi_data[1]) /  8192.0,
                                    jack_midi_event.time);
        }

        event_index++;
    }

    mixer_mixdown (buffer, frames);
    for (i = 0; i < frames; i++)
    {
        l[i] = buffer[i * 2];
        r[i] = buffer[i * 2 + 1];
    }

    return 0;
}


static int sample_rate_change(jack_nframes_t r, void* arg)
{
    (void)arg;
    driver_set_samplerate(rate = r);
    return 0;
}


static int buffer_size_change(jack_nframes_t b, void* arg)
{
    (void)arg;
    float* new;
    float* old;

    if ((new = malloc(sizeof(float) * b * 2)) == NULL)
    {
        pf_error(PF_ERR_JACK_BUF_SIZE_CHANGE);
        stop();
    }

    old = buffer;
    buffer = new;

    if (old != NULL)
        free (old);

    periodsize = b;

    /* let the rest of the world know the good news */
    driver_set_buffersize (b);

    return 0;
}


static void shutdown(void* arg)
{
    (void)arg;
    pthread_mutex_lock (&running_mutex);
    running = 0;
    pthread_mutex_unlock (&running_mutex);
    return;
}


static void init(void)
{
}


static int start(void)
{
    const char** ports;
    const char* instancename = get_instance_name();
    jack_status_t status;

    if (!instancename)
        instancename = PACKAGE;

    debug("JACK initializing driver...\n");
    pthread_mutex_lock (&running_mutex);
    running = 0;

    if (disable_jacksession)
    {
        client = jack_client_open(instancename, JackNullOption, &status);
    }
    else
    {
        #if HAVE_JACK_SESSION_H
        client = jack_client_open(instancename, JackSessionID, &status,
                                                        session_uuid);
        #endif
    }

    if (client == 0)
    {
        pf_error(PF_ERR_JACK_OPEN_CLIENT);
        pthread_mutex_unlock (&running_mutex);
        return -1;
    }

    if (status & JackNameNotUnique)
        set_instance_name(jack_get_client_name(client));

    mixer_set_jack_client(jackdriver_get_client());

    jack_set_process_callback (client, process, 0);

    #if HAVE_JACK_SESSION_H
    if (!disable_jacksession && jack_set_session_callback)
    {
        if (jack_set_session_callback(client, session_cb, 0))
        {
            pf_error(PF_ERR_JACK_SESSION_CB);
            return -1;
        }
    }
    #endif

    jack_on_shutdown (client, shutdown, 0);

    lport = jack_port_register( client,
                                "out_left",
                                JACK_DEFAULT_AUDIO_TYPE,
                                JackPortIsOutput,
                                0);

    rport = jack_port_register( client,
                                "out_right",
                                JACK_DEFAULT_AUDIO_TYPE,
                                JackPortIsOutput,
                                0);

    midiport = jack_port_register(  client,
                                    "midi_input",
                                    JACK_DEFAULT_MIDI_TYPE,
                                    JackPortIsInput,
                                    0);

    rate = jack_get_sample_rate (client);
    driver_set_samplerate (rate);
    jack_set_sample_rate_callback (client, sample_rate_change, 0);

    periodsize = jack_get_buffer_size (client);
    driver_set_buffersize (periodsize);
    jack_set_buffer_size_callback (client, buffer_size_change, 0);

    if ((buffer = malloc (sizeof (float) * periodsize * 2)) == NULL)
    {
        pf_error(PF_ERR_JACK_BUF_ALLOC);
        jack_client_close (client);
        pthread_mutex_unlock (&running_mutex);
        return -1;
    }

    mixer_flush();

    if (jack_activate(client) != 0)
    {
        pf_error(PF_ERR_JACK_ACTIVATE);
        jack_client_close(client);
        pthread_mutex_unlock(&running_mutex);
        return -1;
    }

    ports = jack_get_ports(client, NULL, NULL,
                            JackPortIsInput | JackPortIsPhysical);

    if (autoconnect)
    {
        if (ports[0] != NULL)
        {
            if (jack_connect(client, jack_port_name(lport), ports[0]))
            {
                debug("JACK failed to connect left output port\n");
            }

            if (ports[1] != NULL)
            {
                if (jack_connect(client, jack_port_name (rport), ports[1]))
                {
                    debug("JACK failed to connect right output port\n");
                }
            }
            else
            {
                debug("JACK failed to connect right output port\n");
            }

            free (ports);
        }
        else
        {
            debug("JACK failed to connect output ports\n");
        }
    }

    debug("JACK Initialization complete\n");
    running = 1;
    pthread_mutex_unlock (&running_mutex);

    return 0;
}


static int stop(void)
{
    debug("stopping jack...\n");

    pthread_mutex_lock (&running_mutex);

    if (running)
    {
        debug("JACK shutting down...\n");
        jack_deactivate (client);
        jack_client_close (client);

        debug("JACK stopped\n");

        if (buffer != NULL)
            free (buffer);
    }

    running = 0;
    pthread_mutex_unlock (&running_mutex);

    debug("jackdriver stopped\n");

    return 0;
}


static int getrate(void)
{
     return rate;
}

static int getperiodsize(void)
{
     return periodsize;
}

static const char* getname(void)
{
     return "JACK";
}

static void* getid(void)
{
    return (void*)jack_get_client_name(client);
}


void jackdriver_set_autoconnect(bool ac)
{
    autoconnect = ac;
}


void jackdriver_set_uuid(char *uuid)
{
    session_uuid = uuid;
}


jack_client_t*  jackdriver_get_client(void)
{
    return client;
}


Driver jack_driver = {
     init,
     start,
     stop,
     getrate,
     getperiodsize,
     getname,
     getid
};
