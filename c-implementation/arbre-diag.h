

#ifndef ARBRE_DIAG_H
#define ARBRE_DIAG_H

#include "clset.h"
#include "Types-espace.h"

struct st_node_diag;
typedef struct st_node_diag * node_diag;

/**
 * an edge of the DAG
 */
struct st_edge_diag;
typedef struct st_edge_diag * edge_diag;

struct st_edge_diag 
{
  unsigned label; 
  node_diag son;
  node_diag ancestor;
};

struct st_edgelist;
typedef struct st_edgelist * edgelist;
struct st_edgelist
{
  edge_diag edge;
  edgelist next;
};


enum type_noeud { NONENTYPE, NORMAL, REUSED, END, CLOSED };

/**
 * a node of the dag
 */
struct st_node_diag 
{
  clset label;
  enum type_noeud t;
  unsigned level;
  edgelist son_edges;
  edgelist ancestor_edges;
};

struct st_nodelist;
typedef struct st_nodelist * nodelist;
struct st_nodelist
{
  node_diag node;
  nodelist next;
};


/******************************* Prototypes ********************************/


nodelist build_hstree (type_instance kb);
void  print_kernels (nodelist nl, char **names, type_instance kb);
nodelist build_hstree_partie (partie part, clset setcl);
int nbr_excludedn(type_instance inst, clset setcl);
clsetset concat_kernels (nodelist * nl, type_instance kb,int nbr_partie);
clsetset mult_cartisien(clsetset col,nodelist noeud,type_instance kb);
nodelist *
build_hstree_space(Gstructure gs);
void
print_labels (clset lab, char **names, type_instance kb);
void shrink_rafine(clset ics, type_instance kb);



#endif
