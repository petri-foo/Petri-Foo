#ifndef __SYNC_H__
#define __SYNC_H__

enum
{
     SYNC_DEFAULT_TEMPO = 120,
};

typedef enum
{
     SYNC_METHOD_MIDI,
     SYNC_METHOD_JACK,
}
SyncMethod;

void sync_start_midi (float bpm);
void sync_start_jack (float bpm);
void sync_set_method (SyncMethod method);

#endif /* __SYNC_H__ */

