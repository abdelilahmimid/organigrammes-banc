# firmware_banc : programme de l'Arduino Mega du banc

Programme de la carte du banc (carte2, shield pour Arduino Mega 2560). Il suit la figure 2 et les
figures 3a à 3i des organigrammes de l'équipe, l'organigramme A de la vitesse et l'organigramme de la pente.

## Compiler et téléverser

1. Arduino IDE 2, carte « Arduino Mega or Mega 2560 ».
2. Gestionnaire de bibliothèques : installer « DallasTemperature » (Miles Burton), qui installe aussi « OneWire ».
3. Ouvrir `firmware_banc.ino` (tous les fichiers du dossier s'ouvrent en onglets).
4. Téléverser l'Arduino **seul**, avant de le poser sur la carte du banc.

Taille : environ 21,5 ko de programme (8 %) et 1,8 ko de RAM (22 %).

## Un fichier par bloc

| Fichier | Rôle |
| --- | --- |
| `config.h` | tous les réglages : broches, rythmes, seuils, Kp/Ki, Timer 4 |
| `firmware_banc.ino` | `setup()` et `loop()` (figure 2) |
| `banc_etat` | mode du banc, consignes et défauts partagés |
| `commandes` | commandes du PC (figures 3c, 3d) |
| `vitesse` | vitesse du rouleau sur D2 (figures 3a, 3b) |
| `couple` | couple sur A0 |
| `temperature` | sonde DS18B20 sur D48 (figure 3e) |
| `securite` | arrêt d'urgence D3, sécurité thermique (3f), ventilation (3g) |
| `frein` | pente, frein à hystérésis, freinage de fin, Timer 4 |
| `regulation_vitesse` | PI de vitesse et trame 0x55 (organigramme A) |
| `proto_spi` | liaison SPI avec la voiture |
| `trame_data` | trame DATA vers le PC (figure 3i) |

## Brochage (KiCad carte2)

| Broche | Signal |
| --- | --- |
| D0 / D1 | USB et interface uniquement |
| D2 (INT0) | vitesse du couplemètre, 60 impulsions par tour, optocoupleur U6, pull-down R16 |
| D3 (INT1) | arrêt d'urgence (J5), contact vers GND |
| D6 (OC4A) / D7 (OC4B) | PWMH / PWMHL vers le driver MIC4104 du frein |
| D42 | ventilateurs (relais K1 et K2) |
| D48 | sonde DS18B20 |
| D49 | ENB_PROTO : active U12 (liaison voiture) |
| D50 à D53 | SPI vers la voiture (MISO, MOSI, SCK, SS) |
| A0 | couple du couplemètre (SIG_OUT, filtre R33/C17) |

## Commandes du PC

`PING`, `START`, `PAUSE`, `STOP`, `RESET`, `SET_SPEED:v` (0 à 30 km/h), `SET_SLOPE:p` (0 à la pente max),
`SET_VENT:x`. Un numéro facultatif `#n` transforme `OK` en `ACK#n` et `ERR:raison` en `NACK#n:raison`.

## Trame DATA (toutes les 50 ms)

Clés principales : `torque` (N·m au rouleau), `speed` (tr/min du rouleau), `temp`, `bat_volt`, `bat_curr`,
`mot_volt`, `mot_curr`, `emergency`, `fan_in`, `fan_out`, `slope`, `speed_setpoint`, `speed_cmd` (D envoyé
à la voiture), `brake_pwm`, `brake_torque` (estimé), `brake_setpoint`, `brake_mode` (`TEST`, `END`, `OFF`),
`spi`, `faults`, `status`.

## Réglages à mesurer sur le banc

| Réglage (`config.h`) | Valeur de départ | Comment le trouver |
| --- | --- | --- |
| `RAYON_ROULEAU_M` | 0,078 m | mesure du rouleau (FAT 2.2) |
| `COUPLE_ZERO_V`, `COUPLE_NM_PAR_V` | 2,5 V, 2 N·m/V | tension de SIG_OUT à couple nul, réglage du DYN-200 |
| `KP_VITESSE`, `KI_VITESSE` | 0,04, 0,02 | essai indiciel : Kp = τ / (K × Tbf), Ki = Kp / τ |
| `PENTE_MAX_POURCENT` | 6 % | décision d'équipe (6 ou 7 %) |
| `URGENCE_NIVEAU_ACTIF` | LOW | selon le type de contact (fermé ou ouvert au repos) |
