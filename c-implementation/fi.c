
#include "utils.h"
#include "fi.h"

typedef struct st_sort_occur  *sort_occur;

struct st_sort_occur 
{
  int cl;
  int occur;
  sort_occur next;
};

/**
 * Creates and initializes a new fi context.
 *
 * kb : the knowledge base
 */
fi_context 
init_fi (type_instance kb)
{
  int i;
  fi_context new = NEW(struct st_fi_context);

  assert (new != NULL);

  new->f = NULL;
  new->cl_occur = NEW_ARRAY(int, CLAUSE_NUMBER(kb)+1);
  
  assert (new->cl_occur != NULL);
  for (i = 0 ; i < CLAUSE_NUMBER(kb)+1 ; i++)
    new->cl_occur[i] = 0;

  return new;
}
  

/**
 * adds an Fi set (empty clauses set)
 */
void
addfi (fi_context fc,  garray cl)
{
  int i;
  fi new = NEW (struct st_fi);

  assert (new != NULL);

  new->clauses = cl;
  new->lastch = 0;
  new->next = fc->f;
  fc->f = new;

  for (i = 0 ; i < growable_size (cl) ; i++)
    gc->cl_occur[growable_at (cl, i)]++;

  return new;
}

/**
 * alloacates an int array growable by chunks
 */
garray
new_growable (int init, int grow)
{
  garray new = NEW(struct st_garray);

  
  assert ( init != 0 && grow != 0 );
  assert ( new != NULL );

  new->chksize = grow;
  new->array = NEW_ARRAY(int, init);
  new->allotd = init;
  new->lindex = -1; /* last used index */

  return new;
}

/**
 * adds an element into growable array
 */
void
growable_add (garray a, int e)
{
  assert (a != NULL);

  a->lindex++;
  if ( a->allotd <= a->lindex )
    {
      a->allotd += a->chksize;
      a->array = (int *) realloc (a->array, a->allotd);
      assert ( a->array != NULL );
    }
  a->array[a->lindex] = e;
}

/**
 * access an element from a growable array
 */
int 
growable_at (garray a, int i)
{
  assert ( i <= a->lindex );
  return a->array[i];
}

int 
growable_size (garray a)
{
  return a->lindex + 1;
}

/**
 * free a growable array
 */
void
growable_free (garray a)
{
  free (a->array);
  free (a);
}

static sort_occur 
sort_occurences (fi_context f, type_intance kb)
{
  int i;
  /* sorts the occurences array */
  sort_occur so = NULL;

  for (i = 1 ; i <= CLAUSE_NUMBER(kb) ; i++)
    {
      sort_occur p = so;
      sort_occur prev = NULL;
      while (p && p->occur > f->cl_occur[i])
	{
	  prev = p;
	  p = p->next;
	}
      if (p == NULL)
	{
	  if (prev == NULL)
	    { /* first insertion */
	      so = NEW (struct st_sort_occur);
	      assert (so != NULL);
	      so->cl = i;
	      so->occur = f->cl_occur[i];
	      so->next = NULL;
	    }
	  else
	    { /* appending */
	      sort_occur new = NEW (struct st_sort_occur);
	      assert (new != NULL);
	      new->cl = i;
	      new->occur = f->cl_occur[i];
	      new->next = p;
	      prev->next = new;
	    }
	}
      else
	{
	  if (prev == NULL)
	    { /* prepending */
	      sort_occur new = NEW (struct st_sort_occur);
	      assert (new != NULL);
	      new->cl = i;
	      new->occur = f->cl_occur[i];
	      new->next = so;
	      so->next = new;
	    }
	  else
	    {
	      sort_occur new = NEW (struct st_sort_occur);
	      assert (new != NULL);
	      new->cl = i;
	      new->occur = f->cl_occur[i];
	      new->next = p;
	      prev->next = new;
	    }
	}
    }
  return so;
}

static int
choose_best (garray cl, sort_occur so)
{
  sort_occur ps = so;
  int i ;
  int max_occ = 0;
  int max_cl;

  for (i = 0 ; i < garray_size (cl) ; i++)
    {
      
    }

  assert (max_occ != 0);

clset 
pick_fi (fi_context f, type_intance kb)
{
  sort_occur so = sort_occurences (f, kb);
  clset s = clset_new (CLAUSE_NUMBER(kb));
  
  assert (s != NULL);
  fi p = f->f;

  while ( p )
    {
      clset_add (s, choose_best(p->clauses, so));
      p = p->next;
    }
  return s;

}

/**
 * free a fi list
 */
void
freefi (fi_context fc)
{
  assert (fc != NULL);

  free (fc->cl_occur);
  fi p = fc->f;
  fi del;
  while (p)
    {
      growable_free (p->clauses);
      del = p;
      p = p->next;
      free(del);
    }
}
