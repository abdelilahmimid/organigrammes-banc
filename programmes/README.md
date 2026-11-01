# Banc d'essai Eco-Shell : programmes

Programmes du banc d'essai Eco-Shell (Hénallux, École d'ingénieurs de Pierrard, Master 2) : la carte
du banc (Arduino Mega), la carte de la voiture (dsPIC33CK) et l'outil de test sur maquette (Arduino Uno).

Les organigrammes de ces programmes sont publiés sur le site de l'équipe :
https://abdelilahmimid.github.io/organigrammes-banc/

## Contenu du dépôt

```
banc-eco-shell-programmes/
├── banc_mega/firmware_banc/              Programme de l'Arduino Mega du banc (carte carte2)
├── voiture_dspic/                        Programme du dsPIC33CK32MP105 de la voiture (MPLAB X)
├── outils_test/simulateur_voiture_uno_v3/ Arduino Uno qui joue la voiture pour tester le banc
├── docs/tests/                           Fiches de tests (maquette, dsPIC)
├── CHANGELOG.md                          Historique des versions
└── README.md                             Ce fichier
```

## Vue d'ensemble

```
Interface PC ──USB 115 200 bauds──► Arduino Mega (banc) ──SPI 250 kHz, trames de 10 octets──► dsPIC (voiture)
                                     │  PI de vitesse, frein (Timer 4), DS18B20,          │  applique D au moteur,
                                     │  ventilateurs, arrêt d'urgence, trame DATA 50 ms   │  mesure tension et courants
                                     ◄── couplemètre DYN-200 : vitesse (D2), couple (A0)  │
```

| Programme | Carte | Outil | Dossier |
| --- | --- | --- | --- |
| firmware_banc | Arduino Mega 2560 sur la carte du banc (carte2) | Arduino IDE 2 + bibliothèque DallasTemperature | `banc_mega/firmware_banc/` |
| voiture_spi | carte de la voiture, dsPIC33CK32MP105 | MPLAB X 6.20, XC-DSC v4.00, PICkit 3 | `voiture_dspic/` |
| simulateur_voiture_uno_v3 | Arduino Uno (maquette) | Arduino IDE 2 | `outils_test/simulateur_voiture_uno_v3/` |

## Liaison SPI banc ↔ voiture

Le Mega est maître, le dsPIC esclave. Mode 0, 250 kHz, un échange de 10 octets toutes les 100 ms.
Les niveaux 5 V ↔ 3,3 V passent par U12 (TXS0108E) de la carte du banc, activé par D49.

Trame 0x55 (banc → voiture) :

| Octet | Contenu |
| --- | --- |
| 0 | en-tête 0x55 |
| 1-2 | consigne de vitesse en dixièmes de km/h |
| 3-4 | couple en mN·m (non utilisé, 0) |
| 5 | état : 1 = rouler, 0 = arrêt |
| 6-7 | D en pour mille (0 à 950) |
| 8 | réservé (0) |
| 9 | XOR des octets 0 à 8 |

Trame 0xAA (voiture → banc) : en-tête 0xAA, tension batterie (mV), courant batterie (mA, signé),
tension moteur (mV), courant moteur (mA, signé), XOR des octets 0 à 8.

Sécurité côté voiture : sans trame valide pendant 500 ms, ou avec état = 0, le moteur est coupé
(les deux MOSFET bloqués).

## Règles de sécurité de la carte du banc

- D0/D1 réservés à l'USB et à l'interface : U3 (MAX485) n'est pas soudé.
- D6/D7 pilotées uniquement par le Timer 4 : jamais de `digitalWrite()` ni `analogWrite()`.
- Frein coupé = `OCR4A = 0` et `OCR4B = 400` (les deux transistors bloqués).
- Frein autorisé seulement si : essai en cours ou freinage de fin, pas de défaut thermique,
  pas d'arrêt d'urgence, température mesurée il y a moins de 2,5 s.
- Ne jamais relier directement le Mega (5 V) au dsPIC (3,3 V).

## Règle de travail

- La branche `main` contient toujours une version qui compile.
- Une modification = une branche (par exemple `frein-rampe`), puis une pull request relue par un autre membre.
- Chaque pull request dit ce qui a changé, pourquoi, et comment ça a été testé.
- Les réglages (broches, seuils, Kp/Ki) se changent uniquement dans `config.h` (banc) ou `carte_voiture.h` (voiture).
- Une nouvelle version est notée dans `CHANGELOG.md`.

## Équipe

- [@abdelilahmimid](https://github.com/abdelilahmimid)
- [@ighmorimane](https://github.com/ighmorimane)
- [@mohamedhenallu](https://github.com/mohamedhenallu)
