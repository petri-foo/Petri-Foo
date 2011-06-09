#include <pthread.h>

#include "mixer.h"
#include "patch.h"
#include "petri-foo.h"
#include "driver.h"
#include "sample.h"
#include "maths.h"
#include "ticks.h"
#include "lfo.h"
#include "patch_util.h"
#include "jackdriver.h"
#include "midi_control.h"


/* magic numbers */
enum
{
    EVENTMAX = 1024,
};


/* event codes */
typedef enum
{
    MIXER_NOTEON = 1,
    MIXER_NOTEOFF,
    MIXER_NOTEON_WITH_ID,
    MIXER_NOTEOFF_WITH_ID,
    MIXER_CONTROL,
    MIXER_PITCH_BEND,
}
MixerEventType;


typedef struct
{
    int     chan;
    int     note;
    float   vel;

} MixerNoteEvent;


typedef struct
{
    int     id;
    int     note;
    float   vel;

} MixerIdNoteEvent;


typedef struct
{
    int     chan;
    int     param;
    float   value;

} MixerControlEvent;


/* type for ringbuffer of incoming events */
typedef struct _Event
{
    MixerEventType type;
    Tick ticks;

    union
    {
        MixerNoteEvent      note;
        MixerIdNoteEvent    id_note;
        MixerControlEvent   control;
    };

} Event;


/* special structure for previewing samples */
typedef struct _MixerPreview
{
     Atomic     active;
     Sample*    sample;
     int        next_frame;

     pthread_mutex_t mutex;

} MixerPreview;


/* general variables */
static float            amplitude = 0.0;    /* master amplitude */
static MixerPreview     preview;            /* current preview sample */
static Event            events[EVENTMAX];   /* incoming from MIDI thread */

static Event* volatile  writer = events;
static Event* volatile  reader = events;

static Event            direct_events[EVENTMAX]; /* incoming from audio
                                                    thread              */

static int              direct_events_end;
static int              samplerate = -1;

static jack_client_t*   jc;

/* MIDI controller outputs */
static float            cc[16][CC__CONTROLLER__LAST];
static float            pitch_wheel[16];


inline static void advance_reader(void)
{
     reader = (reader + 1 >= events + EVENTMAX) ? events : reader + 1;
}


inline static void advance_writer(void)
{
     writer = (writer + 1 >= events + EVENTMAX) ? events : writer + 1;
}


inline static void preview_render(float *buf, int frames)
{
    int i, j;

    if (preview.active && pthread_mutex_trylock(&preview.mutex) == 0)
    {
        
        if (preview.sample->sp != NULL)
        {
            float logamp = log_amplitude(DEFAULT_AMPLITUDE);

            for (   i = 0, j = preview.next_frame * 2;
                    i < frames * 2 && j < preview.sample->frames * 2;
                    i++, j++)
            {
                buf[i] += preview.sample->sp[j] * logamp;
            }

            if ((preview.next_frame = j / 2) >= preview.sample->frames)
            {
                preview.active = 0;
                sample_free_data(preview.sample);
            }
        }
        else
            preview.active = 0;

        pthread_mutex_unlock(&preview.mutex);
     }
}


void mixer_flush(void)
{
    /* skip any queued ringbuffer events */
    reader = writer;

    patch_flush_all();

    /* stop any previews */
    pthread_mutex_lock(&preview.mutex);

    if (preview.active && preview.sample->sp)
    {
        /* this will "mark" the preview sample for deletion next
         * mixdown */
        preview.next_frame = preview.sample->frames;
    }

    pthread_mutex_unlock(&preview.mutex);
}


/* constructor */
void mixer_init(void)
{
    int p, c;
    debug ("initializing...\n");
    amplitude = DEFAULT_AMPLITUDE;
    pthread_mutex_init (&preview.mutex, NULL);
    preview.sample = sample_new ( );

    for (c = 0; c < 16; ++c)
    {
        pitch_wheel[c] = 0.0f;

        for (p = 0; p < CC__CONTROLLER__LAST; ++p)
            cc[c][p] = 0.0f;
    }

    debug ("done\n");
}


void mixer_set_jack_client(jack_client_t* client)
{
    jc = client;
}


/* mix current soundscape into buf */
void mixer_mixdown(float *buf, int frames)
{
    Tick curticks = jack_last_frame_time(jc);
    Event* event = NULL;
    int wrote = 0;
    int write;
    int i;
    int d = 0;
    float logvol = 0.0;

    for (i = 0; i < frames * 2; i++)
        buf[i] = 0.0;

    /* adjust the ticks in the direct events */
    for (i = 0; i < direct_events_end; ++i)
         direct_events[i].ticks += curticks - frames;

    /* get the first event */
    if (reader != writer)
    {
        if (d < direct_events_end
         && direct_events[d].ticks < reader->ticks)
        {
            event = &direct_events[d++];
        }
        else
        {
            event = reader;
            advance_reader();
        }
    }
    else if (d < direct_events_end)
        event = &direct_events[d++];


    /* process events */
    while (event)
    {
        if (event->ticks > curticks)
            break;

        write = event->ticks - (curticks - frames + wrote);

        if (write > 0)
        {
            patch_render(buf + wrote*2, write);
            wrote += write;
        }

        switch (event->type)
        {
        case MIXER_NOTEON:
            patch_trigger(  event->note.chan,
                            event->note.note,
                            event->note.vel,
                            event->ticks);
            break;

        case MIXER_NOTEON_WITH_ID:
            patch_trigger_with_id(  event->id_note.id,
                                    event->id_note.note,
                                    event->id_note.vel,
                                    event->ticks);
            break;

        case MIXER_NOTEOFF:
            patch_release(event->note.chan, event->note.note);
            break;

        case MIXER_NOTEOFF_WITH_ID:
            patch_release_with_id(event->id_note.id, event->id_note.note);
            break;

        case MIXER_CONTROL:

            debug("mixer control %d cc:%f\n",   event->control.param,
                                                event->control.value);

            cc[event->control.chan][event->control.param]
                                                = event->control.value;
            break;

        case MIXER_PITCH_BEND:
            pitch_wheel[event->control.chan] = event->control.value;
            break;

        default:
            break;
        }

        /* get next event */
        if (reader != writer)
        {
            if (d < direct_events_end
             && direct_events[d].ticks < reader->ticks)
            {
                event = &direct_events[d++];
            }
            else
            {
                event = reader;
                advance_reader();
            }
        }
        else if (d < direct_events_end)
            event = &direct_events[d++];
        else
            event = NULL;
    }

    /* reset the direct event buffer */
    direct_events_end = 0;

    if (wrote < frames)
        patch_render(buf + wrote*2, frames - wrote);

    preview_render(buf, frames);

    /* scale to master amplitude */
    logvol = log_amplitude(amplitude);

    for (i = 0; i < frames * 2; i++)
        buf[i] *= logvol;
}


