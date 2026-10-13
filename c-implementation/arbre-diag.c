#include <malloc.h>

#include "public-types.h"
#include "utils.h"
#include "clset.h"
#include "dp.h"
#include "arbre-diag.h"
#include "stack.h"
#include "nodetable.h"
#include "clset.h"
#include "utils.h"
#include "opts.h"

/*static nodetable NodeTable;*/

/* root node of the tree */
node_diag RootNode;

static int NBDP = 0 ; /* nb of dp calls per level */
static int NBREUSE = 0 ; /* nb of node reuse per level */
static int NBREDUCE = 0; /* number of label reductions per level */ 
static int NBBRANCH = 0 ; /* number of branching (node saving) */
//static int NBKERNELS = 0; /* number of kernels for each level */

static clset H (edge_diag e, type_instance kb);
static nodelist prune ( node_diag n, clset s, nodetable nt );

/* ======================================================================
 * stack allocation version of some functions
 * (if available)
 * ======================================================================
 */

/* stack allocation verion of H function.
 *  au lieu de la deuxieme ligne y avait
 * NEW_CLSET(h,*max*) mais ce ne voulait pas marcher!!!
 * donc c pour ca j'ivitais le passage par le macro en appelant
 * directement la fct clset_new()
 * h=clset_new( MAX(EXCLUDED(kb))); \
 * CLSET_NEW(h, MAX(EXCLUDED(kb))); \
 * */
#ifdef STACK_ALLOC
#define HTMP(h, e, kb) { \
  h=clset_new( MAX(EXCLUDED(kb)));\
  H_stack(h, e, kb); \
}
#else
#define HTMP(s, e, kb) s = H(e, kb)
#endif 
/* do not use this function. use above macro instead */
#ifdef STACK_ALLOC
static void H_stack (clset hn, edge_diag e, type_instance kb);
#endif



/* ######################################################################
 * ######################################################################
 *
 * Inconsistent subset generation
 *
 * ######################################################################
 * ######################################################################
 */

/**
 * pick a clause in each Fi, (Fi being a set of clauses)
 * generating an inconsistant subset of the base
 * which had not been already used
 */
static clset 
pick_fi (fi f, type_instance kb)
{
  //clset s = clset_new (CLAUSE_NUMBER(kb));
  clset s = clset_new (EXCLUDED(kb)->max);
  int choiceok = 0;
  int inext; /* increment next one */ 
  fi p ;
  
  assert ( f != NULL );
  
  while ( ! choiceok )
    {
      p = f;
      inext = 1;

      /* for each "empty leaf" of the DP tree */
      while ( p )
	{
	  /* take an element from each Fi
	   */
	  clset_add (s, growable_at (p->clauses, p->lastch));
	  if ( inext )
	    {
	      if ( p->lastch + 1 < growable_size (p->clauses) )
		{
		  p->lastch++;
		  inext = 0;
		}
	      else
		if ( p->next != NULL )
		  p->lastch = 0;
		else
		  if ( nodetable_lookup3 (NODE_TABLE(kb), s) != NULL )
		    /* the set had already been generated */
		    FORBIDEN2("impossible de generer un nouvel ensemble");
	    }
	  p = p->next;
	}
      if ( nodetable_lookup3 (NODE_TABLE(kb), s) == NULL )
	choiceok = 1;
      else
	{
	  clset_free (s);
	  s = clset_new (EXCLUDED(kb)->max);
	  //s = clset_new (CLAUSE_NUMBER(kb));
	}
    }
  return s;
}

/**
 * Tries to find a reusable label
 */
static clset
reuse_label (type_instance kb, edge_diag cur)
{
  nodetable_iterator it;
  node_diag n;
  int stop = 0;
  clset reuse = NULL;
  clset hn;
  HTMP(hn, cur, kb);
  NODETABLE_CSORT_ITERATOR_NEW (it, NODE_TABLE(kb));

  while ( (n = nodetable_csort_next_element(it)) != NULL && ! stop )
    {
      if ( n->t != END && n->t != CLOSED )
	{
	  clset inter;
	  CLSET_INTER(inter, hn, n->label); 
	  if ( clset_empty (inter) )
	    {
	      reuse = clset_clone (n->label);
	      stop = 1;
	    }
	  CLSET_FREE(inter);
	}
    }
  CLSET_FREE(hn);
  NODETABLE_ITERATOR_FREE(it);
  return reuse;
}


/**
 * try to shrink an inconsistent subset:
 * take a clause, see if opposite monoliterals corresponding to
 * each literal can be found. If yes, we got it !
 */
static void
shrink (clset ics, type_instance kb)
{
  clset_iterator it;
  unsigned clnum;
  clset smallset = NULL;
  int too_bad = 0;

  /* for each clause in the current set */
  //CLSET_ITERATOR_NEW (it, ics);
  it=clset_iterator_new(ics);
  smallset = clset_new (MAX(EXCLUDED(kb)));
  while ((clnum = clset_next_element (it)) != 0 && (clset_card (smallset) == 0))
    {
      /* if the clause contains at least 2 literals */
      if (CLAUSES(kb)[CL_Indice(clnum,kb)].size > 1) 
	{
	  int *lits = CLAUSES(kb)[CL_Indice(clnum,kb)].literals;

	  //smallset = clset_new (CLAUSE_NUMBER(kb));
	  //smallset = clset_new (MAX(EXCLUDED(kb)));
	  too_bad = 0;
	  while (*lits && ! too_bad)
	    {
	      int var = VAR_Indice(*lits,kb);
	      int sign = SIGN(*lits);
	      int *occs = VARIABLES(kb)[var].occurrences[NOT(sign)];
	      
	      too_bad = 1;
	      while (*occs)
		{
		  if (CLAUSES(kb)[*occs].size == 1 && clset_member(ics, CLAUSES(kb)[*occs].num_cl)) 
		    {
		      too_bad = 0;
		      clset_add (smallset, CLAUSES(kb)[*occs].num_cl);
			break;
		    }
		  occs++;
		}
	      lits++;
	    }
	  if (! too_bad)
	    clset_add (smallset, clnum);
	  else
	    {
	      clset_free (smallset);
	      smallset = clset_new (EXCLUDED(kb)->max);
	      //smallset = clset_new (CLAUSE_NUMBER(kb));
	    }
	}
    }
  //CLSET_ITERATOR_FREE (it);
  clset_iterator_free(it);
  if (smallset != NULL && clset_card (smallset) != 0)
    {
clset_inter_mute (ics, smallset);
    }
	else
	{
		shrink_rafine(ics, kb);
	}
	
//clset_display(smallset);
clset_free (smallset);
}

