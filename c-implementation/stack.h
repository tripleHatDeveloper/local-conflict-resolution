/****************************************************************************
 ****************************************************************************
 **
 ** Assignment stack management
 **
 ****************************************************************************
 ****************************************************************************
 */
#ifndef STACK_H
#define STACK_H

/* ---- Stack type ------------------------------------------------------- */


struct struct_stack
{
  int *assignment_stack;
  int *top_stack;
  int is_stack_consistant;
};

#define ASSIGNMENT_STACK(stack) ((stack)->assignment_stack)
#define TOP_STACK(stack) ((stack)->top_stack)
#define IS_STACK_CONSISTANT(stack) ((stack)->is_stack_consistant)


#define IS_STACK_EMPTY(stack)   (TOP_STACK(stack) == ASSIGNMENT_STACK(stack))

/****************************************************************************/

type_stack stack_new(void);
void stack_free(type_stack stack);
type_stack create_stack(type_instance inst);
void reinit_stack(type_stack stack);


#endif
