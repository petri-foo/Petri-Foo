#include "petri-foo.h"
#include "adsr.h"
#include "maths.h"
#include "ticks.h"


void adsr_init (ADSR* env)
{
     env->state   = ADSR_STATE_IDLE;
     env->ticks   = 0;
     env->val     = 0.0;
     env->rval    = 0.0;
     env->delay   = 0;
     env->attack  = 0;
     env->hold    = 0;
     env->decay   = 0;
     env->sustain = 1.0;
     env->release = 0;
}

void adsr_trigger (ADSR* env)
{
    env->state = ADSR_STATE_DELAY;
    env->ticks = 0;
    env->aval = env->val;

/* no re-zero:     env->val = 0; */
/*  this doesn't need zeroing here anyway env->rval = 0; */
}

void adsr_release (ADSR* env)
{
     env->state = ADSR_STATE_RELEASE;
     env->ticks = 0;
}

/* return current value between 0 and 1 and advance */
float adsr_tick (ADSR* e)
{
     float d;

     switch (e->state)
     {
     case ADSR_STATE_DELAY:
	  if (e->delay)
        e->val = 0.0;

	  if (++e->ticks > e->delay)
	  {
	       e->state = ADSR_STATE_ATTACK;
	       e->ticks = 0;
	  }
	  break;
     case ADSR_STATE_ATTACK:
	  if (e->attack == 0)
	  {
	       e->val = 1.0;
	  }
	  else
	  {
	       d = (e->ticks * 1.0) / e->attack;
	       e->val = lerp (e->aval, 1.0, d);
	  }

	  if (++e->ticks > e->attack)
	  {
	       e->state = ADSR_STATE_HOLD;
	       e->ticks = 0;
	  }
	  break;
     case ADSR_STATE_HOLD:
	  e->val = 1.0;

	  if (++e->ticks > e->hold)
	  {
	       e->state = ADSR_STATE_DECAY;
	       e->ticks = 0;
	  }
	  break;
     case ADSR_STATE_DECAY:
	  if (e->decay == 0)
	  {
	       e->val = e->sustain;
	  }
	  else
	  {
	       d = (e->ticks * 1.0) / e->decay;
	       e->val = lerp (1.0, e->sustain, d);
	  }

	  if (++e->ticks > e->decay)
	  {
	       e->state = ADSR_STATE_SUSTAIN;
	       e->ticks = 0;
	  }
	  break;
     case ADSR_STATE_SUSTAIN:
	  /* e->val just hovers at e->sustain */
	  e->val = e->sustain;
	  break;
     case ADSR_STATE_RELEASE:
	  if (e->release == 0)
	  {
	       e->val = 0.0;
	  }
	  else if (e->ticks == 0)
	  {
	       /* we are just beginning to release, store our current
		* value for later use */
	       e->rval = e->val;
	  }
	  else
	  {
	       d = (e->ticks * 1.0) / e->release;
	       e->val = lerp (e->rval, 0.0, d);
	  }

	  if (++e->ticks > e->release)
	  {
	       e->state = ADSR_STATE_IDLE;
	       e->ticks = 0;
	  }
	  break;
     case ADSR_STATE_IDLE:	/* fall through */
     default:
	  e->val = 0.0;
	  break;
     }

     return e->val;
}

void adsr_set_params (ADSR* env, ADSRParams* params)
{
     env->delay   = ticks_secs_to_ticks (params->delay);
     env->attack  = ticks_secs_to_ticks (params->attack);
     env->hold    = ticks_secs_to_ticks (params->hold);
     env->decay   = ticks_secs_to_ticks (params->decay);
     env->sustain = params->sustain;
     env->release = ticks_secs_to_ticks (params->release);
}
