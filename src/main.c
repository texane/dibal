/*
** Made by fabien le mentec <texane@gmail.com>
** 
** Started on  Sun Sep 20 14:08:30 2009 texane
** Last update Mon Oct 12 10:49:25 2009 texane
*/



#include <pic18fregs.h>
#include "config.h"
#include "int.h"
#include "osc.h"
#include "srf04.h"
#include "adc.h"
#include "sched.h"
#include "light.h"
#include "move.h"



/* bitmap macros */

#define BIT_MAP_SET(M, B) do { (M) |= (B); } while (0)
#define BIT_MAP_CLEAR(M, B) do { (M) &= ~(B); } while (0)
#define BIT_MAP_ISSET(M, B) ((M) & (B))


/* timer handlers */

static volatile unsigned char timer_map = 0;


#define TIMER_MAP_BIT_LIGHT (1 << 0)
#define TIMER_MAP_BIT_DISTANCE (1 << 1)

#define TIMER_MAP_SET(B) BIT_MAP_SET(timer_map, TIMER_MAP_BIT_ ## B)
#define TIMER_MAP_CLEAR(B) BIT_MAP_CLEAR(timer_map, TIMER_MAP_BIT_ ## B)
#define TIMER_MAP_ISSET(B) BIT_MAP_ISSET(timer_map, TIMER_MAP_BIT_ ## B)


static void on_light_timer(void)
{
  TIMER_MAP_SET(LIGHT);
}


static void on_distance_timer(void)
{
  TIMER_MAP_SET(DISTANCE);  
}


/* light tracker behaviour */

struct light_tracker_state
{
  unsigned char is_done;

#define LIGHT_TRACKER_STATE_INIT 0
#define LIGHT_TRACKER_STATE_SENSE 1
#define LIGHT_TRACKER_STATE_ROTATE 2
#define LIGHT_TRACKER_STATE_WAIT 3
#define LIGHT_TRACKER_STATE_DONE 4
  unsigned char state;

  sched_timer_t* timer;
  unsigned short prev_delta;
  unsigned short rem_delta;
};


typedef struct light_tracker_state light_tracker_state_t;


static void light_tracker_start(light_tracker_state_t* lts)
{
  lts->is_done = 0;

  lts->state = LIGHT_TRACKER_STATE_INIT;

#ifndef NULL
# define NULL (void*)0
#endif
  lts->timer = NULL;

  lts->prev_delta = ADC_MAX_VALUE;
  lts->rem_delta = 0;
}


static void light_tracker_stop(light_tracker_state_t* lts)
{
  if (lts->is_done)
    return ;

  move_stop();

  if (lts->timer != NULL)
    {
      sched_del_timer(lts->timer);
      lts->timer = NULL;
    }

  lts->is_done = 1;
}


static volatile unsigned char is_rotation_done;


static void on_light_tracker_timer(void)
{
  is_rotation_done = 1;
}


static unsigned int delta_to_freq(unsigned short delta)
{
#define QUADRAN_FREQ 2
  return (QUADRAN_FREQ * ADC_QUANTIZE_5_10(2.5)) / delta;
}


static unsigned int freq_to_delta(unsigned int freq)
{
  return (QUADRAN_FREQ * ADC_QUANTIZE_5_10(2.5)) / freq;
}


