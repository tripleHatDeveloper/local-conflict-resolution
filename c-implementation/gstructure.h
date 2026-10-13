/**********************************************************************
***********************************************************************
**
** FONCTIONS POUR LA CREATION DES G-STRUCTURES
**
***********************************************************************
***********************************************************************
*/

#ifndef GSTRUCTURE_H
#define GSTRUCTURE_H
#include "Types-espace.h"
#include "LectureAdja.h"

/*
 * la fonction const_gstructure construit une G-structure sur l'espace
 * avec une certaine épaisseur et un certain noyau. 
 */
Gstructure constr_gstructure(espace, int , int );

/*
 * la fonction constr_noyau construit un noyau sur l'espace
 * avec un centre et un rayon donés. 
 */
core constr_noyau(espace space, parcel centre, int rayon);

/*
 * la fonction const_couverture construit une couverture sur l'espace
 * avec un centre et une certaine épaisseur. 
 */
covering constr_couverture(espace space, parcel centre, int epais);

/*
 * la fonction const_partie construit une partie sur l'espace
 * pour un certain noyau, une couverture et un centre. 
 */
partie constr_partie(parcel centre, core noyau, covering couv);

/*
 * la fonction choix_parcel choisit aléatoirement une parcelle
 * pas encore affectée à partir d'un espace donné. 
 */
parcel choix_parcel(espace space);

/*
 * la fonction rdtsc retourne le nombre de cycles utilisés par le 
 * processeur depuis le démarrage. Elle aide d'améliorer l'aspect 
 * aléatoire de la fonction rand.
 */
//int rdtsc();

void affiche_gstructure(Gstructure gs, int rayon, int epais);

#endif
 
