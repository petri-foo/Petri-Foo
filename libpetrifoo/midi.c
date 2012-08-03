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


/* predominantly ripped from the alsa programming tutorial */
#include <stdio.h>
#include <signal.h>
#include <unistd.h>
#include <alsa/asoundlib.h>
#include <pthread.h>


#include "instance.h"
#include "driver.h"
#include "patch_private/err_msg.h"
#include "petri-foo.h"
#include "mixer.h"
#include "midi.h"
#include "sync.h"
#include "midi_control.h"


static Atomic       running = 0;
static pthread_t    midi_thread;
static int          midi_client_id = -1;


MIDI_CONTROL_H__CC_MAP_DEF


/*
 * Work out bpm from midi queue.
 */
static float calc_bpm(snd_seq_t* handle, int q)
{
    float bpm;
    snd_seq_queue_tempo_t* tempo;

    snd_seq_queue_tempo_alloca(&tempo);

    /* convert microsec per quarter to bpm */
    snd_seq_get_queue_tempo(handle, q, tempo);

    if (snd_seq_queue_tempo_get_tempo(tempo) == 0)
    {
        errmsg ("about to encounter a floating point exception\n");
        errmsg ("bailing out and providing an arbitrary tempo of 120.0\n");
        return 120.0;
    }

    bpm = 60000000 / snd_seq_queue_tempo_get_tempo(tempo);

    return bpm;
}


static void action(snd_seq_t* handle)
{
    snd_seq_event_t* ev;

    do
    {
        snd_seq_event_input (handle, &ev);
        switch (ev->type)
        {
        case SND_SEQ_EVENT_NOTEON:
            if (ev->data.note.velocity == 0)
                mixer_note_off(ev->data.note.channel,
                                ev->data.note.note);
            else
                mixer_note_on(ev->data.note.channel,
                                ev->data.note.note,
                                ev->data.note.velocity / 127.0);
            break;

        case SND_SEQ_EVENT_NOTEOFF:
            mixer_note_off (ev->data.note.channel, ev->data.note.note);
            break;

        case SND_SEQ_EVENT_START:
            /* TODO: account for tempo changes throughout the song */
            sync_start_midi (calc_bpm (handle, ev->data.queue.queue));
            break;

        case SND_SEQ_EVENT_CONTROLLER:
            mixer_control(  ev->data.control.channel,
                            ev->data.control.param,
                            cc_map( ev->data.control.param,
                                    ev->data.control.value));
            break;

        case SND_SEQ_EVENT_CONTROL14:
        case SND_SEQ_EVENT_NONREGPARAM:
        case SND_SEQ_EVENT_REGPARAM:
            mixer_control(  ev->data.control.channel,
                            ev->data.control.param,
                            ev->data.control.value / 16383.0);
            break;

        case SND_SEQ_EVENT_PITCHBEND:
            mixer_control(  ev->data.control.channel,
                            CC_PITCH_WHEEL,
                            ev->data.control.value / 8192.0);
            break;

        default:
            break;
        }

        snd_seq_free_event (ev);

    } while (snd_seq_event_input_pending (handle, 0) > 0);
}


static void* poll_events(void* arg)
{
    snd_seq_t* handle = arg;
    int npfd;
    struct pollfd* pfd;

    npfd = snd_seq_poll_descriptors_count (handle, POLLIN);
    pfd = alloca (sizeof(struct pollfd) * npfd);

    snd_seq_poll_descriptors (handle, pfd, npfd, POLLIN);

    while (1)
    {
        if (poll (pfd, npfd, 100) > 0)
            action (handle);

        if (!running)
            break;
    }

     return 0;
}


static int open_seq(snd_seq_t** handle)
{
    int portid;
    const char* instancename = get_instance_name();

    if (!instancename)
        instancename = PACKAGE;

    if (snd_seq_open(handle, "default", SND_SEQ_OPEN_INPUT, 0) < 0)
    {
        debug ("Failed to open ALSA sequencer\n");
        return MIDI_ERR_SEQ;
    }

    snd_seq_set_client_name(*handle, instancename);

    if ((portid = snd_seq_create_simple_port(   *handle,
                                                "midi_input",
                                SND_SEQ_PORT_CAP_WRITE |
                                SND_SEQ_PORT_CAP_SUBS_WRITE,
                                SND_SEQ_PORT_TYPE_APPLICATION)) < 0)
    {
        debug ("Failed to create ALSA sequencer port\n");
        return MIDI_ERR_PORT;
    }

    return 0;
}


int midi_start(void)
{
    int err;
    snd_seq_t* handle;

    if (running)
    {
        debug ("MIDI already running, so not starting\n");
        return 0;
    }

    debug ("Starting MIDI\n");

    if ((err = open_seq(&handle)) < 0)
        return err;

    running = 1;
    midi_client_id = snd_seq_client_id(handle);
    pthread_create(&midi_thread, NULL, poll_events, (void*)handle);
    debug ("MIDI started\n");

    return 0;
}


int midi_get_client_id(void)
{
    return midi_client_id;
}


void midi_stop(void)
{
    debug("midi stop\n");

    if (!running)
    {
        debug ("MIDI not running, so not stopping\n");
        return;
    }

    debug ("Stopping MIDI\n");

    running = 0;
    pthread_join(midi_thread, NULL);

    debug ("MIDI stopped\n");
}
