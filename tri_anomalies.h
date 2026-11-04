/*
 * tri_anomalies.h - Question 3 : tri des mesures et detection des anomalies
 * Travail de l'equipe Digital Minds (compte rendu "EcoCampus", question 3).
 * Les fonctions gardent leurs noms d'origine : afficher, trier, anomalies.
 */
#ifndef TRI_ANOMALIES_H
#define TRI_ANOMALIES_H

#include "gestion.h"

/* Le pourcentage POURCENT_PROCHE (seuil "proche") est defini dans gestion.h. */

/* Niveaux d'une mesure */
#define NIVEAU_NORMAL  0
#define NIVEAU_PROCHE  1   /* proche ou egale au seuil : avertissement */
#define NIVEAU_ALERTE  2   /* valeur > seuil : anomalie                */

/* Renvoie le niveau d'une mesure (NIVEAU_NORMAL, NIVEAU_PROCHE ou NIVEAU_ALERTE). */
int niveau(const Mesure *m);

/* Texte du niveau : "normal", "proche_seuil" ou "alerte". */
const char *niveauTexte(int n);

/* Affiche les mesures (une ligne par mesure). */
void afficher(const Mesure m[], int n);

/* Tri a bulles par date (la date "AAAA-MM-JJ hh:mm" se compare avec strcmp). */
void trier(Mesure m[], int n);

/* Tri a bulles par consommation : croissant = 1 (croissante), 0 (decroissante). */
void trierParConsommation(Mesure m[], int n, int croissant);

/* Detection des anomalies : renseigne le champ alerte de chaque mesure
 * (1 si valeur > seuil, sinon 0), affiche les anomalies, puis les
 * avertissements "proche du seuil". Renvoie le nombre d'anomalies. */
int anomalies(Mesure m[], int n);

/* ------------------------------------------------------------------ */
/* Classement PAR RESSOURCE (remarque de la tutrice)                  */
/* On ne compare jamais des kWh d'electricite avec des m3 d'eau :     */
/* les mesures sont d'abord regroupees par ressource, puis classees   */
/* a l'interieur de chaque ressource.                                 */
/* ------------------------------------------------------------------ */
#define CRITERE_CONSOMMATION 0
#define CRITERE_DATE         1

/* Tri a bulles : regroupe par ressource (electricite, eau, chauffage), puis
 * classe chaque groupe par consommation (croissant = 1 ou 0) ou par date. */
void trierParRessource(Mesure m[], int n, int critere, int croissant);

/* Affiche les mesures, une section par ressource. filtre = -1 pour toutes
 * les ressources, sinon une valeur de Ressource (ELECTRICITE, EAU, CHAUFFAGE). */
void afficherParRessource(const Mesure m[], int n, int filtre);

/* Comme anomalies(), mais presente les alertes et avertissements ressource par
 * ressource, avec un total pour chacune. Renvoie le nombre total d'anomalies. */
int anomaliesParRessource(Mesure m[], int n);

#endif /* TRI_ANOMALIES_H */