/**
 * recherche un conflit minimal
 * mais en enlevant juste des clauses de E
 */

void shrink_rafine(clset ics, type_instance kb)
{
  int i;
  clset_iterator it;
  unsigned clnum;
  clset smallset = clset_clone(ics);
  //fi f;
  type_instance KBase=NULL;
  type_stack st;
  
  it=clset_iterator_new(ics);
  while ((clnum = clset_next_element (it)) != 0)
    {
      /* if the clause is in the set E */
      if (CLAUSES(kb)[CL_Indice(clnum,kb)].from_set == SET_E) 
		{
			//reset_base (kb);
			//f = NULL;
			//printf("clnum =%d\n", CLAUSES(kb)[CL_Indice(clnum,kb)].num_cl);
			clset_remove(smallset, clnum);
			KBase=construct_inst_clset(smallset, kb);
			st = create_stack (KBase);
			reset_base (KBase);
			push_units (KBase, st);
			//affiche_instance(KBase);
			if (dp_sans_fi (KBase, st)>0)
			{
				printf("je vai pas la supprimer\n");
				smallset=clset_clone(ics);
			}
			else
			{
				printf("je vai la supprimer\n");
				clset_remove(ics, clnum);
			}
			//scanf("%d",&i);
			//freefi (f);
			stack_free (st);
		}		
	}
	clset_iterator_free(it);	
	
}

/**
 * generate an inconsistent subset of the knowledge base
 * kb : the knowledge base
 * cur : current incoming edge
 */
static clset
gen_conflict (type_instance kb, edge_diag cur)
{
  fi f = NULL; int * bool1;
  clset ics;

  if ( (ics = reuse_label (kb, cur)) == NULL )
    {
      /* call DP only if there is no reusable label
       */
      type_stack st = create_stack (kb);
      NBDP++;
      reset_base (kb);
      push_units (kb, st);
      if ( dp (kb, st, &f,bool1) > 0 )
	{
	  freefi (f);
	  return NULL;
	}
      stack_free (st);
      
      ics = pick_fi (f, kb);
      shrink (ics, kb);
      freefi (f);
    }
  else
    NBREUSE++;

  return ics;
}
  
static clset
gen_conflict_partie (partie part, edge_diag cur)
{
  int i;
  int bool1=0;
  fi f = NULL;
  clset ics;
  if ( (ics = reuse_label (part->inst, cur)) == NULL )
    {
	  /* call DP only if there is no reusable label
       */
      type_stack st = create_stack (part->inst);
      NBDP++;
      reset_base (part->inst);
      push_units (part->inst, st);
      if ( dp (part->inst, st, &f,&bool1) > 0 )
		{
			freefi (f);
			//scanf("%d",&i);
			return NULL;
		}
	  stack_free (st);      
      ics = pick_fi (f, part->inst);
      freefi (f);
	  //printf("conflit avant shrink:");
	  shrink (ics, part->inst);
	  if(clset_card(ics)>3)
		{
			shrink_rafine (ics, part->inst);
			//printf("conflit apres shrink:\n");			
			//printf("la valeur de bool = %d\n", bool1);
			//if(bool1)
			//{
				//type_instance inst = construct_inst_clset(ics, part->inst);
				//affiche_instance(inst);
				//scanf("%d",&i);
			//}
		}
	  //scanf("%d",&i);
    }
  else
    NBREUSE++;
  return ics;
} 

/* ######################################################################
 * ######################################################################
 *
 * Utility functions
 *
 * ######################################################################
 * ######################################################################
 */



/**
 * tests wether a node pointer belongs to a null terminated
 * array of node pointers
 */
static int 
memberp (node_diag p, node_diag *a)
{
  assert (a != NULL);

  while (*a)
    {
      if (p == *a)
	return 1;
      ++a;
    }
  return 0;
}
	  

/**
 * path from root to (including) edge e 
 */
static clset
H (edge_diag e, type_instance kb)
{
  //clset hn = clset_new (CLAUSE_NUMBER(kb));
  clset hn = clset_new (MAX(EXCLUDED(kb)));
  edge_diag cur = e;
  
  while (cur != NULL)
    {
      clset_add (hn, cur->label);
      assert (cur->ancestor != NULL);
      if (cur->ancestor->ancestor_edges == NULL)
	cur = NULL;
      else
        cur = cur->ancestor->ancestor_edges->edge;
    }
  return hn;
}

/**
 * Stack allocation version of H for temporary allocated sets
 * in order to prevent memory fragmentation.
 * DO NOT USE THIS FUNCTION DIRECTLY. USE THE H_STACK MACRO INSTEAD.
 */
