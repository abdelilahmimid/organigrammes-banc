# Tests du programme Arduino du banc (firmware_banc v3) sur maquette

## Prompt à coller dans la session le jour des tests
```
Contexte : je suis étudiant en Master 2 à l'Hénallux (Pierrard), projet « banc d'essai Eco-Shell »,
responsable de l'électronique. Aujourd'hui je teste le programme Arduino du banc (firmware_banc v3)
sur maquette, sans la carte du banc. Lis dans le projet « claude/Tests_firmware_banc_Arduino.md »
et guide-moi test par test, dans l'ordre. Explique simplement, en français. On ne passe au test
suivant que quand je t'ai donné ce que je vois (lignes DATA du Mega, affichage de l'Uno).
Si un résultat n'est pas celui attendu, aide-moi à trouver la cause avant de continuer.
```

## Ce qu'on teste
Le programme du Mega (`programmes/firmware_banc/`, v3) avec un Uno qui joue la voiture
(`programmes/simulateur_voiture_uno_v3.ino`). L'Uno reçoit D par SPI, calcule une vitesse et envoie
des impulsions sur D2 du Mega, comme le couplemètre. Tout se lit dans les lignes DATA du Mega :
pas besoin de LED. Ce qui demande l'oscilloscope (signaux D6/D7 et temps mort de 0,5 µs) se fera à l'école.

