// banc_etat.cpp : les variables partagées et leur valeur au démarrage
#include "banc_etat.h"

EtatBanc etat            = IDLE;
float    consigneVitesse = 0.0;
float    consignePente   = 0.0;
bool     ventForcee      = false;
bool     defautThermique = false;
bool     urgenceActive   = false;
uint32_t sequence        = 0;

const char *etatTexte()
{
    switch (etat) {
        case RUNNING: return "RUNNING";
        case PAUSED:  return "PAUSED";
        case STOPPED: return "STOPPED";
        case FAULT:   return "FAULT";
        default:      return "IDLE";
    }
}
