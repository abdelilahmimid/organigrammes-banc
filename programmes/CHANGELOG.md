# Historique des versions

## firmware_banc v3 (01/11/2026)
- Nouvelle clé `brake_mode` dans la trame DATA : `TEST` (essai), `END` (freinage de fin), `OFF` (frein coupé).

## firmware_banc v2 (01/11/2026)
- D2 en `INPUT` (au lieu de `INPUT_PULLUP`), comptage sur front montant : la carte a déjà R16 (10 kΩ vers GND).
- Ventilateurs pilotés par la température seule (marche à 60 °C, arrêt sous 50 °C), même frein coupé ;
  température inconnue = ventilateurs en marche (sauf les 3 s après le démarrage).
- Freinage de fin après STOP : rampe jusqu'à 1,3 N·m au volant, anticipation seule, fin quand le rouleau
  est arrêté ou après 30 s, frein coupé en cas de défaut ou d'arrêt d'urgence.

## firmware_banc v1 (01/11/2026)
- Première version complète : commandes du PC, vitesse, couple, DS18B20, sécurités, ventilation, frein
  (pente, anticipation + PI, anti-emballement, BRAKE_SAT), PI de vitesse, SPI, trame DATA non bloquante.

## simulateur_voiture_uno v3 (01/11/2026)
- Voiture virtuelle : SPI esclave, vitesse calculée à partir de D et de la pente (`PENTE:x`),
  60 impulsions par tour sur D9 (Timer 1, sans interruption).

## voiture_spi (25/10/2026)
- Première version : SPI esclave, sécurités 500 ms / état / D ≤ 0,95, PWM 20 kHz avec temps mort de 200 ns,
  mesures de la tension batterie et des deux courants. Compilée sous MPLAB X 6.20 + XC-DSC v4.00.
