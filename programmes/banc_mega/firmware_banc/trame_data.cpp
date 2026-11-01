// =====================================================================
//  trame_data.cpp : trame DATA vers le PC (fig. 3i)
//  DATA;seq=n;t_ms=...;torque=...;speed=...;temp=...;...;brake_mode=TEST/END/OFF;... \n
//
//  La ligne est d'abord écrite dans un tableau, puis envoyée petit à petit :
//  à chaque tour de loop, on n'envoie que ce qui tient dans le tampon de
//  l'USB (Serial.availableForWrite). Le programme n'attend donc jamais :
//  le frein (10 ms) et la mesure du couple (1 ms) gardent leur rythme.
// =====================================================================
#include "trame_data.h"
#include "config.h"
#include "banc_etat.h"
#include "vitesse.h"
#include "couple.h"
#include "temperature.h"
#include "proto_spi.h"
#include "frein.h"
#include "regulation_vitesse.h"
#include "securite.h"

static char     tampon[400];
static uint16_t longueur  = 0;     // longueur de la ligne prête
static uint16_t dejaEnvoye = 0;    // caractères déjà partis

// --- petites fonctions pour remplir la ligne -------------------------------
static void ajouterTexte(const char *t)
{
    while (*t && longueur < sizeof(tampon) - 2) tampon[longueur++] = *t++;
}

static void ajouterCle(const char *cle)
{
    ajouterTexte(";");
    ajouterTexte(cle);
    ajouterTexte("=");
}

static void ajouterNombre(const char *cle, float v, uint8_t decimales)
{
    char nombre[16];
    dtostrf(v, 1, decimales, nombre);     // float -> texte (sprintf n'affiche pas les float)
    ajouterCle(cle);
    ajouterTexte(nombre);
}

static void ajouterEntier(const char *cle, long v)
{
    char nombre[12];
    ltoa(v, nombre, 10);
    ajouterCle(cle);
    ajouterTexte(nombre);
}

// --- construire la ligne -----------------------------------------------------
void trame_data_preparer()
{
    if (dejaEnvoye < longueur) return;    // la précédente n'est pas finie : on saute

    longueur = 0;
    dejaEnvoye = 0;
    ajouterTexte("DATA");
    ajouterEntier("seq", (long)sequence++);
    ajouterEntier("t_ms", (long)millis());
    ajouterNombre("torque", couple_moyenne_data(), 3);   // N.m au rouleau
    ajouterNombre("speed", vitesse_trmin(), 1);          // tr/min du rouleau

    // Clé temp (jamais remplacée par 0)
    if (!temperature_presente()) {
        ajouterCle("temp"); ajouterTexte("-127.0");
    } else if (temperature_bloquee85()) {
        ajouterCle("temp"); ajouterTexte("85.0");
    } else if (temperature_fraiche()) {
        ajouterNombre("temp", temperature_valeur(), 1);
    }
    // sinon : clé temp omise

    // Mesures de la voiture, seulement si elles ont moins de 1 s
    if (proto_frais()) {
        ajouterNombre("bat_volt", protoBatVolt, 3);
        ajouterNombre("bat_curr", protoBatCurr, 3);
        ajouterNombre("mot_volt", protoMotVolt, 3);
        ajouterNombre("mot_curr", protoMotCurr, 3);
    }

    const bool vent = ventilation_marche();
    ajouterEntier("emergency", urgenceActive ? 1 : 0);
    ajouterEntier("brake_contact", urgenceActive ? 1 : 0);  // même contact D3
    ajouterEntier("fan_in", vent ? 1 : 0);                  // K1 et K2 : même sortie D42
    ajouterEntier("fan_out", vent ? 1 : 0);
    ajouterNombre("slope", consignePente, 1);
    ajouterNombre("speed_setpoint", consigneVitesse, 1);
    ajouterNombre("speed_cmd", regulation_d(), 3);          // D envoyé à la voiture
    ajouterEntier("brake_pwm", frein_pwm255());
    ajouterNombre("brake_torque", frein_couple_estime(), 3); // estimé (I non mesuré)
    ajouterNombre("brake_setpoint", frein_consigne(), 3);

    // Mode du frein : pourquoi il est alimenté (ou non)
    //   TEST = essai, le frein simule la pente ;
    //   END  = freinage de fin après STOP, le frein arrête le rouleau ;
    //   OFF  = frein coupé.
    ajouterCle("brake_mode");
    if (frein_freinage())          ajouterTexte("END");
    else if (frein_alpha() > 0.0)  ajouterTexte("TEST");
    else                           ajouterTexte("OFF");
    ajouterCle("rs485"); ajouterTexte("OFF");

    ajouterCle("spi");
    if (!proto_actif())     ajouterTexte("OFF");
    else if (proto_frais()) ajouterTexte("OK");
    else                    ajouterTexte("LOST");

    // Défauts, séparés par des virgules
    ajouterCle("faults");
    uint16_t avant = longueur;
    if (defautThermique) ajouterTexte("TEMP_CRITICAL,");
    if (urgenceActive)   ajouterTexte("EMERGENCY,");
    if (frein_sature())  ajouterTexte("BRAKE_SAT,");
    if (proto_actif() && !proto_frais()) ajouterTexte("SPI_LOST,");
    if (longueur == avant) ajouterTexte("NONE");
    else                   longueur--;                     // enlève la dernière virgule

    ajouterCle("status");
    ajouterTexte(etatTexte());
    tampon[longueur++] = '\n';
}

// --- envoyer sans bloquer ------------------------------------------------------
void trame_data_envoyer()
{
    if (dejaEnvoye >= longueur) return;
    int place = Serial.availableForWrite();
    if (place <= 0) return;
    uint16_t reste = longueur - dejaEnvoye;
    uint16_t n = (reste < (uint16_t)place) ? reste : (uint16_t)place;
    Serial.write((const uint8_t *)&tampon[dejaEnvoye], n);
    dejaEnvoye += n;
}

bool trame_data_occupee() { return dejaEnvoye < longueur; }
