#include "Types-espace.h"
#include "LectureAdja.h" 

/*
 * la fonction LireGraphe lit l'adjacence qui existe 
 * entre les parcelles de l'espace pour reconstituer 
 * cet espace géo. 
 */

espace LireGraphe()
{
       FILE * fichier;
       int i,j;
       int NbSommets;
	   espace space;

       fichier=fopen("graphe.don","r");
       fscanf(fichier,"%d",&NbSommets);
	   space = malloc(sizeof(struct st_espace));
       if (space == NULL)
       {
             printf("espace mémoire insuffisant\n");
             exit(1);
       }
      space->ordre = NbSommets;
      space->matrice = (int **) malloc(NbSommets*sizeof(int *));
       if (space->matrice == NULL)
       {
             printf("espace mémoire insuffisant\n");
             exit(1);
       }
	   
	   space->parcelles = (parcel *) malloc(NbSommets*sizeof(parcel));
       if (space->parcelles == NULL)
       {
             printf("espace mémoire insuffisant\n");
             exit(1);
       }
	   
	   for (i=0;i<NbSommets;i++)
       {
             space->parcelles[i] = (parcel) malloc(sizeof(struct st_parcelle));
             if (space->parcelles[i] == NULL)
             {
                   printf("espace mémoire insuffisant\n");
                   exit(1);
            }
       }
	   
	   
       for (i=0;i<NbSommets;i++)
       {
             space->matrice[i] = (int *) malloc(NbSommets*sizeof(int));
             if (space->matrice[i] == NULL)
             {
                   printf("espace mémoire insuffisant\n");
                   exit(1);
            }
       }
	   
      for (i=0;i<NbSommets;i++)
          {
          for(j=0;j<NbSommets;j++) 
             space->matrice[i][j]=0;
          space->matrice[i][i]=1;
		  space->parcelles[i]->num_parc=i+1;
		  space->parcelles[i]->num_noyau=-1;
          }
      while (fscanf(fichier,"%d",&i)!=EOF)
       {
             fscanf(fichier,"%d",&j);
             space->matrice[i][j]=1;
             space->matrice[j][i]=1;
      }
      fclose(fichier);
      /* [PATCH] num_parc_v : numero reel de parcelle (ex. 223 pour C223) lu dans parcelles.don,
         sinon identique a num_parc */
      {
        FILE * fp = fopen("parcelles.don","r");
        for (i=0;i<NbSommets;i++)
          {
            int v = i+1;
            if (fp != NULL && fscanf(fp,"%d",&v) != 1) v = i+1;
            space->parcelles[i]->num_parc_v = v;
          }
        if (fp != NULL) fclose(fp);
      }
      return space;
}

/*
 * la fonction VérifierSaisie vérifie le bon fonctionnement
 * de la fonction LireGraphe. 
 */

void VerifierSaisie(espace space)
{
      int i,j;

       printf("L'ordre du graphe vaut : %d\n\n",space->ordre);
       for (i=0;i<space->ordre;i++)
       {
             printf("Adjacent du sommet %d :\n",i+1);
             for(j=0;j<space->ordre;j++)
                  if (space->matrice[i][j])
                  {
                  printf("%d ",j+1);
                  printf("\n");
                  }
       }
} 

/*
 * la fonction affiche_adjacence vérifie la saisie des données. 
 */ 

void affiche_adjacence(espace space)
{
	int i,j,k;

       for (k=0;k<=space->ordre;k++)
       for (i=1;i<=space->ordre;i++)
       for (j=1;j<=space->ordre;j++)
       
           if (Relevance(space, space->parcelles[i-1], space->parcelles[j-1], k))
             printf("les sommets %d et %d sont %d-relevants\n", i, j, k);
			 
}

/*
 * la fonction Relevance vérifie la dist-Relevance entre les parcelles s1 et s2. 
 */ 

int Relevance(espace space, parcel s1, parcel s2, int dist)
{
    int i;
    if (s1==s2) return 1;
    if (dist==0) return (s1==s2);
    else{
    if (dist==1) return space->matrice[s1->num_parc -1][s2->num_parc -1];
    else{
       for (i=1;i<=space->ordre;i++)
           if ((space->matrice[s1->num_parc-1][i-1]) && (s1->num_parc!=i) && (Relevance(space, space->parcelles[i-1], s2,dist-1)))
              return 1;
              }
              }
    
    return 0;
    
}
