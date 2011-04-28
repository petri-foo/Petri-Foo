#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <config.h>

#include "jackdriver.h"

#include <jack/midiport.h>
#include <jack/transport.h>

#ifdef HAVE_JACK_SESSION
#include <jack/session.h>
#include "gui/audio-settings.h"
#endif /* HAVE_JACK_SESSION */

#include <gtk/gtk.h>
#include <pthread.h>

#include "instance.h"
#include "petri-foo.h"
#include "driver.h"
#include "patch.h"
#include "mixer.h"
#include "sync.h"
#include "lfo.h"


/* prototypes */
static int start(void);
static int stop(void);

/* file-global variables */
static jack_port_t*    lport;
static jack_port_t*    rport;
static jack_port_t*    midiport;

#ifdef HAVE_JACK_SESSION
static jack_session_event_t *session_event;
#endif

static jack_client_t*  client;
static float*          buffer;
static int             rate = 44100;
static int             periodsize = 2048;
static int             running = 0;
static pthread_mutex_t running_mutex = PTHREAD_MUTEX_INITIALIZER;
static char*           session_uuid = NULL;

/* working together to stop CTS */
typedef jack_default_audio_sample_t jack_sample_t;

static void
map_control(unsigned char chan, int param, float value, Tick tick)
{
    static struct
    {
        int cc;
        ControlParamType param;
        float bias;
        float scale;
    }

    map[] = {
        { 5, CONTROL_PARAM_PORTAMENTO_TIME, 0, 1},
        { 7, CONTROL_PARAM_AMPLITUDE,       0, 1},
        {10, CONTROL_PARAM_PANNING,        -1, 2},
        {65, CONTROL_PARAM_PORTAMENTO,      0, 1},
        {71, CONTROL_PARAM_RESONANCE,       0, 1},
        {74, CONTROL_PARAM_CUTOFF,          0, 1}
    };

    size_t i;
     
    for (i = 0; i < sizeof(map) / sizeof(map[0]); ++i)
        if (map[i].cc == param)
            mixer_direct_control(chan, map[i].param, 
                                       value * map[i].scale + map[i].bias,
                                       tick);
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
            debug ("got transport start\n");
            sync_start_jack (new_tempo);
        }
        else if (new_tempo != last_tempo)
        {
            debug ("got tempo change\n");
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
            map_control(midi_data[0] & 0x0F, midi_data[1],
                                midi_data[2] / 127.0, jack_midi_event.time);
        }
        else if ((midi_data[0] & 0xF0) == 0xE0)
        {   /* pitch bend */
            mixer_direct_control(midi_data[0] & 0x0F, CONTROL_PARAM_PITCH,
                ((midi_data[2] << 7) | midi_data[1]) /  8192.0,
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

     if ((new = malloc (sizeof (float) * b * 2)) == NULL)
     {
	  errmsg ("Failed to change buffer size\n");
	  stop ( );
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
    char* instancename = strdup (get_instance_name());

    debug ("Initializing Jack Driver...\n");
    pthread_mutex_lock (&running_mutex);
    running = 0;

#ifdef HAVE_JACK_SESSION
    client = jack_client_open(instancename,
                               JackSessionID, NULL, session_uuid);
#else
    client = jack_client_open(instancename, JackNullOption, NULL);
#endif

    if (client == 0)
    {
        errmsg ("Failed to open new jack client: %s\n", instancename);
        pthread_mutex_unlock (&running_mutex);
        return -1;
    }

    mixer_set_jack_client(jackdriver_get_client());

    jack_set_process_callback (client, process, 0);

#ifdef HAVE_JACK_SESSION
    debug("HAVE JACK SESSION\n");
    if (jack_set_session_callback)
    {
        debug("setting session callback... ");

        if (jack_set_session_callback(client, audio_settings_session_cb, 0))
        {
            printf("fail\n");
        }
        else
        {
            printf("ok\n");
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
        errmsg ("Failed to allocate space for buffer\n");
        jack_client_close (client);
        pthread_mutex_unlock (&running_mutex);
        return -1;
    }

    mixer_flush();

    if (jack_activate(client) != 0)
    {
        errmsg ("Failed to activate client\n");
        jack_client_close(client);
        pthread_mutex_unlock(&running_mutex);
        return -1;
    }

    ports = jack_get_ports(client, NULL, NULL,
                            JackPortIsInput | JackPortIsPhysical);

    if (ports[0] != NULL)
    {
        if (jack_connect(client, jack_port_name(lport), ports[0]) != 0)
            errmsg ("Cannot connect left output port\n");

        if (ports[1] != NULL)
        {
            if (jack_connect(client, jack_port_name (rport), ports[1]))
                errmsg ("Cannot connect right output port\n");
        }
        else
        {
            errmsg ("Cannot connect right output port\n");
        }

        free (ports);
    }
    else
    {
        errmsg ("Cannot connect output ports\n");
    }

    debug ("Initialization complete\n");
    running = 1;
    pthread_mutex_unlock (&running_mutex);

    return 0;
}


static int stop(void)
{
     pthread_mutex_lock (&running_mutex);
     if (running)
     {
	  debug ("Shutting down...\n");
	  jack_deactivate (client);
	  jack_client_close (client);
	  if (buffer != NULL)
	       free (buffer);
	  debug ("Shutdown complete\n");
     }
     else
     {
	  debug ("Not running, so not shutting down\n");
     }

     running = 0;
     pthread_mutex_unlock (&running_mutex);
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

void jackdriver_set_uuid      (char *uuid)
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