static void light_tracker_next(light_tracker_state_t* lts)
{
  switch (lts->state)
    {
    case LIGHT_TRACKER_STATE_INIT:
      {
	lts->state = LIGHT_TRACKER_STATE_SENSE;

	/* fallthru */
      }

    case LIGHT_TRACKER_STATE_SENSE:
      {
	void (*rotate)(void);
	unsigned short cur_level;
	unsigned short cur_delta;

	cur_level = adc_read(LIGHT_ADC_CHANNEL);

	if (cur_level >= ADC_QUANTIZE_5_10(2.35))
	  if (cur_level <= ADC_QUANTIZE_5_10(2.65))
	    {
	      /* found, we are done */

	      lts->state = LIGHT_TRACKER_STATE_DONE;

	      break;
	    }

	if (cur_level >= ADC_QUANTIZE_5_10(2.5))
	  {
	    cur_delta = cur_level - ADC_QUANTIZE_5_10(2.5);
	    rotate = move_rotate_left;
	  }
	else
	  {
	    cur_delta = ADC_QUANTIZE_5_10(2.5) - cur_level;
	    rotate = move_rotate_right;
	  }

	if (lts->prev_delta < cur_delta)
	  {
	    /* diverging, we are done */

	    lts->state = LIGHT_TRACKER_STATE_DONE;

	    break;
	  }

	lts->prev_delta = cur_delta;
	lts->rem_delta = cur_delta;

	rotate();

	lts->timer = sched_add_timer(0, on_light_tracker_timer, 0);

	lts->state = LIGHT_TRACKER_STATE_ROTATE;

	break;
      }

    case LIGHT_TRACKER_STATE_ROTATE:
      {
	unsigned int tmp;

	if (!lts->rem_delta)
	  {
	    /* rotation done */

	    move_stop();

	    sched_del_timer(lts->timer);
	    lts->timer = NULL;

	    lts->state = LIGHT_TRACKER_STATE_SENSE;

	    break;
	  }

	tmp = delta_to_freq(lts->rem_delta);
	if (tmp > SCHED_MAX_FREQ)
	  tmp = SCHED_MAX_FREQ;

	sched_set_timer_freq(lts->timer, tmp);

	tmp = freq_to_delta(tmp);
	if (tmp > lts->rem_delta)
	  tmp = lts->rem_delta;

	lts->rem_delta -= tmp;

	lts->state = LIGHT_TRACKER_STATE_WAIT;
	is_rotation_done = 0;
	sched_enable_timer(lts->timer);

	break;
      }

    case LIGHT_TRACKER_STATE_WAIT:
      {
	/* wait for the rotation to finish */

	if (!is_rotation_done)
	  break;

	sched_disable_timer(lts->timer);

	lts->state = LIGHT_TRACKER_STATE_ROTATE;

	break;
      }

    case LIGHT_TRACKER_STATE_DONE:
    default:
      {
	light_tracker_stop(lts);

	break;
      }
    }
}


static int light_tracker_is_done(light_tracker_state_t* lts)
{
  return lts->is_done;
}



/* object avoider behaviour */

struct object_avoider_state
{
  unsigned char is_done;
  sched_timer_t* timer;
};


typedef struct object_avoider_state object_avoider_state_t;


static void object_avoider_start(object_avoider_state_t* oas)
{
  oas->is_done = 0;
  oas->timer = NULL;
}


static volatile unsigned char is_rotate_done;

static void on_rotate_timer(void)
{
  is_rotate_done = 1;
}


static void object_avoider_stop(object_avoider_state_t* oas)
{
  if (oas->is_done)
    return ;

  move_stop();

  if (oas->timer != NULL)
    {
      sched_del_timer(oas->timer);
      oas->timer = NULL;
    }

  oas->is_done = 1;
}


static void object_avoider_next(object_avoider_state_t* oas)
{
  if (oas->timer == NULL)
    {
      /* rotate one quadran (freq ~= 2) */

      is_rotate_done = 0;
      move_rotate_left();
      oas->timer = sched_add_timer(2, on_rotate_timer, 1);
    }
  else if (is_rotate_done)
    {
      object_avoider_stop(oas);
    }
}


static int object_avoider_is_done(object_avoider_state_t* oas)
{
  return oas->is_done;
}



/* land explorer behaviour */

struct land_explorer_state
{
  unsigned char is_done;
};


typedef struct land_explorer_state land_explorer_state_t;


static void land_explorer_start(land_explorer_state_t* les)
{
  les->is_done = 0;

  move_go_forward();
}


static void land_explorer_next(land_explorer_state_t* les)
{
  les;

  /* we have to do it here since this is
     the only place we know there is room
     for sleeping without blocking any next
     function. idle mode chosen so that timers
     related events continue to occur, pwms
     signals to be generated...
   */

  osc_set_power(OSC_PMODE_IDLE);
}


static void land_explorer_stop(land_explorer_state_t* les)
{
  move_stop();

  les->is_done = 1;
}