## Matériel
- Arduino Mega + Arduino Uno, 2 câbles USB, platine d'essai, fils
- Sonde DS18B20 + résistance 4,7 kΩ (indispensable : sans sonde, le frein ne s'active jamais)
- Une résistance de 10 kΩ (D2 vers GND, comme R16 sur la carte)
- Deux résistances égales (par exemple 2 × 10 kΩ) pour faire 2,5 V sur A0
- Multimètre si possible (tension continue sur D6 ≈ 5 V × α)

## Programmes
1. Mega : ouvrir `firmware_banc.ino` (dossier firmware_banc v3 complet). Installer la bibliothèque
   « DallasTemperature » (Miles Burton), qui installe aussi « OneWire ». Carte « Arduino Mega 2560 ».
2. Uno : ouvrir `simulateur_voiture_uno_v3.ino`. Carte « Arduino Uno ».
3. Deux fenêtres Arduino IDE, une par carte, chacune avec son moniteur série à 115 200 bauds,
   fin de ligne « Nouvelle ligne » (Mega sur COM5, Uno sur COM6 lors du test d'octobre).

## Câblage
| Mega | Uno / composant | Rôle |
| --- | --- | --- |
| D53 | D10 | SS |
| D51 | D11 | MOSI |
| D50 | D12 | MISO |
| D52 | D13 | SCK |
| GND | GND | masse commune (ne pas relier les 5 V) |
| D2 | D9 de l'Uno + 10 kΩ vers GND | impulsions de vitesse (60 par tour) |
| D48 | fil de données de la DS18B20 + 4,7 kΩ vers 5 V | température |
| 5 V / GND | alimentation de la DS18B20 | |
| A0 | point milieu des 2 résistances entre 5 V et GND (2,5 V) | couple = 0 |
| D3 | libre (un fil vers GND = arrêt d'urgence) | arrêt d'urgence |

Valeurs de A0 possibles sans potentiomètre :
| A0 relié à | Tension | `torque` attendu (N·m au rouleau) |
| --- | --- | --- |
| GND | 0 V | −5,000 |
| point milieu des 2 résistances | 2,5 V | environ 0,000 |
| broche 3,3 V | 3,3 V | environ 1,6 |
| broche 5 V | 5 V | environ 5,000 |

## Comment lire une ligne DATA
Une ligne toutes les 50 ms, par exemple :
`DATA;seq=…;torque=0.000;speed=1020.0;temp=21.5;…;slope=0.0;speed_setpoint=30.0;speed_cmd=0.667;brake_pwm=0;brake_torque=0.000;brake_setpoint=0.000;brake_mode=OFF;rs485=OFF;spi=OK;faults=NONE;status=RUNNING`
- `speed` est en tr/min du rouleau : 30 km/h = 1 020 tr/min (× 0,0294)
- `speed_cmd` = D envoyé à la voiture ; `brake_pwm` = α × 255 ; `brake_mode` = TEST / END / OFF

## Tests dans l'ordre
| # | Action | Résultat attendu | OK ? |
| --- | --- | --- | --- |
| 1 | Brancher le Mega, ouvrir son moniteur | `READY`, puis une ligne DATA toutes les 50 ms ; `status=IDLE`, `spi=OFF`, `brake_mode=OFF` | |
| 2 | Taper `PING` | `PONG` | |
| 3 | Regarder `temp` | température de la pièce ; `fan_in=0` | |
| 4 | Débrancher le fil de données de la sonde | après 1 à 3 s : `temp=-127.0` ou clé `temp` absente, et `fan_in=1` (température inconnue = on refroidit) ; rebrancher : retour normal, `fan_in=0` | |
| 5 | A0 sur 2,5 V, puis 5 V, puis GND | `torque` ≈ 0, puis ≈ 5, puis ≈ −5 ; remettre 2,5 V | |
| 6 | Taper `START` | `OK`, `status=RUNNING`, `spi=OK`, `bat_volt=48.000` ; l'Uno affiche `etat=1` | |
| 7 | Taper `SET_SPEED:30` | l'Uno affiche D qui monte puis se stabilise vers 0,667 ; `speed` monte vers 1 020 tr/min en environ 10 à 20 s ; `speed_cmd` ≈ 0,667 | |
| 8 | Sur l'Uno : `PENTE:3` | la vitesse baisse un peu (≈ 28,7 km/h) puis revient à 1 020 tr/min ; `speed_cmd` monte vers 0,767 | |
| 9 | Sur l'Uno : `PENTE:10` | `speed_cmd` bloqué à 0,950 (saturé), `speed` reste sous 1 020 (≈ 940 tr/min) | |
| 10 | Sur l'Uno : `PENTE:0` | la vitesse dépasse un peu (≈ 32 km/h) puis revient à 1 020 tr/min sans osciller (anti-emballement du PI) | |
| 11 | A0 sur 2,5 V (couple mesuré nul), taper `SET_SLOPE:3` | `slope=3.0`, `brake_mode=TEST`, `brake_setpoint` monte de 0 à 0,660 en ≈ 1,3 s. Comme le couple mesuré reste à 0, le PI pousse : `brake_pwm` monte jusqu'à 242 et `BRAKE_SAT` apparaît dans `faults` après 1 s. C'est normal avec A0 fixe : le PI voit que le frein « ne freine pas » | |
| 12 | A0 sur 5 V (couple mesuré trop fort) | `brake_pwm` descend jusqu'à 0, `BRAKE_SAT` disparaît : le PI réduit le frein. Multimètre sur D6 : la tension suit 5 V × α | |
| 13 | Remettre A0 sur 2,5 V, taper `STOP` | `OK`, `status=STOPPED`, `brake_mode=END`, `brake_setpoint` monte vers 1,300 et `brake_pwm` ≈ 236 ; l'Uno affiche `etat=0`, `D=0` et la voiture ralentit ; `speed` tombe à 0 en ≈ 7 à 8 s (1 s après la dernière impulsion), et au même moment `brake_mode=OFF` et `brake_pwm=0` | |
| 14 | `START`, `SET_SPEED:30`, attendre la vitesse, puis relier D3 à GND | tout de suite : `emergency=1`, `faults=EMERGENCY`, `status=STOPPED`, `brake_mode=OFF` (pas de freinage de fin), `speed_cmd=0` | |
| 15 | D3 toujours à GND, taper `START` | refusé : `ERR:EMERGENCY` | |
| 16 | Retirer le fil de D3, taper `START` | `emergency=0`, `OK`, `status=RUNNING` | |
| 17 | Pendant l'essai, débrancher le fil SCK (D52) | Mega : après 1 s, `spi=LOST` et `SPI_LOST` dans `faults` ; Uno : « SECURITE … D = 0 » après 500 ms et la voiture ralentit ; rebrancher : `spi=OK` | |
| 18 | Débrancher le fil D9 → D2 pendant que la voiture roule | `speed` passe à 0 après 1 s (la résistance de 10 kΩ tient D2 à 0 V, pas de fausses impulsions) ; rebrancher | |
| 19 | `SET_VENT:1` puis `SET_VENT:0` | `fan_in=1` puis `fan_in=0` | |
| 20 | Facultatif : chauffer la sonde au sèche-cheveux | `fan_in=1` à partir de 60 °C, `fan_in=0` sous 50 °C | |
| 21 | Taper `RESET` | `OK`, `status=IDLE`, `spi=OFF`, toutes les consignes à 0 | |

## Résultats et remarques
Noter ici les valeurs lues, les écarts et les questions pour l'équipe.
