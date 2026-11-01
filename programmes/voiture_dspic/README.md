# voiture_spi : programme du dsPIC de la voiture

Programme du dsPIC33CK32MP105 de la carte de la voiture (DriverShellEco). Il reçoit D du banc par SPI
(trame 0x55), l'applique au moteur par le demi-pont Q2/Q3 (PWM 20 kHz, temps mort 200 ns) et renvoie
la tension et les courants (trame 0xAA). La régulation de vitesse est faite par le banc.

## Contenu attendu de ce dossier

```
voiture_dspic/
├── README.md
└── voiture_spi.X/          projet MPLAB X complet
    ├── nbproject/          configurations.xml, project.xml (pas le dossier private/)
    ├── Makefile
    ├── carte_voiture.h     broches, horloge, PWM, mesures
    ├── config_bits.c       bits de configuration (FRC, PGD3, JTAG coupé)
    ├── horloge.c / .h      PLL (FCY = 50 MHz) et base de temps 1 ms
    ├── spi_esclave.c / .h  SPI1 esclave, trame acceptée seulement si 10 octets
    ├── trame.c / .h        lecture de 0x55, construction de 0xAA, sécurités
    ├── pwm.c / .h          PWM1 complémentaire, blocage de Q2 et Q3
    ├── mesures.c / .h      CAN, OA1, U5 (SEL), zéro du courant moteur
    └── main.c              boucle de l'organigramme B
```

Les dossiers `build/`, `dist/` et `nbproject/private/` sont ignorés par Git (voir `.gitignore`).

## Compiler et programmer

1. MPLAB X IDE 6.20 (dernière version qui gère le PICkit 3), compilateur XC-DSC v4.00,
   pack dsPIC33CK-MP_DFP **1.13.366** (dans `nbproject/configurations.xml`).
2. Production → Build Project → `dist/default/production/voiture_spi.X.production.hex`.
3. MPLAB IPE 6.20 + PICkit 3 sur J16 (1 MCLR, 2 +3,3 V, 3 GND, 4 PGED, 5 PGEC).
4. **Avant la première programmation : sauvegarder le programme d'origine du dsPIC** (Read, puis Export Hex).

Taille : 4 197 octets de programme (11 %), 92 octets de données (1 %).

## Sécurités

- Démarrage : broches PWM en sortie à 0 avant tout réglage.
- Moteur coupé (Q2 et Q3 bloqués) si : aucune trame reçue, plus de trame valide depuis 500 ms, ou état = 0.
- D limité à 0,95 (recharge du condensateur de bootstrap C27).

Voir `docs/tests/Tests_programme_dsPIC_voiture.md` pour les tests au labo.
