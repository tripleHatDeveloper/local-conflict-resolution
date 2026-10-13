/***************************************************************************
 ***************************************************************************
 **
 ** various utilities
 **
 ***************************************************************************
 ***************************************************************************
 */

/**************************************************************************
 **
 ** Useful macros
 **
 **************************************************************************
 */
#ifndef UTILS_H
#define UTILS_H

#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <stdarg.h>

/* #ifndef NDEBUG */
/* #include "mnemosyne.h" */
/* #endif */

#define FORBIDEN { fprintf (stderr, "==== PASSAGE INTERDIT = %s:%d\n", \
                            __FILE__, __LINE__); \
			    exit (1);}
#define FORBIDEN2(s) { fprintf (stderr, "==== PASSAGE INTERDIT = %s:%d. %s\n", \
                            __FILE__, __LINE__, (s)); \
			    exit (1);}


#define NEW(Object) (Object *)malloc(sizeof (Object))
#define NEW_ARRAY(Object,nb) (Object *)calloc(nb,sizeof (Object))


#ifdef min
#undef min
#endif
#ifdef max
#undef max
#endif

#define min(x,y) ((x) < (y) ? (x) : (y) )  
#define max(x,y) ((x) < (y) ? (x) : (y) )  

/* ****************** prototypes ************************************** */

char ** load_names (char *file, type_instance kb);
void error (int code, char *fmt, ...);
void vmsg (char *fmt, ...);


#endif




