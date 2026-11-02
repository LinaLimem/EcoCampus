/*
 * main.c - Menu interactif du module C du projet EcoCampus
 * Utilise les modules : gestion, chargement, tri_anomalies, statistiques, export.
 * Usage : EcoCampus [fichier_entree.csv]
 *   - si un fichier est donne, il est charge automatiquement au demarrage.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "gestion.h"
#include "chargement.h"
#include "tri_anomalies.h"
#include "statistiques.h"
#include "export.h"

/* ------------------------------------------------------------------ */
/* Outils de saisie (fgets : plus robuste que scanf)                  */
/* ------------------------------------------------------------------ */

static int lireTexte(const char *invite, char *dest, size_t taille)
{
    printf("%s", invite);
    if (fgets(dest, (int)taille, stdin) == NULL) {
        dest[0] = '\0';
        return 0;
    }
    dest[strcspn(dest, "\r\n")] = '\0';
    return 1;
}

static int lireEntier(const char *invite, int *val)
{
    char buf[64];
    lireTexte(invite, buf, sizeof(buf));
    return sscanf(buf, "%d", val) == 1;
}

static int lireReel(const char *invite, double *val)
{
    char buf[64];
    lireTexte(invite, buf, sizeof(buf));
    return sscanf(buf, "%lf", val) == 1;
}

/* Met a jour le champ alerte de chaque mesure, sans rien afficher. */
static void majAlertes(TableauMesures *t)
{
    for (int i = 0; i < t->taille; i++)
        t->donnees[i].alerte = (t->donnees[i].valeur > t->donnees[i].seuil);
}

static void afficherMenu(void)
{
    printf("\n===== EcoCampus : module C =====\n");
    printf(" 1. Charger le fichier CSV\n");
    printf(" 2. Afficher / rechercher (batiment, ressource)\n");
    printf(" 3. Ajouter une mesure\n");
    printf(" 4. Supprimer une mesure\n");
    printf(" 5. Classer les mesures (par ressource)\n");
    printf(" 6. Statistiques par batiment\n");
    printf(" 7. Afficher les alertes (par ressource)\n");
    printf(" 8. Exporter data.json\n");
    printf(" 0. Quitter\n");
}

/* ------------------------------------------------------------------ */
/* Actions du menu                                                    */
/* ------------------------------------------------------------------ */

static void actionCharger(TableauMesures *t, char *source, size_t tailleSource)
{
    char nom[128];
    lireTexte("Nom du fichier CSV [measures_in.csv] : ", nom, sizeof(nom));
    if (nom[0] == '\0')
        strcpy(nom, "measures_in.csv");

    /* On repart d'un tableau vide pour ne pas dupliquer les mesures */
    tableau_liberer(t);
    if (tableau_init(t) != GEST_OK) {
        fprintf(stderr, "Erreur : memoire insuffisante.\n");
        exit(1);
    }

    int n = chargerMesures(nom, t);
    if (n < 0)
        return;
    snprintf(source, tailleSource, "%s", nom);
    majAlertes(t);
    printf("\n%d mesure(s) chargee(s) depuis %s.\n", n, nom);
}

static void afficherIndices(const TableauMesures *t, const int *idx, int nb)
{
    printf("%-4s %-16s %-12s %-17s %10s %10s  %s\n",
           "ID", "Batiment", "Ressource", "Date", "Valeur", "Seuil", "Statut");
    printf("-------------------------------------------------------------------------------\n");
    for (int i = 0; i < nb; i++)
        afficher_mesure(&t->donnees[idx[i]]);
    printf("(%d mesure(s))\n", nb);
}

static void actionAfficherRechercher(TableauMesures *t)
{
    int choix, nb;
    int *idx;
    char texte[64];

    printf("\n  1. Afficher toutes les mesures\n");
    printf("  2. Rechercher par batiment\n");
    printf("  3. Rechercher par ressource\n");
    if (!lireEntier("Votre choix : ", &choix))
        choix = 0;

    majAlertes(t);

    if (choix == 1) {
        printf("\n");
        afficher_tableau(t);
    } else if (choix == 2) {
        lireTexte("Batiment : ", texte, sizeof(texte));
        idx = rechercher_par_batiment(t, texte, &nb);
        if (idx == NULL)
            printf("Aucune mesure pour ce batiment.\n");
        else {
            printf("\n");
            afficherIndices(t, idx, nb);
            free(idx);
        }
    } else if (choix == 3) {
        lireTexte("Ressource (electricite/eau/chauffage) : ", texte, sizeof(texte));
        Ressource r = texte_vers_ressource(texte);
        if (r == RESSOURCE_INCONNUE) {
            printf("Ressource inconnue.\n");
            return;
        }
        idx = rechercher_par_ressource(t, r, &nb);
        if (idx == NULL)
            printf("Aucune mesure pour cette ressource.\n");
        else {
            printf("\n");
            afficherIndices(t, idx, nb);
            free(idx);
        }
    } else {
        printf("Choix invalide.\n");
    }
}

