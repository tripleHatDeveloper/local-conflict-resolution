/****************************************************************************
 ****************************************************************************
 **
 ** Clauses sets manipulation primitives
 **
 ****************************************************************************
 ***************************************************************************/

#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <string.h>
#include <malloc.h>

#include "public-types.h"
#include "clset.h"
#include "utils.h"


static clset_iterator_list ItList = NULL;

/**
 * for hash function
 */

#define CHECKSTATE 8
#define hashsize(n) ((u_int32_t)1<<(n))
#define hashmask(n) (hashsize(n)-1)
#define mix(a,b,c) \
{ \
  a -= b; a -= c; a ^= (c>>13); \
  b -= c; b -= a; b ^= (a<<8); \
  c -= a; c -= b; c ^= (b>>13); \
  a -= b; a -= c; a ^= (c>>12);  \
  b -= c; b -= a; b ^= (a<<16); \
  c -= a; c -= b; c ^= (b>>5); \
  a -= b; a -= c; a ^= (c>>3);  \
  b -= c; b -= a; b ^= (a<<10); \
  c -= a; c -= b; c ^= (b>>15); \
}


/**
 * returns the chunk index of an element
 * WARNING ! Clauses are numbered from 1
 */
static int
chk_idx (int i)
{
  div_t d = div (i, CLELEM_CHK_SIZE);
  return ( d.rem == 0 ? d.quot - 1 : d.quot );
}

/**
 * acesses an element 
 */
int 
get (int i, unsigned *t)
{
  int r = i % CLELEM_CHK_SIZE;

  return ( t[chk_idx (i)] & 
	   ( 1U << ( r == 0 ? CLELEM_CHK_SIZE - 1 : r - 1 ) ) );
}

/**
 * sets an element
 */
static void 
set (int i, unsigned *t)
{
  int r = i % CLELEM_CHK_SIZE;
  
  t[chk_idx (i)] |= ( r == 0 ?
		      1U << (CLELEM_CHK_SIZE-1) :
		      1U << (r-1) );
}

/**
 * unsets an element
 */
static void 
unset (int i, unsigned *t)
{
  int r = i % CLELEM_CHK_SIZE;

  if ( get (i, t) )
    t[chk_idx (i)] ^= ( r == 0 ?
			1U << (CLELEM_CHK_SIZE-1) :
			1U << (r-1) );
}

/**
 * creates a new set of clauses. max is the max number of clauses,
 */
clset 
clset_new (unsigned max)
{
  div_t d;

  clset newset = NEW (struct st_clset);

  assert (newset != NULL);

  d = div (max, CLELEM_CHK_SIZE);

  if ( d.rem == 0 ) 
    {
      newset->chk = d.quot;
    }
  else
    {
      newset->chk = d.quot + 1;
    }
  newset->elems = NEW_ARRAY (unsigned, newset->chk);

  assert (newset->elems != NULL);

  memset (newset->elems, 0, newset->chk * sizeof (unsigned));

  newset->max = max;
  newset->card = 0;
  
  return newset;
}

/**
 * frees a clause set
 * la condition était grisée ... c moi qui l'ai changée
 */
void
clset_free (clset s)
{
  /* ##HACK */
   if (s != NULL) 
     { 
      free (s->elems);
      free (s);
     } 
}

/**
 * clones a set of clauses
 */
clset
clset_clone (clset s)
{
  clset new = NEW (struct st_clset);

  assert ( new != NULL );

  new->chk = s->chk;

  new->elems = NEW_ARRAY (unsigned, new->chk);

  assert (new->elems != NULL);

  memcpy (new->elems, s->elems, new->chk * sizeof (unsigned));

  new->max = s->max;;
  new->card = s->card;

  return new;
}


/**
 * return the number of elements in set s
 */
unsigned 
clset_card (clset s)
{
  return s->card;
}

/**
 * add a new clause to a clause set
 */
