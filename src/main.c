/*
** Made by fabien le mentec <texane@gmail.com>
** 
** Started on  Sun Sep 20 14:08:30 2009 texane
** Last update Sun Oct  4 23:26:32 2009 texane
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


static void light_tracker_init(light_tracker_state_t* lts)
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
	lts->is_done = 1;

	break;
      }
    }
}



/* object avoider behaviour */

struct object_avoider_state
{
  unsigned char is_done;
  sched_timer_t* timer;
};


typedef struct object_avoider_state object_avoider_state_t;


static void object_avoider_init(object_avoider_state_t* oas)
{
  oas->is_done = 0;
  oas->timer = NULL;
}


static volatile unsigned char is_rotate_done;

static void on_rotate_timer(void)
{
  is_rotate_done = 1;
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
      sched_del_timer(oas->timer);
      oas->timer = NULL;
      oas->is_done = 1;
    }
}



/* land explorer behaviour */

struct land_explorer_state
{
  unsigned char is_done;
};


typedef struct land_explorer_state land_explorer_state_t;


static void land_explorer_init(land_explorer_state_t* les)
{
  les->is_done = 0;
  move_go_forward();
}


static void land_explorer_next(land_explorer_state_t* les)
{
  les;
}


static void land_explorer_stop(land_explorer_state_t* les)
{
  les;

  move_stop();
}


static void land_explorer_resume(land_explorer_state_t* les)
{
  les;

  move_go_forward();
}


/* behaviour interface */

struct behavior
{
#define BEHAVIOR_ID_LAND_EXPLORER 0
#define BEHAVIOR_ID_LIGHT_TRACKER 1
#define BEHAVIOR_IDOBJECT_AVOIDER 2
  unsigned char id;

};


typedef struct behavior behavior_t;


static void fill_ops(struct behavior)
{
}


static void behavior_init(behavior_t* b, unsigned char id)
{
  b->id = id;
}


static void behavior_switch(behavior_t* b, unsigned char id)
{
  if (b->id == id)
    return ;

  if (b->stop != NULL)
    b->stop();
}




/* main */

void main(void)
{
  sched_timer_t* light_timer;
  sched_timer_t* dist_timer;

  land_explorer_state_t les;
  object_avoider_state_t oas;
  light_tracker_state_t lts;

  osc_setup();
  int_setup();
  srf04_setup();
  sched_setup();

  /* sensor timers */

  light_timer = sched_add_timer(1, on_light_timer, 1);
  dist_timer = sched_add_timer(2, on_distance_timer, 1);

  /* default behaviour */

  behaviour = BEHAVIOR_LAND_EXPLORER;
  land_explorer_init(&les);

  /* main loop */

  sched_enable();

  while (1)
    {
      /* sensors */

      if (TIMER_MAP_ISSET(DISTANCE))
	{
	  TIMER_MAP_CLEAR(DISTANCE);

	  {
	    unsigned int dist = srf04_get_distance();

#define MIN_DISTANCE_VALUE 0x0a00 /* 20 cms */
	    if (dist <= MIN_DISTANCE_VALUE)
	      {
		sched_disable_timer(dist_timer);

		BEHAV_MAP_CLEAR(LAND_EXPLORER);
		land_explorer_stop(&les);

		BEHAV_MAP_SET(OBJECT_AVOIDER);
		object_avoider_init(&oas);
	      }
	  }
	}

      if (TIMER_MAP_ISSET(LIGHT))
	{
	  TIMER_MAP_CLEAR(LIGHT);

	  {
	    unsigned short light = adc_read(LIGHT_ADC_CHANNEL);

	    if ((light <= ADC_QUANTIZE_5_10(2.35)) || (light >= ADC_QUANTIZE_5_10(2.65)))
	      {
		sched_disable_timer(light_timer);

		BEHAV_MAP_SET(LIGHT_TRACKER);
		light_tracker_init(&lts);
	      }
	  }
	}

      /* behaviors */

      if (BEHAV_MAP_ISSET(OBJECT_AVOIDER))
	{
	  object_avoider_next(&oas);

	  if (oas.is_done)
	    {
	      sched_enable_timer(dist_timer);

	      BEHAV_MAP_CLEAR(OBJECT_AVOIDER);

	      BEHAV_MAP_SET(LAND_EXPLORER);
	      land_explorer_resume(&les);
	    }
	}

      if (BEHAV_MAP_ISSET(LIGHT_TRACKER))
	{
	  light_tracker_next(&lts);

	  if (lts.is_done)
	    {
	      sched_enable_timer(light_timer);

	      BEHAV_MAP_CLEAR(LIGHT_TRACKER);
	    }
	}

      if (BEHAV_MAP_ISSET(LAND_EXPLORER))
	{
	  land_explorer_next(&les);
	  if (les.is_done)
	    {
	      BEHAV_MAP_CLEAR(LAND_EXPLORER);
	    }
	}
    }
}
