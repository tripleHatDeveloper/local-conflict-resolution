#ifndef MPL_H
#define MPL_H

#ifndef MAIN
#define DECLARE extern
#else
#define DECLARE
#endif

#include <stdio.h>
#include "Types-espace.h"
#include "nodetable.h" 



DECLARE char ** VarNames;


/* #define PI_NUMBER 500000 */
/*#define PI_NUMBER 50000 */
#define PI_NUMBER 25000

/*
 * Data structures
 * ---------------
 */

typedef enum { ZERO=0, ONE=1, NONE=2, NEVER=3 } enum_value;
typedef enum { IDLE, TODO, DONE } enum_assignment;


#define POSITIVE 1
#define NEGATIVE 0

  /*
   * Variables
   */

struct struct_variable;
typedef struct struct_variable* type_variable;

struct struct_variable 
{
  enum_value      value;
  enum_assignment assignment;
  int  count[2]       /* counts of occurrences */;
  int* occurrences[2] /* indices of clauses terminated with a 0) */;
  int  from_e;        /* 1: monoliteral in set E */ 
  int  from_parc;        /* numéro de parcelle */ 
  int  num_var;        /* numéro de variable */ 
};

  /*
   * Clauses
   */

#define SET_ID 0x01
#define SET_IC 0x02
#define SET_E  0x04

struct struct_clause;
typedef struct struct_clause* type_clause;

struct struct_clause 
{
  int* literals    /* + or - indices of variables (terminated with a 0) */;
  int  size        /* == number of literals in the clause */;
  int  neutral     /* == number of neutral literals in the clause */;
  int  falsified   /* == number of falsified literals in the clause */;
  int  satisfied   /* == number of satisfied literals in the clause */;
  int  from_set;   /* identifies the owning set */
  int  card;       /* cardinality constraint */
  int  num_cl; /* cardinality constraint */
};


#define CLAUSES_ALLOC_CHUNK 10

/**
 * an instance (a set of variables and clauses
 */

struct struct_instance
{
  type_clause clauses;
  type_variable variables;

  int variable_number;            /* number of variables */
  int clause_number;              /* number of clauses */
  int current_clause_index;       /* current clause index */
  int satisfied_clause_number;    /* number of satisfied clauses */
  unsigned long assignment_number;/* number of assignments */
  unsigned long unit_number;      /* number of unit clauses */
  unsigned long monotone_number;  /* number of monotone literals */
  unsigned long choice_number;    /* number of choice points */
  unsigned long subsum_number;    /* number of subsummed clauses */
  unsigned long present_variable_number;
  int next_unassigned_variable;
  int *e_set;
  int e_card;
  int *id_set;
  int id_card;
  int *ic_set;
  int ic_card;
  int excluded_card[2];
  clset excluded;                 /* excluded clauses */
  nodetable nt;
};

/**
 * accessor macros 
 */

/* the vector of clauses in the knowledge base */
#define CLAUSES(instance) ((instance)->clauses)

/* the variables present in the knowledge base */
#define VARIABLES(instance) ((instance)->variables)

/* the total number of variables */
#define VARIABLE_NUMBER(instance) ((instance)->variable_number)

/* the total number of clauses */
#define CLAUSE_NUMBER(instance) ((instance)->clause_number)

/* index of the current clause (for DP, MPL,...) */
#define CURRENT_CLAUSE_INDEX(instance) ((instance)->current_clause_index)

/* number of satisfied clauses (for DP, MPL, ...) */
#define SATISFIED_CLAUSE_NUMBER(instance) \
                 ((instance)->satisfied_clause_number)

/* number of assigned variables */
#define ASSIGNMENT_NUMBER(instance) ((instance)->assignment_number)

/* number of unit clauses */
#define UNIT_NUMBER(instance) ((instance)->unit_number)

/* number of monotone literals */
#define MONOTONE_NUMBER(instance) ((instance)->monotone_number)

/* number of choice points in DP algorithm */
#define CHOICE_NUMBER(instance) ((instance)->choice_number)

/* number of subsumed clauses (DP, etc...) */
#define SUBSUM_NUMBER(instance) ((instance)->subsum_number)

/* number of present variables (don't remember which alogo needs this... it it not used during
 * knowledge base loading) 
 */
