#include "Types-espace.h"
#include "gstructure.h"
#include "LectureAdja.h" 

/*
 * la fonction const_gstructure construit une G-structure sur l'espace
 * avec une certaine épaisseur et un certain noyau. 
 */

Gstructure constr_gstructure(espace space, int rayon, int epais)
{
	parcel centre;
	core noyau;
	covering couv;
	Gstructure gs;
	int i=0;
	int nbr_affec=space->ordre; /* calcul ne nombre des parcelles affectées dans des noyaux */
	
	gs = (Gstructure) malloc(sizeof(struct st_Gstructure));
       if (gs == NULL)
       {
             printf("espace mémoire insuffisant\n");
             exit(1);
       }
	   
	gs->parties = (partie *) malloc((space->ordre)*sizeof(partie));
       if (gs->parties == NULL)
       {
             printf("espace mémoire insuffisant\n");
             exit(1);
       }
	   
	//printf("j'ai fini de declarer et allouer la gstructure \n");	
	while (nbr_affec>0)
	{
		centre=choix_parcel(space);
		centre->num_noyau=i+1;
		//printf("etape %d: j'ai choisi comme centre la parcelle: %d\n",i+1,centre->num_parc);
		noyau=constr_noyau(space,centre,rayon);
		//printf("j'ai construit un noyau contenant %d parcelles \n", noyau->nbr_parcel);
		couv=constr_couverture(space, centre, epais);
		//printf("j'ai construit la couverture contenant %d parcelles \n", couv->nbr_parcel);
		gs->parties[i]=constr_partie(centre,noyau,couv);
		//printf("j'ai construit la partie contenant %d parcelles \n", noyau->nbr_parcel+couv->nbr_parcel);
		nbr_affec-= gs->parties[i]->noyau->nbr_parcel;
		//printf("il reste %d parcelles a affecter \n",nbr_affec);
		i++;
	}
	gs->nbr_parties=i;
	gs->rayon=rayon;
	gs->epais=epais;
	//printf("... en tout notre gstructure de rayon %d et epaisseur %d contient %d parties\n",gs->rayon,gs->epais,gs->nbr_parties);
	
	return gs;
}

/*
 * la fonction constr_noyau construit un noyau sur l'espace
 * avec un centre et un rayon donés. 
 */
core constr_noyau(espace space, parcel centre, int rayon)
{
	int i,j=0;
	core noy;
	
	noy = (core) malloc(sizeof(struct st_noyau));
       if (noy == NULL)
       {
             printf("espace mémoire insuffisant\n");
             exit(1);
       }
	   
	noy->noyau = (parcel *) malloc(space->ordre*sizeof(parcel));
       if (noy->noyau == NULL)
       {
             printf("espace mémoire insuffisant\n");
             exit(1);
       }
	
	noy->centre=centre;
	noy->noyau[j]=centre;	
	for(i=0;i<space->ordre;i++)
		{
			if((Relevance(space,centre,space->parcelles[i], rayon)) && (space->parcelles[i]->num_noyau==-1))
				{
					j++;
					noy->noyau[j]=space->parcelles[i];
					space->parcelles[i]->num_noyau=centre->num_noyau;
				}
		}
	noy->nbr_parcel=j+1;
	return noy;
}

/*
 * la fonction const_couverture construit une couverture sur l'espace
 * avec un centre et une certaine épaisseur. 
 */
covering constr_couverture(espace space, parcel centre, int epais)
{
	int i,j=0;
	covering couv;
	
	couv = (covering) malloc(sizeof(struct st_couverture));
       if (couv == NULL)
       {
             printf("espace mémoire insuffisant\n");
             exit(1);
       }
	   
	couv->couv = (parcel *) malloc((space->ordre-1)*sizeof(parcel));
       if (couv->couv == NULL)
       {
             printf("espace mémoire insuffisant\n");
             exit(1);
       }
	
	for(i=0;i<space->ordre;i++)
		{
			if ((Relevance(space,centre,space->parcelles[i], epais)) && (centre->num_noyau!=space->parcelles[i]->num_noyau))
				{
					couv->couv[j]=space->parcelles[i];
					j++;
				}
		}
	couv->nbr_parcel=j;
	return couv;
}

/*
 * la fonction const_partie construit une partie sur l'espace
 * pour un certain noyau, une couverture et un centre. 
 */
partie constr_partie(parcel centre, core noyau, covering couv)
{
	partie part;
	
	part = (partie) malloc(sizeof(struct st_partie));
       if (part == NULL)
       {
             printf("espace mémoire insuffisant\n");
             exit(1);
       }
	 part->centre=centre;
	 part->noyau=noyau;
	 part->couv=couv;
	 return part;
}

/*
 * la fonction choix_parcel choisit aléatoirement une parcelle
 * pas encore affectée à partir d'un espace donné. 
 */
parcel choix_parcel(espace space)
{
	int alea=rand() % (space->ordre);
	//printf("le num alea egale a : %d\n",alea);
	int num=space->parcelles[alea]->num_noyau;
	//printf("... juste avant la boucle est notre num_noyau egale a: %d\n", num);
	while(num!=-1)
	{
		alea=rand() % (space->ordre);
		//printf("le num alea egale a : %d\n",alea);
		num=space->parcelles[alea]->num_noyau;
		//printf("le num parcelle aleatoirement choisi est: %d\n",alea+1);
	}
	//printf("le num parcelle aleatoirement choisi est: %d\n",alea+1);
	return space->parcelles[alea];
}

/*
 * la fonction affiche_gstructure affiche en détail
 * une gstructure donnée.
*/
void affiche_gstructure(Gstructure gs, int rayon, int epais)
{
	printf("\033[2J" );
    int i,j;
	printf("Rayon des noyaux egale a: %d ---- Epaisseur des couvertures egale a: %d\n\n", rayon, epais);
	for(i=0;i<gs->nbr_parties;i++)
	{
		printf("Partie N° : %d\n",i+1);
		printf("---- Centre : %d\n",gs->parties[i]->centre->num_parc);
		printf("---- Parcelles du noyau : ");
		for(j=0;j<gs->parties[i]->noyau->nbr_parcel-1;j++)
		{
			printf("%d -- ",gs->parties[i]->noyau->noyau[j]->num_parc);
		}
		printf("%d",gs->parties[i]->noyau->noyau[j]->num_parc);
		printf("\n---- Parcelles de la couverture : ");
		
		for(j=0;j<gs->parties[i]->couv->nbr_parcel-1;j++)
		{
			printf("%d -- ",gs->parties[i]->couv->couv[j]->num_parc);
		}
		if(j==gs->parties[i]->couv->nbr_parcel-1)
			printf("%d",gs->parties[i]->couv->couv[j]->num_parc);
		printf("\n\n\n");
	}
}