void 
clset_add (clset s, unsigned cl)
{
  assert (s != NULL);
  assert (cl <= s->max);

  if ( ! get (cl, s->elems) ) 
    {
      s->card++;
      set (cl, s->elems);
    }
}

/**
 * remove a clause from a clause set if present
 */
void 
clset_remove (clset s, unsigned cl)
{
  assert (s != NULL);
  assert (cl <= s->max);


  if ( get (cl, s->elems) ) 
    {
      s->card--;
      unset (cl, s->elems);
    }
}

/**
 * checks wether a clause is present in a clause set
 */
int 
clset_member (clset s, unsigned cl)
{
  
 /* rajout doukari*/
  //assert (s != NULL);
  if(s==NULL) return 0;
  /* fin rajout */
  if ( cl > s->max )
    return 0;
  
  return get (cl, s->elems);
}

/**
 * tests wether a clause set is empty
 */
int 
clset_empty (clset s)
{
  return s->card == 0;
}

/**
 * computes the union of two clsets, returns a new clset.
 * s1 and s2 should have the same max size
 */
clset 
clset_union (clset s1, clset s2)
{
  clset new;
  int i, j;

  assert (s1 != NULL);
  assert (s2 != NULL);
  assert (s1->max == s2->max);

  new = clset_new (s1->max);
  assert (new != NULL);

  /* we work chunk by chunk */
  for ( i = 0 ; i < s1->chk ; i++ )
    {
      new->elems[i] = s1->elems[i] | s2->elems[i];

      for ( j = 0 ; j < CLELEM_CHK_SIZE ; j++ )
	{
	  if ( 1U & (new->elems[i] >> j) )
	    new->card++;
	}
    }
  return new;
}

/**
 * computes the intersection of two clsets, returns a new clset.
 * s1 and s2 should have the same max size
 */
clset 
clset_inter (clset s1, clset s2)
{
  clset new;
  int i, j;

  assert (s1 != NULL);
  assert (s2 != NULL);
  assert (s1->max == s2->max);

  new = clset_new (s1->max);
  assert (new != NULL);

  /* we work chunk by chunk */
  for ( i = 0 ; i < s1->chk ; i++ )
    {
      new->elems[i] = s1->elems[i] & s2->elems[i];

      for ( j = 0 ; j < CLELEM_CHK_SIZE ; j++ )
	{
	  if ( 1U & (new->elems[i] >> j) )
	    new->card++;
	}
    }
  return new;
}

/**
 * computes the intersection of two clsets, operating on the first set.
 * s1 and s2 should have the same max size
 */
clset 
clset_inter_mute (clset s1, clset s2)
{
  int i, j;
  assert (s1 != NULL);
  assert (s2 != NULL);
  assert (s1->max == s2->max);

  /* we work chunk by chunk */
  s1->card = 0;
  for ( i = 0 ; i < s1->chk ; i++ )
    {
      s1->elems[i] = s1->elems[i] & s2->elems[i];

      for ( j = 0 ; j < CLELEM_CHK_SIZE ; j++ )
	{
	  if ( 1U & (s1->elems[i] >> j) )
	    s1->card++;
	}
    }
	
  return s1;
}

/**
 * computes the intersection of two clsets, returns a new clset.
 * s1 and s2 should have the same max size
 * STACK VERSION. NEVER USE DIRECTLY. CALL MACRO STACK_CLSET_INTER
 * INSTEAD.
 */
#ifdef STACK_ALLOC
void 
stack_clset_inter (clset new, clset s1, clset s2)
{
  int i, j;

  assert (s1 != NULL);
  assert (s2 != NULL);
  assert (s1->max == s2->max);

  /* we work chunk by chunk */
  for ( i = 0 ; i < s1->chk ; i++ )
    {
      new->elems[i] = s1->elems[i] & s2->elems[i];

      for ( j = 0 ; j < CLELEM_CHK_SIZE ; j++ )
	{
	  if ( 1U & (new->elems[i] >> j) )
	    new->card++;
	}
    }
}
#endif