static int land_explorer_is_done(land_explorer_state_t* les)
{
  return les->is_done;
}


/* behaviour interface */

struct behavior_state
{
#define BEHAVIOR_ID_LAND_EXPLORER 0
#define BEHAVIOR_ID_LIGHT_TRACKER 1
#define BEHAVIOR_ID_OBJECT_AVOIDER 2
#define BEHAVIOR_ID_DEFAULT BEHAVIOR_ID_LAND_EXPLORER
  unsigned char id;

  void (*next)(void*);
  void (*stop)(void*);
  int (*is_done)(void*);

  void* statep;

  union
  {
    land_explorer_state_t land_explorer;
    object_avoider_state_t object_avoider;
    light_tracker_state_t light_tracker;
  } state;
};


typedef struct behavior_state behavior_state_t;


static behavior_state_t current_behavior;


static void clear_behavior(behavior_state_t* bs)
{
  bs->next = NULL;
  bs->stop = NULL;
  bs->is_done = NULL;

  bs->statep = NULL;
}


static void behavior_setup(void)
{
  /* must be called before others */

  clear_behavior(&current_behavior);
}


static void behavior_start(unsigned char id)
{
  clear_behavior(&current_behavior);

  current_behavior.id = id;

  /* fill dependant ops and data */

  switch (id)
    {
    case BEHAVIOR_ID_LAND_EXPLORER:
      {
	current_behavior.next = (void(*)(void*))land_explorer_next;
	current_behavior.stop = (void(*)(void*))land_explorer_stop;
	current_behavior.is_done = (int(*)(void*))land_explorer_is_done;

	current_behavior.statep = &current_behavior.state.land_explorer;

	land_explorer_start(current_behavior.statep);

	break;
      }

    case BEHAVIOR_ID_LIGHT_TRACKER:
      {
	current_behavior.next = (void(*)(void*))light_tracker_next;
	current_behavior.stop = (void(*)(void*))light_tracker_stop;
	current_behavior.is_done = (int(*)(void*))light_tracker_is_done;

	current_behavior.statep = &current_behavior.state.light_tracker;

	light_tracker_start(current_behavior.statep);

	break;
      }

    case BEHAVIOR_ID_OBJECT_AVOIDER:
      {
	current_behavior.next = (void(*)(void*))object_avoider_next;
	current_behavior.stop = (void(*)(void*))object_avoider_stop;
	current_behavior.is_done = (int(*)(void*))object_avoider_is_done;

	current_behavior.statep = &current_behavior.state.object_avoider;

	object_avoider_start(current_behavior.statep);

	break;
      }

    default:
      {
	break;
      }
    }
}


static void behavior_stop(void)
{
#if 0

  if (current_behavior.stop != NULL)
    current_behavior.stop(current_behavior.statep);

#else

  switch (current_behavior.id)
    {
    case BEHAVIOR_ID_LAND_EXPLORER:
      {
	land_explorer_stop(current_behavior.statep);
	break;
      }

    case BEHAVIOR_ID_LIGHT_TRACKER:
      {
	light_tracker_stop(current_behavior.statep);
	break;
      }

    case BEHAVIOR_ID_OBJECT_AVOIDER:
      {
	object_avoider_stop(current_behavior.statep);
	break;
      }

    default:
      break;
    }

#endif
}


static int behavior_is_done(void)
{
  /* is the current behavior done */

#if 0

  if (current_behavior.is_done != NULL)
    return current_behavior.is_done(current_behavior.statep);

  return 0;

#else

  switch (current_behavior.id)
    {
    case BEHAVIOR_ID_LAND_EXPLORER:
      {
	return land_explorer_is_done(current_behavior.statep);
	break;
      }

    case BEHAVIOR_ID_LIGHT_TRACKER:
      {
	return light_tracker_is_done(current_behavior.statep);
	break;
      }

    case BEHAVIOR_ID_OBJECT_AVOIDER:
      {
	return object_avoider_is_done(current_behavior.statep);
	break;
      }

    default:
      break;
    }

  return 0;

#endif
}


