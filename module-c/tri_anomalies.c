/*
 * tri_anomalies.c - Question 3 : tri des mesures et detection des anomalies
 * Travail de l'equipe Digital Minds (compte rendu "EcoCampus", question 3).
 */
#include <stdio.h>
#include <string.h>
#include "tri_anomalies.h"

int niveau(const Mesure *m)
{
    if (m->valeur > m->seuil)
        return NIVEAU_ALERTE;
    if (m->valeur * 100.0 >= m->seuil * POURCENT_PROCHE)
        return NIVEAU_PROCHE;
    return NIVEAU_NORMAL;
}

const char *niveauTexte(int n)
{
    if (n == NIVEAU_ALERTE) return "alerte";
    if (n == NIVEAU_PROCHE) return "proche_seuil";
    return "normal";
}

void afficher(const Mesure m[], int n)
{
    int i;

    for (i = 0; i < n; i++)
    {
        printf("%s %s %s %.2f %.2f\n",
               m[i].batiment, ressource_vers_texte(m[i].ressource), m[i].date,
               m[i].valeur, m[i].seuil);
    }
}

void trier(Mesure m[], int n)
{
    int i, j;
    Mesure temp;

    for (i = 0; i < n - 1; i++)
    {
        for (j = 0; j < n - 1 - i; j++)
        {
            if (strcmp(m[j].date, m[j + 1].date) > 0)
            {
                temp = m[j];
                m[j] = m[j + 1];
                m[j + 1] = temp;
            }
        }
    }
}

void trierParConsommation(Mesure m[], int n, int croissant)
{
    int i, j, echanger;
    Mesure temp;

    for (i = 0; i < n - 1; i++)
    {
        for (j = 0; j < n - 1 - i; j++)
        {
            if (croissant)
                echanger = (m[j].valeur > m[j + 1].valeur);
            else
                echanger = (m[j].valeur < m[j + 1].valeur);

            if (echanger)
            {
                temp = m[j];
                m[j] = m[j + 1];
                m[j + 1] = temp;
            }
        }
    }
}

int anomalies(Mesure m[], int n)
{
    int i, trouve = 0;

    for (i = 0; i < n; i++)
    {
        if (m[i].valeur > m[i].seuil)
        {
            m[i].alerte = 1;
            printf("Anomalie : %s, %s, consommation = %.2f\n",
                   m[i].batiment, ressource_vers_texte(m[i].ressource),
                   m[i].valeur);
            trouve++;
        }
        else
        {
            m[i].alerte = 0;
        }
    }

    if (trouve == 0)
        printf("Aucune anomalie detectee.\n");

    /* Avertissements : egale ou proche du seuil, mais pas encore au-dessus */
    for (i = 0; i < n; i++)
    {
        if (niveau(&m[i]) == NIVEAU_PROCHE)
        {
            printf("Attention : %s, %s, consommation = %.2f proche du seuil (%.2f)\n",
                   m[i].batiment, ressource_vers_texte(m[i].ressource),
                   m[i].valeur, m[i].seuil);
        }
    }

    return trouve;
}

/* ------------------------------------------------------------------ */
/* Classement par ressource                                           */
/* ------------------------------------------------------------------ */

/* Renvoie 1 si a doit etre place apres b.
 * Regle 1 : on regroupe par ressource (ordre de l'enum : electricite, eau, chauffage).
 * Regle 2 : a l'interieur d'une ressource, on applique le critere choisi. */
static int doitEtreApres(const Mesure *a, const Mesure *b, int critere, int croissant)
{
    if (a->ressource != b->ressource)
        return a->ressource > b->ressource;
    if (critere == CRITERE_DATE)
        return strcmp(a->date, b->date) > 0;
    return croissant ? (a->valeur > b->valeur) : (a->valeur < b->valeur);
}

void trierParRessource(Mesure m[], int n, int critere, int croissant)
{
    int i, j;
    Mesure temp;

    for (i = 0; i < n - 1; i++)
    {
        for (j = 0; j < n - 1 - i; j++)
        {
            if (doitEtreApres(&m[j], &m[j + 1], critere, croissant))
            {
                temp = m[j];
                m[j] = m[j + 1];
                m[j + 1] = temp;
            }
        }
    }
}

static const char *uniteDe(Ressource r)
{
    return (r == EAU) ? "m3" : "kWh";
}

void afficherParRessource(const Mesure m[], int n, int filtre)
{
    int r, i, nb;

    for (r = ELECTRICITE; r <= CHAUFFAGE; r++)
    {
        if (filtre >= 0 && r != filtre)
            continue;

        printf("\n===== %s (%s) =====\n",
               ressource_vers_texte((Ressource)r), uniteDe((Ressource)r));
        printf("%-4s %-16s %-12s %-17s %10s %10s  %s\n",
               "ID", "Batiment", "Ressource", "Date", "Valeur", "Seuil", "Statut");
        printf("-------------------------------------------------------------------------------\n");

        nb = 0;
        for (i = 0; i < n; i++)
        {
            if (m[i].ressource == (Ressource)r)
            {
                afficher_mesure(&m[i]);
                nb++;
            }
        }
        printf("(%d mesure(s))\n", nb);
    }
}

int anomaliesParRessource(Mesure m[], int n)
{
    int r, i, total = 0, alertes, proches;

    /* Mise a jour du champ alerte de toutes les mesures */
    for (i = 0; i < n; i++)
        m[i].alerte = (m[i].valeur > m[i].seuil);

    for (r = ELECTRICITE; r <= CHAUFFAGE; r++)
    {
        alertes = 0;
        proches = 0;
        printf("\n----- %s (%s) -----\n",
               ressource_vers_texte((Ressource)r), uniteDe((Ressource)r));

        for (i = 0; i < n; i++)
        {
            if (m[i].ressource != (Ressource)r)
                continue;
            if (niveau(&m[i]) == NIVEAU_ALERTE)
            {
                printf("Anomalie : %s, consommation = %.2f (seuil %.2f)\n",
                       m[i].batiment, m[i].valeur, m[i].seuil);
                alertes++;
            }
        }
        for (i = 0; i < n; i++)
        {
            if (m[i].ressource != (Ressource)r)
                continue;
            if (niveau(&m[i]) == NIVEAU_PROCHE)
            {
                printf("Attention : %s, consommation = %.2f proche du seuil (%.2f)\n",
                       m[i].batiment, m[i].valeur, m[i].seuil);
                proches++;
            }
        }

        if (alertes == 0 && proches == 0)
            printf("Aucune anomalie.\n");
        printf("=> %d alerte(s), %d avertissement(s)\n", alertes, proches);
        total += alertes;
    }
    return total;
}
