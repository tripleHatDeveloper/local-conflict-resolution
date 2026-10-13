/*********************************************************************
**********************************************************************
**
** Time utilities
**
**********************************************************************
*********************************************************************/

#include <stdlib.h>
#include <stdio.h>
#include <sys/times.h>
#include <sys/resource.h>
#include <time.h>
#include <unistd.h>
#include <assert.h>

#include "public-types.h"
#include "utils.h"
#include "timer.h"

/*======================================================================
 *
 * static global data
 *
 *======================================================================*/

/* stop points */
static clock_t *Top;
static int NumTops;
static int CurTop;

/*======================================================================
 * 
 * local functions prototypes
 *
 *======================================================================*/


/*======================================================================
 *
 * Functions
 *
 *======================================================================*/

/**
 * init
 *
 * n: number of available tops
 */
void init_timer(int n)
{
  CurTop = 0;
  NumTops = n;
  Top = NEW_ARRAY(clock_t, n);
}

/**
 * take a time measurement
 */
int top(void)
{
  Top[CurTop] = clock();
  CurTop++;
  assert(CurTop <= NumTops);
  return CurTop-1;
}

/**
 * print time info about a task between two tops
 *
 * s: string description of the task
 * t1, t2: the two tops
 */
void printTime(char *st, int t1, int t2)
{

  assert(t1 <= CurTop && t2 <= CurTop && t1 < t2);

  printf("%ld / %ld\n", Top[t1], Top[t2]);
  
  if ( Top[t1] == (clock_t)(-1) || Top[t2] == (clock_t)(-1) )
    {
      fprintf(stderr, "######### WARNING !!! bad clock value !\n");
    }

  printf(">>> %s : %f s\n", 
	 st,
	 ((double)(Top[t2] - Top[t1]) / (double)CLOCKS_PER_SEC));
}