/**
 * creates a clset iterator (after use, free it ! )
 */
clset_iterator 
clset_iterator_new (clset p)
{
  clset_iterator new = NEW (struct st_clset_iterator);
  clset_iterator_list newi = NEW (struct st_clset_iterator_list);
  int i;

  assert ( p != NULL );
  assert ( new != NULL );
  assert ( newi != NULL );
  
  new->index = 0;
  new->set = p;

  //printf("avant ajout ItList=%d, ItList->next=%d\n",ItList,ItList->next);
  newi->it = new;
  newi->next = ItList;
  //printf("newi=%d\n",newi);
  ItList = newi;
  //printf("apres ajout ItList=%d\n",ItList);
  //printf("newi->next=%d, ItList->next=%d\n",ItList,ItList->next);
  //scanf("%d",&i);

  return new;
}


/**
 * creates a clset iterator (stack allocation)
 * NEVER USE THIS FUNCTION DIRECTLY 
 */
#ifdef STACK_ALLOC
void
stack_clset_iterator_new (clset_iterator new, 
			  clset_iterator_list newi,
			  clset p)
{
  assert ( p != NULL );
  
  int i;
  //printf("Debut stack_clset_iterator_new: newi=%d, ItList=%d, newi->next=%d\n",newi,ItList,newi->next);
  new->index = 0;
  new->set = p;

  newi->it = new;
  newi->next = ItList;
  ItList = newi;
  //printf("Fin stack_clset_iterator_new: newi=%d, ItList=%d, newi->next=%d\n",newi,ItList,newi->next);
  //scanf("%d",&i);
}
#endif

void
clset_iterator_free (clset_iterator it)
{
  int i;
  clset_iterator_list p = ItList, prev = NULL;
  
   
  while ( p && p->it != it )
    {
      prev = p;
      p = p->next;
    }

  assert ( p != NULL ); /* we must find the iterator */

  if ( prev )
    {
      prev->next = p->next;
    }
  else
    {
      ItList = ItList->next;
    }
  free (p);
  free (it);
    
  //scanf("%d",&i);
}

/**
 * update the iterators acting on set s1 to operate on set s2
 */
void
clset_update_iterators (clset s1, clset s2)
{
  clset_iterator_list p = ItList;

  assert ( s2->max == s1->max );
	//printf("clset_update_iterators (avant if): p=%d et s1=%d",p,s1);
	//clset_display(s1);
	//printf("\n");
  while ( p )
    {
      //printf("clset_update_iterators (dans if): p=%d et p->it->set=%d",p,p->it->set);
	  //clset_display(p->it->set);
	 // printf("\n");
	  if ( p->it->set == s1 )
		p->it->set = s2;
      p = p->next;
    }
}


/**
 ** returns next element in clset. returns 0 if no more elems
 **/
unsigned 
clset_next_element (clset_iterator i)
{
  int cl = 0;

  assert ( i != NULL );

  ++(i->index);

  while ( ( i->index <= i->set->max ) && 
	  (! get (i->index, i->set->elems)) )
    {
      (i->index)++;
    }

  if ( i->index > i->set->max )
    cl = 0;
  else
    cl = i->index;

  assert ( cl <= i->set->max );

  return cl;
}

/**
 * displays clset contents
 */
void 
clset_display (clset s)
{
  int i;

  if ( s == NULL )
    {
      printf ("<NULL>\n");
      return;
    }
  printf ("{ ");
  for ( i = 1 ; i <= s->max ; i++ )
    if ( get (i, s->elems) )
      printf ("%d ", i);
  printf ("}\n");
}

/**
 * computes the difference between two clsets.
 * the resulting clset containts elements of e1 which are not
 * in e2
 */
clset
clset_diff (clset e1, clset e2)
{
  clset new = clset_new (e1->max);
  int i, j;

  assert ( e1->max == e2->max );
  assert ( new != NULL );

  for ( i = 0 ; i < new->chk ; i++ )
    {
      new->elems[i] = ( e1->elems[i] ^ e2->elems[i] ) & e1->elems[i] ;

      for ( j = 0 ; j < CLELEM_CHK_SIZE ; j++ )
	if ( 1U & (new->elems[i] >> j) )
	  new->card++;
    }

  return new;
}

