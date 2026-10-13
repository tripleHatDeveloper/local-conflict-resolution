/****************************************************************************
 ****************************************************************************
 ** 
 ** DP functions used to determine inconsistant subsets
 **
 ****************************************************************************
 ***************************************************************************/

#include <stdlib.h>

#include "public-types.h"
#include "utils.h"
#include "stack.h"
#include "dp.h"
#include "nodetable.h"
#include "opts.h"
//#include "clset.h"

	
static int 
cmpint (int *a, int *b)
{
  return *a - *b;
}

/**
 * tests wether clause cl is in set E
 */
int 
in_E (unsigned cl, type_instance kb)
{
  return ( bsearch (&cl, E_SET(kb), 
		    E_CARD(kb), sizeof (int),
		    (comparison_fn_t) cmpint) != NULL );
}

/************************************************************************
 ************************************************************************
 ** 
 ** Clause base reading functions
 **
 ************************************************************************
 ***********************************************************************/

/**
 * reads the file describing clauses membership
 * to I and E sets
 */

static void
read_member (FILE *f, type_instance inst)
{
  int i;
  char c;
  char label[255];
  int e_card, id_card, ic_card;
  int e_idx, id_idx, ic_idx;

  c = fgetc (f);
  while ( (c != 'm' ) && ( ! feof (f) ) ) 
    c = fgetc (f);
  i = 0;
  while ( ( c != ' ' ) && ( ! feof (f) ) ) 
    {
      label[i++] = c; 
      c = fgetc (f);
    }
  label[i] = '\0';
  if ( feof (f) || ( strcmp (label, "membership") != 0 ) ) 
    {
      fprintf (stderr, "membership header expected.\n");
      fflush (stderr);
      exit (0);
    }
e_card = 0;
  fscanf (f, "%d", &E_CARD(inst));
/*    if ( E_CARD(inst) == 0 )  */
/*      { */
/*        fprintf (stderr, "The cardinal of E was expected.\n");  */
/*        fflush (stderr); */
/*        exit (0); */
/*      } */
  id_card = 0;
  fscanf (f, "%d", &ID_CARD(inst));
/*   if ( ID_CARD(inst) == 0 )  */
/*     { */
/*       fprintf (stderr, "The cardinal of Id was expected.\n");  */
/*       fflush (stderr); */
/*       exit (0); */
/*     } */
  ic_card = 0;
  fscanf (f, "%d", &IC_CARD(inst));
/*   if ( IC_CARD(inst) == 0 )  */
/*     { */
/*       fprintf (stderr, "The cardinal of Ic was expected.\n");  */
/*       fflush (stderr); */
/*       exit (0); */
/*     } */
  E_SET(inst) = NEW_ARRAY (int, E_CARD(inst));
  IC_SET(inst) = NEW_ARRAY (int, IC_CARD(inst));
  ID_SET(inst) = NEW_ARRAY (int, ID_CARD(inst));

  e_idx = id_idx = ic_idx = 0;
  while ( e_idx + id_idx + ic_idx <
	  E_CARD(inst) + IC_CARD(inst) + ID_CARD(inst) )
    {
      int numcl;
#ifdef _GNU_SOURCE
      char *set;
#else
      char set[255];
#endif
	
#ifdef _GNU_SOURCE      
      // warning ! This is a GNU extension allowing dynamic allocation
      // of set by scanf
      fscanf (f, "%d%ms", &numcl, &set);
#else
      fscanf (f, "%d%s", &numcl, set);
#endif

      if ( ! strcmp (set, "E") )
	{
	  E_SET(inst)[e_idx++] = numcl;
	}
      else if ( ! strcmp (set, "Id") ) 
	{
	  ID_SET(inst)[id_idx++] = numcl;
	}
      else if ( ! strcmp (set, "Ic") )
	{
	  IC_SET(inst)[ic_idx++] = numcl;
	}
      else
	{
	  fprintf(stderr, "bad set specification : %s", set);
	  exit(0);
	}
#ifdef _GNU_SOURCE
      free (set);
#endif
    }
  qsort (IC_SET(inst), IC_CARD(inst), sizeof (int), (comparison_fn_t )cmpint);
  qsort (ID_SET(inst), ID_CARD(inst), sizeof (int), (comparison_fn_t) cmpint);
  qsort (E_SET(inst), E_CARD(inst), sizeof (int), (comparison_fn_t) cmpint);
}


/**
 * fill clause structure
 */
static void 
encode_clause (int* literals, int size, type_instance inst, int card)
{
  int   i, lit;
  
  CURRENT_CLAUSE_INDEX(inst)++;
  CLAUSES(inst)[CURRENT_CLAUSE_INDEX(inst)].literals = 
    (int*) calloc (sizeof (int), size + 1);
  assert (CLAUSES(inst)[CURRENT_CLAUSE_INDEX(inst)].literals != NULL);
  CLAUSES(inst)[CURRENT_CLAUSE_INDEX(inst)].size = size;
  CLAUSES(inst)[CURRENT_CLAUSE_INDEX(inst)].neutral = size;
  CLAUSES(inst)[CURRENT_CLAUSE_INDEX(inst)].falsified = 0;
  CLAUSES(inst)[CURRENT_CLAUSE_INDEX(inst)].satisfied = 0;
  CLAUSES(inst)[CURRENT_CLAUSE_INDEX(inst)].card = card;
  if ( bsearch(&CURRENT_CLAUSE_INDEX(inst), E_SET(inst), E_CARD(inst),
	       sizeof (int), (comparison_fn_t) cmpint) )
    {
      CLAUSES(inst)[CURRENT_CLAUSE_INDEX(inst)].from_set = SET_E;
    }
  else if ( bsearch(&CURRENT_CLAUSE_INDEX(inst), IC_SET(inst), IC_CARD(inst),
	       sizeof (int), (comparison_fn_t) cmpint) )
    {
      CLAUSES(inst)[CURRENT_CLAUSE_INDEX(inst)].from_set = SET_IC;
    }
  else if ( bsearch(&CURRENT_CLAUSE_INDEX(inst), ID_SET(inst), ID_CARD(inst),
	       sizeof (int), (comparison_fn_t) cmpint) )
    {
      CLAUSES(inst)[CURRENT_CLAUSE_INDEX(inst)].from_set = SET_ID;
    }
  else 
    {
      fprintf (stderr, "clause %d : unidentified set", 
	       CURRENT_CLAUSE_INDEX(inst));
      exit (1);
    }

  for ( i = 0 ; i < size ; i++ ) 
    {
      lit = literals[i];
      CLAUSES(inst)[CURRENT_CLAUSE_INDEX(inst)].literals[i] = lit;
      VARIABLES(inst)[VAR_Indice(lit, inst)].count[SIGN (lit)]++;
      VARIABLES(inst)[VAR_Indice(lit, inst)].from_e =
	( CLAUSES(inst)[CURRENT_CLAUSE_INDEX(inst)].from_set == SET_E );
    }
  CLAUSES(inst)[CURRENT_CLAUSE_INDEX(inst)].literals[size] = 0;
}

/**
 * fill clause structure "duplicate" for gstructure
 */
static void 
encode_clause_duplic (int* literals, int size, type_instance inst, int card, int from_e, unsigned num_cl)
{
  int   i, lit;
  
  CURRENT_CLAUSE_INDEX(inst)++;
  CLAUSES(inst)[CURRENT_CLAUSE_INDEX(inst)].literals = 
    (int*) calloc (sizeof (int), size + 1);
  assert (CLAUSES(inst)[CURRENT_CLAUSE_INDEX(inst)].literals != NULL);
  CLAUSES(inst)[CURRENT_CLAUSE_INDEX(inst)].size = size;
  CLAUSES(inst)[CURRENT_CLAUSE_INDEX(inst)].neutral = size;
  CLAUSES(inst)[CURRENT_CLAUSE_INDEX(inst)].falsified = 0;
  CLAUSES(inst)[CURRENT_CLAUSE_INDEX(inst)].satisfied = 0;
  CLAUSES(inst)[CURRENT_CLAUSE_INDEX(inst)].card = card;
  CLAUSES(inst)[CURRENT_CLAUSE_INDEX(inst)].num_cl = num_cl;
  
  if ( from_e== SET_E )
    {
      CLAUSES(inst)[CURRENT_CLAUSE_INDEX(inst)].from_set = SET_E;
    }
  else if ( from_e== SET_IC )
    {
      CLAUSES(inst)[CURRENT_CLAUSE_INDEX(inst)].from_set = SET_IC;
    }
  else if ( from_e== SET_ID )
    {
      CLAUSES(inst)[CURRENT_CLAUSE_INDEX(inst)].from_set = SET_ID;
    }
  else 
    {
      fprintf (stderr, "clause %d : unidentified set", 
	       CURRENT_CLAUSE_INDEX(inst));
      exit (1);
    }
  
  for ( i = 0 ; i < size ; i++ ) 
    {
      lit = literals[i];
      CLAUSES(inst)[CURRENT_CLAUSE_INDEX(inst)].literals[i] = lit;
      VARIABLES(inst)[VAR_Indice(lit, inst)].count[SIGN (lit)]++;
	  VARIABLES(inst)[VAR_Indice(lit, inst)].from_e = (from_e== SET_E);
    }
  CLAUSES(inst)[CURRENT_CLAUSE_INDEX(inst)].literals[size] = 0;
}

