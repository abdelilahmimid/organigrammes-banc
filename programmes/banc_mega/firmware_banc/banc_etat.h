// =====================================================================
//  banc_etat.h : l'état général du banc, partagé par tous les fichiers
//  (mode, consignes, défauts).
// =====================================================================
#ifndef BANC_ETAT_H
#define BANC_ETAT_H

#include <Arduino.h>

enum EtatBanc { IDLE, RUNNING, PAUSED, STOPPED, FAULT };

extern EtatBanc etat;              // mode du banc (commandes START, STOP...)
extern float    consigneVitesse;   // km/h, 0 à 30
extern float    consignePente;     // %, 0 à PENTE_MAX_POURCENT
extern bool     ventForcee;        // SET_VENT:1
extern bool     defautThermique;   // TEMP_CRITICAL mémorisé
extern bool     urgenceActive;     // arrêt d'urgence (D3)
extern uint32_t sequence;          // numéro de la trame DATA

const char *etatTexte();           // "IDLE", "RUNNING", ...

#endif
