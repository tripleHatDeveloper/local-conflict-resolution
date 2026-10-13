/***************************************************************************
 **
 ** fi.h
 **
 ** describes a set of sets of clauses.
 ** maintains some intersection information
 **
 **************************************************************************/

#ifndef FI_H
#define FI_H

#include "clset.h"




/**     
 * growable int array
 */

struct st_garray;
typedef struct st_garray *garray;

struct st_garray
{
  int *array;
  int allotd;    /* size allocated */
  int lindex;    /* last used index */
  int chksize;   /* chunk size */
};


/**
 * stockage des sous ensembles de clause Fi pour constitution
 * d'un sous-ensemble inconsistant de la base
 */

struct st_fi;
typedef struct st_fi *fi;

struct st_fi 
{
  garray clauses;
  int lastch;
  fi next;
};

/**
 * fi context
 */
typedef struct st_fi_context *fi_context ;

struct st_fi_context 
{
  int *cl_occur;
  fi f;
};




int growable_at (garray a, int i);
int growable_size (garray a);
void growable_free (garray a);

void add_fi (fi_context fc, garray cl);
clset pick_fi (fi_context f, type_instance kb)
void freefi (fi_context f);


#endif

