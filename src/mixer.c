#include <pthread.h>
#include <glib.h>
#include "patch.h"
#include "petri-foo.h"
#include "driver.h"
#include "sample.h"
#include "maths.h"
#include "ticks.h"
#include "lfo.h"

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
     MIXER_CONTROL
}
MixerEventType;

typedef struct {
     int chan;
     int note;
     float vel;
} MixerNoteEvent;

typedef struct {
     int id;
     int note;
     float vel;
} MixerIdNoteEvent;

typedef struct {
     int chan;
     PatchParamType param;
     float value;
} MixerControlEvent;

/* type for ringbuffer of incoming events */
typedef struct _Event
{
     MixerEventType type;	/* type of event */
     Tick ticks;		/* time of event's occurence */
     union {
	  MixerNoteEvent    note;
	  MixerIdNoteEvent  id_note;
	  MixerControlEvent control;
     };
}
Event;

/* special structure for previewing samples */
typedef struct _MixerPreview
{
     Atomic active;
     Sample* sample;
     int next_frame;
     pthread_mutex_t mutex;
}
MixerPreview;

/* general variables */
static float volume = 0.0;	/* master volume */
static MixerPreview preview;	/* current preview sample */
static Event events[EVENTMAX];  /* events coming from the MIDI thread */
static Event* volatile writer = events;
static Event* volatile reader = events;
static Event direct_events[EVENTMAX]; /* events coming from the audio thread */
static int direct_events_end;
static int samplerate = DRIVER_DEFAULT_SAMPLERATE;

/* advance reader event vector */
inline static void advance_reader ( )
{
     reader = (reader + 1 >= events + EVENTMAX) ? events : reader + 1;
}

/* advance writer event vector */
inline static void advance_writer ( )
{
     writer = (writer + 1 >= events + EVENTMAX) ? events : writer + 1;
}

/* mixdown current preview sample (if any) into buf */
inline static void preview_render (float *buf, int frames)
{
     int i, j;

     /* if there is a sample currently being previewed let's throw it into the mix */
     if (preview.active && pthread_mutex_trylock (&preview.mutex) == 0)
     {
	  if (preview.sample->sp != NULL)
	  {
	       /* fill the buf with samples from preview (two samples per frame) */
	       for (i = 0, j = preview.next_frame * 2;
		    i < frames * 2 && j < preview.sample->frames * 2; i++, j++)
		   buf[i] += preview.sample->sp[j] * log_volume(DEFAULT_VOLUME);

	       /* increment next frame indicator (half as much as the number of samples written) */
	       if ((preview.next_frame = j / 2) >= preview.sample->frames)
	       {
		    preview.active = 0;
		    sample_free_file (preview.sample);
	       }
	  }
	  else
	  {
	       preview.active = 0;
	  }
	  pthread_mutex_unlock (&preview.mutex);
     }
}

