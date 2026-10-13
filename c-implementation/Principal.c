#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <malloc.h>
#include <stdio.h>
#include <assert.h>

#define MAIN
#include "public-types.h"
#include "Types-espace.h"
#include "gstructure.h"
#include "LectureAdja.h"
#include "clset.h"
#include "timer.h"
#include "stack.h"
#include "utils.h"
#include "arbre-diag.h"
#include "nodetable.h"
#include "opts.h"
#include "dp.h"


/* set files extension */
#define SET_EXT ".set"

/* variable names mapping file extension */
#define NAMES_EXT ".corresp"

static type_instance KBase; /* the knowledge base */


/*
lecture d'une matrice d'adjacence de l'espace géo. à partir d'un fichier 
graphe.don pour re-construire l'espace géographique.
*/


int main(int argc, char ** argv)
{
	/* initialisation de l'instance globale */
	
  char * kbfile;       /* filename of the clause base */
  char * kbfile_set;   /* filename of the set description */
  char * kbfile_names; /* filename of the variable name mapping */
  FILE * pkb;          /* clause base file descriptor */
  FILE * pkb_set;      /* set description file descriptor */
  nodelist * endnodes;   /* list of final nodes (removed sets) */
  int top1, top2, topB; /* timing variables */
  int optchar;          /* command line option char */
  FILE * pvparcel;      /* les parcelles correspondantes aux variables */

  /* options initialization */
  Opt_Min = 0;
  Opt_All = 0;
  Opt_Verb = 0;
  Opt_Time = 0;
  Opt_FullRem = 1;

  /* command line processing */

  init_timer(4);
  topB = top();
  if ( argc < 2 )
    error(255, "wrong number of arguments");

  while ((optchar = getopt (argc, argv, "mavtb")) != EOF)
    {      
      switch (optchar)
	{
	case 'm':
	  Opt_Min = 1;
	  break;
	case 'a':
	  Opt_All = 1;
	  break;
	case 'v':
	  Opt_Verb = 1;
	  break;
	case 't':
	  Opt_Time = 1;
	  break;
	case 'b':
	  Opt_FullRem = 0;
	  break;
	default:
	  error(255, "bad option");
	  break;
	}
    }
  if (optind >= argc)
    error (255, "wrong number of parameters\n");

  /* files : naming and opening */

  kbfile = strdup (argv[optind]); /* strdup(chaine): crée une mémoire et duplique 'chaine' */

  assert ( kbfile != NULL );/* assert: vérifie une assertion et si faux, elle déclanche une exception si possible sinon sortie du pg */

  kbfile_set = malloc ((strlen (kbfile) + 5) * sizeof (char));
  kbfile_names = malloc ((strlen (kbfile) + 9) * sizeof (char));
  
  assert ( kbfile_set != NULL && kbfile_names != NULL );

  strcpy (kbfile_set, kbfile);
  strcpy (kbfile_names, kbfile);
  strcat (kbfile_set, SET_EXT); /* strcat: effectue la concatenation dans kbfile_set */
  strcat (kbfile_names, NAMES_EXT);

  /* load instance file and set membership file */

  if ( (pkb = fopen (kbfile, "r")) == NULL )
    error (1, "unable to open %s", kbfile);
  pkb_set  = NULL;
  if (Opt_FullRem)
    {
      if ( (pkb_set = fopen (kbfile_set, "r")) == NULL )
	error (1, "unable to open %s", kbfile_set);
    }
	
	/*if ( (pvparcel = fopen (kbfile_names, "r")) == NULL )
    {
      fprintf (stderr, "unable to open file %s\n", kbfile_names);
      exit (1);
    }
	*/
	  KBase = read_instance (pkb, pkb_set,kbfile_names);
	  	   
	   /* affichage instance globale */
	   
	  //affiche_instance(KBase);
  
		/* load variable names */

	  if ( (pvparcel = fopen (kbfile_names, "r")) == NULL )
		{
			fprintf (stderr, "unable to open file %s\n", kbfile_names);
			exit (1);
		}
	  if (Opt_FullRem)
		VarNames = load_names (kbfile_names, KBase);
	/* *
	 * *
	 * * construction de la G-structure
	 * *
	 * */
	
	   /* partitionnement de l'espace */
	
       espace space;
	   Gstructure gs;
       int epais,rayon;
	   int i;
	   
       space=LireGraphe();
	   
       printf("Entrer le rayon des noyaux et l'epaisseur des couvertures sachant que:\n   Le rayon reprsente l'epaisseur du noyau, et\n   L'epaisseur c'est la distance du centre du noyau jusqu'a la frontiere\n");
	  
	   scanf("%d %d",&rayon, &epais);
	   
	   /* partitionnement de l'espace geog */
	   
	   gs=constr_gstructure(space,rayon,epais);
	   
	   /* initialisation des sous-bases (sous-instances) de la gstructure */
	   	   
	   initialise_insts(gs,KBase);
	   
	   /* affichage gstructure construite */
	   
	   affiche_gstructure(gs,rayon,epais);
	   //scanf("%d",&i);

	   
	/* build HS-tree for the G-structure gs*/
	   
	top1 = top();
	
	printf ("\n\nETAPE de calcul des Kernels locaux:\n");
	printf ("-----------------------------------\n\n");
	endnodes=build_hstree_space(gs);
	
	printf ("\n\nETAPE de La concatenation:\n");
	printf ("--------------------------\n\n");
	clsetset collection=concat_kernels(endnodes,KBase,gs->nbr_parties);
	printf ("\n\nETAPE de La minimalite:\n");	
	printf ("-----------------------\n\n");	
	//collection=clsetset_minimal(collection);
	printf ("\n\nAFFICHAGE du resultat finial:\n");
	printf ("-----------------------------\n\n");
	clsetset_dispaly(collection);
						
	top2 = top();
	if (Opt_Time)
    {
      printTime("kernel computation", top1, top2);
      printTime("Total time", topB, top());
    }
		
	return 0;
		
}