static void actionAjouter(TableauMesures *t)
{
    char batiment[TAILLE_BATIMENT], ressource[32], date[TAILLE_DATE];
    double valeur, seuil;
    int id;

    lireTexte("Batiment : ", batiment, sizeof(batiment));
    lireTexte("Ressource (electricite/eau/chauffage) : ", ressource, sizeof(ressource));
    Ressource r = texte_vers_ressource(ressource);
    if (r == RESSOURCE_INCONNUE) {
        printf("Ressource inconnue : mesure refusee.\n");
        return;
    }

    lireTexte("Date (AAAA-MM-JJ HH:MM) [vide = maintenant] : ", date, sizeof(date));
    if (date[0] == '\0') {
        time_t now = time(NULL);
        strftime(date, sizeof(date), "%Y-%m-%d %H:%M", localtime(&now));
        printf("Date utilisee : %s\n", date);
    }

    if (!lireReel("Valeur : ", &valeur) || !lireReel("Seuil : ", &seuil)) {
        printf("Nombre invalide : mesure refusee.\n");
        return;
    }

    int code = ajouter_mesure(t, batiment, r, date, valeur, seuil, &id);
    if (code == GEST_OK) {
        majAlertes(t);
        printf("Mesure ajoutee (id = %d).\n", id);
        if (valeur > seuil)
            printf("ALERTE : la valeur depasse le seuil !\n");
    } else if (code == GEST_ERR_PARAM) {
        printf("Donnees incorrectes (valeur >= 0 et seuil > 0 obligatoires) : mesure refusee.\n");
    } else {
        printf("Erreur memoire.\n");
    }
}

static void actionSupprimer(TableauMesures *t)
{
    int id;
    afficher_tableau(t);
    if (!lireEntier("ID de la mesure a supprimer : ", &id)) {
        printf("ID invalide.\n");
        return;
    }
    if (supprimer_mesure(t, id) == GEST_OK)
        printf("Mesure %d supprimee.\n", id);
    else
        printf("Aucune mesure avec l'ID %d.\n", id);
}

static void actionTrier(TableauMesures *t)
{
    int ress, crit, filtre, critere, croissant = 0;

    if (t->taille == 0) {
        printf("Aucune mesure : chargez d'abord un fichier (option 1).\n");
        return;
    }

    /* On ne compare pas des kWh d'electricite avec des m3 d'eau :
     * le classement se fait ressource par ressource. */
    printf("\n  Ressource a classer :\n");
    printf("  1. Electricite\n");
    printf("  2. Eau\n");
    printf("  3. Chauffage\n");
    printf("  4. Toutes (une section par ressource)\n");
    if (!lireEntier("Votre choix : ", &ress) || ress < 1 || ress > 4) {
        printf("Choix invalide.\n");
        return;
    }

    printf("\n  Critere de classement :\n");
    printf("  1. Consommation decroissante\n");
    printf("  2. Consommation croissante\n");
    printf("  3. Date (la plus ancienne d'abord)\n");
    if (!lireEntier("Votre choix : ", &crit) || crit < 1 || crit > 3) {
        printf("Choix invalide.\n");
        return;
    }

    if (crit == 3) {
        critere = CRITERE_DATE;
    } else {
        critere = CRITERE_CONSOMMATION;
        croissant = (crit == 2);
    }
    filtre = (ress == 4) ? -1 : ress - 1;   /* ELECTRICITE=0, EAU=1, CHAUFFAGE=2 */

    trierParRessource(t->donnees, t->taille, critere, croissant);
    majAlertes(t);
    afficherParRessource(t->donnees, t->taille, filtre);
}

static void actionStatistiques(TableauMesures *t)
{
    Stat *stats = NULL;
    majAlertes(t);
    int nb = calculerStatistiques(t->donnees, t->taille, &stats);
    if (nb == 0) {
        printf("Aucune mesure : chargez d'abord un fichier (option 1).\n");
        return;
    }
    printf("\n========== STATISTIQUES ==========\n");
    afficherStatistiques(stats, nb);
    free(stats);
}

static void actionAlertes(TableauMesures *t)
{
    if (t->taille == 0) {
        printf("Aucune mesure : chargez d'abord un fichier (option 1).\n");
        return;
    }
    printf("\n========== DETECTION DES ANOMALIES (par ressource) ==========\n");
    anomaliesParRessource(t->donnees, t->taille);
}

static void actionExporter(TableauMesures *t, const char *source)
{
    Stat *stats = NULL;
    if (t->taille == 0) {
        printf("Aucune mesure : chargez d'abord un fichier (option 1).\n");
        return;
    }
    majAlertes(t);
    int nb = calculerStatistiques(t->donnees, t->taille, &stats);
    exporterVersJSON("data.json", t->donnees, t->taille, stats, nb, source);
    exporterVersCSV("data.csv", t->donnees, t->taille);
    free(stats);
}

/* ------------------------------------------------------------------ */

int main(int argc, char *argv[])
{
    TableauMesures t;
    char source[128] = "saisie_manuelle";
    int choix;

    if (tableau_init(&t) != GEST_OK) {
        fprintf(stderr, "Erreur : memoire insuffisante.\n");
        return 1;
    }

    /* Chargement automatique si un fichier est passe en argument */
    if (argc > 1) {
        int n = chargerMesures(argv[1], &t);
        if (n >= 0) {
            snprintf(source, sizeof(source), "%s", argv[1]);
            majAlertes(&t);
            printf("%d mesure(s) chargee(s) depuis %s.\n", n, argv[1]);
        }
    }

    do {
        afficherMenu();
        if (!lireEntier("Votre choix : ", &choix)) {
            if (feof(stdin))
                break;           /* fin de l'entree : on quitte */
            choix = -1;
        }

        switch (choix) {
            case 1: actionCharger(&t, source, sizeof(source)); break;
            case 2: actionAfficherRechercher(&t);              break;
            case 3: actionAjouter(&t);                         break;
            case 4: actionSupprimer(&t);                       break;
            case 5: actionTrier(&t);                           break;
            case 6: actionStatistiques(&t);                    break;
            case 7: actionAlertes(&t);                         break;
            case 8: actionExporter(&t, source);                break;
            case 0: printf("Au revoir !\n");                   break;
            default: printf("Choix invalide, reessayez.\n");   break;
        }
    } while (choix != 0);

    tableau_liberer(&t);
    return 0;
}