/**
 * initialize occurence vectors for variables
 */
static void 
build_occurrences (type_instance inst, int alloc)
{
  int i,j,val,lit,var,occ;
  
  for ( i = 1 ; i <= VARIABLE_NUMBER(inst) ; i++ )
    for ( val = 0 ; val <= 1 ; val++ ) 
      {
	if ( alloc )
	  VARIABLES(inst)[i].occurrences[val] = 
	    (int*) calloc (sizeof (int), 
			   VARIABLES(inst)[i].count[val] + 1);
	
	assert (VARIABLES(inst)[i].occurrences[val] != NULL);
		
	VARIABLES(inst)[i].count[val] = 0;
      }
  
  for ( i = 1 ; i <= CLAUSE_NUMBER(inst) ; i++ )
    for ( j = 0 ; CLAUSES(inst)[i].literals[j] != 0 ; j++ ) 
      {
	lit = CLAUSES(inst)[i].literals[j];
	var = VAR_Indice(lit, inst);
	val = SIGN (lit);
	occ = VARIABLES(inst)[var].count[val];
	VARIABLES(inst)[var].occurrences[val][occ] = i;
	//l'instruction suivante met le dernier elemt à 0
	VARIABLES(inst)[var].occurrences[val][occ+1] = 0;
	VARIABLES(inst)[var].count[val]++;
	if (Opt_FullRem)
	  /* E set used only in full REM mode */
	  VARIABLES(inst)[var].from_e = ( CLAUSES(inst)[i].size == 1 );
	else
	  /* in "brute" mode, every var is in E */
	  VARIABLES(inst)[var].from_e = 1;
	 
      }
	  
}


/**
 * Fill E set with all clauses
 */  
void
brute_member (type_instance inst)
{
  int i;

  E_CARD(inst) = CLAUSE_NUMBER(inst);
  E_SET(inst) = NEW_ARRAY (int, E_CARD(inst));
  for (i = 1 ; i <= CLAUSE_NUMBER(inst) ; i++)
    {
      E_SET(inst)[i-1] = i;
    }
}

/**
 * chercher le num de parcelle où se trouve la variable
 */

int get_num_parc(char * kbfile_names,int num_var)
{
    int i,j;
	char c;
	FILE * pvparcel;
	if ( (pvparcel = fopen (kbfile_names, "r")) == NULL )
    {
      fprintf (stderr, "unable to open file %s\n", kbfile_names);
      exit (1);
    }
	while (! feof (pvparcel))
	{
		c = fgetc (pvparcel);
		fscanf (pvparcel, "%d", &i);
		c = fgetc (pvparcel);
		c = fgetc (pvparcel);
		fscanf (pvparcel, "%d", &j);
		c = fgetc (pvparcel);
		fscanf (pvparcel, "%d", &j);
		c = fgetc (pvparcel);
		if(j==num_var)
		{
			fclose(pvparcel);
			return i;
		}
		//while (c != '\n') 
		//c = fgetc (member);
	}
	//printf("probleme: ce numero de variable:%d n'existe pas dans le fichier crrespond.zeta\n",num_var);
	return -1;
	
}

/**
 * create the tables which hold the clauses & variables
 */
static void 
create_tables (type_instance inst,char * kbfile_names)
{
  int i;
  
  CLAUSES(inst) = ( (type_clause)
		    calloc (sizeof (struct struct_clause),
			    CLAUSE_NUMBER(inst) + 1) );
  
  assert ( CLAUSES(inst) != NULL );


  EXCLUDED(inst) = clset_new (CLAUSE_NUMBER(inst));
  for ( i = 1 ; i <= CLAUSE_NUMBER(inst) ; i++ )  
    {	
	  CLAUSES(inst)[i].num_cl=i;
	  
	}
	
  NODE_TABLE(inst) = nodetable_new();

  SATISFIED_CLAUSE_NUMBER(inst) = 0;
  VARIABLES(inst) = ( (type_variable)
		      calloc (sizeof (struct struct_variable),
			      VARIABLE_NUMBER(inst) + 1) );
  assert (VARIABLES(inst) != NULL );
  for ( i = 1 ; i <= VARIABLE_NUMBER(inst) ; i++ )  
    {	
	  VARIABLES(inst)[i].from_parc=get_num_parc(kbfile_names,i);
      VARIABLES(inst)[i].num_var=i;
	  VARIABLES(inst)[i].value=NONE;
      VARIABLES(inst)[i].assignment=IDLE;
      VARIABLES(inst)[i].count[0]=0;
      VARIABLES(inst)[i].count[1]=0;
    }

  NEXT_UNASSIGNED_VARIABLE(inst) = 1;

}


/**
 * read file header (number of variables and number of clauses
 */
static void 
read_header (FILE* input, type_instance inst)
{
  char c, cnf[256];
  int  i;
  
  c = fgetc (input);
  while ( (c != 'c' ) && ( ! feof (input) ) ) 
    c = fgetc (input);
  i = 0;
  while ( ( c != ' ' ) && ( ! feof (input) ) ) 
    {
      cnf[i++] = c; 
      c = fgetc (input);
    }
  cnf[i] = '\0';
  if ( feof (input) || ( strcmp (cnf, "cnf") != 0 ) ) 
    {
      fprintf (stderr, "A set of clauses in the Zeta format was expected.\n");
      fflush (stderr);
      exit (0);
    }
    
  fscanf (input, "%d", &VARIABLE_NUMBER(inst));
  if ( VARIABLE_NUMBER(inst) == 0 ) 
    {
      fprintf (stderr, "The number of variables was expected.\n"); 
      fflush (stderr);
      exit (0);
    }
  fscanf (input, "%d", &CLAUSE_NUMBER(inst));
  if ( CLAUSE_NUMBER(inst) == 0 ) 
    {
      fprintf (stderr, "The number of clauses was expected.\n"); 
      fflush (stderr);
      exit (0);
    }

  c = fgetc (input);
  while ( c != '\n' ) 
    c = fgetc (input);
}

/**
 * read a clause
 */
static void 
read_clause (FILE* input, int* clause, int* size, int* card)
{
  int i;

  *card = 0;

  if ( fscanf (input, "%d", size) == 0 )
      fscanf (input, "*%d %d", card, size);
  for ( i = 0 ; i < *size ; i++ ) 
    fscanf (input, "%d", &clause[i]);
  clause[*size] = 0;
}


/**
 * read a problem instance file, return a new instance
 */
type_instance
read_instance (FILE * input, FILE * member1, char * kbfile_names)
{
  int *clause, size, card;
  
  type_instance newinst = NEW (struct struct_instance);
  assert (newinst != NULL);

  read_header (input, newinst);
  create_tables (newinst,kbfile_names);
  
  if (Opt_FullRem) 
    /* E, Ic Id sets used only in Full REM mode */
    read_member (member1, newinst);
  else
    /* all clauses in E ! */
    brute_member(newinst);

  CURRENT_CLAUSE_INDEX(newinst) = 0;
  clause = (int*) calloc (sizeof (int), VARIABLE_NUMBER(newinst) + 1);
  assert (clause != NULL);
  while ( ( CURRENT_CLAUSE_INDEX(newinst) < CLAUSE_NUMBER(newinst) )&&
	  ( ! feof (input) ) ) 
    {
      read_clause (input, clause, &size, &card);
      if ( size == 0 ) 
	{
	  fprintf (stderr, "Warning, empty clause.\n"); 
	  fflush(stderr);
	}
      encode_clause (clause, size, newinst, card);
    }
  free (clause);
  
  if ( CURRENT_CLAUSE_INDEX(newinst) != CLAUSE_NUMBER(newinst) ) 
    {
      fprintf (stderr, "Unexpected end of file.\n"); 
      fflush (stderr);
      exit (0);
    }
  build_occurrences (newinst, 1);
  return newinst;
}

