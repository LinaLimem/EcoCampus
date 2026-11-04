/*
 * export.c - Export des resultats (C9) : data.json et data.csv
 * Travail de l'equipe Digital Minds (partie export).
 * Meme principe que la version d'origine : fopen("w") + fprintf.
 */
#include <stdio.h>
#include <string.h>
#include <time.h>
#include "export.h"
#include "tri_anomalies.h"   /* niveau(), niveauTexte() */

/* Ecrit une chaine entre guillemets en protegeant " et \ (JSON valide). */
static void ecrireTexte(FILE *f, const char *s)
{
    fputc('"', f);
    for (; *s; s++) {
        if (*s == '"' || *s == '\\')
            fprintf(f, "\\%c", *s);
        else if ((unsigned char)*s < 0x20)
            fprintf(f, " ");
        else
            fputc(*s, f);
    }
    fputc('"', f);
}

/* Format de la fiche : "AAAA-MM-JJ hh:mm". Si le CSV ne donne que la date,
 * on complete avec " 00:00". */
static void dateHeure(const char *date, char *sortie, size_t taille)
{
    if (strlen(date) == 10)
        snprintf(sortie, taille, "%s 00:00", date);
    else
        snprintf(sortie, taille, "%s", date);
}

int exporterVersJSON(const char *nomFichier, const Mesure mesures[], int taille,
                     const Stat stats[], int nbStats, const char *source)
{
    int i, nbAlertes = 0, nbProches = 0;
    char horodatage[32], dh[40];
    time_t maintenant = time(NULL);

    FILE *f = fopen(nomFichier, "w");
    if (f == NULL) {
        perror("Erreur lors de l'ouverture du fichier JSON pour l'export");
        return -1;
    }

    for (i = 0; i < taille; i++) {
        if (mesures[i].alerte)
            nbAlertes++;
        else if (niveau(&mesures[i]) == NIVEAU_PROCHE)
            nbProches++;
    }
    strftime(horodatage, sizeof(horodatage), "%Y-%m-%d %H:%M", localtime(&maintenant));

    fprintf(f, "{\n");

    /* --- bloc meta --- */
    fprintf(f, "  \"meta\": {\n");
    fprintf(f, "    \"version\": \"1.0\",\n");
    fprintf(f, "    \"genere_le\": \"%s\",\n", horodatage);
    fprintf(f, "    \"source\": ");
    ecrireTexte(f, source);
    fprintf(f, ",\n");
    fprintf(f, "    \"nb_mesures\": %d,\n", taille);
    fprintf(f, "    \"nb_alertes\": %d,\n", nbAlertes);
    fprintf(f, "    \"nb_proches_seuil\": %d\n", nbProches);
    fprintf(f, "  },\n");

    /* --- bloc mesures --- */
    fprintf(f, "  \"mesures\": [\n");
    for (i = 0; i < taille; i++) {
        dateHeure(mesures[i].date, dh, sizeof(dh));
        fprintf(f, "    {\n");
        fprintf(f, "      \"id_batiment\": %d,\n",
                rangBatiment(mesures, taille, mesures[i].batiment));
        fprintf(f, "      \"nom_batiment\": ");
        ecrireTexte(f, mesures[i].batiment);
        fprintf(f, ",\n");
        fprintf(f, "      \"type_ressource\": \"%s\",\n",
                ressource_vers_texte(mesures[i].ressource));
        fprintf(f, "      \"valeur_consommation\": %.2f,\n", mesures[i].valeur);
        fprintf(f, "      \"unite\": \"%s\",\n", uniteRessource(mesures[i].ressource));
        fprintf(f, "      \"date_heure\": \"%s\",\n", dh);
        fprintf(f, "      \"seuil_reference\": %.2f,\n", mesures[i].seuil);
        fprintf(f, "      \"alerte\": %s,\n", mesures[i].alerte ? "true" : "false");
        fprintf(f, "      \"niveau\": \"%s\"\n", niveauTexte(niveau(&mesures[i])));
        if (i < taille - 1) {
            fprintf(f, "    },\n");
        } else {
            fprintf(f, "    }\n");
        }
    }
    fprintf(f, "  ],\n");

    /* --- bloc statistiques --- */
    fprintf(f, "  \"statistiques\": [\n");
    for (i = 0; i < nbStats; i++) {
        fprintf(f, "    {\n");
        fprintf(f, "      \"id_batiment\": %d,\n", stats[i].id_batiment);
        fprintf(f, "      \"type_ressource\": \"%s\",\n",
                ressource_vers_texte(stats[i].ressource));
        fprintf(f, "      \"total\": %.2f,\n", stats[i].total);
        fprintf(f, "      \"moyenne\": %.2f,\n", stats[i].moyenne);
        fprintf(f, "      \"minimum\": %.2f,\n", stats[i].minimum);
        fprintf(f, "      \"maximum\": %.2f,\n", stats[i].maximum);
        fprintf(f, "      \"nb_alertes\": %d\n", stats[i].nb_alertes);
        if (i < nbStats - 1) {
            fprintf(f, "    },\n");
        } else {
            fprintf(f, "    }\n");
        }
    }
    fprintf(f, "  ]\n");
    fprintf(f, "}\n");

    fclose(f);
    printf("Fichier '%s' genere avec succes !\n", nomFichier);
    return 0;
}

int exporterVersCSV(const char *nomFichier, const Mesure mesures[], int taille)
{
    FILE *f = fopen(nomFichier, "w");
    if (f == NULL) {
        perror("Erreur lors de l'ouverture du fichier CSV pour l'export");
        return -1;
    }
    fprintf(f, "id,batiment,ressource,valeur,date,seuil,alerte,niveau\n");
    for (int i = 0; i < taille; i++) {
        fprintf(f, "%d,%s,%s,%.2f,%s,%.2f,%d,%s\n",
                mesures[i].id,
                mesures[i].batiment,
                ressource_vers_texte(mesures[i].ressource),
                mesures[i].valeur,
                mesures[i].date,
                mesures[i].seuil,
                mesures[i].alerte,
                niveauTexte(niveau(&mesures[i])));
    }
    fclose(f);
    printf("Fichier '%s' genere avec succes !\n", nomFichier);
    return 0;
}
