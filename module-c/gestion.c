/*
 * gestion.c - Module C du projet EcoCampus (Atelier mini-projet DSI2)
 * Implementation des structures et des fonctions de gestion des mesures.
 * Equipe : Digital Minds
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "gestion.h"

/* ------------------------------------------------------------------ */
/* Outils internes                                                    */
/* ------------------------------------------------------------------ */

/* Compare deux chaines sans tenir compte de la casse (0 si egales). */
static int comparer_sans_casse(const char *a, const char *b)
{
    while (*a && *b) {
        int ca = tolower((unsigned char)*a);
        int cb = tolower((unsigned char)*b);
        if (ca != cb)
            return ca - cb;
        a++;
        b++;
    }
    return tolower((unsigned char)*a) - tolower((unsigned char)*b);
}

/* Agrandit le tableau (capacite x 2) quand il est plein. */
static int tableau_agrandir(TableauMesures *t)
{
    int nouvelle_capacite = t->capacite * 2;
    Mesure *nouveau = realloc(t->donnees, nouvelle_capacite * sizeof(Mesure));
    if (nouveau == NULL)
        return GEST_ERR_MEMOIRE;
    t->donnees = nouveau;
    t->capacite = nouvelle_capacite;
    return GEST_OK;
}

/* Alloue et remplit un tableau d'indices a partir d'un tampon temporaire. */
static int *copier_indices(const int *source, int nb)
{
    int *res;
    if (nb == 0)
        return NULL;
    res = malloc(nb * sizeof(int));
    if (res != NULL)
        memcpy(res, source, nb * sizeof(int));
    return res;
}

/* ------------------------------------------------------------------ */
/* Conversion du type de ressource                                    */
/* ------------------------------------------------------------------ */

const char *ressource_vers_texte(Ressource r)
{
    switch (r) {
        case ELECTRICITE: return "electricite";
        case EAU:         return "eau";
        case CHAUFFAGE:   return "chauffage";
        default:          return "inconnue";
    }
}

Ressource texte_vers_ressource(const char *texte)
{
    if (texte == NULL)
        return RESSOURCE_INCONNUE;
    if (comparer_sans_casse(texte, "electricite") == 0 ||
        comparer_sans_casse(texte, "electricité") == 0)
        return ELECTRICITE;
    if (comparer_sans_casse(texte, "eau") == 0)
        return EAU;
    if (comparer_sans_casse(texte, "chauffage") == 0)
        return CHAUFFAGE;
    return RESSOURCE_INCONNUE;
}

/* ------------------------------------------------------------------ */
/* Gestion du tableau dynamique                                       */
/* ------------------------------------------------------------------ */

int tableau_init(TableauMesures *t)
{
    if (t == NULL)
        return GEST_ERR_PARAM;
    t->donnees = malloc(CAPACITE_INITIALE * sizeof(Mesure));
    if (t->donnees == NULL) {
        t->taille = 0;
        t->capacite = 0;
        t->prochain_id = 1;
        return GEST_ERR_MEMOIRE;
    }
    t->taille = 0;
    t->capacite = CAPACITE_INITIALE;
    t->prochain_id = 1;
    return GEST_OK;
}

void tableau_liberer(TableauMesures *t)
{
    if (t == NULL)
        return;
    free(t->donnees);
    t->donnees = NULL;
    t->taille = 0;
    t->capacite = 0;
}

/* ------------------------------------------------------------------ */
/* Ajout                                                              */
/* ------------------------------------------------------------------ */

int ajouter_mesure(TableauMesures *t, const char *batiment, Ressource r,
                   const char *date, double valeur, double seuil, int *id_cree)
{
    Mesure *m;
    int code;

    /* Controle des parametres : on refuse les donnees invalides */
    if (t == NULL || t->donnees == NULL)
        return GEST_ERR_PARAM;
    if (batiment == NULL || batiment[0] == '\0' ||
        strlen(batiment) >= TAILLE_BATIMENT)
        return GEST_ERR_PARAM;
    if (date == NULL || date[0] == '\0' || strlen(date) >= TAILLE_DATE)
        return GEST_ERR_PARAM;
    if (r == RESSOURCE_INCONNUE)
        return GEST_ERR_PARAM;
    if (valeur < 0 || seuil <= 0)   /* valeur >= 0 et seuil > 0 (fiche seance 1) */
        return GEST_ERR_PARAM;

    /* Agrandissement si le tableau est plein */
    if (t->taille == t->capacite) {
        code = tableau_agrandir(t);
        if (code != GEST_OK)
            return code;
    }

    m = &t->donnees[t->taille];
    m->id = t->prochain_id++;
    strcpy(m->batiment, batiment);
    m->ressource = r;
    strcpy(m->date, date);
    m->valeur = valeur;
    m->seuil = seuil;
    m->alerte = 0;   /* la detection d'anomalies est faite plus tard (TAF 3) */
    t->taille++;

    if (id_cree != NULL)
        *id_cree = m->id;
    return GEST_OK;
}