/**
 * reset the base to start a new DP
 */
void 
reset_base (type_instance kb)
{
  register int i;

  SATISFIED_CLAUSE_NUMBER(kb) = 0;
  NEXT_UNASSIGNED_VARIABLE(kb) = 1;
  /* un rajout de doukari */
  //ASSIGNMENT_NUMBER(kb)=0;
  //MONOTONE_NUMBER(kb)=0;
  //CHOICE_NUMBER(kb)=0;
  //SUBSUM_NUMBER(kb)=0;
  //PRESENT_VARIABLE_NUMBER(kb)=0;
  //UNIT_NUMBER(kb)=0;
  /* reset each clause
   */
  for ( i = 1 ; i <= CURRENT_CLAUSE_INDEX(kb) ; i++ )
    {
      CLAUSES(kb)[i].neutral = CLAUSES(kb)[i].size;
      CLAUSES(kb)[i].falsified = 0;
      CLAUSES(kb)[i].satisfied = 0;
    }

  /* reset variables
   */
  for ( i = 1 ; i <= VARIABLE_NUMBER(kb) ; i++ )  
    {
      VARIABLES(kb)[i].value=NONE;
      VARIABLES(kb)[i].assignment=IDLE;
      VARIABLES(kb)[i].count[0]=0;
      VARIABLES(kb)[i].count[1]=0;
    }

  /* rebuild occurences
   */
  build_occurrences (kb, 0);
}

/**********************************************************************
 **********************************************************************
 **
 ** Various
 **
 **********************************************************************
 **********************************************************************
 */


/**
 * adds an Fi set (empty clauses set)
 */
static fi 
addfi (fi e,  garray cl)
{
  fi new = NEW (struct st_fi);

  new->clauses = cl;
  new->lastch = 0;
  new->next = e;
  return new;
}

/**
 * alloacates an int array growable by chunks
 */
static garray
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
static void
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
 * removes the last element of a growable array
 */