#ifdef STACK_ALLOC
static void 
H_stack (clset hn, edge_diag e, type_instance kb)
{
  edge_diag cur = e;
  
  while (cur != NULL)
    {
	  clset_add (hn, cur->label);
      assert (cur->ancestor != NULL);
      if (cur->ancestor->ancestor_edges == NULL)
	cur = NULL;
      else
        cur = cur->ancestor->ancestor_edges->edge;
    }
}  
#endif

//static void 
//H_stack_display (edge_diag e)
//{
  //edge_diag cur = e;
  
  //while (cur != NULL)
    //{
      //printf("\nje suis dans STACK_ALLOC et le noeud diff de NULL\n");
	  
      //assert (cur->ancestor != NULL);
      //if (cur->ancestor->ancestor_edges == NULL)
	//cur = NULL;
      //else
        //cur = cur->ancestor->ancestor_edges->edge;
    //}
//}  

/**
 * array of pointers to nodes in path from root to (including) edge e 
 */
static node_diag *
Hpoint (edge_diag e, unsigned level)
{
  node_diag * hnp = NEW_ARRAY(node_diag, level+1);
  int i = 0;
  edge_diag cur = e;
  
  while (cur != NULL)
    {
      hnp[i++] = cur->ancestor;
      assert (cur->ancestor != NULL);
      if (cur->ancestor->ancestor_edges == NULL)
	cur = NULL;
      else
        cur = cur->ancestor->ancestor_edges->edge;
    }
  hnp[i] = NULL;
  return hnp;
}




/**
 * add an edge to the ancestor edge list of node n and connect the edge
 * to the node
 */
static void 
add_ancestor_edge (node_diag n, edge_diag e)
{
  edgelist el;

  if (e == NULL)
    /* root node has no ancestor */
    return;

  el = NEW (struct st_edgelist);

  assert (n != NULL);
  assert (e != NULL);
  assert (el != NULL);

  el->edge = e;
  el->next = n->ancestor_edges;
  n->ancestor_edges = el;
  e->son = n;
}

/**
 * add a son edge to a node using the given label
 */  
static edge_diag 
add_son_edge (node_diag n, unsigned label)
{
  edgelist el = NEW (struct st_edgelist);
  edge_diag e = NEW (struct st_edge_diag);

  assert (e != NULL);
  assert (el != NULL);
  assert (n != NULL);
      
  e->label = label;
  e->ancestor = n;
  e->son = NULL; /* pending edge */
  el->edge = e;
  el->next = n->son_edges;
  n->son_edges = el;

  return e;
}
 
 

enum node_creat { LABEL=0, NOLABEL };

/**
 * creates a new node.
 * son edges are not created
 *
 * cur   : the current incoming edge
 * kb    : the knowledge base
 * nolab : generate a label or not
 */
static node_diag
create_node (edge_diag cur, type_instance kb, 
	     enum node_creat  nolab, unsigned level)
{
  node_diag n;
  clset label = NULL;

  if (! nolab)
    {
      HTMP(EXCLUDED(kb), cur, kb);
      label = gen_conflict (kb, cur);
      CLSET_FREE(EXCLUDED(kb));
    }

  n = NEW (struct st_node_diag);
  assert (n != NULL);

  n->label = label;
#ifndef NTRACE
  printf ("new node : ");
  clset_display (label);
#endif
  n->ancestor_edges = NULL;
  add_ancestor_edge (n, cur);
  n->son_edges = NULL;
  n->level = level;
  if (nolab)
    n->t = CLOSED;
  else
    n->t = ((label == NULL) ? END : NORMAL);

  nodetable_add (NODE_TABLE(kb), n);

  return n;
}


/*
 * la fct nbr_excludedn calcule le nombre de clause exclues des
 * noyaux precedents 
 * */
int
nbr_excludedn(type_instance inst, clset setcl)
{
	int i,j,cmpt=0;
	for(i=1;i<=CLAUSE_NUMBER(inst);i++)
	{
		for(j=1;j<=setcl->max;j++)
		{
			if ( get (j, setcl->elems) )
			{
				if(j==CLAUSES(inst)[i].num_cl)
				{
					cmpt++;
					break;
				}
				
			}
			
		}
		
	}
	
	return cmpt;
	
}


static node_diag
create_node_partie (edge_diag cur, partie part, 
	     enum node_creat  nolab, clset setcl, unsigned level)
{
  int i;
  node_diag n;
  clset label1 = NULL,tempclset, label2=NULL;

  if (! nolab)
    {
	  //clset_display(setcl);
	  HTMP(tempclset, cur, part->inst);
	  //clset_display(tempclset);
	  //scanf("%d",&i);
	  EXCLUDEDTM(part->inst)=clset_card(tempclset)+nbr_excludedn(part->inst,setcl);	
	  EXCLUDEDN(part->inst)=clset_card(setcl);
	  EXCLUDED(part->inst)=clset_union (tempclset, setcl);
	  label1 = gen_conflict_partie (part, cur);
	  //if(label1)
	  //{
		  //for(i=0;i<3;i++)
			//{
				//clset_display(label1);
				//label2 = gen_conflict_partie (part, cur);
				//clset_display(label2);
				//if(label1->card>label2->card)
				//{
					//CLSET_FREE(label1);
					//label1=clset_clone(label2);
					//printf("j'ai trouvé mieux!!\n");
				//}
				//CLSET_FREE(label2);
			//}
		//scanf("%d",&i);		
		//}
	  CLSET_FREE(EXCLUDED(part->inst));
	  CLSET_FREE(tempclset);
	  
    }
      //printf("voila les clauses excluses du conflit suivant:\n");
	  //clset_display(EXCLUDED(part->inst));
	  
  n = NEW (struct st_node_diag);
  assert (n != NULL);

