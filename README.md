# EcoCampus - Module C (Digital Minds)

Lit un fichier CSV de mesures, detecte les anomalies, trie, calcule les
statistiques et genere `data.json` pour le module Web.

## Fichiers
| Fichier | Role |
|---|---|
| `gestion.h` / `gestion.c` | Structures `Mesure` et tableau dynamique ; ajout, recherche, suppression |
| `chargement.h` / `chargement.c` | Chargement de `measures_in.csv` (gestion des lignes invalides) |
| `tri_anomalies.h` / `tri_anomalies.c` | Tri a bulles, detection des anomalies, classement et alertes par ressource |
| `statistiques.h` / `statistiques.c` | Total, moyenne, min, max, alertes par batiment et ressource |
| `export.h` / `export.c` | Ecriture de `data.json` (format seance 1) et `data.csv` |
| `main.c` | Menu interactif (options 1 a 8, 0 pour quitter) |
| `test_q1.c`, `test_q3.c` | Programmes de test (ajout/recherche/suppression ; tri/anomalies) |
| `measures_in.csv` | Jeu de donnees fourni (5 mesures) |
| `measures_test.csv` | Jeu de test : 18 mesures valides (7 anomalies, 1 cas valeur = seuil) + 3 lignes invalides |

## Compilation (Windows, terminal de VS Code, dans le dossier du projet)
    gcc main.c gestion.c chargement.c tri_anomalies.c statistiques.c export.c -o EcoCampus.exe

## Execution
    .\EcoCampus.exe                      (menu ; option 1 pour charger un CSV)
    .\EcoCampus.exe measures_test.csv    (charge le fichier au demarrage, puis menu)

## Menu
| Option | Action |
|---|---|
| 1 | Charger un fichier CSV |
| 2 | Afficher toutes les mesures / rechercher par batiment ou par ressource |
| 3 | Ajouter une mesure (le programme demande batiment, ressource, date, valeur, seuil) |
| 4 | Supprimer une mesure (par son ID) |
| 5 | Classer les mesures **par ressource** (electricite, eau, chauffage ou toutes) |
| 6 | Statistiques par batiment et ressource |
| 7 | Afficher les alertes **par ressource** |
| 8 | Exporter `data.json` (et `data.csv`) |
| 0 | Quitter |

Le classement (option 5) ne melange jamais les ressources : on ne compare pas des kWh
d'electricite avec des m3 d'eau. Les mesures sont regroupees par ressource, puis classees
dans chaque groupe par consommation (croissante/decroissante) ou par date.
Les alertes (option 7) sont aussi presentees ressource par ressource, avec un total pour chacune.

Le fichier CSV d'entree doit se trouver dans le dossier courant.
En-tete attendu : `id,batiment,ressource,valeur,date,seuil`.

## Tests unitaires
    gcc test_q1.c gestion.c -o test_q1.exe
    gcc test_q3.c gestion.c tri_anomalies.c -o test_q3.exe

## Regles
- Anomalie : `valeur > seuil` (strict) -> `"alerte": true`.
- Statut affiche dans les tableaux : `normal`, `PROCHE` (avertissement) ou `ALERTE`.
- Avertissement "proche du seuil" : `valeur <= seuil` mais `valeur >= 90 %` du seuil
  (donc aussi `valeur == seuil`). Ce n'est pas une alerte, mais un message
  "Attention ..." s'affiche et `"niveau": "proche_seuil"` est ecrit dans data.json.
  Le pourcentage se change dans `gestion.h` (`POURCENT_PROCHE`, 100 = egale au seuil seulement).
- Champ `niveau` de chaque mesure : `normal`, `proche_seuil` ou `alerte`.
  `meta.nb_proches_seuil` donne le nombre d'avertissements.
- Une ligne invalide (champ manquant, nombre incorrect, ressource inconnue,
  valeur negative, seuil <= 0) est signalee puis ignoree ; le programme continue.
- Fichier absent : message d'erreur explicite, code de retour 1.
- `id_batiment` = rang alphabetique du nom du batiment (le CSV ne fournit pas cet identifiant).
- `date_heure` : si le CSV ne donne que la date, " 00:00" est ajoute.