/**
 * computes the difference between two clsets.
 * the resulting clset containts elements of e1 which are not
 * in e2
 * NEVER USE THIS FUNCTION ! USE STACK_CLSET_DIFF INSTEAD
 */
#ifdef STACK_ALLOC
void
stack_clset_diff (clset new, clset e1, clset e2)
{
  int i, j;

  assert ( e1->max == e2->max );

  for ( i = 0 ; i < new->chk ; i++ )
    {
      new->elems[i] = ( e1->elems[i] ^ e2->elems[i] ) & e1->elems[i] ;

      for ( j = 0 ; j < CLELEM_CHK_SIZE ; j++ )
	if ( 1U & (new->elems[i] >> j) )
	  new->card++;
    }
}
#endif

/**
 * tests wether s1 is included into s2 (or equal)
 * Returns 1 if true, 0 otherwise.
 */
int 
clset_included (clset s1, clset s2)
{
  clset_iterator it = clset_iterator_new(s1);
  unsigned cl;

  if ( s1->card > s2->card )
    return 0; /* no need to go further on */

  while ( (cl = clset_next_element(it)) != 0 )
    {
      if ( ! clset_member(s2, cl) )
	{
	  free (it);
	  return 0;
	}
    }
  clset_iterator_free (it);
  return 1;
}
  

/**
 * compares two clsets. If they are equal, it returns 0, else it
 * returns a non zero value
 */
int 
clset_compare (clset s1, clset s2)
{
  assert ( s1 != NULL && s2 != NULL );

  if ( s1->card != s2->card || s1->max != s2->max || s1->chk != s2->chk )
    return 1;

  return memcmp (s1->elems, s2->elems, s1->chk * sizeof (unsigned));
}

/**
 * computes the hash value of a clset
 * we use gnu types to ensure type lengths
 */
u_int32_t 
clset_hash (clset s)
{
   register u_int8_t  * k = (u_int8_t *) s->elems;
   register u_int32_t   length = s->chk * sizeof (unsigned);
   register u_int32_t   level = 0x9ea08df9; /* arbitrary value */
   register u_int32_t   a,b,c,len;


   /* Set up the internal state */
   len = length;
   a = b = 0x9e3779b9;  /* the golden ratio; an arbitrary value */
   c = level;           /* the previous hash value */

   /*---------------------------------------- handle most of the key */
   while (len >= 12)
   {
      a += (k[0] 
	    +((u_int32_t)k[1]<<8) 
	    +((u_int32_t)k[2]<<16) 
	    +((u_int32_t)k[3]<<24));
      b += (k[4] 
	    +((u_int32_t)k[5]<<8) 
	    +((u_int32_t)k[6]<<16) 
	    +((u_int32_t)k[7]<<24));
      c += (k[8] 
	    +((u_int32_t)k[9]<<8) 
	    +((u_int32_t)k[10]<<16)
	    +((u_int32_t)k[11]<<24));
      mix(a,b,c);
      k += 12; len -= 12;
   }

   /*------------------------------------- handle the last 11 bytes */
   c += length;
   switch(len)              /* all the case statements fall through */
   {
   case 11: c+=((u_int32_t)k[10]<<24);
   case 10: c+=((u_int32_t)k[9]<<16);
   case 9 : c+=((u_int32_t)k[8]<<8);
      /* the first byte of c is reserved for the length */
   case 8 : b+=((u_int32_t)k[7]<<24);
   case 7 : b+=((u_int32_t)k[6]<<16);
   case 6 : b+=((u_int32_t)k[5]<<8);
   case 5 : b+=k[4];
   case 4 : a+=((u_int32_t)k[3]<<24);
   case 3 : a+=((u_int32_t)k[2]<<16);
   case 2 : a+=((u_int32_t)k[1]<<8);
   case 1 : a+=k[0];
     /* case 0: nothing left to add */
   }
   mix(a,b,c);
   /*-------------------------------------------- report the result */
   return c;
}


