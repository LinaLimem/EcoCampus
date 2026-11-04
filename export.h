/*
 * export.h - Export des resultats (C9) : data.json et data.csv
 * Travail de l'equipe Digital Minds (partie export).
 * Le JSON suit exactement le format defini en seance 1 :
 * blocs "meta", "mesures" et "statistiques".
 */
#ifndef EXPORT_H
#define EXPORT_H

#include "gestion.h"
#include "statistiques.h"

/* Ecrit data.json. Renvoie 0 si tout va bien, -1 si le fichier ne s'ouvre pas.
 * "source" est le nom du CSV d'entree (ecrit dans meta.source). */
int exporterVersJSON(const char *nomFichier, const Mesure mesures[], int taille,
                     const Stat stats[], int nbStats, const char *source);

/* Ecrit data.csv (alternative autorisee par l'enonce). Renvoie 0 ou -1. */
int exporterVersCSV(const char *nomFichier, const Mesure mesures[], int taille);

#endif /* EXPORT_H */
