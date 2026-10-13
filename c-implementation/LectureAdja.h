/**********************************************************************
***********************************************************************
**
** fonctions pour la lecture du fichier spécifiant l'adjacence parcelles
** et vérifiant la relevance entre parcelles
**
***********************************************************************
***********************************************************************
*/


#ifndef LECTURE_ADJA_H
#define LECTURE_ADJA_H
#include "Types-espace.h" 
 
void VerifierSaisie(espace);

int Relevance(espace, parcel, parcel, int);

espace LireGraphe();

void affiche_adjacence(espace space);





#endif
