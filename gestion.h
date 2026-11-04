/*
 * gestion.h - Module C du projet EcoCampus (Atelier mini-projet DSI2)
 * Structures de donnees et fonctions de gestion des mesures de consommation.
 * Equipe : Digital Minds
 */
#ifndef GESTION_H
#define GESTION_H

#define TAILLE_BATIMENT   50   /* longueur max du nom d'un batiment      */
#define TAILLE_DATE       20   /* "AAAA-MM-JJ hh:mm" + '\0'              */
#define CAPACITE_INITIALE 20   /* capacite de depart du tableau dynamique */

/* Une mesure est "proche du seuil" quand elle atteint ce pourcentage du seuil
 * sans le depasser (90 -> a partir de 90 % ; mettre 100 pour "egale au seuil
 * seulement"). Remarque de l'enseignante : valeur == seuil n'est pas une
 * alerte, mais doit declencher un avertissement. */
#define POURCENT_PROCHE   90

/* Codes de retour des fonctions */
#define GEST_OK            0
#define GEST_ERR_PARAM    -1   /* parametre invalide ou donnee incorrecte */
#define GEST_ERR_MEMOIRE  -2   /* echec d'allocation memoire              */
#define GEST_ERR_ABSENT   -3   /* enregistrement introuvable              */

/* Type de ressource mesuree */
typedef enum {
    ELECTRICITE,
    EAU,
    CHAUFFAGE,
    RESSOURCE_INCONNUE
} Ressource;

/* Un point de mesure */
typedef struct {
    int       id;                       /* identifiant unique              */
    char      batiment[TAILLE_BATIMENT];/* ex. "Bibliotheque"              */
    Ressource ressource;                /* electricite / eau / chauffage   */
    char      date[TAILLE_DATE];        /* format "AAAA-MM-JJ hh:mm"       */
    double    valeur;                   /* consommation mesuree            */
    double    seuil;                    /* seuil de reference              */
    int       alerte;                   /* 1 si valeur > seuil, sinon 0    */
} Mesure;

/* Tableau dynamique de mesures */
typedef struct {
    Mesure *donnees;     /* zone allouee dynamiquement                     */
    int     taille;      /* nombre de mesures stockees                     */
    int     capacite;    /* nombre de mesures que la zone peut contenir    */
    int     prochain_id; /* prochain identifiant a attribuer               */
} TableauMesures;

/* --- Conversion du type de ressource --- */
const char *ressource_vers_texte(Ressource r);
Ressource   texte_vers_ressource(const char *texte);

/* --- Gestion du tableau --- */
int  tableau_init(TableauMesures *t);
void tableau_liberer(TableauMesures *t);

/* --- Fonctions de gestion --- */
/* Ajoute une mesure. Si id_cree n'est pas NULL, y range l'id attribue. */
int ajouter_mesure(TableauMesures *t, const char *batiment, Ressource r,
                   const char *date, double valeur, double seuil, int *id_cree);

/* Recherches : renvoient un tableau d'indices alloue (a liberer avec free)
 * et placent le nombre de resultats dans *nb. Renvoient NULL si aucun
 * resultat (avec *nb = 0) ou en cas d'erreur. */
int *rechercher_par_batiment(const TableauMesures *t, const char *batiment, int *nb);
int *rechercher_par_ressource(const TableauMesures *t, Ressource r, int *nb);

/* Supprime la mesure d'identifiant id. */
int supprimer_mesure(TableauMesures *t, int id);

/* --- Affichage (controle en console) --- */
void afficher_mesure(const Mesure *m);
void afficher_tableau(const TableauMesures *t);

#endif /* GESTION_H */