/* queue a note-off event */
void mixer_note_off(int chan, int note)
{
    writer->type = MIXER_NOTEOFF;
    writer->ticks = jack_last_frame_time(jc);
    writer->note.chan = chan;
    writer->note.note = note;
    advance_writer();
}


/* queue a note-off event by patch id */
void mixer_note_off_with_id(int id, int note)
{
    writer->type = MIXER_NOTEOFF_WITH_ID;
    writer->ticks = jack_last_frame_time(jc);
    writer->id_note.id = id;
    writer->id_note.note = note;
    advance_writer();
}


/* queue a note-on event */
void mixer_note_on(int chan, int note, float vel)
{
    writer->type = MIXER_NOTEON;
    writer->ticks = jack_last_frame_time(jc);
    writer->note.chan = chan;
    writer->note.note = note;
    writer->note.vel = vel;
    advance_writer();
}


/* queue a note-on event by patch id */
void mixer_note_on_with_id(int id, int note, float vel)
{
    writer->type = MIXER_NOTEON_WITH_ID;
    writer->ticks = jack_last_frame_time(jc);
    writer->id_note.id = id;
    writer->id_note.note = note;
    writer->id_note.vel = vel;
    advance_writer();
}


/* queue control change event */
void mixer_control(int chan, int param, float value)
{
    writer->type = (param == MIXER_CC_PITCH_BEND)
                    ? MIXER_PITCH_BEND
                    : MIXER_CONTROL;
    writer->ticks = jack_last_frame_time(jc);
    writer->control.chan = chan;
    writer->control.param = param;
    writer->control.value = value;
    advance_writer();
}


/* queue a note-off event from the audio thread */
void mixer_direct_note_off(int chan, int note, Tick tick)
{
    if (direct_events_end < EVENTMAX)
    {
        direct_events[direct_events_end].type = MIXER_NOTEOFF;
        direct_events[direct_events_end].ticks = tick;
        direct_events[direct_events_end].note.chan = chan;
        direct_events[direct_events_end].note.note = note;
        ++direct_events_end;
    }
}


/* queue a note-on event from the audio thread */
void mixer_direct_note_on(int chan, int note, float vel, Tick tick)
{
    if (direct_events_end < EVENTMAX)
    {
        direct_events[direct_events_end].type = MIXER_NOTEON;
        direct_events[direct_events_end].ticks = tick;
        direct_events[direct_events_end].note.chan = chan;
        direct_events[direct_events_end].note.note = note;
        direct_events[direct_events_end].note.vel = vel;
        ++direct_events_end;
    }
}


/* queue control change event from the audio thread */
void mixer_direct_control(int chan, int param, float value, Tick tick)
{
    if (direct_events_end < EVENTMAX)
    {
        direct_events[direct_events_end].type =
                        (param == MIXER_CC_PITCH_BEND)
                                ? MIXER_PITCH_BEND
                                : MIXER_CONTROL;
        direct_events[direct_events_end].ticks = tick;
        direct_events[direct_events_end].control.chan = chan;
        direct_events[direct_events_end].control.param = param;
        direct_events[direct_events_end].control.value = value;
        ++direct_events_end;
    }
}


void mixer_preview(char *name,  int raw_samplerate,
                                    int raw_channels,
                                    int sndfile_format)
{
    pthread_mutex_lock(&preview.mutex);
    preview.active = 0;
    sample_load_file(preview.sample, name,  samplerate,
                                            raw_samplerate,
                                            raw_channels,
                                            sndfile_format);
    preview.next_frame = 0;
    preview.active = 1;
    pthread_mutex_unlock(&preview.mutex);
}


/* set the master amplitude */
int mixer_set_amplitude(float vol)
{
    if (vol < 0.0 || vol > 1.0)
        return -1;

    amplitude = vol;

    return 0;
}


/* return the master amplitude */
float mixer_get_amplitude(void)
{
    return amplitude;
}


/* set internally assumed samplerate */
void mixer_set_samplerate(int rate)
{
    samplerate = rate;
}


/* destructor */
void mixer_shutdown(void)
{
    debug ("shutting down...\n");
    sample_free (preview.sample);
    debug ("done\n");
}


float* mixer_get_control_output(int chan, int param)
{
    if (chan < 0 || chan > 15)
        return 0;

    if (param == MIXER_CC_PITCH_BEND)
        return &pitch_wheel[chan];

    return (chan < 0 || chan > 15 || param < 0 || param > 119)
                ? 0
                : &cc[chan][param];
}
