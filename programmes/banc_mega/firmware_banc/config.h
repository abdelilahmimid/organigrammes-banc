// =====================================================================
//  config.h : TOUS les réglages du banc sont ici.
//  On ne change une valeur qu'à cet endroit ; les autres fichiers la lisent.
//  Carte : carte2 (shield Arduino Mega 2560), brochage tiré du KiCad.
// =====================================================================
#ifndef CONFIG_H
#define CONFIG_H

#include <Arduino.h>

// ---------------------------------------------------------------------
// Broches (KiCad carte2)
//   D0 / D1 : USB et interface PC uniquement (U3 MAX485 non soudé,
//             D4 / D5 laissées libres).
// ---------------------------------------------------------------------
const uint8_t PIN_VITESSE   = 2;    // RPM+ du couplemètre, optocoupleur U6, INT0 (pull-down R16 sur la carte)
const uint8_t PIN_URGENCE   = 3;    // arrêt d'urgence : contact Reed vers GND, INT1
const uint8_t PIN_PWMH      = 6;    // OC4A -> driver -> transistor du haut du frein
const uint8_t PIN_PWMHL     = 7;    // OC4B -> driver -> transistor du bas du frein
const uint8_t PIN_VENT      = 42;   // ventilateurs (Q3/Q4 -> relais K1 et K2)
const uint8_t PIN_TEMP      = 48;   // sonde DS18B20 (1-Wire, pull-up R25 sur la carte)
const uint8_t PIN_ENB_PROTO = 49;   // OE de U12 (TXS0108) : 1 = liaison voiture active
const uint8_t PIN_SS_PROTO  = 53;   // SS du SPI vers la voiture (D50 MISO, D51 MOSI, D52 SCK)
const uint8_t PIN_COUPLE    = A0;   // SIG_OUT du couplemètre (couple)

// ---------------------------------------------------------------------
// Liaison avec le PC (interface)
// ---------------------------------------------------------------------
const uint32_t BAUD_PC    = 115200;
const uint8_t  LIGNE_MAX  = 47;     // caractères max d'une commande (fig. 3c)

// ---------------------------------------------------------------------
// Rythmes du programme (en millisecondes)
// ---------------------------------------------------------------------
const uint16_t PERIODE_DATA_MS     = 50;   // trame DATA vers le PC (fig. 2)
const uint16_t PERIODE_VITESSE_MS  = 100;  // régulation de vitesse + SPI (organigramme A)
const uint16_t PERIODE_FREIN_MS    = 10;   // pilotage du frein (PENTE.docx)
const uint16_t ANTIREBOND_MS       = 20;   // anti-rebond du contact d'arrêt d'urgence

// ---------------------------------------------------------------------
// Rouleau et mesure de vitesse (fig. 3a et 3b)
// ---------------------------------------------------------------------
const uint8_t  IMPULSIONS_PAR_TOUR   = 60;
const uint16_t FILTRE_PARASITE_US    = 200;   // impulsions plus rapprochées = parasite
const uint16_t DELAI_VITESSE_NULLE_MS = 1000; // plus d'impulsion pendant 1 s -> 0 tr/min
const float    RAYON_ROULEAU_M       = 0.078; // à confirmer par la mesure (FAT 2.2)
// km/h pour 1 tr/min du rouleau : 2 x pi x r x 60 / 1000
const float    KMH_PAR_TRMIN = 2.0 * PI * RAYON_ROULEAU_M * 60.0 / 1000.0;

// ---------------------------------------------------------------------
// Couple (A0) : C = (tension - zéro) x pente
//   Valeurs de départ ; zéro et pente à mesurer sur le banc.
// ---------------------------------------------------------------------
const float VREF_ADC_V        = 5.0;
const float COUPLE_ZERO_V     = 2.5;   // tension de SIG_OUT à couple nul
const float COUPLE_NM_PAR_V   = 2.0;   // N.m au rouleau par volt

// ---------------------------------------------------------------------
// Régulation de vitesse (organigramme A) : PI dans le Mega
//   Kp et Ki : valeurs de départ, à remplacer après l'essai indiciel
//   (Kp = tau / (K x Tbf), Ki = Kp / tau).
// ---------------------------------------------------------------------
const float VITESSE_MAX_KMH = 30.0;
const float KP_VITESSE      = 0.04;   // par km/h
const float KI_VITESSE      = 0.02;   // par km/h et par seconde
const float D_MAX           = 0.95;   // laisse le bootstrap de la voiture se recharger

