#include <assert.h>

#include "public-types.h"
#include "utils.h"
#include "clset.h"

#include "nodetable.h"

#define HASH_NODE(n) ( (n)->label == NULL ? 0UL : (clset_hash((n)->label) & NODETABLE_HASH_MASK) )
#define HASH_SET(s) (clset_hash(s) & NODETABLE_HASH_MASK)

nodetable 
nodetable_new ()
{
  nodetable t = NEW (struct st_nodetable);
  int i;
  size_t sz = NODETABLE_HASH_SIZE;

  assert (t != NULL);

  t->nbnodes = 0;
  t->table = NEW_ARRAY (nodetable_entry, sz);
  assert (t->table != NULL);

  t->csort_head = NULL;
  t->csort_tail = NULL;

  for ( i = 0 ; i < NODETABLE_HASH_SIZE ; i++ )
    t->table[i] = NULL;

  return t;
}

/**
 * add a node to the node hash table.
 * The key of a node is its clause set (the label)
 */
void 
nodetable_add (nodetable t, node_diag n)
{
  u_int32_t c; /* hash code */
  nodetable_entry e = NEW (struct st_nodetable_entry);
  nodetable_entry p;

  assert (t != NULL);
  assert (n != NULL);
  assert (e != NULL);

  c = HASH_NODE(n);
  
  e->node = n;
  e->next = t->table[c];
  t->table[c] = e;

  if ( n->label != NULL )
    {
      if ( t->csort_head == NULL )
	{
	  t->csort_head = t->csort_tail = e;
	  e->csort_next = e->csort_prev = NULL;
	}
      else
	{
	  /* if head is non null, so is tail */
	  assert ( t->csort_tail != NULL );
	  
	  for ( p = t->csort_head ; 
		p && clset_card(p->node->label) < clset_card(n->label) ;
		p = p->csort_next )
	    ;
	  
	  if ( p == NULL )
	    {
	      /* tail */
	      e->csort_next = NULL ;
	      e->csort_prev = t->csort_tail;
	      t->csort_tail->csort_next = e;
	      t->csort_tail = e;
	    }
	  else if ( p->csort_prev == NULL ) 
	    {
	      e->csort_next = p;
	      e->csort_prev = NULL;
	      p->csort_prev = e;
	      t->csort_head = e;
	    }
	  else
	    {
	      e->csort_next = p->csort_prev->csort_next;
	      e->csort_prev = p->csort_prev;
	      p->csort_prev->csort_next = e;
	      p->csort_prev = e;
	    }
	}    
    }

  t->nbnodes++;

  return;
}

/**
 * remove a node from the node hash table
 */
void
nodetable_remove (nodetable t, node_diag n)
{
  u_int32_t c; /* hash code */
  nodetable_entry e;
  nodetable_entry prev = NULL;

  assert (t != NULL);
  assert (n != NULL);
  
  c = HASH_NODE(n);
  for ( e = t->table[c] ; e && e->node != n ; e = e->next)
    prev = e;

  if ( e )
    {
      if ( prev )
	prev->next = e->next;
      else
	t->table[c] = e->next;

      if ( n->label != NULL )
	{
	  if ( e->csort_prev == NULL )
	    t->csort_head = e->csort_next;
	  else
	    e->csort_prev->csort_next = e->csort_next;
	  
	  if ( e->csort_next == NULL )
	    t->csort_tail = e->csort_prev;
	  else
	    e->csort_next->csort_prev = e->csort_prev;
	}

      free (e);
      t->nbnodes--;
    }
  return;
}
  

/**
 * look up a node in the hash table.
 * if several nodes exist, returns the first found
 */
node_diag 
nodetable_lookup1 (nodetable t, node_diag n)
{
  u_int32_t c;

  assert (t != NULL);
  assert (n != NULL);

  c = HASH_NODE(n);
  if ( t->table[c] == NULL )
    return NULL;

  return t->table[c]->node;
}

/**
 * look up a node in the hash table
 * returns an iterator
 * over nodes which have the same key
 */
nodetable_iterator
nodetable_lookup2 (nodetable t, node_diag n)
{
  u_int32_t c;
  nodetable_iterator i;
  
  
  assert (t != NULL);
  assert (n != NULL);

  c = HASH_NODE(n);
  if ( t->table[c] == NULL )
    return NULL;

  i = NEW (struct st_nodetable_iterator);
  assert (i != NULL);

  i->type = SAME_KEY;
  i->current = t->table[c];

  return i;
}