  n->label = label1;
#ifndef NTRACE
  printf ("new node : ");
  clset_display (label1);
#endif
  n->ancestor_edges = NULL;
  add_ancestor_edge (n, cur);
  n->son_edges = NULL;
  n->level = level;
  if (nolab)
    n->t = CLOSED;
  else
    n->t = ((label1 == NULL) ? END : NORMAL);

  nodetable_add (NODE_TABLE(part->inst), n);

  return n;
}

/**
 * free resources allocated by a node
 */
static void 
node_free (node_diag n)
{
  assert (n != NULL);

  if (n->label != NULL)
    clset_free (n->label);
  free (n);
}

/* ======================================================================= 
 * 
 * Edge and node lists 
 *
 * =======================================================================
 */
  
/**
 * add the e edge to the el edgelist
 *
 * e : the edge to add
 * el : the edgelist to add to
 * returns the new head of the edgelist
 */
static edgelist 
add_edge (edge_diag e, edgelist el)
{
  edgelist n = NEW (struct st_edgelist);

  assert (n != NULL);
  assert (e != NULL);

  n->edge = e;
  n->next = el;

  return n;
}


/**
 * adds a node to a nodelist
 *
 * n : the node to add
 * nl : the nodelist to add to
 * returns a pointer to the new nodelist
 */
static nodelist
add_node (node_diag n, nodelist nl)
{
  nodelist new = NEW (struct st_nodelist);

  assert (new != NULL);
  assert (n != NULL);

  new->node = n;
  new->next = nl;

  return new;
}

/**
 * Displays a node
 *
 * n : node to be displayed
 */
void node_display (node_diag n)
{
  printf ("node : ");
  clset_display (n->label);
  printf ("\tlevel : %d\n", n->level); 
  if (n->ancestor_edges)
    {
      edgelist l = n->ancestor_edges;
      printf ("\tparents : ");
      while (l)
	{
	  printf("%u ", l->edge->label);
	  l = l->next;
	}
      printf ("\n");
    }
  else
    {
      printf ("\tparents : sans (root ou a effacer)\n");
    }
  if (n->son_edges)
    {
      edgelist l = n->son_edges;
      printf ("\tfils : ");
      while (l)
	{
	  printf("%u ", l->edge->label);
	  l = l->next;
	}
      printf ("\n");
    }
  else
    {
      printf ("\tfils : sans\n");
    }
}


/**
 * Displays a node list
 *
 * l : The node list to be displayed
 */
void
nodelist_display (nodelist l)
{
  while ( l )
    {
      node_display (l->node);
      l = l->next;
    }
}

/**
 * remove from topurge all nodes present in filter
 * (doesn't remove node_diag objects themselve)
 */
static nodelist 
purge_nodelist (nodelist topurge, nodelist filter)
{
  nodelist tp;
  nodelist tprev = NULL;
  nodelist pdel = NULL;
  nodelist f = filter;
  int stop;

  /* for each filter element 
   */
  while (f)
    {
      /* for each element in the list to be purged
       */
      tp = topurge;
      stop = 0;
      tprev = NULL;
      while (tp && ! stop)
	if (tp->node == f->node)
	  {
	    /* remove current element */
	    pdel = tp;
	    if (tprev)
	      tprev->next = tp->next;
	    else /* head deleting */
	      topurge = tp->next;
	    tp = tp->next;
	    free (pdel);
	    stop = 1;
	  }
	else
	  {
	    tprev = tp;
	    tp = tp->next;
	  }
      f = f->next;
    }
  return topurge;
}

/**
 * tests wether a node n belongs to a nodelist l
 * (the exact node, not a clone)
 *
 * n : the node
 * l : the node list
 */
static int
nodelist_member (node_diag n, nodelist l)
{
  while (l) 
    {
      if (n == l->node)
	return 1;
      l = l->next;
    }
  return 0;
}
	
/**
 * concatenates two nodelists (append l2 to l1 without creating
 * a new list).
 *
 * l1 : first list
 * l2 : second list
 * returns : the l1 list with l2 concatenated to it. If l1 is NULL,
 *           the function returns l2.
 */
static nodelist 
nodelist_concat (nodelist l1, nodelist l2)
{
  if (l1 == NULL)
    return l2;
  l1->next = nodelist_concat (l1->next, l2);
  return l1;
}
      
/**
 * frees a nodelist withmout freeing the nodes themselves
 */
static void
free_nodelist (nodelist l)
{
  register nodelist p1, p2;

  for (p1 = l ; p1 ; p1 = p2)
    {
      p2 = p1->next;
      free (p1);
    }
}
  

/**
 * remove nodes in nodetable and free them
 */
static void 
free_nodes (nodelist l, nodetable nt)
{
  nodelist p = l;

  while (p)
    {
      nodetable_remove (nt, p->node);
      node_free (p->node);
      p = p->next;
    }
}

/* ######################################################################
 * ######################################################################
 *
 * Tree pruning and optimizations
 *
 * ######################################################################
 * ######################################################################
 */

/**
 * look for nodes which labels are proper supersets of the label
 * of node n. For each of these nodes, do some pruning.
 *
 * n : the reference node
 * kb : the knowledge base
 * returns : a list of nodes which have to be deleted (the caller
 * needs to do some tricks on these nodes before deletion
 */ 
