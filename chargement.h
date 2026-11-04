/*
 * chargement.h - Phase 2 : chargement du fichier measures_in.csv
 * Travail de l'equipe Digital Minds (partie chargement CSV).
 * Adapte pour fonctionner avec le tableau dynamique de gestion.h.
 */
#ifndef CHARGEMENT_H
#define CHARGEMENT_H

#include "gestion.h"

/* Lit measures_in.csv (en-tete : id,batiment,ressource,valeur,date,seuil)
 * et ajoute chaque ligne valide dans le tableau t.
 * Renvoie le nombre de mesures chargees, ou -1 si le fichier est introuvable.
 * Une ligne invalide est signalee puis ignoree (le programme ne plante pas). */
int chargerMesures(const char *nomFichier, TableauMesures *t);

/* Affichage de controle en console. */
void afficherMesures(const TableauMesures *t);

#endif /* CHARGEMENT_H */
