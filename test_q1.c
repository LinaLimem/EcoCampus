/* Petit programme de test du TAF 1 (ajout, recherche, suppression). */
#include <stdio.h>
#include <stdlib.h>
#include "gestion.h"

int main(void)
{
    TableauMesures t;
    int id, nb, i, *idx;

    if (tableau_init(&t) != GEST_OK) {
        fprintf(stderr, "Erreur : memoire insuffisante.\n");
        return 1;
    }

    ajouter_mesure(&t, "Bibliotheque", ELECTRICITE, "2026-10-01 08:00", 320.5, 300.0, &id);
    ajouter_mesure(&t, "Laboratoire",  EAU,          "2026-10-01 08:00",  45.2,  60.0, NULL);
    ajouter_mesure(&t, "Residence",    CHAUFFAGE,    "2026-10-01 09:00", 510.0, 450.0, NULL);
    ajouter_mesure(&t, "Bibliotheque", CHAUFFAGE,    "2026-10-01 09:00", 280.0, 400.0, NULL);
    printf("== Apres 4 ajouts ==\n");
    afficher_tableau(&t);

    printf("\n== Ajout invalide (valeur negative) -> code %d ==\n",
           ajouter_mesure(&t, "Residence", EAU, "2026-10-01 10:00", -5.0, 50.0, NULL));

    printf("\n== Recherche : batiment Bibliotheque ==\n");
    idx = rechercher_par_batiment(&t, "bibliotheque", &nb);
    for (i = 0; i < nb; i++) afficher_mesure(&t.donnees[idx[i]]);
    free(idx);

    printf("\n== Recherche : ressource chauffage ==\n");
    idx = rechercher_par_ressource(&t, CHAUFFAGE, &nb);
    for (i = 0; i < nb; i++) afficher_mesure(&t.donnees[idx[i]]);
    free(idx);

    printf("\n== Suppression de l'id %d -> code %d ==\n", id, supprimer_mesure(&t, id));
    printf("== Suppression d'un id inexistant (99) -> code %d ==\n", supprimer_mesure(&t, 99));
    afficher_tableau(&t);

    printf("\n== Test d'agrandissement (30 ajouts) ==\n");
    for (i = 0; i < 30; i++)
        ajouter_mesure(&t, "Administration", EAU, "2026-10-02 08:00", 10.0 + i, 50.0, NULL);
    printf("taille = %d, capacite = %d\n", t.taille, t.capacite);

    tableau_liberer(&t);
    return 0;
}
