/*
 * Project : Zeta, a toolbox for NP-complete problems.
 *
 * Module  : mtime : functions to get running times
 *
 * Author  : Antoine Rauzy
 *
 * Created : 95/09/17
 */

#include "mtime.h"

#ifdef __SUN_OS_TIME__
static struct tms tcpu;

clock_t clock()
{
  times(&tcpu);
  return(tcpu.tms_utime);
}
#endif

#ifdef __NO_TIME__
clock_t clock()
{
  return(0);
}
#endif

         /*---------*/

type_timer create_timer()
{
  type_timer timer;
  timer=(type_timer) malloc(sizeof(struct struct_timer));
  timer->accumulator = 0;

  return(timer);
}

         /*---------*/

void reinitialize_timer(type_timer self)
       /*
        -- This resets the accumulator to 0 and starts
        -- the chronometer.
        */
{
  self->accumulator = 0;
  self->chronometer = clock();
}

         /*---------*/

void start_timer(type_timer self)
{
  self->chronometer = clock();
}

         /*---------*/

void stop_timer(type_timer self)
{
  self->accumulator += clock() - self->chronometer;
}

         /*---------*/

void display_time_in_seconds(type_timer self, FILE* output)
{
  clock_t value = self->accumulator;

  fprintf(output,"%ld",value/MY_CLOCKS_PER_SEC);
  fprintf(output,"s");
  if ((value*100/MY_CLOCKS_PER_SEC)%100<10) fprintf(output,"0");
  fprintf(output,"%ld",(value*100/MY_CLOCKS_PER_SEC)%100);
}

         /*---------*/

char* string_time_in_seconds(type_timer self)
{
  char    buffer[256], *string_time;
  clock_t value, seconds, hundreds;
  short   n;
  
  for (n=0;n<256;n++) buffer[n]=0;
  value = self->accumulator;

  seconds=value/MY_CLOCKS_PER_SEC;
  hundreds=(value*100/MY_CLOCKS_PER_SEC)%100;
  if (hundreds<10) sprintf(buffer,"%lus0%lu",seconds,hundreds);
  else             sprintf(buffer,"%lus%lu",seconds,hundreds);

  string_time=(char*) calloc(sizeof(char),strlen(buffer)+1);
  strcpy(string_time,buffer);
  
  return(string_time);
}

         /*---------*/

long get_time_in_milliseconds(type_timer self)
{
  clock_t value = self->accumulator;

  return( (value*1000)/MY_CLOCKS_PER_SEC );
}

         /*---------*/
