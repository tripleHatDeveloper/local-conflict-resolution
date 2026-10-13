/*
 * Project : Zeta, a toolbox for NP-complete problems.
 *
 * Module  : mtime : functions to get running times
 *
 * Author  : Antoine Rauzy
 *
 * Created : 95/09/17
 */

/*
 -- Description :
 -- -------------
 --   A timer must be view as a counter in which the running
 --   is stored. A chronometer is associated to the counter.
 --   It can be started and stopped. The time beetween the start
 --   and stop commands is added to the counter.
 --   CAREFUL : Despite what is written on ANSI C manual
 --     running time measurement is not standard.
 --     This file would surely be modified when working
 --     on a new platform.
 --     If the environment variable __ANSI_C_TIME__ is defined in
 --     the include file <zetaenv.h>, the ANSI C norm is applied.
 --     For instance, on SUN OS machines, that do not implement this
 --     norm, the variable __ANSI_C_TIME__ must be undefined (special
 --     instructions are given through the directive __SUN_OS_TIME__).
 --     If everything fails, the variable __NO_TIME__
 --     can be defined and no computation times are
 --     get (0s00 is always displayed).
 --     The ANSI C norm defines the macro CLOCKS_PER_SEC
 --     that normaly gives the number of clock tics per
 --     second. It appears that this value is in general
 --     false. It must be customized by hand (macro
 --     MY_CLOCKS_PER_SEC)
 --
 -- Last modifications :
 -- --------------------
 --   95/02/06 by Antoine Rauzy
 */

#ifndef __RUNNING_TIMES__
#define __RUNNING_TIMES__
#include "zetaenv.h"

       /*
        -- ANSI C jokes
        */
#ifdef __ANSI_C_TIME__
#include <time.h>
#define MY_CLOCKS_PER_SEC CLOCKS_PER_SEC
#endif

#ifdef __SUN_OS_TIME__
#include <sys/times.h>
#define MY_CLOCKS_PER_SEC 60
#endif

#ifdef __NO_TIME__
/*
typedef unsigned long clock_t;
*/
#define MY_CLOCKS_PER_SEC 1
#endif


/*
 -- Timers
 -- ------
 */

struct struct_timer;
typedef struct struct_timer* type_timer;

struct struct_timer {
  clock_t chronometer;
  clock_t accumulator;
  };


type_timer create_timer();
       /*
        -- This creates a new timer
        -- ENSURE  : accumulator = 0
        */

void reinitialize_timer(type_timer timer);
       /*
        -- This resets the accumulator to 0 and starts
        -- the chronometer.
        */

void start_timer(type_timer timer);
       /*
        -- This starts the chronometer.
        */

void stop_timer(type_timer timer);
       /*
        -- This stops the chronometer and adds its value to the accumulator.
        -- REQUIRE : the start_timer command must have been called
        */

void display_time_in_seconds(type_timer timer, FILE* output);
       /*
        -- This displays the value of the accumulator in seconds
        --    example : 123s45
        */

char* string_time_in_seconds(type_timer timer);
       /*
        -- This returns a string containing the value of the
        -- accumulator in seconds
        --    example : 123s45
        */

long get_time_in_milliseconds(type_timer self);
       /*
        -- This returns the time in milliseconds.
        */

#endif


