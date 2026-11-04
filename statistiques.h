/*
 * statistiques.h - C7 : statistiques par batiment et par ressource
 * Partie qui n'existait dans aucun des travaux de l'equipe : ajoutee pour
 * produire le bloc "statistiques" de data.json (format de la seance 1).
 */
#ifndef STATISTIQUES_H
#define STATISTIQUES_H

#include "gestion.h"

typedef struct {
    int       id_batiment;
    char      batiment[TAILLE_BATIMENT];
    Ressource ressource;
    int       nb_mesures;
    double    total;
    double    moyenne;
    double    minimum;
    double    maximum;
    int       nb_alertes;
} Stat;

/* Numero de batiment (1, 2, 3...) = rang du nom dans l'ordre alphabetique
 * des batiments presents. Stable meme apres un tri des mesures. */
int rangBatiment(const Mesure m[], int n, const char *nom);

/* Unite d'une ressource : "m3" pour l'eau, "kWh" sinon. */
const char *uniteRessource(Ressource r);

/* Calcule une statistique par couple (batiment, ressource).
 * Renvoie le nombre de statistiques et place dans *stats un tableau alloue
 * (a liberer avec free). Renvoie 0 et *stats = NULL s'il n'y a aucune mesure. */
int calculerStatistiques(const Mesure m[], int n, Stat **stats);

void afficherStatistiques(const Stat stats[], int nb);

#endif /* STATISTIQUES_H */