static nodelist
pruning (node_diag n, type_instance kb)
{
  int i;
  nodetable_iterator it;
  node_diag cur, tmp;
  nodelist del = NULL;
  NODETABLE_ITERATOR_NEW (it, NODE_TABLE(kb));  

  assert (n != NULL);
  assert (n->label != NULL);

  /* for each node in the nodetable
   */
  cur = nodetable_next_element (it);
  while (cur != NULL)
    {
      if (cur->t != END && cur->t != CLOSED)
	{
	  assert (cur->label != NULL);
	  /* if the current node is a proper superset of n and is not
	   * in the del list, then prune from this node
	   */
	  if (cur != n &&
	      clset_included (n->label, cur->label) &&
	      clset_compare (n->label, cur->label) &&
	      ! nodelist_member (cur, del)) 
	    {
	      /* !!!! cur is rehashed during pruning (prune()), so we have to 
	       * retrieve next element in hashtable before (otherwise the
	       * iterator will be trashed)
	       */
		   
	      NBREDUCE++;
	      tmp = nodetable_next_element (it);	
	      del = nodelist_concat (del, prune (cur, n->label, NODE_TABLE(kb)));
	      cur = tmp;
	    }
	  else
	    cur = nodetable_next_element (it);
	}
      else
	cur = nodetable_next_element (it);
    }
  NODETABLE_ITERATOR_FREE (it);
  return del;
}
				    



/**
 * tests wether there exist a node n' such that path from
 * e to root = H(n') 
 * cln is the built nodes list for current level.
 */
static node_diag
reuse (edge_diag e, nodelist cln, type_instance kb)
{
  node_diag nprime = NULL;
  clset hn;
  int found = 0;

  assert (e != NULL);

  /* build H(n), then add label of e to it
   */
  HTMP(hn, e, kb);

  found = 0;
  while (cln && ! found)
    {
      clset hnprime; 
      nprime = cln->node;
      /* always trust first edge */
      assert ( nprime->ancestor_edges != NULL );
      HTMP(hnprime, nprime->ancestor_edges->edge, kb);
      found = ! clset_compare (hn, hnprime);
      CLSET_FREE(hnprime);
   
      cln = cln->next;
    }
  CLSET_FREE (hn);
  return (found ? nprime : NULL);
} 
   
  
/**
 * connect the son edge to a node, eventually specifying a new
 * node type
 */
static void
connect_edge_son (edge_diag e, node_diag n, enum type_noeud t)
{
  e->son = n;
  n->ancestor_edges = add_edge (e, n->ancestor_edges);
  if (t != NONE)
    n->t = t;
}



/**
 * test if there exists n' such that H(n') subsetof H(n) and n' is
 * t type node
 * e is the incoming edge to n
 * k is the knowledge base
 * level is the level of node n
 */
static int
exist_sub_path (edge_diag e, type_instance kb, 
		unsigned level, 
		enum type_noeud t)  
{
  clset hn;           /* H(n) */
  node_diag *hnpoint; /* array of pointers to nodes in H(n) */
  nodetable_iterator it;
  node_diag cur;

  NODETABLE_ITERATOR_NEW (it, NODE_TABLE(kb)); 
  assert (e != NULL);

  HTMP(hn, e, kb);
  hnpoint = Hpoint(e, level);
  
  /* Browse all nodes with a level < level
   */
  while ( (cur = nodetable_next_element(it)) != NULL )
    {
      if ((! memberp(cur, hnpoint)) && 
	  cur->level < level && 
	  cur->t == t)
	{
	  /* for each ancestor of this node, test inclusion */
	  edgelist el = cur->ancestor_edges;
	  while (el)
	    {
	      clset hnp;
	      HTMP(hnp, el->edge, kb);
	      if (clset_included (hnp, hn))
		{
		  CLSET_FREE(hn);
		  CLSET_FREE(hnp);
		  free (hnpoint);
		  NODETABLE_ITERATOR_FREE(it);
		  return 1;
		}
	      CLSET_FREE(hnp);
	      el = el->next;
	    }		      
	}
    }
  CLSET_FREE(hn);
  free (hnpoint);
  NODETABLE_ITERATOR_FREE(it);
  return 0;
}

/**
 * free a subtree starting et edge e if possible : a subtree can't be
 * deallocated further on when a node has more than 1 ancestor
 * (remember we deal with a DAG)
 */

static nodelist 
depth_free (edge_diag e, nodetable nt, nodelist deleted)
{
  node_diag n;
  edgelist l;

  /* stop condition
   */
  if (e)
    {
      n = e->son;
      /* if n has no more than one ancestor 
       */
      assert (n->ancestor_edges != NULL);
      if (n->ancestor_edges->next == NULL)
	{
	  edgelist tofree = NULL, prev = NULL;
	  
	  /* free each son edge, and try to free as deep as
	   * possible
	   */
	  l = n->son_edges;
	  while (l)
	    {
	      deleted = depth_free (l->edge, nt, deleted);
	      tofree = l;
	      if (prev)
		prev->next = l->next;
	      else
		n->son_edges = l->next;
	      l = l->next;
	      free (tofree); /* tofree->edge already freed by recursive call */
	    }
	  /* free the ancestor edge list (only one element)
	   * but not the edge (e itself !) 
	   */
	  free (n->ancestor_edges);
	  n->ancestor_edges = NULL; /* cleaner operation ! */
	  
	  /* mark node n as "to be deleted" */
	  deleted = add_node (n, deleted);
	}
      else
	{
	  /* node n has more than one ancestor
	   * remove e from its ancestors 
	   */
	  edgelist prev = NULL;
	  l = n->ancestor_edges;
	  while (l && (l->edge != e))
	    {
	      prev = l;
	      l = l->next;
	    }

	  assert (l != NULL); /* we must find e ! */
	  if (prev)
	    prev->next = l->next;
	  else
	    n->ancestor_edges = l->next;
	  free (l);
	}
      /* free the edge itself
       */
      free (e);
    } 
  return deleted;
}