/* stop all noise */
void mixer_flush ( )
{
     /* skip any queued ringbuffer events */
     reader = writer;

     patch_flush_all ( );

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
void mixer_init ( )
{
     debug ("initializing...\n");
     volume = DEFAULT_VOLUME;
     pthread_mutex_init (&preview.mutex, NULL);
     preview.sample = sample_new ( );
     debug ("done\n");
}

/* mix current soundscape into buf */
void mixer_mixdown (float *buf, int frames)
{
     Tick curticks = ticks_get_ticks ( ); /* - jack_frames_since_cycle_start() */
     Event* event = NULL;
     int wrote = 0;
     int write;
     int i;
     int d = 0;
     float logvol = 0.0;

     /* clear the buffer */
     for (i = 0; i < frames * 2; i++)
	     buf[i] = 0.0;
     
     /* adjust the ticks in the direct events */
     for (i = 0; i < direct_events_end; ++i)
          direct_events[i].ticks += curticks - frames;
     
     /* get the first event */
     if (reader != writer) {
          if (d < direct_events_end && direct_events[d].ticks < reader->ticks) {
               event = &direct_events[d++];
          }
          else {
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
	       patch_trigger (event->note.chan, event->note.note, event->note.vel, event->ticks);
	       break;
	  case MIXER_NOTEON_WITH_ID:
	       patch_trigger_with_id (event->id_note.id, event->id_note.note,
				      event->id_note.vel, event->ticks);
	       break;
	  case MIXER_NOTEOFF:
	       patch_release (event->note.chan, event->note.note);
	       break;
	  case MIXER_NOTEOFF_WITH_ID:
	       patch_release_with_id (event->id_note.id, event->id_note.note);
	       break;
	  case MIXER_CONTROL:
	       patch_control(event->control.chan, event->control.param, event->control.value);
	       break;
	  default:
	       break;
	  }
	  
	  /* get next event */
       if (reader != writer) {
            if (d < direct_events_end && 
                direct_events[d].ticks < reader->ticks) {
                 event = &direct_events[d++];
            }
            else {
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
         patch_render (buf + wrote*2, frames - wrote);

     preview_render (buf, frames);
     
     /* scale to master volume */
     logvol = log_volume(volume);
     for (i = 0; i < frames * 2; i++)
	  buf[i] *= logvol;
}

/* queue a note-off event */
void mixer_note_off (int chan, int note)
{
     writer->type = MIXER_NOTEOFF;
     writer->ticks = ticks_get_ticks ( );
     writer->note.chan = chan;
     writer->note.note = note;
     advance_writer ( );
}

/* queue a note-off event by patch id */
void mixer_note_off_with_id (int id, int note)
{
     writer->type = MIXER_NOTEOFF_WITH_ID;
     writer->ticks = ticks_get_ticks ( );
     writer->id_note.id = id;
     writer->id_note.note = note;
     advance_writer ( );
}

/* queue a note-on event */
void mixer_note_on (int chan, int note, float vel)
{
     writer->type = MIXER_NOTEON;
     writer->ticks = ticks_get_ticks ( );
     writer->note.chan = chan;
     writer->note.note = note;
     writer->note.vel = vel;
     advance_writer ( );
}

/* queue a note-on event by patch id */
void mixer_note_on_with_id (int id, int note, float vel)
{
     writer->type = MIXER_NOTEON_WITH_ID;
     writer->ticks = ticks_get_ticks ( );
     writer->id_note.id = id;
     writer->id_note.note = note;
     writer->id_note.vel = vel;
     advance_writer ( );
}

/* queue control change event */
void mixer_control(int chan, ControlParamType param, float value)
{
     writer->type = MIXER_CONTROL;
     writer->ticks = ticks_get_ticks ( );
     writer->control.chan = chan;
     writer->control.param = param;
     writer->control.value = value;
     advance_writer ( );
}

/* queue a note-off event from the audio thread */
void mixer_direct_note_off (int chan, int note, Tick tick)
{
     if (direct_events_end < EVENTMAX) {
          direct_events[direct_events_end].type = MIXER_NOTEOFF;
          direct_events[direct_events_end].ticks = tick;
          direct_events[direct_events_end].note.chan = chan;
          direct_events[direct_events_end].note.note = note;
          ++direct_events_end;
     }
}

/* queue a note-on event from the audio thread */
void mixer_direct_note_on (int chan, int note, float vel, Tick tick)
{
     if (direct_events_end < EVENTMAX) {
          direct_events[direct_events_end].type = MIXER_NOTEON;
          direct_events[direct_events_end].ticks = tick;
          direct_events[direct_events_end].note.chan = chan;
          direct_events[direct_events_end].note.note = note;
          direct_events[direct_events_end].note.vel = vel;
          ++direct_events_end;
     }
}

/* queue control change event from the audio thread */
void mixer_direct_control(int chan, ControlParamType param, 
			  float value, Tick tick)
{
     if (direct_events_end < EVENTMAX) {
          direct_events[direct_events_end].type = MIXER_CONTROL;
          direct_events[direct_events_end].ticks = tick;
          direct_events[direct_events_end].control.chan = chan;
          direct_events[direct_events_end].control.param = param;
          direct_events[direct_events_end].control.value = value;
          ++direct_events_end;
     }
}

/* preview a sample file */
void mixer_preview (char *name)
{
     pthread_mutex_lock (&preview.mutex);
     preview.active = 0;
     sample_load_file (preview.sample, name, samplerate);

     preview.next_frame = 0;
     preview.active = 1;
     pthread_mutex_unlock (&preview.mutex);
}

/* set the master volume */
int mixer_set_volume (float vol)
{
     if (vol < 0.0 || vol > 1.0)
	  return -1;

     volume = vol;
     return 0;
}

/* return the master volume */
float mixer_get_volume(void)
{
    return volume;
}

/* set internally assumed samplerate */
void mixer_set_samplerate (int rate)
{
     samplerate = rate;
}

/* destructor */
void mixer_shutdown ( )
{
     debug ("shutting down...\n");
     sample_free (preview.sample);
     debug ("done\n");
}