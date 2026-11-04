/*
 * statistiques.c - C7 : statistiques par batiment et par ressource
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "statistiques.h"

int rangBatiment(const Mesure m[], int n, const char *nom)
{
    int i, j, rang = 1;

    /* On compte les noms distincts strictement plus petits que "nom". */
    for (i = 0; i < n; i++) {
        if (strcmp(m[i].batiment, nom) < 0) {
            int dejaVu = 0;
            for (j = 0; j < i; j++)
                if (strcmp(m[j].batiment, m[i].batiment) == 0) {
                    dejaVu = 1;
                    break;
                }
            if (!dejaVu)
                rang++;
        }
    }
    return rang;
}

const char *uniteRessource(Ressource r)
{
    return (r == EAU) ? "m3" : "kWh";
}

int calculerStatistiques(const Mesure m[], int n, Stat **stats)
{
    Stat *res;
    int i, k, nb = 0;

    *stats = NULL;
    if (n <= 0)
        return 0;

    res = malloc(n * sizeof(Stat));
    if (res == NULL)
        return 0;

    for (i = 0; i < n; i++) {
        /* Chercher si le couple (batiment, ressource) existe deja */
        for (k = 0; k < nb; k++)
            if (strcmp(res[k].batiment, m[i].batiment) == 0 &&
                res[k].ressource == m[i].ressource)
                break;

        if (k == nb) {   /* nouveau couple */
            res[k].id_batiment = rangBatiment(m, n, m[i].batiment);
            strcpy(res[k].batiment, m[i].batiment);
            res[k].ressource = m[i].ressource;
            res[k].nb_mesures = 0;
            res[k].total = 0.0;
            res[k].minimum = m[i].valeur;
            res[k].maximum = m[i].valeur;
            res[k].nb_alertes = 0;
            nb++;
        }

        res[k].nb_mesures++;
        res[k].total += m[i].valeur;
        if (m[i].valeur < res[k].minimum) res[k].minimum = m[i].valeur;
        if (m[i].valeur > res[k].maximum) res[k].maximum = m[i].valeur;
        if (m[i].alerte) res[k].nb_alertes++;
    }

    for (k = 0; k < nb; k++)
        res[k].moyenne = res[k].total / res[k].nb_mesures;

    *stats = res;
    return nb;
}

void afficherStatistiques(const Stat stats[], int nb)
{
    int k;
    printf("%-3s %-16s %-12s %4s %10s %10s %10s %10s %7s\n",
           "ID", "Batiment", "Ressource", "Nb", "Total", "Moyenne",
           "Min", "Max", "Alertes");
    printf("---------------------------------------------------------------------------------------\n");
    for (k = 0; k < nb; k++)
        printf("%-3d %-16s %-12s %4d %10.2f %10.2f %10.2f %10.2f %7d\n",
               stats[k].id_batiment, stats[k].batiment,
               ressource_vers_texte(stats[k].ressource), stats[k].nb_mesures,
               stats[k].total, stats[k].moyenne, stats[k].minimum,
               stats[k].maximum, stats[k].nb_alertes);
}
