// =====================================================================
//  commandes.cpp : commandes du PC (USB, 115 200 bauds), fig. 3c et 3d
//  PING, START, PAUSE, STOP, RESET, SET_SPEED:v, SET_SLOPE:p, SET_VENT:x
//  Numéro facultatif « #n » : OK devient ACK#n, ERR:raison devient NACK#n:raison.
// =====================================================================
#include "commandes.h"
#include "config.h"
#include "banc_etat.h"
#include "temperature.h"
#include "proto_spi.h"
#include "trame_data.h"

static char    ligne[LIGNE_MAX + 1];
static uint8_t longueur  = 0;
static bool    debordement = false;

static float borner(float v, float bas, float haut)
{
    if (v < bas)  return bas;
    if (v > haut) return haut;
    return v;
}

// Réponse au PC, avec ou sans numéro de commande
static void repondre(bool ok, const char *raison, bool numerote, long numero)
{
    if (numerote) {
        Serial.print(ok ? F("ACK#") : F("NACK#"));
        Serial.print(numero);
        if (!ok) { Serial.print(':'); Serial.print(raison); }
        Serial.println();
    } else {
        if (ok) Serial.println(F("OK"));
        else  { Serial.print(F("ERR:")); Serial.println(raison); }
    }
}

// --- Fig. 3d : traiter une ligne complète -----------------------------------
static void traiter(char *l)
{
    // 1) Séparer le numéro facultatif « #n » et passer en majuscules
    bool numerote = false;
    long numero = 0;
    char *diese = strchr(l, '#');
    if (diese != NULL) {
        *diese = '\0';
        numero = atol(diese + 1);
        numerote = true;
    }
    for (char *c = l; *c; c++) *c = toupper(*c);

    // 2) Commandes sans valeur
    if (strcmp(l, "PING") == 0) {
        Serial.println(F("PONG"));
        return;
    }
    if (strcmp(l, "START") == 0) {
        if (etat == FAULT)  { repondre(false, "FAULT", numerote, numero); return; }
        if (urgenceActive)  { repondre(false, "EMERGENCY", numerote, numero); return; }
        etat = RUNNING;
        proto_activer(true);          // D49 = 1 : liaison voiture active
        repondre(true, "", numerote, numero);
        return;
    }
    if (strcmp(l, "PAUSE") == 0) {
        if (etat == RUNNING) etat = PAUSED;
        repondre(true, "", numerote, numero);
        return;
    }
    if (strcmp(l, "STOP") == 0) {
        consigneVitesse = 0.0;
        consignePente   = 0.0;
        if (etat != FAULT) etat = STOPPED;
        // D49 reste à 1 : la voiture reçoit état = 0 et s'arrête tout de suite
        repondre(true, "", numerote, numero);
        return;
    }
    if (strcmp(l, "RESET") == 0) {
        consigneVitesse = 0.0;
        consignePente   = 0.0;
        ventForcee      = false;
        sequence        = 0;
        // Défaut thermique acquitté seulement si T < 95 °C (mesure fraîche)
        if (defautThermique && temperature_fraiche()
            && temperature_valeur() < TEMP_REARMEMENT_C) {
            defautThermique = false;
        }
        proto_activer(false);         // D49 = 0 : cartes isolées
        if (defautThermique) {
            etat = FAULT;
            repondre(false, "TEMP_CRITICAL", numerote, numero);
        } else {
            etat = IDLE;
            repondre(true, "", numerote, numero);
        }
        return;
    }

    // 3) Commandes avec valeur « NOM:valeur »
    char *deuxPoints = strchr(l, ':');
    if (deuxPoints == NULL) { repondre(false, "UNKNOWN", numerote, numero); return; }
    *deuxPoints = '\0';
    const float valeur = atof(deuxPoints + 1);

    if (strcmp(l, "SET_SPEED") == 0) {
        consigneVitesse = borner(valeur, 0.0, VITESSE_MAX_KMH);
        repondre(true, "", numerote, numero);
    } else if (strcmp(l, "SET_SLOPE") == 0) {
        if (etat == FAULT) { repondre(false, "FAULT", numerote, numero); return; }
        consignePente = borner(valeur, 0.0, PENTE_MAX_POURCENT);
        repondre(true, "", numerote, numero);
    } else if (strcmp(l, "SET_VENT") == 0) {
        ventForcee = (valeur >= 0.5);
        repondre(true, "", numerote, numero);
    } else {
        repondre(false, "UNKNOWN", numerote, numero);
    }
}

// --- Fig. 3c : lire les caractères reçus ------------------------------------
void commandes_lire()
{
    // Pendant l'envoi d'une ligne DATA, on attend : sinon la réponse (OK...)
    // se glisserait au milieu de la ligne. Les caractères restent dans le
    // tampon de réception (64 octets) et sont lus quelques ms plus tard.
    if (trame_data_occupee()) return;

    while (Serial.available() > 0) {
        const char c = (char)Serial.read();
        if (c == '\r') continue;                       // caractère ignoré
        if (c == '\n') {                               // fin de ligne
            if (longueur > 0 && !debordement) {
                ligne[longueur] = '\0';
                traiter(ligne);
            }
            longueur = 0;
            debordement = false;
        } else if (longueur < LIGNE_MAX) {
            ligne[longueur++] = c;
        } else {
            debordement = true;                        // ligne trop longue : ignorée
        }
    }
}
