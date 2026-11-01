// =====================================================================
//  vitesse.cpp : vitesse du rouleau
//  Couplemètre DYN-200 (sortie RPM+, 60 impulsions par tour)
//  -> optocoupleur PC817 -> D2 (INT0).
//  Sur la carte, le phototransistor de U6 relie D2 au 5 V de l'Arduino
//  quand sa LED s'allume, et R16 (10 kOhm) tire D2 à 0 V le reste du temps.
//  Le signal sur D2 est donc 0 V au repos, 5 V pendant l'impulsion.
// =====================================================================
#include "vitesse.h"
#include "config.h"

static volatile uint32_t compteur  = 0;   // impulsions depuis le démarrage
static volatile uint32_t tDerniere = 0;   // instant de la dernière impulsion (us)

static uint32_t compteurRef = 0;          // référence du calcul précédent
static uint32_t tRef        = 0;
static bool     refValide   = false;
static uint32_t tDernierChangementMs = 0;
static float    nTrMin      = 0.0;

// --- Fig. 3a : une impulsion arrive sur D2 (front montant) ------------
static void impulsion()
{
    uint32_t t = micros();
    if (t - tDerniere < FILTRE_PARASITE_US) {
        return;                    // impulsion trop rapprochée : parasite ignoré
    }
    tDerniere = t;
    compteur++;
}

void vitesse_init()
{
    // PAS de pull-up interne : R16 (10 kOhm vers GND) est déjà sur la carte.
    // Avec INPUT_PULLUP, le repos serait entre 0,8 et 1,7 V (zone indéfinie).
    pinMode(PIN_VITESSE, INPUT);
    // Une impulsion = un front montant (la LED de U6 s'allume).
    attachInterrupt(digitalPinToInterrupt(PIN_VITESSE), impulsion, RISING);
}

// --- Fig. 3b : calcul de la vitesse ------------------------------------
void vitesse_calculer()
{
    uint32_t c, t;
    noInterrupts();                 // copie sans être interrompu
    c = compteur;
    t = tDerniere;
    interrupts();

    if (c != compteurRef) {
        // De nouvelles impulsions sont arrivées
        if (refValide && t != tRef) {
            // n = impulsions x 60 000 000 / (60 imp/tr x durée en us)
            float dt_us = (float)(t - tRef);
            nTrMin = (float)(c - compteurRef) * 60000000.0
                     / ((float)IMPULSIONS_PAR_TOUR * dt_us);
        }
        compteurRef = c;
        tRef        = t;
        refValide   = true;
        tDernierChangementMs = millis();
    } else if (millis() - tDernierChangementMs >= DELAI_VITESSE_NULLE_MS) {
        nTrMin    = 0.0;            // plus aucune impulsion depuis 1 s
        refValide = false;
    }
    // sinon : vitesse inchangée
}

float vitesse_trmin() { return nTrMin; }
float vitesse_kmh()   { return nTrMin * KMH_PAR_TRMIN; }
