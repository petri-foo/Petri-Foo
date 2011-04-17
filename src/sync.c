#include "petri-foo.h"
#include "midi.h"
#include "patch.h"
#include "patch_util.h"
#include "sync.h"

static SyncMethod sync_method = SYNC_METHOD_MIDI;

void sync_start_midi (float bpm)
{
     if (bpm <= 0)
     {
	  debug ("ignoring midi sync start with invalid bpm: %f\n", bpm);
	  return;
     }

     if (sync_method == SYNC_METHOD_MIDI)
     {
	  debug ("midi sync started, bpm: %f\n", bpm);
	  patch_sync (bpm);
     }
     else
     {
	  debug ("ignoring midi sync start\n");
     }
}

void sync_start_jack (float bpm)
{
     if (bpm <= 0)
     {
	  debug ("ignoring jack sync start with invalid bpm: %f\n", bpm);
	  return;
     }

     if (sync_method == SYNC_METHOD_JACK)
     {
	  debug ("jack sync started, bpm: %f\n", bpm);
	  patch_sync (bpm);
     }
     else
     {
	  debug ("ignoring jack sync start\n");
     }
}

void sync_set_method (SyncMethod method)
{
     if (method == SYNC_METHOD_JACK)
     {
	  debug ("syncing to JACK now\n");
	  sync_method = SYNC_METHOD_JACK;
     }
     else
     {
	  debug ("syncing to MIDI now\n");
	  sync_method = SYNC_METHOD_MIDI;
     }
}