/****************************************************************************
 ****************************************************************************
 **
 ** collections of sets of clauses
 **
 ****************************************************************************
 ****************************************************************************
 */

clsetset 
clsetset_new()
{
  clsetset new = NEW (struct st_clsetset);

  assert ( new != NULL ) ;

  new->card = 0;
  new->head = NULL;

  return new;
}

void
clsetset_add (clsetset ss, clset s)
{
  clsetset_elem e = NEW (struct st_clsetset_elem);
  
  assert ( e != NULL ) ;

  e->next = ss->head;
  e->elem = s;
  ss->head = e;

  ss->card++;
}

clsetset_iterator
clsetset_iterator_new (clsetset s)
{
  clsetset_iterator new = NEW (struct st_clsetset_iterator);

  assert ( new != NULL );
  assert ( s != NULL );

  new->s = s;
  new->current = s->head;
  
  return new;
}

clset
clsetset_next_element (clsetset_iterator i)
{
  clset c = NULL;

  if ( i->current != NULL )
    {
      c = i->current->elem;
      i->current = i->current->next;
    }
  return c;
}

clsetset 
clsetset_minimal(clsetset s)
{
	//assert (s != NULL);
if(s==NULL)
{
	printf("\nit's OK!\n");
	return NULL;
}
else
	{
	int nbr_egal=0,bool=0;
	clset setcl1,setcl2;
	clsetset_elem precedent;
	
	clsetset_iterator it2, it1= clsetset_iterator_new(s);
	precedent=s->head;
	
	while((setcl1=clsetset_next_element(it1))!=NULL)
		{
			
			//clset_display(setcl1);
			//printf("%%%%%%%%%%%%%%\n");
			it2= clsetset_iterator_new(s);
			while((setcl2=clsetset_next_element(it2))!=NULL)
			{
				//clset_display(setcl2);
				if(clset_included(setcl2,setcl1)) /* l'inclusion peut etre stricte comme elle peut ne pas l'etre*/
				{
					if(!clset_compare(setcl2,setcl1) && !nbr_egal) /* nbr_egal consiste a laisser juste 1 seule copie d'un elet*/
						nbr_egal++;
					else
					{
						clsetset_remove(it1,setcl1,precedent);
						bool=1; /* bool indique la désallocation de precedent apres */
						break; /* dès qu'on supprime setcl1 on passe au suivant car on enleve de it1 et non pas de it2 */
					}
				}
			}
			
			if (bool) 
				clsetset_free(precedent);
			precedent= it1->current;
			bool=nbr_egal=0;
		}
		printf("\nit's OK!\n");
		return it1->s;
	}
	
}


void
clsetset_remove(clsetset_iterator it,clset setcl,clsetset_elem prec)
{
	clsetset_elem parc = it->s->head;
	assert (it != NULL);
	assert (setcl != NULL);
	
	
		if(it->s->head== prec)
		{
			it->s->head=it->current;
			//printf("je vais supprimer la tete:\n");
			//clset_display(prec->elem);
		}
		else
		{
			while(parc!=NULL)
				if(parc->next!=prec)
				{
					parc=parc->next;
					//clset_display(parc->next->elem);
				}
				else
				{
					break;
				}
			parc->next=it->current;
		}
				
		it->s->card--;
				
	
}

void
clsetset_free(clsetset_elem elem)
{
	
	clset_free(elem->elem);
	free(elem);
	
}

void
clsetset_dispaly(clsetset collection)
{
if(collection==NULL)
	printf("\nThere is any HS to display!\n");
else{
	clset setcl;
	clsetset_iterator it= clsetset_iterator_new(collection);
	while((setcl=clsetset_next_element(it))!=NULL)
		{
			clset_display(setcl);
		}
	}
}