static void
growable_remove(garray a)
{
  assert (a != NULL);
  
  if (a->lindex >= 0)
    a->lindex--;
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

/**
 * free a fi list
 */
void
freefi (fi e)
{
  fi p = e;
  fi del;
  while (p)
    {
      growable_free (p->clauses);
      del = p;
      p = p->next;
      free(del);
    }
}

/**********************************************************************
 **********************************************************************
 **
 ** DP utility functions
 **
 **********************************************************************
 *********************************************************************/

/**
 * pushes initial monoliteral clauses on stack 
 */
void
push_units (type_instance kb, type_stack st)
{
  int i;

  for (i = 1 ; i <= CLAUSE_NUMBER(kb) ; i++)
    {
      if (CLAUSES(kb)[i].size == 1 && !clset_member (EXCLUDED(kb), CLAUSES(kb)[i].num_cl))
	dp_push_literal (CLAUSES(kb)[i].literals[0], kb, st);
    }
}

/**
 * chooses a literal...
 *
 * the first unassigned variable one 
 * */
static int
dp_choose_literal_first (type_instance inst)
{
  int i, lit, count, maxl, maxl2;
  int lit2;
  int *occ;

  lit2 = 0;
  lit = 0;
  maxl2 = 0;
  maxl = 0;
  /* for each variable
   */
  for ( i = 1 ; i <= VARIABLE_NUMBER(inst) ; i++)
    {
      if ( VARIABLES(inst)[i].assignment != DONE )
	{
	  /* unassigned variable */
	  if ( VARIABLES(inst)[i].from_e )
	    { /* variable is in E */
	      /* for each negative instance of variable i 
	       */
	      lit = VARIABLES(inst)[i].num_var;
		  
	    }
	  else
	    { /* variable is not in E */
	      /* for each negative occurence of variable
	       */
	     lit2 = VARIABLES(inst)[i].num_var;
		 
	    }
	}
    }
  if ( ! lit )
    lit = lit2;  

#ifndef NTRACE
  printf("choose %d\n",lit); fflush(stdout);
#endif
  assert ( lit != 0 );
  return (lit);
}

/**
 * chooses a literal
 *
 * first tries to choose a monoliteral from E which appears in the
 * greatest number of clauses in Ic. If none, choose the literal
 * which appears in the greatest number of binary clauses.
 */
static int
dp_choose_literal (type_instance inst)
{
  int i, lit, count, maxl, maxl2;
  int lit2;
  int *occ;

  lit2 = 0;
  lit = 0;
  maxl2 = 0;
  maxl = 0;
  /* for each variable
   */
  for ( i = 1 ; i <= VARIABLE_NUMBER(inst) ; i++)
    {
      if ( VARIABLES(inst)[i].assignment != DONE )
	{
	  /* unassigned variable */
	  if ( VARIABLES(inst)[i].from_e )
	    { /* variable is in E */
	      /* for each negative instance of variable i 
	       */
	      occ = VARIABLES(inst)[i].occurrences[NEGATIVE];
	      count = 0;
	      while ( *occ )
		{
		  /* don't choose in excluded clauses */
		  if ( ! clset_member (EXCLUDED(inst),  CLAUSES(inst)[*occ].num_cl) )
		    {
		      /* if variable appears negatively in a binary clause
		       * not affected at all, then increment count
		       */
		      if (Opt_FullRem)
			{
			  if ( (CLAUSES(inst)[*occ].from_set & SET_IC) != 0 &&
			       CLAUSES(inst)[*occ].neutral == 2 &&
			       CLAUSES(inst)[*occ].satisfied == 0 )
			    {
			      count++;
			    }
			}
		      else
			{
			  if ( CLAUSES(inst)[*occ].neutral == 2 &&
			       CLAUSES(inst)[*occ].satisfied == 0 )
			    {
			      count++;
			    }
			}
		    }
		  occ++;
		}
	      if ( count > maxl )
		{
		  lit = VARIABLES(inst)[i].num_var;
		  maxl = count;
		}
	     // printf("rentree 1: variable = %d est from E, maxl=%d\n",VARIABLES(inst)[i].num_var,maxl);
	    }
	  else
	    { /* variable is not in E */
	      /* for each negative occurence of variable
	       */
	      occ = VARIABLES(inst)[i].occurrences[NEGATIVE];
	      count = 0;
	      while ( *occ )
		{
		  /* don't choose in excluded clauses 
		   */
		  if ( ! clset_member (EXCLUDED(inst),  CLAUSES(inst)[*occ].num_cl) )
		    {
		      /* if variable appears in an unsatisfied
		       * binary clause
		       */
		      if ( CLAUSES(inst)[*occ].neutral == 2 &&
			   CLAUSES(inst)[*occ].satisfied == 0 )
			{
			  count++;
			}
		    }
		  occ++;
		}
	      if ( count > maxl2 )
		{
		  lit2 = VARIABLES(inst)[i].num_var;
		  maxl2 = count;
		}
	     // printf("rentree 2: variable = %d n'est pas from E, maxl2=%d\n",VARIABLES(inst)[i].num_var,maxl2);
	    }
	}
    }
  if ( ! lit )
    lit = lit2;

  if ( ! lit )
    {
      /* for each variable
       */
      for ( i = 1 ; i <= VARIABLE_NUMBER(inst) ; i++)
      if ( VARIABLES(inst)[i].assignment != DONE )
	  {
	/* unassigned variable */
	if ( VARIABLES(inst)[i].from_e )
	  {
      maxl = 0; 
	  /* variable is in E */
	    /* for each positive instance of variable i 
	     */
	    occ = VARIABLES(inst)[i].occurrences[POSITIVE];
	    count = 0;
	    while ( *occ )
	      {
		/* don't choose in excluded clauses */
		if ( ! clset_member (EXCLUDED(inst),  CLAUSES(inst)[*occ].num_cl) )
		  {
		    if ( CLAUSES(inst)[*occ].satisfied == 0 )
		      count++;
		  }
		occ++;
	      }
	    if ( count > maxl )
	      {
		lit = VARIABLES(inst)[i].num_var;
		maxl = count;
	      }
		  
	     // printf("rentree 3: variable = %d est from E, maxl=%d\n",VARIABLES(inst)[i].num_var,maxl);
	  }
	else
	{
		maxl2=0;
		occ = VARIABLES(inst)[i].occurrences[POSITIVE];
	    count = 0;
	    while ( *occ )
	      {
		/* don't choose in excluded clauses */
		if ( ! clset_member (EXCLUDED(inst),  CLAUSES(inst)[*occ].num_cl) )
		  {
		    if ( CLAUSES(inst)[*occ].satisfied == 0 )
		      count++;
		  }
		occ++;
	      }
	    if ( count > maxl2 )
	      {
		lit2 = VARIABLES(inst)[i].num_var;
		maxl2 = count;
	      }
		
	  //  printf("rentree 4: variable = %d n'est pas from E, maxl2=%d\n",VARIABLES(inst)[i].num_var,maxl2);
		
	}
	}
    }
	if ( ! lit )
		lit = lit2;  

#ifndef NTRACE
  printf("choose %d\n",lit); fflush(stdout);
#endif
  assert ( lit != 0 );
  return (lit);
}

/**
 * pops a literal out of the stack
 */
static int 
dp_pop_literal (type_stack stack)
  /* require : ! IS_STACK_EMPTY */
{
  assert ( ! IS_STACK_EMPTY (stack) );

  TOP_STACK(stack)--;
#ifndef NTRACE
  printf("pop %d\n",*(TOP_STACK(stack))); 
  fflush(stdout);
#endif
  return ( *(TOP_STACK(stack)) );
}

/**
 * pushes a literal on a stack
 * Returns NULL if the stack is consistent after push operation,
 * if the stack is inconsistant it returns the array of monoliteral clause(s)
 * which would became empty in case of assignment to the opposite value
 * if the stack is inconsistent AND clauses which became empty are in the
 * excluded set, return just 1. This is a quite dirty hack because
 * we know that, pointers being aligned on 2 or 4 bytes boundaries, 1 is
 * not a valid pointer
 */
garray
dp_push_literal (int lit, type_instance inst, type_stack stack)
{
  int var, sig;
  garray ret = NULL;

  var = VAR_Indice(lit, inst);
  sig = SIGN(lit);

  if ( VARIABLES(inst)[var].value == NONE ) 
    {
      *TOP_STACK(stack) = lit;
      TOP_STACK(stack)++;
      VARIABLES(inst)[var].value=(enum_value) sig;
      VARIABLES(inst)[var].assignment=TODO;
#ifndef NTRACE
      printf("push %d\n",lit); fflush(stdout);
#endif
    }

  else if ( VARIABLES(inst)[var].value != sig ) 
    {
      int *occ;
      /* this means we have already pushed the opposite literal on */
      /* stack, so look for the monoliteral clause(s) containing this */
      /* opposite literal                                          */
      for (occ = VARIABLES(inst)[var].occurrences[NOT(sig)] ;
	   *occ;
	   occ++)
	if (CLAUSES(inst)[*occ].satisfied == 0 &&
	    CLAUSES(inst)[*occ].neutral == 1)
	  {
	    if (!clset_member (EXCLUDED(inst),  CLAUSES(inst)[*occ].num_cl))
	      {
		if (!ret || ((int)ret == 1)) 
		  ret = new_growable(30,10);
		growable_add(ret,  CLAUSES(inst)[*occ].num_cl);
	      }
	    else
	      if ((int)ret == 0)
		ret = (garray)1;
	  }
      IS_STACK_CONSISTANT(stack) = 0;
#ifndef NTRACE
      printf("stack unconsistant\n"); fflush(stdout);
#endif
    }
  return ret;
}


/**
 * in a clause which have reached the cardinality constraint, 
 * push opposite of remaining literals on stack.
 */
static void
push_others (int l, type_clause clause, type_instance inst, type_stack stack)
{
  int *lit = clause->literals;
  int i;
  
  for ( i = 0 ; *lit ; lit++, i++ )
    {
      if ( *lit != l && VARIABLES(inst)[VAR_Indice(*lit,inst)].assignment != DONE )
	{
	  dp_push_literal (OPPOSITE(*lit), inst, stack);
	}
    }
}


/**
 * assigns a literal
 */
static void
dp_assign_literal (int lit, type_instance inst, type_stack stack, fi *f)
{
  int var, sig, *occ, *lits, opp, v, s;
  garray emptycls = new_growable(30, 10);

#ifndef NTRACE
  printf("assign %d\n",lit); fflush(stdout);
#endif

  ASSIGNMENT_NUMBER(inst)++;

  var = VAR_Indice(lit, inst);
  sig = SIGN(lit);

  /* assigns the value (1 : true, -1 : false) */
  VARIABLES(inst)[var].value=(enum_value) sig;
 
  /* for each clause where the literal occurs */
  occ = VARIABLES(inst)[var].occurrences[sig];
  
  while ( *occ != 0 ) 
    {
      /* don't take excluded clauses into account */
      if ( ! clset_member (EXCLUDED(inst),  CLAUSES(inst)[*occ].num_cl) )
	{
	  /* clause is satisfied */
	  CLAUSES(inst)[*occ].neutral--;
	  CLAUSES(inst)[*occ].satisfied++;
      
	  /* if satisfied for the first time */
	  if ( CLAUSES(inst)[*occ].satisfied == 1 ) 
	    {
		  /* for each variable in the clause, update occurence count. */
	      lits = CLAUSES(inst)[*occ].literals;
	      while ( *lits != 0 ) 
		{
		  v = VAR_Indice(*lits,inst); 
		  s=SIGN(*lits);
		  VARIABLES(inst)[v].count[s]--;
		  lits++;
		}
	      SATISFIED_CLAUSE_NUMBER(inst)++;
	    }
	  /* if clause has a cardinality constraint, check this */
	  if ( CLAUSES(inst)[*occ].card != 0 )
	    {
	      if ( CLAUSES(inst)[*occ].card == CLAUSES(inst)[*occ].satisfied )
		{
		  push_others(lit, &CLAUSES(inst)[*occ], inst, stack);
		}
	      else if ( CLAUSES(inst)[*occ].card < CLAUSES(inst)[*occ].satisfied )
		{
		  /* cardinality constraint violation             */
		  /* act as if an empty clause has been generated */
		  IS_STACK_CONSISTANT (stack) = 0;
		  growable_add (emptycls, CLAUSES(inst)[*occ].num_cl);
		}
	    }
	}
      occ++;
    }
  
  /* for each clause where the opposite literal occurs */
  occ = VARIABLES(inst)[var].occurrences[NOT(sig)];
  opp = OPPOSITE(lit);
  while ( *occ != 0 ) 
    {
      /* don't take excluded clauses into account */
      if ( ! clset_member (EXCLUDED(inst),  CLAUSES(inst)[*occ].num_cl) )
	{      
	  /* clause is unsat */
	  CLAUSES(inst)[*occ].neutral--;
	  CLAUSES(inst)[*occ].falsified++;

	  /* *not sat* monoliteral clause */
	  if ( CLAUSES(inst)[*occ].satisfied == 0 )
	    {
	      if ( CLAUSES(inst)[*occ].neutral == 1 ) 
		{
		  lits = CLAUSES(inst)[*occ].literals;
		  while ( *lits != 0 ) 
		    {
		      garray ecl = 0;
		      v = VAR_Indice(*lits,inst); 
		      s = SIGN(*lits);
		      if ( ( v != var ) && 
			   ( VARIABLES(inst)[v].assignment != DONE ) ) 
			if ((ecl = dp_push_literal (*lits, inst, stack)) != NULL)
			  {
			    /* stack is inconsistent add falsified clauses */
			    /* ecl : clauses falsified by -*lits */
			    *f = addfi(*f, ecl);
			    growable_remove(emptycls);
			    growable_add (emptycls,  CLAUSES(inst)[*occ].num_cl); /* falsified by *lits */
			  }
		      lits++;
		    }
		}
	      else if ( CLAUSES(inst)[*occ].neutral == 0 )
		{
		  /* empty clause */
		  IS_STACK_CONSISTANT (stack) = 0;
		  growable_add (emptycls,  CLAUSES(inst)[*occ].num_cl);
		}
	    }
	}
      occ++;
    }
  if ( growable_size (emptycls) != 0 )
    *f = addfi (*f, emptycls);
  else
    growable_free (emptycls);

  VARIABLES(inst)[var].assignment=DONE;
}

/**
 * assigns a literal : une nouvelle version sans les "fi"
 */
static void
dp_assign_literal_sans_fi (int lit, type_instance inst, type_stack stack)
{
  int var, sig, *occ, *lits, opp, v, s;
  //garray emptycls = new_growable(30, 10);

#ifndef NTRACE
  printf("assign %d\n",lit); fflush(stdout);
#endif

  ASSIGNMENT_NUMBER(inst)++;

  var = VAR_Indice(lit, inst);
  sig = SIGN(lit);

  /* assigns the value (1 : true, -1 : false) */
  VARIABLES(inst)[var].value=(enum_value) sig;
 
  /* for each clause where the literal occurs */
  occ = VARIABLES(inst)[var].occurrences[sig];
  
  while ( *occ != 0 ) 
    {
	  /* clause is satisfied */
	  CLAUSES(inst)[*occ].neutral--;
	  CLAUSES(inst)[*occ].satisfied++;
      
	  /* if satisfied for the first time */
	  if ( CLAUSES(inst)[*occ].satisfied == 1 ) 
	    {
		  /* for each variable in the clause, update occurence count. */
	      lits = CLAUSES(inst)[*occ].literals;
	      while ( *lits != 0 ) 
		{
		  v = VAR_Indice(*lits,inst); 
		  s=SIGN(*lits);
		  VARIABLES(inst)[v].count[s]--;
		  lits++;
		}
	      SATISFIED_CLAUSE_NUMBER(inst)++;
	    }
	  /* if clause has a cardinality constraint, check this */
	  if ( CLAUSES(inst)[*occ].card != 0 )
	    {
	      if ( CLAUSES(inst)[*occ].card == CLAUSES(inst)[*occ].satisfied )
		{
		  push_others(lit, &CLAUSES(inst)[*occ], inst, stack);
		}
	      else if ( CLAUSES(inst)[*occ].card < CLAUSES(inst)[*occ].satisfied )
		{
		  /* cardinality constraint violation             */
		  /* act as if an empty clause has been generated */
		  IS_STACK_CONSISTANT (stack) = 0;
		  //growable_add (emptycls, CLAUSES(inst)[*occ].num_cl);
		}
	    }
      occ++;
    }
  
  /* for each clause where the opposite literal occurs */
  occ = VARIABLES(inst)[var].occurrences[NOT(sig)];
  opp = OPPOSITE(lit);
  while ( *occ != 0 ) 
    {      
	  /* clause is unsat */
	  CLAUSES(inst)[*occ].neutral--;
	  CLAUSES(inst)[*occ].falsified++;

	  /* *not sat* monoliteral clause */
	  if ( CLAUSES(inst)[*occ].satisfied == 0 )
	    {
	      if ( CLAUSES(inst)[*occ].neutral == 1 ) 
		{
		  lits = CLAUSES(inst)[*occ].literals;
		  while ( *lits != 0 ) 
		    {
		      garray ecl = 0;
		      v = VAR_Indice(*lits,inst); 
		      s = SIGN(*lits);
		      if ( ( v != var ) && 
			   ( VARIABLES(inst)[v].assignment != DONE ) ) 
			if ((ecl = dp_push_literal (*lits, inst, stack)) != NULL)
			  {
			    /* stack is inconsistent add falsified clauses */
			    /* ecl : clauses falsified by -*lits */
			    //*f = addfi(*f, ecl);
			    //growable_remove(emptycls);
			    //growable_add (emptycls,  CLAUSES(inst)[*occ].num_cl); /* falsified by *lits */
			  }
		      lits++;
		    }
		}
	      else if ( CLAUSES(inst)[*occ].neutral == 0 )
		{
		  /* empty clause */
		  IS_STACK_CONSISTANT (stack) = 0;
		  //growable_add (emptycls,  CLAUSES(inst)[*occ].num_cl);
		}
	    }
      occ++;
    }
  //if ( growable_size (emptycls) != 0 )
  //  *f = addfi (*f, emptycls);
  //else
  //  growable_free (emptycls);

  VARIABLES(inst)[var].assignment=DONE;
}
  
/**
 * Deassigns a literal
 */
static void
dp_deassign_literal (int lit, type_instance inst)
{
  int var, sig, *occ, *lits, v, s;

#ifndef NTRACE
  printf("deassign %d\n",lit); fflush(stdout);
#endif

  var = VAR_Indice(lit,inst);
  sig = SIGN(lit);

  /* for each clause where the literal occurs */
  occ = VARIABLES(inst)[var].occurrences[sig];
  while ( *occ != 0 ) 
    {
      /* do not take excluded clauses into account */
      if ( ! clset_member (EXCLUDED(inst),  CLAUSES(inst)[*occ].num_cl) )
	{
	  /* unsat the clause */
	  CLAUSES(inst)[*occ].neutral++;
	  CLAUSES(inst)[*occ].satisfied--;

	  /* if clause is now to be satisfied */
	  if ( CLAUSES(inst)[*occ].satisfied == 0 ) 
	    {
	      /* for each literal in the clause update occurences */
	      lits = CLAUSES(inst)[*occ].literals;
	      while ( *lits != 0 ) 
		{
		  v = VAR_Indice(*lits,inst); 
		  s = SIGN(*lits);
		  VARIABLES(inst)[v].count[s]++;
		  lits++;
		}
	      SATISFIED_CLAUSE_NUMBER(inst)--;
	    }
	}
      occ++;
    }
  
  /* for each clause where the opposite literal occurs */
  occ = VARIABLES(inst)[var].occurrences[NOT(sig)];
  while ( *occ != 0 ) 
    {
      /* do not take excluded clauses into account */
      if ( ! clset_member (EXCLUDED(inst),  CLAUSES(inst)[*occ].num_cl) )
	{    
	  /* unfalsify */
	  CLAUSES(inst)[*occ].neutral++;
	  CLAUSES(inst)[*occ].falsified--;
	}
      occ++;
    }

  VARIABLES(inst)[var].value=NONE;
  VARIABLES(inst)[var].assignment=IDLE;
}


int 
dp (type_instance kb, type_stack stack, fi *f, int * bool1)
{
  int lit,i,cmpt=0;
  short result, left, right;

  /*
   * Unit propagation raised an inconsistency
   * ----------------------------------------
   */

  if ( ! IS_STACK_CONSISTANT (stack) )
    {
      while ( ! IS_STACK_EMPTY (stack) )
	{
	  lit = dp_pop_literal (stack);
	  printf("Unit propagation raised an inconsistency: lit=%d\n",lit);
	  VARIABLES(kb)[VAR_Indice(lit, kb)].value = NONE;
	  VARIABLES(kb)[VAR_Indice(lit, kb)].assignment = IDLE;
	}
      IS_STACK_CONSISTANT (stack) = 1;
      result = (-2);
    }

  /*
   * It remains unit and/or monotone literals to propagate
   * -----------------------------------------------------
   */

  else if ( ! IS_STACK_EMPTY (stack) ) 
    {
	  lit = dp_pop_literal (stack);
	  printf("It remains unit and/or monotone literals to propagate: lit=%d\n",lit);
      dp_assign_literal (lit, kb, stack, f);
	  UNIT_NUMBER(kb)++;
      left = dp (kb, stack, f,bool1);
      if ( left < 0 )
	  {
		dp_deassign_literal (lit, kb);
		}
      if ( left > 0 )
	  {
		  result = left;
	  }
      else
	{
	  lit = OPPOSITE(lit);
	  dp_assign_literal (lit, kb, stack, f);
	  right = dp (kb, stack, f,bool1);
	  if ( right < 0 ) 
	  {
	    dp_deassign_literal(lit, kb);
		}
	  if ( right > 0 ) 
	    {
	      result = -left;
	    }
	  else 
	    {
	      if ( left < right )      
		{
		  result = left;
		}
	      else if ( right < left ) 
		{
		  result = right; 
		}
	      else
		{ 
		  result = left-1;
		}
	    }
	}
    }

  /*
   * The instance is satisfied
   * -------------------------
   */

  else if ( SATISFIED_CLAUSE_NUMBER (kb) == 
	   ( CURRENT_CLAUSE_INDEX (kb) - EXCLUDEDTM(kb) ) ) 
    {
	  printf("The instance is satisfied \n");
	  //scanf("%d",&i);
#ifndef NTRACE
      printf ("Un modele !\n");
#endif
	  result = 2 ;
    }

  /*
   * Branching rule 
   * --------------
   */

  else 
    {
	  *bool1=1;
	  lit = dp_choose_literal(kb);
	  printf("Branching rule: lit=%d\n",lit);
      CHOICE_NUMBER(kb)++;
      dp_assign_literal (lit, kb, stack, f);
      left = dp(kb, stack, f,bool1);
      if ( left < 0 ) 
	{
	  dp_deassign_literal (lit, kb);
	}
      if ( left > 0 ) 
	{
	  result = left;
	}
      else 
	{
	  lit = OPPOSITE(lit);
	  dp_assign_literal (lit, kb, stack, f);
	  right = dp(kb, stack, f,bool1);
	  if ( right < 0 ) 
	    dp_deassign_literal(lit, kb);
	  if ( right > 0 ) 
	    {
	      result = -left;
	    }
	  else 
	    {
	      if ( left < right )      
		{
		  result = left;
		}
	      else if ( right < left ) 
		{
		  result = right; 
		}
	      else
		{ 
		  result = left-1;
		}
	    }
	}
    }
  return (result);
}

/** dp une nouvelle version sans les "fi", juste pour vérifier la cohérence */

int 
dp_sans_fi (type_instance kb, type_stack stack)
{
  int lit,i;
  short result, left, right;

  /*
   * Unit propagation raised an inconsistency
   * ----------------------------------------
   */

  if ( ! IS_STACK_CONSISTANT (stack) )
    {
      while ( ! IS_STACK_EMPTY (stack) )
	{
	  lit = dp_pop_literal (stack);
	  //printf("Unit propagation raised an inconsistency: lit=%d\n",lit);
	  VARIABLES(kb)[VAR_Indice(lit, kb)].value = NONE;
	  VARIABLES(kb)[VAR_Indice(lit, kb)].assignment = IDLE;
	}
      IS_STACK_CONSISTANT (stack) = 1;
      result = (-2);
    }

  /*
   * It remains unit and/or monotone literals to propagate
   * -----------------------------------------------------
   */

  else if ( ! IS_STACK_EMPTY (stack) ) 
    {
	  lit = dp_pop_literal (stack);
	  //printf("It remains unit and/or monotone literals to propagate: lit=%d\n",lit);
      dp_assign_literal_sans_fi (lit, kb, stack);
	  UNIT_NUMBER(kb)++;
      left = dp_sans_fi (kb, stack);
      if ( left < 0 )
	  {
		dp_deassign_literal (lit, kb);
		}
      if ( left > 0 )
	  {
		  result = left;
	  }
      else
	{
	  lit = OPPOSITE(lit);
	  dp_assign_literal_sans_fi (lit, kb, stack);
	  right = dp_sans_fi (kb, stack);
	  if ( right < 0 ) 
	  {
	    dp_deassign_literal(lit, kb);
		}
	  if ( right > 0 ) 
	    {
	      result = -left;
	    }
	  else 
	    {
	      if ( left < right )      
		{
		  result = left;
		}
	      else if ( right < left ) 
		{
		  result = right; 
		}
	      else
		{ 
		  result = left-1;
		}
	    }
	}
    }

  /*
   * The instance is satisfied
   * -------------------------
   */

  else if ( SATISFIED_CLAUSE_NUMBER (kb) == 
	    ( CURRENT_CLAUSE_INDEX (kb) - EXCLUDEDTM(kb) ) ) 
    {
	  //printf("The instance is satisfied \n");
	  //scanf("%d",&i);
#ifndef NTRACE
      printf ("Un modele !\n");
#endif
	  result = 2 ;
    }

  /*
   * Branching rule 
   * --------------
   */

  else 
    {
	  //printf("SATISFIED_CLAUSE_NUMBER (kb)=%d, CURRENT_CLAUSE_INDEX (kb)=%d et EXCLUDEDTM(kb)=%d\n",SATISFIED_CLAUSE_NUMBER (kb), CURRENT_CLAUSE_INDEX (kb), EXCLUDEDTM(kb));
	  lit = dp_choose_literal(kb);
	  //printf("Branching rule: lit=%d\n",lit);
	  //if(lit==22)
		//scanf("%d",&i);
      CHOICE_NUMBER(kb)++;
      dp_assign_literal_sans_fi (lit, kb, stack);
      left = dp_sans_fi(kb, stack);
      if ( left < 0 ) 
	{
	  dp_deassign_literal(lit, kb);
	}
      if ( left > 0 ) 
	{
	  result = left;
	}
      else 
	{
	  lit = OPPOSITE(lit);
	  dp_assign_literal_sans_fi (lit, kb, stack);
	  right = dp_sans_fi(kb, stack);
	  if ( right < 0 ) 
	    dp_deassign_literal(lit, kb);
	  if ( right > 0 ) 
	    {
	      result = -left;
	    }
	  else 
	    {
	      if ( left < right )      
		{
		  result = left;
		}
	      else if ( right < left ) 
		{
		  result = right; 
		}
	      else
		{ 
		  result = left-1;
		}
	    }
	}
    }
  return (result);
}


/**
 * indice de la clause correspondant à un num_cl
 */

int CL_Indice(int num_cl, type_instance inst)
{
	int i;
	for(i=1; i<=CLAUSE_NUMBER(inst);i++)
		if(CLAUSES(inst)[i].num_cl==num_cl)
			return i;
			
	return 0;	
	
}



/**
 * indice de la variable correspondante à un littéral
 */

int VAR_Indice(int lit, type_instance inst)
{
	int i;
	for(i=1; i<=VARIABLE_NUMBER(inst);i++)
		if(VARIABLES(inst)[i].num_var==VARIABLE(lit))
			return i;
			
	//printf("le literal:%d n'a pas pu etre trouve!\n", lit);
	return 0;	
	
}

void initialise_insts(Gstructure gs,type_instance KBase)
{
  int * tab_var, * tab_cl;
  int i,j, k, nbr_var, nbr_cl;
  int e_card, id_card, ic_card;
  int e_idx, id_idx, ic_idx;
  
  for(i=0;i<gs->nbr_parties;i++)
  {
	  
	  printf("initialisation sous-instantce n:%d\n",i+1);
	  gs->parties[i]->inst= NEW (struct struct_instance);
	  assert (gs->parties[i]->inst != NULL);
	  
	  tab_var=(int *) malloc((VARIABLE_NUMBER(KBase))*sizeof(int));
	  tab_cl=(int *) malloc((CLAUSE_NUMBER(KBase))*sizeof(int));
	  
	  /* calcul des variables  */
	  nbr_var=0;
	  
	  for(j=0;j<gs->parties[i]->noyau->nbr_parcel;j++)
	  for(k=1;k<=VARIABLE_NUMBER(KBase);k++)
		if(VARIABLES(KBase)[k].from_parc==gs->parties[i]->noyau->noyau[j]->num_parc_v)
			{
				tab_var[nbr_var]=k;
				nbr_var++;				
			}
			
	  for(j=0;j<gs->parties[i]->couv->nbr_parcel;j++)
	  for(k=1;k<=VARIABLE_NUMBER(KBase);k++)
		if(VARIABLES(KBase)[k].from_parc==gs->parties[i]->couv->couv[j]->num_parc_v)
			{
				tab_var[nbr_var]=k;
				nbr_var++;
			}
	  
	  VARIABLE_NUMBER(gs->parties[i]->inst)=nbr_var;
	  
	  VARIABLES(gs->parties[i]->inst) = ( (type_variable)
		      calloc (sizeof (struct struct_variable),
			      VARIABLE_NUMBER(gs->parties[i]->inst) + 1) );
      assert (VARIABLES(gs->parties[i]->inst) != NULL );
      for ( k = 1 ; k <= VARIABLE_NUMBER(gs->parties[i]->inst) ; k++ )  
		{
			VARIABLES(gs->parties[i]->inst)[k].from_parc=VARIABLES(KBase)[tab_var[k-1]].from_parc;
			VARIABLES(gs->parties[i]->inst)[k].num_var=VARIABLES(KBase)[tab_var[k-1]].num_var;
			VARIABLES(gs->parties[i]->inst)[k].value=NONE;
			VARIABLES(gs->parties[i]->inst)[k].assignment=IDLE;
			VARIABLES(gs->parties[i]->inst)[k].count[0]=0;
			VARIABLES(gs->parties[i]->inst)[k].count[1]=0;
		}

		NEXT_UNASSIGNED_VARIABLE(gs->parties[i]->inst) = 1;
		
	  /* calcul de clauses */
	  nbr_cl=0;
	  
	  for(j=1;j<=CLAUSE_NUMBER(KBase);j++)
		if(in_space(KBase, &CLAUSES(KBase)[j],gs->parties[i]->noyau,gs->parties[i]->couv))
			{
				tab_cl[nbr_cl]=j;
				nbr_cl++;				
			}
			
	  CLAUSE_NUMBER(gs->parties[i]->inst)=nbr_cl;
	  
	  CLAUSES(gs->parties[i]->inst) = ( (type_clause) calloc (sizeof (struct struct_clause),
			    CLAUSE_NUMBER(gs->parties[i]->inst) + 1) );
	  assert ( CLAUSES(gs->parties[i]->inst) != NULL );

	  EXCLUDED(gs->parties[i]->inst) =  clset_new (CLAUSE_NUMBER(KBase));
	  NODE_TABLE(gs->parties[i]->inst) = nodetable_new();
	  
	  SATISFIED_CLAUSE_NUMBER(gs->parties[i]->inst) = 0;
	  CURRENT_CLAUSE_INDEX(gs->parties[i]->inst) = 0;
	  
	  e_card=0, id_card=0, ic_card=0;
	  for(j=1;j<=CLAUSE_NUMBER(gs->parties[i]->inst);j++)
	  {
		if(CLAUSES(KBase)[tab_cl[j-1]].from_set==SET_E)
			e_card++;
		else if(CLAUSES(KBase)[tab_cl[j-1]].from_set==SET_IC)
			ic_card++;
		else if(CLAUSES(KBase)[tab_cl[j-1]].from_set==SET_ID)
			id_card++;
			
		
		encode_clause_duplic (CLAUSES(KBase)[tab_cl[j-1]].literals, CLAUSES(KBase)[tab_cl[j-1]].size, gs->parties[i]->inst, CLAUSES(KBase)[tab_cl[j-1]].card,CLAUSES(KBase)[tab_cl[j-1]].from_set, CLAUSES(KBase)[tab_cl[j-1]].num_cl);
				
		}
		
	  if ( CURRENT_CLAUSE_INDEX(gs->parties[i]->inst) != CLAUSE_NUMBER(gs->parties[i]->inst) ) 
		{
			fprintf (stderr, "une confusion.\n"); 
			fflush (stderr);
			exit (0);
		}
  
	  /* initialisation quelques valeurs E Ic Id et leur ensemble de clauses */
	  
	  if (Opt_FullRem) 
		/* E, Ic Id sets used only in Full REM mode */
		{
			   
		E_CARD(gs->parties[i]->inst)=e_card;
		ID_CARD(gs->parties[i]->inst)=id_card;
		IC_CARD(gs->parties[i]->inst)=ic_card;

		E_SET(gs->parties[i]->inst) = NEW_ARRAY (int, E_CARD(gs->parties[i]->inst));
		IC_SET(gs->parties[i]->inst) = NEW_ARRAY (int, IC_CARD(gs->parties[i]->inst));
		ID_SET(gs->parties[i]->inst) = NEW_ARRAY (int, ID_CARD(gs->parties[i]->inst));
		
		/* 
		 * E_SET, ID_SET, IC_SET contient les indices de clauses dans l'instance d'origine et pas les 
		 * indices de clauses de la sous-instance. Donc il faut voir leur role pour bien les initialiser
		 */
		
		e_idx = id_idx = ic_idx = 0;
		while ( (e_idx + id_idx + ic_idx) < nbr_cl )
			{
			if (CLAUSES(KBase)[tab_cl[e_idx + id_idx + ic_idx]].from_set==SET_E)
			{
				E_SET(gs->parties[i]->inst)[e_idx] = e_idx + id_idx + ic_idx+1;
				e_idx++;
			}
			else if (CLAUSES(KBase)[tab_cl[e_idx + id_idx + ic_idx]].from_set==SET_ID) 
			{
				ID_SET(gs->parties[i]->inst)[id_idx] = e_idx + id_idx + ic_idx+1;
				id_idx++;
			}
			else if (CLAUSES(KBase)[tab_cl[e_idx + id_idx + ic_idx]].from_set==SET_IC)
			{
				IC_SET(gs->parties[i]->inst)[ic_idx] = e_idx + id_idx + ic_idx+1;
				ic_idx++;
			}
			}
		qsort (IC_SET(gs->parties[i]->inst), IC_CARD(gs->parties[i]->inst), sizeof (int), (comparison_fn_t)cmpint);
		qsort (ID_SET(gs->parties[i]->inst), ID_CARD(gs->parties[i]->inst), sizeof (int), (comparison_fn_t) cmpint);
		qsort (E_SET(gs->parties[i]->inst), E_CARD(gs->parties[i]->inst), sizeof (int), (comparison_fn_t) cmpint);
			
		}
		
		else
			/* all clauses in E ! */
			{
				E_CARD(gs->parties[i]->inst) = CLAUSE_NUMBER(gs->parties[i]->inst);
				E_SET(gs->parties[i]->inst) = NEW_ARRAY (int, E_CARD(gs->parties[i]->inst));
				for (j = 0 ; j < CLAUSE_NUMBER(gs->parties[i]->inst) ; j++)
				{
					E_SET(gs->parties[i]->inst)[j] = j+1;
					
				}
				
			}

	build_occurrences (gs->parties[i]->inst, 1);
	free(tab_cl);
	free(tab_var);	
}
EXCLUDEDP(gs) = clset_new (CLAUSE_NUMBER(KBase));
}

/* 
* la fonction Not_present permet de savoir
* si une variable (entier) est présnt dans
* un tableaux de variables (d'entiers)
*/

int Not_present(int* tab, int var, int taille)
{
	int i;
	for (i=0; i<taille; i++)
	{
		if(tab[i]==var)
			return 0;
	}
	return 1;
}

/* 
* la fonction construct_inst_clset permet de construire
* une instance Kbase à partir d'un ensemble de clauses
*/

type_instance construct_inst_clset(clset ics, type_instance KBase)
{
  int * tab_var, * tab_cl;
  int k, nbr_var=0, nbr_cl=0;
  int e_card, id_card, ic_card;
  int e_idx, id_idx, ic_idx;
  
  type_instance inst;
  
  inst= NEW (struct struct_instance);
  assert (inst != NULL);
  tab_var=(int *) malloc((VARIABLE_NUMBER(KBase))*sizeof(int));
  tab_cl=(int *) malloc((CLAUSE_NUMBER(KBase))*sizeof(int));
  
  for ( k = 1 ; k <= ics->max ; k++ )
	if ( get (k, ics->elems) )
	{
		/* calcul de variables */
	    int *lits = CLAUSES(KBase)[CL_Indice(k,KBase)].literals;
	    while (*lits)
	    {
			if(Not_present(tab_var, VAR_Indice(*lits, KBase), nbr_var))
			{
				tab_var[nbr_var]=VAR_Indice(*lits, KBase);			 
				nbr_var++;
			}
			lits++;
	    } 
		//printf ("%d \n", k);
		tab_cl[nbr_cl] = CL_Indice(k,KBase);
		nbr_cl++;
	}
  
  VARIABLE_NUMBER(inst) = nbr_var;
  VARIABLES(inst) = ( (type_variable) calloc (sizeof (struct struct_variable), VARIABLE_NUMBER(inst) + 1) );
  assert (VARIABLES(inst) != NULL );
  for ( k = 1 ; k <= VARIABLE_NUMBER(inst) ; k++ )  
  {
	  VARIABLES(inst)[k].from_parc=VARIABLES(KBase)[tab_var[k-1]].from_parc;
	  VARIABLES(inst)[k].num_var=VARIABLES(KBase)[tab_var[k-1]].num_var;
	  VARIABLES(inst)[k].value=NONE;
	  VARIABLES(inst)[k].assignment=IDLE;
	  VARIABLES(inst)[k].count[0]=0;
	  VARIABLES(inst)[k].count[1]=0;
  }
  
  NEXT_UNASSIGNED_VARIABLE(inst) = 1;
  
  CLAUSE_NUMBER(inst)=nbr_cl;
  
  CLAUSES(inst) = ( (type_clause) calloc (sizeof (struct struct_clause), CLAUSE_NUMBER(inst) + 1) );
  assert ( CLAUSES(inst) != NULL );
  
  EXCLUDED(inst) =  NULL;
  NODE_TABLE(inst) = NULL;
  
  SATISFIED_CLAUSE_NUMBER(inst) = 0;
  CURRENT_CLAUSE_INDEX(inst) = 0;
  
  e_card=0, id_card=0, ic_card=0;
  
  for(k=1;k<=CLAUSE_NUMBER(inst);k++)
  {
	  if(CLAUSES(KBase)[tab_cl[k-1]].from_set==SET_E)
			e_card++;
	  else if(CLAUSES(KBase)[tab_cl[k-1]].from_set==SET_IC)
			ic_card++;
	  else if(CLAUSES(KBase)[tab_cl[k-1]].from_set==SET_ID)
			id_card++;
	  
	  encode_clause_duplic (CLAUSES(KBase)[tab_cl[k-1]].literals, CLAUSES(KBase)[tab_cl[k-1]].size, inst, CLAUSES(KBase)[tab_cl[k-1]].card,CLAUSES(KBase)[tab_cl[k-1]].from_set, CLAUSES(KBase)[tab_cl[k-1]].num_cl);
  }
  
  if ( CURRENT_CLAUSE_INDEX(inst) != CLAUSE_NUMBER(inst) ) 
  {
	  fprintf (stderr, "une confusion.\n"); 
	  fflush (stderr);
	  exit (0);
  }
  
  /* initialisation quelques valeurs E Ic Id et leur ensemble de clauses */
  
  if (Opt_FullRem) 
  /* E, Ic Id sets used only in Full REM mode */
  {
	  E_CARD(inst)=e_card;
	  ID_CARD(inst)=id_card;
	  IC_CARD(inst)=ic_card;
	  
	  E_SET(inst) = NEW_ARRAY (int, E_CARD(inst));
	  IC_SET(inst) = NEW_ARRAY (int, IC_CARD(inst));
	  ID_SET(inst) = NEW_ARRAY (int, ID_CARD(inst));
	  
	  /* 
	   * E_SET, ID_SET, IC_SET contient les indices de clauses dans l'instance d'origine et pas les 
	   * indices de clauses de la sous-instance. Donc il faut voir leur role pour bien les initialiser
	  */
	  e_idx = id_idx = ic_idx = 0;
	  while ( (e_idx + id_idx + ic_idx) < nbr_cl )
	  {
		  if (CLAUSES(KBase)[tab_cl[e_idx + id_idx + ic_idx]].from_set==SET_E)
		  {
			  E_SET(inst)[e_idx] = e_idx + id_idx + ic_idx+1;
			  e_idx++;
		  }
		  else if (CLAUSES(KBase)[tab_cl[e_idx + id_idx + ic_idx]].from_set==SET_ID) 
		  {
			  ID_SET(inst)[id_idx] = e_idx + id_idx + ic_idx+1;
			  id_idx++;
		  }
		  else if (CLAUSES(KBase)[tab_cl[e_idx + id_idx + ic_idx]].from_set==SET_IC)
		  {
			  IC_SET(inst)[ic_idx] = e_idx + id_idx + ic_idx+1;
			  ic_idx++;
		  }
	  }
	  qsort (IC_SET(inst), IC_CARD(inst), sizeof (int), (comparison_fn_t)cmpint);
	  //qsort (ID_SET(inst), ID_CARD(inst), sizeof (int), (comparison_fn_t) cmpint);
	  qsort (E_SET(inst), E_CARD(inst), sizeof (int), (comparison_fn_t) cmpint);
  }
  else
  /* all clauses in E ! */
  {
	  E_CARD(inst) = CLAUSE_NUMBER(inst);
	  E_SET(inst) = NEW_ARRAY (int, E_CARD(inst));
	  for (k = 0 ; k < CLAUSE_NUMBER(inst) ; k++)
	  {
		  E_SET(inst)[k] = k+1;
	  }
  }
  build_occurrences (inst, 1);
  EXCLUDEDTM(inst)=0;
  free(tab_cl);
  free(tab_var);
  
  //affiche_instance(inst);
  
  return inst;	
}

/* 
 * la fonction in_space vérifie la possiblité qu'une clause soit 
 * dans un noyau et/ou sa couverture.
 */
int in_space(type_instance inst, type_clause clause, core noyau, covering couv)
{
	int i,j=0,trouv=0;
	for(i=0; i<clause->size; i++)
		{
			while(trouv==0 && j<noyau->nbr_parcel) 
				{
					if(VARIABLES(inst)[VAR_Indice(clause->literals[i], inst)].from_parc==noyau->noyau[j]->num_parc_v)
						trouv=1;
					j++;
					
				}
			j=0;
			while(trouv==0 && j<couv->nbr_parcel) 
				{
					if(VARIABLES(inst)[VAR_Indice(clause->literals[i], inst)].from_parc==couv->couv[j]->num_parc_v)
						trouv=1;
					j++;
					
				}
			j=0;
			if(!trouv)
				return 0;
			
			trouv=0;
	}
	return 1;
}

int in_noyau(type_instance inst, struct struct_clause clause, core noyau)
{
	int i,j=0,trouv=0;
	for(i=0; i<clause.size; i++)
		{
			while(trouv==0 && j<noyau->nbr_parcel) 
				{
					if(VARIABLES(inst)[VAR_Indice(clause.literals[i], inst)].from_parc==noyau->noyau[j]->num_parc_v)
						trouv=1;
					j++;
					
				}
			j=0;
			if(!trouv)
				return 0;
			
			trouv=0;
	}
	return 1;
}

/* 
 * la fonction affiche_instance affiche les infos concernant
 * une instance donnee. 
 */

void affiche_instance(type_instance inst)
{
	int i,j;
	
	printf ("\ncurrent_clause_index est: %d\n", CURRENT_CLAUSE_INDEX(inst));
	printf ("satisfied_clause_number est: %d\n", SATISFIED_CLAUSE_NUMBER(inst));
	printf ("assignment_number est: %lu\n", ASSIGNMENT_NUMBER(inst));
	printf ("monotone_number est: %lu\n", MONOTONE_NUMBER(inst));
	printf ("choice_number est: %lu\n", CHOICE_NUMBER(inst));
	printf ("subsum_number est: %lu\n", SUBSUM_NUMBER(inst));
	printf ("unit_number est: %lu\n", UNIT_NUMBER(inst));
	printf ("present_variable_number est: %lu\n", PRESENT_VARIABLE_NUMBER(inst));
	printf ("next_unassigned_variable est: %d\n", NEXT_UNASSIGNED_VARIABLE(inst));
	if (EXCLUDED(inst)!=NULL) printf ("max excluded set of clauses est: %d\n", EXCLUDED(inst)->max);
	printf ("-------------------------------------\n");
	printf ("-------------------------------------\n");
	printf ("nombre de clauses dans E est: %d\n", E_CARD(inst));
	printf ("ET leurs numeros sont: ");
	for(i=0;i<E_CARD(inst);i++)
	{
		printf ("%d ", E_SET(inst)[i]);
	}
	printf ("\n");
	printf ("-------------------------------------\n");
	printf ("nombre de clauses dans IC est: %d\n", IC_CARD(inst));
	printf ("ET leurs numeros sont: ");
	for(i=0;i<IC_CARD(inst);i++)
	{
		printf ("%d ", IC_SET(inst)[i]);
		
	}
	printf ("\n");
	printf ("-------------------------------------\n");
	printf ("nombre de clauses dans ID est: %d\n", ID_CARD(inst));
	printf ("ET leurs numeros sont: ");
	for(i=0;i<ID_CARD(inst);i++)
	{
		printf ("%d ", ID_SET(inst)[i]);
	}
	printf ("\n");
	printf ("-------------------------------------\n");
	printf ("-------------------------------------\n");
	printf ("nombre de varaibles est: %d\n", VARIABLE_NUMBER(inst));
	for(i=1;i<=VARIABLE_NUMBER(inst);i++)
	{
		printf ("la varaible %d est de la parcelle %d\n", VARIABLES(inst)[i].num_var,VARIABLES(inst)[i].from_parc);
	}
	
	printf ("-------------------------------------\n");
	printf ("-------------------------------------\n");
	printf ("nombre de clauses est: %d\n", CLAUSE_NUMBER(inst));
	for(i=1;i<=CLAUSE_NUMBER(inst);i++)
	{
		printf ("la clause: ");
		for(j=0;j<CLAUSES(inst)[i].size-1;j++)
		{
			printf ("%d v ",CLAUSES(inst)[i].literals[j]);
		}
		printf ("%d ",CLAUSES(inst)[i].literals[j]);
		printf ("contient %d literals\n", CLAUSES(inst)[i].size);
		
	}
	
}

/*
 *  la fonction exlude_noyau met apres chaque traitement d'une partie
 *  les clauses du noyau dans excluded_perm de la gstructre pour ne pas
 *  les considérer ulterieurement.
 * */

void exlude_noyau(Gstructure gs, int indice)
{
	 int i;
	 for(i=1;i<=CLAUSE_NUMBER(gs->parties[indice]->inst);i++)
		if(in_noyau(gs->parties[indice]->inst,CLAUSES(gs->parties[indice]->inst)[i],gs->parties[indice]->noyau))
			clset_add(EXCLUDEDP(gs),CLAUSES(gs->parties[indice]->inst)[i].num_cl);
	
	
}