#define PRESENT_VARIABLE_NUMBER(instance) \
                 ((instance)->present_variable_number)
/* index of the next unassigned variable (DP, etc...) */
#define NEXT_UNASSIGNED_VARIABLE(instance) \
                 ((instance)->next_unassigned_variable)

/* particular sets of clauses and various information used by REM algorithm */
#define E_SET(instance) ((instance)->e_set)
#define ID_SET(instance) ((instance)->id_set)
#define IC_SET(instance) ((instance)->ic_set)
#define E_CARD(instance) ((instance)->e_card)
#define ID_CARD(instance) ((instance)->id_card)
#define IC_CARD(instance) ((instance)->ic_card)
#define EXCLUDED(instance) ((instance)->excluded)
#define EXCLUDEDTM(instance) ((instance)->excluded_card[0])
#define EXCLUDEDN(instance) ((instance)->excluded_card[1])
#define EXCLUDEDP(gstructure) ((gstructure)->excluded_perm)
#define MAX(setcl) ((setcl)->max)
//#define EXCLUDEDP_MAX(gstructure) ((gstructure)->excluded_perm)
#define NODE_TABLE(instance) ((instance)->nt)

/*
 * Global variables
 * ----------------
 */

int VERBOSE;     /* verbosity flag */

/*
 * Useful macros
 * -------------
 */

/* returns the variable corresponding to a literal */
#define VARIABLE(lit)    ((lit)>0 ? (lit) : -(lit))
/* returns the sign of a literal (1 if positive, 0 if negative) */
#define SIGN(lit)        ((lit)>0 ? 1 : 0)
/* invert a literal */
#define OPPOSITE(lit)    (-(lit))
/* is lit a monotone literal ? */
#define IS_MONOTONE(lit) (VARIABLES[VARIABLE(lit)].count[NOT(SIGN(lit))]==0) 
/* invert a sign */
#define NOT(sig)         (1-(sig))

/**
 * used by MPL algorithm
 */
#define IN_EMPTY_CLAUSES_B(inst, ci) \
     intlist_member (EMPTY_CLAUSES_IN_B(inst), ci)
#define IN_EMPTY_CLAUSES_BA(inst, ci) \
     intlist_member (EMPTY_CLAUSES_IN_BA(inst), ci)
#define REMOVE_EMPTY_CLAUSE_B(inst, ci) \
     intlist_remove (EMPTY_CLAUSES_IN_B(inst), ci)
#define REMOVE_EMPTY_CLAUSE_BA(inst, ci) \
     intlist_remove (EMPTY_CLAUSES_IN_BA(inst), ci)
#define ADD_EMPTY_CLAUSE_B(inst, ci) \
     intlist_add (EMPTY_CLAUSES_IN_B(inst), ci) 
#define ADD_EMPTY_CLAUSE_BA(inst, ci) \
     intlist_add (EMPTY_CLAUSES_IN_BA(inst), ci) 

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

int in_E (unsigned cl, type_instance kb);

type_instance read_instance (FILE * input, FILE * member1, char * kbfile_names);
int dp (type_instance inst, type_stack stack, fi *f, int * bool1);
int get_num_parc(char * kbfile_names,int num_var);
void reset_base(type_instance kb);
void push_units (type_instance kb, type_stack st);
garray dp_push_literal (int lit, type_instance inst, type_stack stack);

int growable_at (garray a, int i);
int growable_size (garray a);
void growable_free (garray a);

void freefi (fi e);


int VAR_Indice(int lit, type_instance inst);

/*
 * la fonction initialise_insts initialise les sous-instances des parties
 * de la gstructure considerée.
 */

int Not_present(int* tab, int var, int taille);

type_instance construct_inst_clset(clset ics, type_instance kb);

void initialise_insts(Gstructure gs,type_instance KBase);

int in_space(type_instance inst, type_clause clause, core noyau, covering couv);

int in_noyau(type_instance inst, struct struct_clause clause, core noyau);

void affiche_instance(type_instance inst);

void exlude_noyau(Gstructure , int);

int CL_Indice(int num_cl, type_instance inst);

int dp_sans_fi (type_instance kb, type_stack stack);

#endif





