#ifndef __ADSR_H__
#define __ADSR_H__


#include "petri-foo.h"
#include "ticks.h"


/* envelope state flags */
typedef enum _ADSRState
{
     ADSR_STATE_IDLE,
     ADSR_STATE_DELAY,
     ADSR_STATE_ATTACK,
     ADSR_STATE_HOLD,
     ADSR_STATE_DECAY,
     ADSR_STATE_SUSTAIN,
     ADSR_STATE_RELEASE
}
ADSRState;

/* envelope parameters */
typedef struct _ADSRParams
{
    gboolean env_on;
     float delay;		/* delay length in seconds */
     float attack;		/* attack length in seconds */
     float decay;		/* decay length in seconds */
     float hold;		/* hold length in seconds */
     float sustain;		/* sustain level [0.0, 1.0] */
     float release;		/* release length in seconds */
}
ADSRParams;

/* adsr envelope structure */
typedef struct _ADSR
{
    ADSRState state;		/* what phase of the envelope we are in */
     Tick      ticks;		/* how far along we are in the current state */
     float     val;		/* our current value [0.0, 1.0] */
     float     rval;		/* value we released at */
     Tick      delay;		/* delay length in ticks */
     Tick      attack;		/* attack length in ticks */
     Tick      hold;		/* hold length in ticks */
     Tick      decay;		/* decay length in ticks */
     float     sustain;		/* sustain level [0.0, 1.0] */
     Tick      release;		/* release length in ticks */
}
ADSR;

void  adsr_init       (ADSR* env);
void  adsr_release    (ADSR* env);
void  adsr_set_params (ADSR* env, ADSRParams* params);
float adsr_tick       (ADSR* env);
void  adsr_trigger    (ADSR* env);

#endif /* __ADSR_H__ */