// ---------------------------------------------------------------------
// Liaison SPI avec la voiture (trames de 10 octets)
// ---------------------------------------------------------------------
const uint32_t PROTO_HORLOGE_HZ  = 250000;
const uint8_t  PROTO_LONGUEUR    = 10;
const uint8_t  ENTETE_CONSIGNE   = 0x55;   // banc -> voiture
const uint8_t  ENTETE_MESURES    = 0xAA;   // voiture -> banc
const uint8_t  PROTO_PAUSE_US    = 20;     // pause entre deux octets
const uint16_t PROTO_FRAICHEUR_MS = 1000;  // mesures voiture publiées si < 1 s

// ---------------------------------------------------------------------
// Pente et frein à hystérésis (frein.cpp, organigrammes C et D)
// ---------------------------------------------------------------------
const float PENTE_MAX_POURCENT = 6.0;    // 6 ou 7 % : à décider en équipe
const float MASSE_KG           = 115.0;  // voiture + pilote
const float GRAVITE            = 9.81;
const float RAPPORT_COURROIE   = 4.0;    // rouleau -> volant du frein
// Couple au volant pour 1 % de pente : m x g x 0,01 x r / k = 0,2200 N.m
const float COUPLE_PAR_POURCENT = MASSE_KG * GRAVITE * 0.01 * RAYON_ROULEAU_M / RAPPORT_COURROIE;

const float FREIN_NM_PAR_A   = 3.75;   // fiche du frein : 400 mA -> 1,50 N.m
const float R_BOBINE_OHM     = 64.0;   // à froid (monte d'environ 30 % à 100 °C)
const float U_FREIN_V        = 24.0;   // alimentation du demi-pont
const float ALPHA_MAX        = 0.95;   // laisse le bootstrap du MIC4104 se recharger
const float RAMPE_NM_PAR_S   = 0.5;    // montée / descente de la consigne
const float KP_FREIN         = 0.05;   // par N.m
const float KI_FREIN         = 0.2;    // par N.m et par seconde
const float VITESSE_MIN_PI_TRMIN = 20.0; // en dessous : anticipation seule
const float SAT_ECART_NM     = 0.05;   // écart pour l'alerte BRAKE_SAT
const uint16_t SAT_DELAI_MS  = 1000;   // durée avant l'alerte BRAKE_SAT

// Freinage de fin (après STOP) : arrêter le rouleau avec le frein
//   La consigne monte de 0,5 N.m/s (même rampe) jusqu'à 1,3 N.m au volant,
//   en anticipation seule (sans PI), jusqu'à ce que le rouleau soit arrêté.
const float    FREINAGE_COUPLE_NM    = 1.3;    // N.m au volant (alpha environ 0,92)
const uint16_t FREINAGE_DUREE_MAX_MS = 30000;  // sécurité : 30 s au maximum

// Timer 4 : PWM 20 kHz, phase correcte, 16 MHz / (2 x 400) = 20 kHz
const uint16_t TIMER4_TOP       = 400;
const uint16_t TEMPS_MORT_TICKS = 8;   // 8 x 62,5 ns = 0,5 us

// ---------------------------------------------------------------------
// Température (DS18B20) et sécurité thermique (fig. 3e, 3f, 3g)
// ---------------------------------------------------------------------
const uint16_t TEMP_CONVERSION_MS  = 800;   // 750 ms en 12 bits + marge
const uint16_t TEMP_FRAICHE_MS     = 2500;  // au-delà, la mesure est trop vieille
const uint8_t  TEMP_REPETITIONS_85 = 3;     // 85,0 lu 3 fois -> sonde bloquée
const float    TEMP_CRITIQUE_C     = 100.0; // défaut thermique
const float    TEMP_REARMEMENT_C   = 95.0;  // RESET accepté en dessous
const float    VENT_MARCHE_C       = 60.0;  // ventilateurs en marche
const float    VENT_ARRET_C        = 50.0;  // ventilateurs arrêtés
const uint16_t VENT_DELAI_DEMARRAGE_MS = 3000; // au démarrage : laisser la sonde faire sa 1re mesure

// ---------------------------------------------------------------------
// Arrêt d'urgence (D3, contact Reed vers GND, pull-up interne)
//   LOW = contact fermé. Mettre HIGH si l'urgence correspond au contact ouvert.
// ---------------------------------------------------------------------
const uint8_t URGENCE_NIVEAU_ACTIF = LOW;

#endif
