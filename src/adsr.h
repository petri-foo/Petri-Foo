#ifndef __ADSR_H__
#define __ADSR_H__


#include <stdbool.h>

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

} ADSRState;


/* envelope parameters */
typedef struct _ADSRParams
{
    bool env_on;
    float delay;    /* delay length in seconds  */
    float attack;   /* attack length in seconds */
    float decay;    /* decay length in seconds  */
    float hold;     /* hold length in seconds   */
    float sustain;  /* sustain level [0.0, 1.0] */
    float release;  /* release length in seconds*/

    float key_amt;  /* key tracking amount      */
/*  float vel_amt;     velocity sensing amount  */

} ADSRParams;


/* adsr envelope structure */
typedef struct _ADSR ADSR;

void    adsr_params_init(ADSRParams*, float attack, float release);


ADSR*   adsr_new        (void);
void    adsr_free       (ADSR*);
void    adsr_init       (ADSR*);
void    adsr_release    (ADSR*);
void    adsr_set_params (ADSR*, ADSRParams*);
float   adsr_tick       (ADSR*);
void    adsr_trigger    (ADSR*, float key_val, float vel_val);


float const* adsr_output (ADSR*);


#endif /* __ADSR_H__ */
