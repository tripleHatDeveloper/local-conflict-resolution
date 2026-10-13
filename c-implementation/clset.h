/****************************************************************************
 ****************************************************************************
 **
 ** declarations for sets of clauses
 ** the underlying representation is a bitset to speed up comparisions
 **
 ** The macros STACK_xxx uses stack allocation (with alloca) for clsets.
 ** Use this allocation scheme for temporary sets in order to reduce
 ** memory fragmentation. But beware, these function are not type-safe !
 **
 **
 ****************************************************************************
 ***************************************************************************/


#ifndef CLSET_H
#define CLSET_H

#include <limits.h>

#ifdef STACK_ALLOC
#include <string.h>
#endif

struct st_clset;
typedef struct st_clset * clset;

/* size of a chunk in bits */
#define CLELEM_CHK_SIZE ( sizeof (unsigned) * CHAR_BIT )

/**
 * a set of clauses
 *
 * elements are stored in bit sets to speed up execution.
 * WARNING ! element numbering begins at 1 while bit numbering
 * begins at 0 ! 
 * bits:        |7...0|15...8|23...16|...
 * elements:    |8...1|16...9|24...17|...
 */
struct st_clset 
{
  unsigned *elems;  /* elements vector */
  unsigned card;    /* set cardinality */
  unsigned max;     /* maximum allowed cardinality */
  unsigned chk;
};

typedef struct st_clset_iterator *clset_iterator;

/**
 * an iterator over clauses sets 
 */
struct st_clset_iterator
{
  clset set;
  unsigned index;
};

/**
 * Collections of sets of clauses
 */
struct st_clsetset;
typedef struct st_clsetset *clsetset;
struct st_clsetset_elem;
typedef struct st_clsetset_elem *clsetset_elem;

struct st_clsetset_elem
{
  clset elem;
  clsetset_elem next;
};

struct st_clsetset
{
  unsigned card;
  clsetset_elem head;
};

/**
 * iterators over collections of sets of clauses
 */

struct st_clsetset_iterator;
typedef struct st_clsetset_iterator *clsetset_iterator;

struct st_clsetset_iterator
{
  clsetset s;
  clsetset_elem current;
};

/**
 * internal structure to track iterators
 */
typedef struct st_clset_iterator_list *clset_iterator_list;
struct st_clset_iterator_list
{
  clset_iterator it;
  clset_iterator_list next;
};


/****************************** Prototypes ********************************/

clset clset_new (unsigned max);
void clset_free (clset s);
clset clset_clone (clset s);
unsigned clset_card (clset s);
void clset_add (clset s, unsigned cl);
void clset_remove (clset s, unsigned elem);
int clset_member (clset s, unsigned elem);
clset clset_union (clset s1, clset s2);
clset clset_inter (clset s1, clset s2);
clset clset_inter_mute (clset s1, clset s2);
void clset_display (clset s);
clset_iterator clset_iterator_new (clset s);
unsigned clset_next_element (clset_iterator i);
void clset_iterator_free (clset_iterator it);
void clset_update_iterators (clset s1, clset s2);
clset clset_diff (clset s1, clset s2);
int clset_empty (clset s);
int clset_compare (clset s1, clset s2);
int clset_included (clset s1, clset s2);
u_int32_t clset_hash (clset s);
int get (int i, unsigned *t);
clsetset_iterator
clsetset_iterator_new (clsetset s);
clset
clsetset_next_element (clsetset_iterator i);
void
clsetset_add (clsetset ss, clset s);
clsetset 
clsetset_new();
clsetset 
clsetset_minimal(clsetset s);
void
clsetset_remove(clsetset_iterator it,clset setcl,clsetset_elem prec);
void
clsetset_free(clsetset_elem elem);
void
clsetset_dispaly(clsetset collection);
void 
Itlist_free(void);
void
Itlist_affiche(void);


/* ---- stack aloocated sets -------------------------------------------- */


/**
 * allocate a set on stack at current stack frame
 */
#ifdef STACK_ALLOC
#define CLSET_NEW(s, maxi) { \
  unsigned max=maxi;\
  div_t d = div ((max), CLELEM_CHK_SIZE); \
  (s) = (clset)alloca (sizeof (struct st_clset)); \
  (s)->chk = ( d.rem == 0 ? d.quot : d.quot + 1 ); \
  (s)->elems = (unsigned *) alloca ((s)->chk * sizeof (unsigned)); \
  memset ((s)->elems, 0, (s)->chk * sizeof (unsigned)); \
  (s)->max = maxi; \
  (s)->card = 0; \
}
#else
#define CLSET_NEW(s, max) (s) = clset_new (max)
#endif

#ifdef STACK_ALLOC
#define CLSET_FREE(s)
#else
#define CLSET_FREE(s) clset_free(s)
#endif
 
/**
 * compute intersection of s1 and s2, put result in s
 */
#ifdef STACK_ALLOC
#define CLSET_INTER(s, s1, s2) { \
  CLSET_NEW(s, (s1)->max); \
  stack_clset_inter(s, s1, s2); \
}
/* do not use this function ! use CLSET_INTER macro */
void stack_clset_inter (clset new, clset s1, clset s2);
#else
#define CLSET_INTER(s, s1, s2) s = clset_inter (s1, s2)
#endif

/**
 * compute difference of s1 and s2, put result in s
 */
#ifdef STACK_ALLOC
#define CLSET_DIFF(s, s1, s2) { \
  CLSET_NEW(s, (s1)->max); \
  stack_clset_diff(s, s1, s2); \
}
/* do not use this function ! use CLSET_DIFF macro */
void stack_clset_diff (clset new, clset e1, clset e2);
#else 
#define CLSET_DIFF(s, s1, s2) s = clset_diff (s1, s2)
#endif

/**
 * allocate an iterator
 */
#ifdef STACK_ALLOC
#define CLSET_ITERATOR_NEW(it, s) { \
  clset_iterator_list nl = (clset_iterator_list) alloca (sizeof (struct st_clset_iterator_list)); \
  it = (clset_iterator) alloca (sizeof (struct st_clset_iterator)); \
  stack_clset_iterator_new (it, nl, s); \
}
#define CLSET_ITERATOR_FREE(it)
/* Do not call this function directly (use above macro) */
void stack_clset_iterator_new (clset_iterator new, 
			       clset_iterator_list newi,
			       clset p);
#else 
#define CLSET_ITERATOR_NEW(it, s) it = clset_iterator_new(s)
#define CLSET_ITERATOR_FREE(it) clset_iterator_free(it)
#endif

#endif