/**
 * look up a node wich have the given label
 *
 * t : the nodetable
 * l : the label
 * return : a pointer to the found node or NULL if no matching node
 */
node_diag
nodetable_lookup3 (nodetable t, clset l)
{
  u_int32_t c;
  nodetable_entry e;
  int found = 0;

  assert ( t != NULL );
  assert ( l != NULL );

  c = HASH_SET(l);
  if ( t->table[c] == NULL )
    return NULL;

  for ( e = t->table[c] ; e && ! found ; e = e->next )
    if ( e->node->label != NULL )
      /* not an END node or a CLOSED node */
      found = ( clset_compare (e->node->label, l) == 0 );

  if ( e )
    return e->node;
  return NULL;
}

/**
 * creates a new iterator over the cardinality-sorted list
 * of nodes
 */
nodetable_iterator
nodetable_csort_iterator_new (nodetable nt)
{
  nodetable_iterator i;

  assert (nt != NULL);

  i = NEW (struct st_nodetable_iterator);
  assert (i != NULL);

  i->t = nt;
  i->type = SORT;
  i->current = NULL;
  i->index = 0;
  i->at_begin = 1;

  return i;
}

/**
 * creates a new iterator over the cardinality-sorted list
 * of nodes
 */
#ifdef STACK_ALLOC
void
stack_nodetable_csort_iterator_new (nodetable_iterator i, nodetable nt)
{
  assert (nt != NULL);

  i->t = nt;
  i->type = SORT;
  i->current = NULL;
  i->index = 0;
  i->at_begin = 1;
}
#endif

/**
 * creates a new iterator over the node table	
 */
nodetable_iterator
nodetable_iterator_new (nodetable nt)
{
  nodetable_iterator i;

  assert (nt != NULL);

  i = NEW (struct st_nodetable_iterator);
  assert (i != NULL);

  i->t = nt;
  i->type = ALL;
  i->current = NULL;
  i->index = 0;
  i->at_begin = 1;

  return i;
}

/**
 * creates a new iterator over the node table.
 * NEVER USE DIRECTLY THIS FUNCTION !
 */
#ifdef STACK_ALLOC
void
stack_nodetable_iterator_new (nodetable_iterator i, nodetable nt)
{
  assert (nt != NULL);

  i->t = nt;
  i->type = ALL;
  i->current = NULL;
  i->index = 0;
  i->at_begin = 1;
}
#endif

/**
 * gets the next element in table using the cardinality sort list
 */
node_diag
nodetable_csort_next_element (nodetable_iterator i)
{
  assert ( i->type == SORT );

  if ( i->at_begin )
    {
      i->at_begin = 0;
      i->current = i->t->csort_head;
    }
  else
    if ( i->current != NULL )
      i->current = i->current->csort_next;

  return ( i->current != NULL ? i->current->node : NULL );
}

/**
 * gets the next element in table
 */
node_diag
nodetable_next_element (nodetable_iterator i)
{
  register unsigned j;

  assert (i->type == ALL);

  if ( i->index >= NODETABLE_HASH_SIZE )
    /* already at end */
    return NULL;
  if ( i->at_begin )
    {
      i->at_begin = 0;
      if ( i->t->nbnodes == 0 )
	return NULL;
      /* find first non NULL entry */
      for ( j = 0 ; j < NODETABLE_HASH_SIZE && i->t->table[j] == NULL ; j++ )
	;
      i->index = j;
      if ( j >= NODETABLE_HASH_SIZE )
	return NULL; /* empty table */
      i->current = i->t->table[j];
    }
  else if ( i->current == NULL || i->current->next == NULL )
    {
      /* next entry in table */
      j = i->index + 1;
      while ( ( j < NODETABLE_HASH_SIZE ) && ( i->t->table[j] == NULL ) )
	j++;
      i->index = j;
      if ( j == NODETABLE_HASH_SIZE )
	return NULL;
      i->current = i->t->table[j];
    }
  else
    /* next bucket */
    i->current = i->current->next;

  return i->current->node;
}

      
      