/**
 * Prune branches under n which are not members of s.
 * Stop when reaching a node which have more than one ancestor
 *
 * WARNING !! This function can rehash nodes !
 *
 * n the node where to start pruning
 * s the set of branches not to be pruned
 * nt the nodetable
 */
static nodelist
prune ( node_diag n, clset s, nodetable nt )
{
  register edgelist cur;
  edgelist prev = NULL, tofree = NULL;
  nodelist deleted = NULL;
  clset oldlab;
  clset diff;
  CLSET_DIFF(diff, n->label, s);

  assert (diff != NULL);
  assert (n != NULL);
  assert (n->label != NULL); 

  /* for each edge starting at n and belonging to diff, 
   * free the subtrees when possible
   */
  cur = n->son_edges;
  while (cur)
    {
      if (clset_member (diff, cur->edge->label)) 
	{
	  deleted = depth_free (cur->edge, nt, deleted);
	  tofree = cur;
	  if (! prev)
	    n->son_edges = cur->next;
	  else
	    prev->next = cur->next;
	}
      if (tofree)
	{
	  cur = cur->next;
	  free (tofree);
	  tofree = NULL;
	  /* current node freed, so prev doesn't move */
	}
      else
	{
	  prev = cur;
	  cur = cur->next;
	}
    }

  /* clean node n label (beware ! this changes the hash code for
   * the node, so we have to remove and reinsert it in the nodetable
   */
  nodetable_remove (nt, n);
  oldlab = n->label;
  n->label = clset_diff (n->label, diff);
  nodetable_add (nt, n);
  CLSET_FREE(diff);

  /* if there is an iterator acting on the old label, 
   * update it to reflect label change
   */
  clset_update_iterators(oldlab, n->label);
  clset_free (oldlab);

  return deleted;
}


  


/* ######################################################################
 * ######################################################################
 *
 * Tree building routine
 *
 * ######################################################################
 * ######################################################################
 */

/**
 * build the HS-tree
 * kb is the knowledge base
 * nt is a hashtable off all presently existing nodes
 * returns a list of end nodes;
 */

nodelist
build_hstree (type_instance kb)
{
  node_diag n;
  nodelist cln;       /* current level already built nodes */
  nodelist pln;       /* previous level already built nodes */
  nodelist plb;        /* previous level browser */
  unsigned level = 0;
  nodelist endnodes = NULL;
  int stopmin = 0,i;
  int NBKERNELS = 0;

  vmsg ("==== level 0\n");
  /* build the root node and its pending son edges 
   */
  n = create_node (NULL, kb, LABEL, level);
  if ( n->t == END )
    {
	  return NULL;
    }

  RootNode = n;

  pln = add_node (n, NULL);

  /* NEW HEURISTIC : kernels composed from E set elements cannot have
   * more elements that E itself.
   */
  while (pln && level < E_CARD (kb) && !stopmin)
    {
      level++;
      vmsg ("==== starting level %d: ", level); fflush(stdout);
      cln = NULL; /* begining a new level */
      plb = pln;
      NBDP = 0 ; /* nb of dp calls per level */
      NBREUSE = 0 ; /* nb of node reuse per level */
      NBREDUCE = 0; /* number of label reductions per level */ 
      NBBRANCH = 0 ; /* number of branching (node saving) */
      /*
       * for each node at preceding level
       */
      while (plb)
	{
	  int plbAdvanced = 0;
	  if (plb->node->t != END && plb->node->t != CLOSED)
	    {
	      clset_iterator it ;
	      unsigned el ; /* current edge label */
	      CLSET_ITERATOR_NEW (it, plb->node->label);

	      /* for each clause in the label
	       * beware ! iterator can be confused by deleted clauses
	       * in label. we have to track this
	       * also, stop iterating over label clauses if node
	       * has been deleted
	       */
	      while (! plbAdvanced && (el = clset_next_element(it)) != 0)
		{
		  /* we compute kernels with elements in E 
		   * (except if option -a is specified)
		   */
		  
		  if (Opt_All || in_E (el, kb))
		    {
			  node_diag nprime;
		      edge_diag sigma = add_son_edge (plb->node, el); /* current incoming edge */
#ifndef NTRACE
		      printf("current edge : %u\n", sigma->label);
#endif
		      /* if there exist a node n' such that 
		       * H(current_node) = H(n') U {sigma}
		       * then connect sigma to n'  
		       * [NO NEW NODE CREATION]
		       * (rule GSW1)
		       */
		      if ((nprime = reuse (sigma, cln, kb)) != NULL)
			{
			  connect_edge_son (sigma, nprime, REUSED);
			  NBBRANCH++;
			}
		      
		      /* if there exists n' such that H(n') subsetof 
		       * H(current node), 
		       * and n' is an END node, create the current node and 
		       * close it !
		       * (Reiter 2) (GSW2)
		       * [CREATION OF A CLOSED NODE]
		       */
		      else if (exist_sub_path (sigma, kb, level, END))
			{
			  n = create_node (sigma, kb, NOLABEL, level);
			  assert ( n != NULL );
			  /* do not add to list of current nodes */
			}
	      
		      /* build a new node and its edges 
		       */
		      else  
			{
			  nodelist del;
			  n = create_node(sigma, kb, LABEL, level);
			  assert ( n != NULL );
			  
			  if (n->t != END)
			    { 
			      cln = add_node (n, cln);	      
			      if ((del = pruning (n, kb)) != NULL)
				{
				  cln = purge_nodelist (cln, del);
				  /* current node has been deleted !
				   */
				  if (nodelist_member (plb->node, del))
				    {
				      while (plb && 
					     nodelist_member(plb->node, del))
					plb = plb->next;
				      plbAdvanced = 1;
				    }
				  pln = purge_nodelist (pln, del);
				  free_nodes (del, NODE_TABLE(kb));
				  free_nodelist (del);
				}
			    }
			  else 
			    {
			      endnodes = add_node (n, endnodes);
			      vmsg ("\n#### New Kernel");
			      if (Opt_Verb)
				clset_display (H (n->ancestor_edges->edge, kb));
			      NBKERNELS++;
			    }
			}
		    }
		}
	      CLSET_ITERATOR_FREE (it);
		  
	    }
	  if ( ! plbAdvanced )
	    plb = plb->next;
	}
     
	  free_nodelist (pln);
      pln = cln;

      vmsg ("\n==== finishing level %d: DP calls:%d / Label reuse:%d / Branching:%d / Label reduce:%d\n",
	    level, NBDP, NBREUSE, NBBRANCH, NBREDUCE);
      if (Opt_Min && NBKERNELS != 0)
		stopmin = 1;
    }
  if ( pln != NULL )
    free_nodelist (pln);
  return endnodes;
}

