#ifndef __MIDI_H__
#define __MIDI_H__

/* magic numbers */
enum
{
    MIDI_ERR_SEQ = -1,	/* error opening sequencer */
    MIDI_ERR_PORT = -2,	/* error creating port */
    MIDI_NOTES = 128,	/* number of notes */
    MIDI_CHANS = 16,	/* number of channels */
};

int  midi_start         (void);
int  midi_get_client_id (void);
void midi_stop          (void);

#endif /* __MIDI_H__ */