/* ------------------------------------------------------------------ */
/* Recherche                                                          */
/* ------------------------------------------------------------------ */

int *rechercher_par_batiment(const TableauMesures *t, const char *batiment, int *nb)
{
    int *tampon, *res;
    int i, n = 0;

    if (nb != NULL)
        *nb = 0;
    if (t == NULL || t->donnees == NULL || batiment == NULL || nb == NULL ||
        t->taille == 0)
        return NULL;

    tampon = malloc(t->taille * sizeof(int));
    if (tampon == NULL)
        return NULL;

    for (i = 0; i < t->taille; i++)
        if (comparer_sans_casse(t->donnees[i].batiment, batiment) == 0)
            tampon[n++] = i;

    res = copier_indices(tampon, n);
    free(tampon);
    *nb = (res != NULL) ? n : 0;
    return res;
}

int *rechercher_par_ressource(const TableauMesures *t, Ressource r, int *nb)
{
    int *tampon, *res;
    int i, n = 0;

    if (nb != NULL)
        *nb = 0;
    if (t == NULL || t->donnees == NULL || nb == NULL || t->taille == 0)
        return NULL;

    tampon = malloc(t->taille * sizeof(int));
    if (tampon == NULL)
        return NULL;

    for (i = 0; i < t->taille; i++)
        if (t->donnees[i].ressource == r)
            tampon[n++] = i;

    res = copier_indices(tampon, n);
    free(tampon);
    *nb = (res != NULL) ? n : 0;
    return res;
}

/* ------------------------------------------------------------------ */
/* Suppression                                                        */
/* ------------------------------------------------------------------ */

int supprimer_mesure(TableauMesures *t, int id)
{
    int i, pos = -1;

    if (t == NULL || t->donnees == NULL)
        return GEST_ERR_PARAM;

    for (i = 0; i < t->taille; i++) {
        if (t->donnees[i].id == id) {
            pos = i;
            break;
        }
    }
    if (pos < 0)
        return GEST_ERR_ABSENT;

    /* Decalage vers la gauche pour combler le trou */
    for (i = pos; i < t->taille - 1; i++)
        t->donnees[i] = t->donnees[i + 1];
    t->taille--;
    return GEST_OK;
}

/* ------------------------------------------------------------------ */
/* Affichage                                                          */
/* ------------------------------------------------------------------ */

void afficher_mesure(const Mesure *m)
{
    if (m == NULL)
        return;
    /* Statut calcule a partir des valeurs : ALERTE (valeur > seuil),
     * PROCHE (>= POURCENT_PROCHE % du seuil, sans le depasser) ou normal. */
    const char *statut = "normal";
    if (m->valeur > m->seuil)
        statut = "ALERTE";
    else if (m->valeur * 100.0 >= m->seuil * POURCENT_PROCHE)
        statut = "PROCHE";

    printf("%-4d %-16s %-12s %-17s %10.2f %10.2f  %s\n",
           m->id, m->batiment, ressource_vers_texte(m->ressource),
           m->date, m->valeur, m->seuil, statut);
}

void afficher_tableau(const TableauMesures *t)
{
    int i;
    if (t == NULL || t->donnees == NULL) {
        printf("Tableau non initialise.\n");
        return;
    }
    printf("%-4s %-16s %-12s %-17s %10s %10s  %s\n",
           "ID", "Batiment", "Ressource", "Date", "Valeur", "Seuil", "Statut");
    printf("-------------------------------------------------------------------------------\n");
    for (i = 0; i < t->taille; i++)
        afficher_mesure(&t->donnees[i]);
    printf("(%d mesure(s))\n", t->taille);
}