/**
 * build the HS-tree of the considered gstructure gs
 * returns a collection of lists of end nodes;
 */
nodelist *
build_hstree_space(Gstructure gs)
{
	int i,j;
	nodelist * endnodes=( (nodelist *) calloc (sizeof (nodelist), gs->nbr_parties +1) );
	assert (endnodes != NULL );
	
	for(i=0;i<gs->nbr_parties;i++)
	{
		printf ("\nPartie %d traitement...\n",i+1);
		printf ("...\n");
		//scanf("%d",&j);
		if (j==1)
			continue;
		endnodes[i] = build_hstree_partie (gs->parties[i],EXCLUDEDP(gs));
		exlude_noyau(gs, i);
  
		if ( endnodes[i] != NULL )
			print_kernels(endnodes[i], VarNames, gs->parties[i]->inst);
		else
			printf ("\n>> Le sous-ensemble de clauses %d est coherent\n\n",i+1);
			
		
	}
	endnodes[i] = NULL;	 
	return endnodes;
	
} 

/**
 * build the HS-tree of the considered partie part
 * setcl represents the excluded clauses of cores 
 * from the parts which have been already treated
 * returns a list of end nodes;
 */

nodelist
build_hstree_partie (partie part, clset setcl)
{
  type_instance kb=part->inst;
  node_diag n;
  nodelist cln;       /* current level already built nodes */
  nodelist pln;       /* previous level already built nodes */
  nodelist plb;        /* previous level browser */
  unsigned level = 0;
  nodelist endnodes = NULL;
  int stopmin = 0,i;
  int NBKERNELS=0;

  vmsg ("==== level 0\n");
  /* build the root node and its pending son edges 
   */
  n = create_node_partie (NULL, part, LABEL, setcl, level);
  printf("root label: n->label=");
  clset_display(n->label);
  //scanf("%d",&i);
	
  if ( n->t == END )
    {
	  return NULL;
    }
  
  RootNode = n;

  pln = add_node (n, NULL);

  /* NEW HEURISTIC : kernels composed from E set elements cannot have
   * more elements that E itself.
   */
  while (pln && level < E_CARD (kb) && !stopmin)
    {
      level++;
      vmsg ("==== starting level %d: ", level); fflush(stdout);
      cln = NULL; /* begining a new level */
      plb = pln;
      NBDP = 0 ; /* nb of dp calls per level */
      NBREUSE = 0 ; /* nb of node reuse per level */
      NBREDUCE = 0; /* number of label reductions per level */ 
      NBBRANCH = 0 ; /* number of branching (node saving) */
      /*
       * for each node at preceding level
       */
      while (plb)
	{
	  int plbAdvanced = 0;
	  if (plb->node->t != END && plb->node->t != CLOSED)
	    {
	      clset_iterator it ;
	      unsigned el ; /* current edge label */
	      //CLSET_ITERATOR_NEW (it, plb->node->label);
		  it=clset_iterator_new(plb->node->label);

	      /* for each clause in the label
	       * beware ! iterator can be confused by deleted clauses
	       * in label. we have to track this
	       * also, stop iterating over label clauses if node
	       * has been deleted
	       */
	      while (! plbAdvanced && (el = clset_next_element(it)) != 0)
		{
		  /* we compute kernels with elements in E 
		   * (except if option -a is specified)
		   */
		  
		  //if (Opt_All || in_E(CL_Indice(el,kb), kb))
		  if (in_E(CL_Indice(el,kb), kb))
		    {
			  node_diag nprime;
		      edge_diag sigma = add_son_edge (plb->node, el); /* current incoming edge */
#ifndef NTRACE
		      printf("current edge : %u\n", sigma->label);
#endif
		      /* if there exist a node n' such that 
		       * H(current_node) = H(n') U {sigma}
		       * then connect sigma to n'  
		       * [NO NEW NODE CREATION]
		       * (rule GSW1)
		       */ 
		      if ((nprime = reuse (sigma, cln, kb)) != NULL)
			{
			  connect_edge_son (sigma, nprime, REUSED);
			  NBBRANCH++;
			}
		      
		      /* if there exists n' such that H(n') subsetof 
		       * H(current node), 
		       * and n' is an END node, create the current node and 
		       * close it !
		       * (Reiter 2) (GSW2)
		       * [CREATION OF A CLOSED NODE]
		       */
		      else if (exist_sub_path (sigma, kb, level, END))
			{
			  
			  n = create_node_partie (sigma, part, NOLABEL, setcl, level);
			  
			  assert ( n != NULL );
				printf("conflit fils genere pour le edge = %u par GSW2\n",el);
			  //clset_display(n->label);
			  //scanf("%d",&i);
			  /* do not add to list of current nodes */
			}
	      
		      /* build a new node and its edges 
		       */
		      else  
			{
			  nodelist del;
			  n = create_node_partie(sigma, part, LABEL, setcl, level);
			  
			  assert ( n != NULL );
				printf("conflit fils genere pour le edge = %u\n",el);
			  //clset_display(n->label);
			  //print_labels (n->label, VarNames, part->inst);
			  //scanf("%d",&i);
			  
			  if (n->t != END)
			    { 			      
				  cln = add_node (n, cln);     
			      if ((del = pruning (n, kb)) != NULL)
				{
				  cln = purge_nodelist (cln, del);
				  /* current node has been deleted !
				   */
				  if (nodelist_member (plb->node, del))
				    {
				      while (plb && 
					     nodelist_member(plb->node, del))
					plb = plb->next;
				      plbAdvanced = 1;
				    }
				  pln = purge_nodelist (pln, del);
				  free_nodes (del, NODE_TABLE(kb));
				  free_nodelist (del);
				}
			    }
			  else 
			    {
			      endnodes = add_node (n, endnodes);
			      vmsg ("\n#### New Kernel");
			      if (Opt_Verb)
				  clset_display (H (n->ancestor_edges->edge, kb));
			      NBKERNELS++;
			    }
			}
		    }
		}
	      //CLSET_ITERATOR_FREE (it);
		  clset_iterator_free(it);
	    }
	  if ( ! plbAdvanced )
	    plb = plb->next;
	}
    
	  free_nodelist (pln);
      pln = cln;

      vmsg ("\n==== finishing level %d: DP calls:%d / Label reuse:%d / Branching:%d / Label reduce:%d\n",
	    level, NBDP, NBREUSE, NBBRANCH, NBREDUCE);
      if (Opt_Min && NBKERNELS != 0)
	  {
		stopmin = 1;
	}
    }
  if ( pln != NULL )
    free_nodelist (pln);
  return endnodes;
}	

