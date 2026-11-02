/*
 * test_q3.c - Test de la question 3 (tri par date + detection des anomalies)
 * Memes trois mesures que dans le compte rendu : B1, B2, B3.
 * Compilation : gcc test_q3.c gestion.c tri_anomalies.c -o test_q3
 */
#include <stdio.h>
#include "gestion.h"
#include "tri_anomalies.h"

int main(void)
{
    TableauMesures t;

    if (tableau_init(&t) != GEST_OK) {
        fprintf(stderr, "Erreur : memoire insuffisante.\n");
        return 1;
    }

    ajouter_mesure(&t, "B1", ELECTRICITE, "2026-10-15", 80, 100, NULL);
    ajouter_mesure(&t, "B2", EAU,         "2026-10-10", 70, 100, NULL);
    ajouter_mesure(&t, "B3", CHAUFFAGE,   "2026-10-08", 150, 120, NULL);
    /* Cas limite : valeur == seuil -> pas une alerte (">" strict),
     * mais un avertissement "proche du seuil" */
    ajouter_mesure(&t, "B4", EAU,         "2026-10-12", 100, 100, NULL);
    /* 95 % du seuil -> avertissement aussi */
    ajouter_mesure(&t, "B5", CHAUFFAGE,   "2026-10-13", 95, 100, NULL);

    printf("Mesures de depart :\n");
    afficher(t.donnees, t.taille);

    trier(t.donnees, t.taille);

    printf("\nMesures triees par date :\n");
    afficher(t.donnees, t.taille);

    printf("\nMesures triees par consommation decroissante :\n");
    trierParConsommation(t.donnees, t.taille, 0);
    afficher(t.donnees, t.taille);

    printf("\nDetection des anomalies :\n");
    anomalies(t.donnees, t.taille);

    tableau_liberer(&t);
    return 0;
}
