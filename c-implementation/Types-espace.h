/**********************************************************************
***********************************************************************
**
** To be included FIRST
**
***********************************************************************
***********************************************************************
*/

#ifndef TYPES_H
#define TYPES_H

#include <stdio.h>
#include <stdlib.h>
#include <time.h> 

#include "public-types.h"
#include "clset.h"

/*
 * Data structures
 * ---------------
 */

/**
 * une parcelle
 */
struct st_parcelle;
typedef struct st_parcelle * parcel;

struct st_parcelle 
{
  int     num_parc        /* numéro de la parcelle */;
  int     num_parc_v      /* [PATCH inféré] numéro de parcelle dans les noms de variables */;
  int	  num_noyau		 /* numéro du noyau à laquelle appartient la parcelle */;
//  type_variable variables	 /* liste de variables */
};

/*
 * espace géographique
 */
struct st_espace;
typedef struct st_espace * espace;

struct st_espace 
{
  int     ordre        /* ordre de la matrice d'adjacence */;
  int ** matrice       /* la matirice d'adjacence du graphe */;
  parcel * parcelles	/* l'ensemble de prcelles de l'espace */;
};


/*
 * un noyau 
 */
struct st_noyau;
typedef struct st_noyau * core;

struct st_noyau 
{
  parcel   centre       /* centre du noyau */;
  parcel * noyau        /* la liste de parcelles qui compose le noyau */;
  int nbr_parcel			  /* nombre de parcelles dans noyaux */;
};

/*
 * une couverture
 */
struct st_couverture;
typedef struct st_couverture * covering;

struct st_couverture 
{
  parcel   centre       /* centre du noyau */;
  parcel * couv         /* la liste de parcelles qui compose le covering */;
  int nbr_parcel			  /* nombre de parcelles dans couverture */;
};


/*
 * une partie (une structure de la G-structure)
 */
struct st_partie;
typedef struct st_partie * partie;

struct st_partie 
{
  parcel   centre       /* centre du noyau */;
  core noyau            /* un noyau */;
  covering couv         /* un covering */;
  type_instance inst    /* base de croyances */;
  
};

/*
 * une G-structure
 */
struct st_Gstructure;
typedef struct st_Gstructure * Gstructure;

struct st_Gstructure 
{
  int      nbr_parties      /* ordre de la matrice d'adjacence */;  
  int      rayon            /* rayon des noyaux */;
  int      epais            /* épaisseur des couvertures */;
  partie * parties          /* la matirice d'adjacence du graphe */;
  clset    excluded_perm    /* [PATCH inféré] clauses exclues */;
};

#include "dp.h"
#endif
