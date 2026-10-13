
#ifndef NODE_TABLE_H
#define NODE_TABLE_H

#include <sys/types.h>
#include "arbre-diag.h"

struct st_nodetable_entry;
typedef struct st_nodetable_entry * nodetable_entry;

struct st_nodetable_entry
{
  node_diag node;
  nodetable_entry next;

  nodetable_entry csort_next; /* next (sorted by label cardinality) */
  nodetable_entry csort_prev; /* next (sorted by label cardinality) */
};

#define CHECKSTATE 8
#define hashsize(n) ((u_int32_t)1<<(n))
#define hashmask(n) (hashsize(n)-1)

#define NODETABLE_HASH_BITS 13
#define NODETABLE_HASH_SIZE hashsize(NODETABLE_HASH_BITS)
#define NODETABLE_HASH_MASK hashmask(NODETABLE_HASH_BITS)

struct st_nodetable {
  nodetable_entry  *table;
  unsigned nbnodes;
  nodetable_entry csort_head; /* chain of sorted nodes */
                              /* (cardinality sort)    */
  nodetable_entry csort_tail;
};


enum ittype { SAME_KEY, ALL, SORT };

struct st_nodetable_iterator
{
  nodetable t;              /* nodetable to iterate           */
  enum ittype type;         /* iterator type                  */
  int at_begin;
  nodetable_entry current;  /* current element                */
  unsigned index;           /* index of current browsed entry */
};


/******************************** prototypes **************************/

nodetable nodetable_new();
void nodetable_add (nodetable t, node_diag n);
void nodetable_remove (nodetable t, node_diag n);
node_diag nodetable_lookup1 (nodetable t, node_diag n);
nodetable_iterator nodetable_lookup2 (nodetable t, node_diag n);
node_diag nodetable_lookup3 (nodetable t, clset l);
nodetable_iterator nodetable_csort_iterator_new (nodetable nt);
node_diag nodetable_csort_next_element (nodetable_iterator i);
nodetable_iterator nodetable_iterator_new (nodetable nt);
node_diag nodetable_next_element (nodetable_iterator i);

/* ---- Stack allocated iterators (for temporary usage) -------------------- */

/**
 * allocate an iterator
 */
#ifdef STACK_ALLOC
#define NODETABLE_ITERATOR_NEW(it, t) { \
  it = (nodetable_iterator) alloca (sizeof (struct st_nodetable_iterator)); \
  stack_nodetable_iterator_new (it, t); \
}
#define NODETABLE_CSORT_ITERATOR_NEW(it, t) { \
  it = (nodetable_iterator) alloca (sizeof (struct st_nodetable_iterator)); \
  stack_nodetable_csort_iterator_new (it, t); \
}
#define NODETABLE_ITERATOR_FREE(it)
#else 
#define NODETABLE_ITERATOR_NEW(it, t) it = nodetable_iterator_new(t)
#define NODETABLE_CSORT_ITERATOR_NEW(it, t) it = nodetable_csort_iterator_new(t)
#define NODETABLE_ITERATOR_FREE(it) nodetable_iterator_free(it)
#endif
/* Do not call this function directly (use above macro) */
void stack_nodetable_iterator_new (nodetable_iterator i, 
				   nodetable nt);
void stack_nodetable_csort_iterator_new (nodetable_iterator i, 
					 nodetable nt);



#endif