static unsigned int prio_by_id(unsigned char id)
{
  static unsigned char prio_table[] =
    {
#define BEHAVIOR_PRIO_LAND_EXPLORER 0
#define BEHAVIOR_PRIO_LIGHT_TRACKER 1
#define BEHAVIOR_PRIO_OBJECT_AVOIDER 2

      BEHAVIOR_PRIO_LAND_EXPLORER,
      BEHAVIOR_PRIO_LIGHT_TRACKER,
      BEHAVIOR_PRIO_OBJECT_AVOIDER
    };

  return prio_table[id];
}


static int behavior_switch(unsigned char id)
{
  /* if the current behavior is done, dont
     take into the account the priority and
     dont stop it.
   */

  if (!behavior_is_done())
    {
      if (prio_by_id(id) < prio_by_id(current_behavior.id))
	return -1;

      behavior_stop();
    }

  /* start the new one */

  behavior_start(id);

  return 0;
}


static void behavior_next(void)
{
  /* next behavior iter */

#if 0

  if (current_behavior.next != NULL)
    current_behavior.next(current_behavior.statep);

#else

  switch (current_behavior.id)
    {
    case BEHAVIOR_ID_LAND_EXPLORER:
      {
	land_explorer_next(current_behavior.statep);
	break;
      }

    case BEHAVIOR_ID_LIGHT_TRACKER:
      {
	light_tracker_next(current_behavior.statep);
	break;
      }

    case BEHAVIOR_ID_OBJECT_AVOIDER:
      {
	object_avoider_next(current_behavior.statep);
	break;
      }

    default:
      break;
    }

#endif
}



/* main */

void main(void)
{
  sched_timer_t* light_timer;
  sched_timer_t* dist_timer;

  unsigned char is_light_disabled;
  unsigned char is_dist_disabled;

  osc_setup();
  int_setup();

  srf04_setup();
  sched_setup();

  behavior_setup();

  /* sensor timers */

  light_timer = sched_add_timer(2, on_light_timer, 1);
  is_light_disabled = 0;

  dist_timer = sched_add_timer(2, on_distance_timer, 1);
  is_dist_disabled = 0;

  /* default behaviour */

  behavior_start(BEHAVIOR_ID_LAND_EXPLORER);

  /* main loop */

  sched_enable();

  while (1)
    {
      /* sensors. order matters so that object
	 avoider takes priority over light tracker.
	 this can be removed as soon as behavior
	 priority is implemented.
       */

      if (TIMER_MAP_ISSET(DISTANCE))
	{
	  {
#define MIN_DISTANCE_VALUE 0x0a00 /* 20 cms */
	    unsigned int dist = srf04_get_distance(MIN_DISTANCE_VALUE);

	    if (dist <= MIN_DISTANCE_VALUE)
	      {
		if (behavior_switch(BEHAVIOR_ID_OBJECT_AVOIDER) != -1)
		  {
		    sched_disable_timer(dist_timer);
		    is_dist_disabled = 1;
		  }
	      }
	  }

	  /* clear after the timer may have been disabled */

	  TIMER_MAP_CLEAR(DISTANCE);
	}

      if (TIMER_MAP_ISSET(LIGHT))
	{
	  {
	    unsigned short light = adc_read(LIGHT_ADC_CHANNEL);

	    if ((light <= ADC_QUANTIZE_5_10(2.35)) || (light >= ADC_QUANTIZE_5_10(2.65)))
	      {
		if (behavior_switch(BEHAVIOR_ID_LIGHT_TRACKER) != -1)
		  {
		    sched_disable_timer(light_timer);
		    is_light_disabled = 1;
		  }
	      }
	  }

	  TIMER_MAP_CLEAR(LIGHT);
	}

      /* schedule behavior */

      behavior_next();

      if (behavior_is_done())
	{
	  /* reenable timer */

	  if (is_light_disabled)
	    {
	      sched_enable_timer(light_timer);
	      is_light_disabled = 0;
	    }

	  if (is_dist_disabled)
	    {
	      sched_enable_timer(dist_timer);
	      is_dist_disabled = 0;
	    }

	  /* switch to default behavior */

	  behavior_switch(BEHAVIOR_ID_DEFAULT);
	}
    }
}