/**
 * Prints a clause.
 */
static void
print_clause (unsigned num, char **names, type_instance kb)
{
  int *lits = CLAUSES(kb)[CL_Indice(num,kb)].literals;
  int n = CLAUSES(kb)[CL_Indice(num,kb)].size;

  printf ("  ( ");
  while (*lits)
    {
      if (names == NULL)
	printf ("%d ", *lits);
      else
	{
	  if (*lits < 0)
	    printf ("-%s ", names[-*lits]);
	  else
	    printf ("%s ", names[*lits]);
	}
      if (n != 1)
	printf ("| ");
      n--;
      lits++;
    }
  printf (" )\n");
}

/**
 * print kernels
 */
void
print_kernels (nodelist nl, char **names, type_instance kb)
{
  nodelist p = nl;
	printf ("\nnos kernels sont les suivants:\n");
  while (p)
    {
      clset krnl = H (p->node->ancestor_edges->edge, kb);
      clset_iterator it = clset_iterator_new (krnl);
      unsigned cl;

      printf ("{");
      while ((cl = clset_next_element (it)) != 0)
	{
	  if (Opt_FullRem)
		printf("%d ",cl);
	    //print_clause (cl, names, kb);
	  else
	    printf("%d ",cl);
		//print_clause (cl, NULL, kb);
	}
      printf ("}\n");
      clset_iterator_free (it);
      p = p->next;
    }
}

/**
 * print labels
 */
void
print_labels (clset lab, char **names, type_instance kb)
{
  if(lab)
  {
      clset_iterator it = clset_iterator_new (lab);
      unsigned cl;

      printf ("{");
      while ((cl = clset_next_element (it)) != 0)
		{
			if (Opt_FullRem)
				//printf("%d ",cl);
				print_clause (cl, names, kb);
			else
				print_clause (cl, names, kb);
				//printf("%d ",cl);
				//print_clause (cl, NULL, kb);
		}
      printf ("}\n");
      clset_iterator_free (it);
   }
   
}

/**
 * concatener les EMs locaux
 */
clsetset
concat_kernels (nodelist * nl, type_instance kb, int nbr_partie)
{
  int i=0;
  nodelist * noeud = nl;
  clsetset collection=NULL;
  
  while (i<nbr_partie)
    {
	  printf("%d-eme iteration pour la concatenation\n",i);
	  collection=mult_cartisien(collection,noeud[i], kb);
	  if(noeud[i])
		collection=clsetset_minimal(collection);
	  i++;
    }
	printf("\nit's OK!\n");
	return collection;
}

/**
 * multiplication cartisienne entre deux noeuds
 */
clsetset
mult_cartisien(clsetset col,nodelist noeud,type_instance kb)
{
	int enter=0;
	nodelist p = noeud;
	clsetset collect=clsetset_new();
	clset krnl,setcl;
	
	if(col==NULL)
	{
		while (p)
		{
		  printf("col est NULL et p est NULL\n");
		  enter=1;
		  krnl = H (p->node->ancestor_edges->edge, kb);
		  clsetset_add (collect, krnl);
		  p=p->next;
		}
	}
	else
	{
		
		clsetset_iterator it = clsetset_iterator_new(col);
		while ((setcl = clsetset_next_element (it)) != NULL)
		{
			while (p)
			{
				printf("col non pas NULL et p non pas NULL\n");
				enter=1;
				krnl = H (p->node->ancestor_edges->edge, kb);			
				clsetset_add (collect, clset_union(setcl,krnl));
				p=p->next;
			}
			p=noeud;	
		}
			
	}
	
	if (!enter) 
		{
			return col;
		}
		
	//clset_free(krnl);
	//clset_free(setcl);
	//clsetset_iterator_free(it); à définir après
	return collect;
	
}


  
  




