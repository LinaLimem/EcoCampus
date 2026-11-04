/*
 * chargement.c - Phase 2 : chargement du fichier measures_in.csv
 * Travail de l'equipe Digital Minds (partie chargement CSV).
 * Meme principe que la version d'origine : fopen, fgets, sscanf, fclose.
 */
#include <stdio.h>
#include <string.h>
#include "chargement.h"

int chargerMesures(const char *nomFichier, TableauMesures *t)
{
    FILE *fichier = fopen(nomFichier, "r");

    if (fichier == NULL) {
        printf("Erreur : impossible d'ouvrir %s\n", nomFichier);
        return -1;
    }

    char ligne[256];
    int nombre = 0;
    int numeroLigne = 0;

    /* Ignorer la ligne d'en-tete du CSV */
    if (fgets(ligne, sizeof(ligne), fichier) == NULL) {
        fclose(fichier);
        return 0;
    }
    numeroLigne = 1;

    while (fgets(ligne, sizeof(ligne), fichier) != NULL) {
        numeroLigne++;

        /* Ligne vide : on l'ignore sans message */
        if (ligne[0] == '\n' || ligne[0] == '\r' || ligne[0] == '\0')
            continue;

        int id;
        char batiment[50];
        char ressource[30];
        double valeur, seuil;
        char date[20];

        int champs = sscanf(
            ligne,
            "%d,%49[^,],%29[^,],%lf,%19[^,],%lf",
            &id,
            batiment,
            ressource,
            &valeur,
            date,
            &seuil
        );

        if (champs != 6) {
            printf("Ligne %d invalide : %s", numeroLigne, ligne);
            if (strchr(ligne, '\n') == NULL)
                printf("\n");
            continue;
        }

        /* Controle des valeurs (ressource connue, valeur >= 0, seuil > 0) */
        Ressource r = texte_vers_ressource(ressource);
        if (r == RESSOURCE_INCONNUE ||
            ajouter_mesure(t, batiment, r, date, valeur, seuil, NULL) != GEST_OK) {
            printf("Ligne %d refusee (donnee incorrecte) : %s", numeroLigne, ligne);
            if (strchr(ligne, '\n') == NULL)
                printf("\n");
            continue;
        }

        nombre++;
    }

    fclose(fichier);
    return nombre;
}

void afficherMesures(const TableauMesures *t)
{
    printf("\n========== MESURES CHARGEES ==========\n");

    for (int i = 0; i < t->taille; i++) {
        printf("\nID : %d\n", t->donnees[i].id);
        printf("Batiment : %s\n", t->donnees[i].batiment);
        printf("Ressource : %s\n", ressource_vers_texte(t->donnees[i].ressource));
        printf("Valeur : %.2f\n", t->donnees[i].valeur);
        printf("Date : %s\n", t->donnees[i].date);
        printf("Seuil : %.2f\n", t->donnees[i].seuil);
    }

    printf("\nNombre de mesures chargees : %d\n", t->taille);
    printf("Chargement termine !\n");
}
