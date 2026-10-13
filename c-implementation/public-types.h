/**********************************************************************
***********************************************************************
**
** To be included FIRST
**
***********************************************************************
***********************************************************************
*/

#ifndef PUBLIC_TYPES_H
#define PUBLIC_TYPES_H

/**
 * hashtable of nodes
 */
struct st_nodetable;
typedef struct st_nodetable * nodetable;

/**
 * an iterator over node table
 */
struct st_nodetable_iterator;
typedef struct st_nodetable_iterator * nodetable_iterator;

/**
 * an instance (a knowledge base)
 */
struct struct_instance;
typedef struct struct_instance *type_instance;

/**
 * dp stack
 */
struct struct_stack;
typedef struct struct_stack *type_stack;

#endif
