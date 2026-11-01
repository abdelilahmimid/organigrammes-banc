# Tests du programme du dsPIC de la voiture (voiture_spi) avec le PICkit 3

## Prompt à coller dans la session le jour des tests
```
Contexte : je suis étudiant en Master 2 à l'Hénallux (Pierrard), projet « banc d'essai Eco-Shell »,
responsable de l'électronique. Aujourd'hui, au labo, je programme et je teste le dsPIC33CK32MP105
de la carte de la voiture avec le PICkit 3 (programme voiture_spi). Lis dans le projet
« claude/Tests_programme_dsPIC_voiture.md » et « claude/Programme_dsPIC_decisions.md », puis
guide-moi étape par étape, dans l'ordre. Explique simplement, en français. On ne passe à l'étape
suivante que quand je t'ai donné le résultat (mesure, message de MPLAB, photo).
Si un résultat n'est pas celui attendu, aide-moi à trouver la cause avant de continuer.
```

## Règles avant tout
- À faire **au labo, avec l'accord du prof** : c'est la carte qui fait rouler la voiture (carte de Sacha).
- **Moteur débranché** pendant tous les tests, sauf la dernière étape, faite avec le prof.
- Alimentation de laboratoire **avec limitation de courant**, à la place de la batterie 48 V.
- **Sauvegarder le programme d'origine du dsPIC avant de programmer le nôtre** (étape 5).
- Ne jamais relier directement le Mega (5 V) au dsPIC (3,3 V) : toujours passer par U12 de la carte
  du banc ou par un module de conversion de niveaux 5 V ↔ 3,3 V.

## Matériel et logiciels
- Carte de la voiture, PICkit 3 + câble mini-USB, câble vers J16
- PC avec MPLAB X IDE 6.20, MPLAB IPE 6.20, XC-DSC v4.00, pack dsPIC33CK-MP_DFP 1.13.366
- Projet `voiture_spi` (fichiers dans `programmes/voiture_spi/`), fichier à envoyer :
  `dist/default/production/voiture_spi.X.production.hex`
- Alimentation de laboratoire (tension et limite de courant décidées avec le prof), multimètre, oscilloscope
- Pour le test SPI : Arduino Mega avec firmware_banc v3, et la carte du banc montée (U12) ou un module
  de conversion de niveaux ; câble J2 avec les fils 3 et 4 croisés (réglage `SPI_INVERSER_SDI_SDO = 0`)

## Repères sur la carte
| Repère | Rôle |
| --- | --- |
| J16 | programmation : 1 MCLR, 2 +3,3 V, 3 GND, 4 PGED, 5 PGEC, 6 libre |
| Broche 1 du dsPIC (RB14 / PWM1H) | commande du MOSFET du haut Q2 |
| Broche 2 du dsPIC (RB15 / PWM1L) | commande du MOSFET du bas Q3 |
| Broche 20 (AN15) | tension batterie / 16 (pont R7 / R8) |
| J2 | liaison SPI vers le banc (SCK broche 38, SS 39, SDI 40, SDO 41) |
| J1 (XT60) | entrée batterie |

## Étapes
| # | Action | Résultat attendu | OK ? |
| --- | --- | --- | --- |
| 1 | Photos recto / verso et de chaque connecteur avant de débrancher quoi que ce soit ; étiqueter les fils | photos rangées dans OneNote | |
| 2 | Vérifier à la loupe les composants soudés : U4 (dsPIC), U1 / U2 (régulateurs), U6 (driver), Q2 / Q3, C27, J16, J2, R56 à R59 | tous présents, pas de pont d'étain | |
| 3 | Hors tension, au multimètre : 3,3 V ↔ GND, 12 V ↔ GND, VBUS ↔ GND | pas de court-circuit (la valeur peut monter doucement : condensateurs) | |
| 4 | Moteur débranché, alimentation de labo sur J1 (tension et limite décidées avec le prof), allumer | courant faible et stable ; 12 V et 3,3 V présents sur la carte | |
| 5 | **Sauvegarde** : MPLAB IPE 6.20, composant dsPIC33CK32MP105, outil PICkit 3, Connect (alimentation par la carte, pas par le PICkit), puis Read, puis File → Export → Hex | fichier `voiture_original_AAAAMMJJ.hex` enregistré sur le PC et sur OneDrive. Si la lecture ne donne que des 0 ou des FFFF : programme protégé, s'arrêter et demander au prof le fichier d'origine | |
| 6 | MPLAB X 6.20 : ouvrir voiture_spi, Production → Build Project | BUILD SUCCESSFUL | |
| 7 | IPE : charger `voiture_spi.X.production.hex`, Program, puis Verify | « Programming complete » puis « Verify complete » | |
| 8 | Carte seule, sans liaison avec le banc : multimètre en tension continue sur les broches 1 et 2 du dsPIC | 0 V et 0 V : sans trame du banc, le moteur reste coupé (Q2 et Q3 bloqués) | |
| 9 | Relier J2 au banc (via U12 ou module de niveaux, fils 3 et 4 croisés ; sans la carte du banc, mettre aussi 10 kΩ de D2 vers GND sur le Mega). Sur le Mega : `START` | trame DATA du Mega : `spi=OK`, `bat_volt` ≈ tension de l'alimentation de labo | |
| 10 | Sur le Mega : `SET_SPEED:10` (pas de couplemètre : la vitesse mesurée reste 0) | le PI du banc monte `speed_cmd` jusqu'à 0,950 ; broche 1 du dsPIC ≈ 3,3 V × 0,95 ≈ 3,1 V au multimètre ; `mot_volt` ≈ 0,95 × tension d'alimentation | |
| 11 | Sur le Mega : `STOP` | broche 1 → 0 V en moins de 100 ms (la voiture reçoit état = 0) | |
| 12 | `START` + `SET_SPEED:10`, puis débrancher le fil SCK | broche 1 → 0 V après 500 ms (sécurité de la voiture) ; rebrancher : la commande revient | |
| 13 | Sur le Mega : `RESET` (D49 = 0, U12 isole les cartes) | broche 1 → 0 V après 500 ms | |
| 14 | Oscilloscope sur les broches 1 et 2 (`START`, `SET_SPEED:10`) | deux signaux 0 / 3,3 V complémentaires à 20 kHz (période 50 µs) ; un trou d'environ 200 ns où les deux sont à 0 (temps mort) ; broche 1 à 1 pendant ≈ 47,5 µs pour D = 0,95 | |
| 15 | Oscilloscope sur les grilles de Q2 et Q3 (sorties du driver U6), moteur toujours débranché | mêmes signaux amplifiés, jamais les deux grilles hautes en même temps | |
| 16 | **Avec le prof seulement** : moteur branché, alimentation limitée | voir la remarque ci-dessous avant de lancer | |
| 17 | Si la voiture doit rouler à nouveau sur piste : IPE, charger `voiture_original_AAAAMMJJ.hex`, Program, Verify | la voiture retrouve son programme d'origine (accélérateur) | |

## Remarque importante pour l'étape 16
Sans couplemètre (voiture hors du banc), la vitesse mesurée par le banc reste à 0. Le PI du banc
monte alors D jusqu'à 0,95 : le moteur partirait presque à fond. Le premier essai moteur se fait donc
soit sur le banc avec le couplemètre branché, soit avec un programme de test qui envoie un petit D fixe.
Décider avec le prof avant de brancher le moteur.

## Résultats et remarques
Noter ici les valeurs lues, les messages de MPLAB, les écarts et les questions pour l'équipe.
