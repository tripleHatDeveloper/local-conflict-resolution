
/****************************************************************************
 ****************************************************************************
 **
 ** Assignment stack management
 **
 ****************************************************************************
 ****************************************************************************
 */

#include <assert.h>

#include "public-types.h"
#include "dp.h"
#include "utils.h"
#include "stack.h"


/**
 * creates a new stack (doesn't initialize anything)
 */
type_stack 
stack_new(void)
{
  type_stack new = NEW(struct struct_stack);
  assert (new != NULL);

  
  return new;
}

/**
 * frees a stack
 */
void
stack_free (type_stack stack)
{
  free (ASSIGNMENT_STACK(stack));
  free (stack);
}

/**
 * allocates and initializes a new stack for unit propagation
 */
type_stack
create_stack(type_instance inst)
{
  type_stack stack = stack_new();

  ASSIGNMENT_STACK(stack) = ( (int*) 
		       calloc (sizeof (struct struct_variable),
			       VARIABLE_NUMBER(inst)) );
  assert (ASSIGNMENT_STACK(stack) != NULL);
  TOP_STACK(stack) = ASSIGNMENT_STACK(stack);
  IS_STACK_CONSISTANT(stack) = 1;

  return stack;
}

/**
 * reinitializes a stack
 */
void
reinit_stack(type_stack stack)
{
  assert (ASSIGNMENT_STACK(stack) != NULL);
  TOP_STACK(stack) = ASSIGNMENT_STACK(stack);
  IS_STACK_CONSISTANT(stack) = 1;
}



