// =====================================================================
//  firmware_banc.ino : programme de l'Arduino Mega 2560 du banc Eco-Shell
//  Carte : carte2 (shield). Programme principal (fig. 2 de MIMID).
//
//  Un fichier par bloc :
//    config.h               tous les réglages (broches, seuils, Kp/Ki...)
//    banc_etat              mode du banc, consignes, défauts
//    commandes              commandes du PC (fig. 3c, 3d)
//    vitesse                vitesse du rouleau sur D2 (fig. 3a, 3b)
//    couple                 couple sur A0
//    temperature            sonde DS18B20 sur D48 (fig. 3e)
//    securite               arrêt d'urgence D3, sécurité thermique (3f), ventilation (3g)
//    frein                  pente et frein à hystérésis, Timer 4 (PENTE.docx)
//    regulation_vitesse     PI de vitesse + trame 0x55 (organigramme A)
//    proto_spi              liaison SPI avec la voiture
//    trame_data             trame DATA vers le PC (fig. 3i)
//
//  Bibliothèques à installer : « DallasTemperature » (Miles Burton),
//  qui installe aussi « OneWire ». SPI est fournie avec l'Arduino.
//
//  Règles de la carte :
//    - D0 / D1 : USB et interface uniquement ;
//    - D6 / D7 : jamais de digitalWrite() ni analogWrite() (Timer 4).
// =====================================================================
#include "config.h"
#include "banc_etat.h"
#include "frein.h"
#include "securite.h"
#include "vitesse.h"
#include "couple.h"
#include "temperature.h"
#include "proto_spi.h"
#include "regulation_vitesse.h"
#include "commandes.h"
#include "trame_data.h"

static uint32_t tCoupleMs  = 0;
static uint32_t tFreinMs   = 0;
static uint32_t tVitesseMs = 0;
static uint32_t tDataMs    = 0;

// Vrai quand « periode » ms sont passées ; garde un rythme régulier.
static bool echeance(uint32_t &t, uint16_t periode)
{
    uint32_t maintenant = millis();
    if (maintenant - t < periode) return false;
    t += periode;
    if (maintenant - t > 5UL * periode) t = maintenant;   // gros retard : on se recale
    return true;
}

void setup()
{
    // 1-2) EN PREMIER : Timer 4 réglé (20 kHz, temps mort 0,5 us), frein coupé
    frein_init();
    // 3) Ventilateurs arrêtés ; arrêt d'urgence (D3, pull-up, INT1) lu
    securite_init();
    // 5) Vitesse : D2 en interruption INT0 (front descendant)
    vitesse_init();
    // 6) SPI voiture : SS = 1, D49 = 0 (U12 isole les deux cartes)
    proto_init();
    // 7) USB vers le PC
    Serial.begin(BAUD_PC);
    // 8) Sonde DS18B20 : recherche puis 1re conversion, sans attendre
    temperature_init();

    uint32_t t = millis();
    tCoupleMs = tFreinMs = tVitesseMs = tDataMs = t;

    Serial.println(F("READY"));
}

void loop()
{
    commandes_lire();                                // fig. 3c / 3d

    if (echeance(tCoupleMs, 1)) couple_echantillonner();   // une lecture de A0 par ms

    urgence_gerer();                                 // D3
    temperature_gerer();                             // fig. 3e
    thermique_gerer();                               // fig. 3f

    if (echeance(tFreinMs, PERIODE_FREIN_MS))     frein_cycle();               // pente
    if (echeance(tVitesseMs, PERIODE_VITESSE_MS)) regulation_vitesse_cycle();  // PI + SPI

    ventilation_gerer();                             // fig. 3g

    if (echeance(tDataMs, PERIODE_DATA_MS)) {        // toutes les 50 ms
        vitesse_calculer();                          // fig. 3b
        trame_data_preparer();                       // fig. 3i
    }
    trame_data_envoyer();                            // envoi sans bloquer
}
