// =====================================================================
//  securite.cpp
//  1) Arrêt d'urgence : contact Reed sur D3 (INT1) vers GND, pull-up interne.
//     Déclenchement immédiat (interruption), relâchement après 20 ms stables.
//  2) Sécurité thermique (fig. 3f) : T >= 100 °C -> TEMP_CRITICAL, état FAULT.
//  3) Ventilation (fig. 3g) : D42 -> Q3/Q4 -> relais K1 et K2, selon la température.
// =====================================================================
#include "securite.h"
#include "config.h"
#include "banc_etat.h"
#include "temperature.h"

static volatile bool urgenceVue = false;   // posé par l'interruption
static bool     derniereLecture = false;
static uint32_t tChangementMs   = 0;
static bool     ventMarche      = false;

static bool contactActif()
{
    return digitalRead(PIN_URGENCE) == URGENCE_NIVEAU_ACTIF;
}

// Interruption INT1 : le contact vient de changer
static void urgence_isr()
{
    if (contactActif()) urgenceVue = true;
}

static void declencherUrgence()
{
    urgenceActive   = true;
    consigneVitesse = 0.0;          // comme STOP : consignes remises à 0
    consignePente   = 0.0;
    if (etat == RUNNING || etat == PAUSED) etat = STOPPED;
    // Le moteur de la voiture (D = 0, état = 0) et le frein (coupé) sont
    // arrêtés au cycle suivant de regulation_vitesse.cpp et frein.cpp.
}

void securite_init()
{
    pinMode(PIN_URGENCE, INPUT_PULLUP);
    derniereLecture = contactActif();
    tChangementMs   = millis();
    if (derniereLecture) declencherUrgence();     // état initial du contact
    attachInterrupt(digitalPinToInterrupt(PIN_URGENCE), urgence_isr, CHANGE);

    digitalWrite(PIN_VENT, LOW);                  // ventilateurs arrêtés
    pinMode(PIN_VENT, OUTPUT);
}

void urgence_gerer()
{
    // a) Déclenchement : immédiat, sans attendre l'anti-rebond
    if (urgenceVue) {
        urgenceVue = false;
        if (!urgenceActive) declencherUrgence();
    }

    // b) Anti-rebond de 20 ms pour suivre l'état réel du contact
    bool lecture = contactActif();
    if (lecture != derniereLecture) {
        derniereLecture = lecture;
        tChangementMs   = millis();
    }
    if (millis() - tChangementMs >= ANTIREBOND_MS) {
        if (lecture && !urgenceActive) declencherUrgence();
        if (!lecture && urgenceActive) urgenceActive = false;   // relâché : il faudra un nouveau START
    }
}

// --- Fig. 3f : sécurité thermique ---------------------------------------
void thermique_gerer()
{
    if (temperature_fraiche() && temperature_valeur() >= TEMP_CRITIQUE_C
        && !defautThermique) {
        defautThermique = true;     // TEMP_CRITICAL mémorisé
        consignePente   = 0.0;
        etat            = FAULT;    // le frein est coupé par frein.cpp
    }
}

// --- Fig. 3g : ventilation ----------------------------------------------
//  Les ventilateurs suivent la TEMPÉRATURE, que le frein soit alimenté ou non :
//  après un essai, le frein reste chaud et doit continuer à être refroidi.
//    - forçage (SET_VENT:1) ou défaut thermique : marche ;
//    - température connue : marche à 60 °C, arrêt sous 50 °C,
//      inchangé entre les deux (hystérésis : pas de marche/arrêt en boucle) ;
//    - température inconnue (sonde absente ou mesure trop vieille) : marche,
//      par sécurité, sauf pendant les 3 premières secondes après le démarrage
//      (la sonde n'a pas encore fait sa première mesure).
void ventilation_gerer()
{
    if (ventForcee || defautThermique) {
        ventMarche = true;
    } else if (temperature_fraiche()) {
        float t = temperature_valeur();
        if (t >= VENT_MARCHE_C)     ventMarche = true;
        else if (t < VENT_ARRET_C)  ventMarche = false;
        // entre 50 et 60 °C : inchangé (hystérésis)
    } else if (millis() >= VENT_DELAI_DEMARRAGE_MS) {
        ventMarche = true;          // température inconnue : on refroidit
    }
    digitalWrite(PIN_VENT, ventMarche ? HIGH : LOW);
}

bool ventilation_marche() { return ventMarche; }
