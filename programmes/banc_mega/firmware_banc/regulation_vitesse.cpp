// =====================================================================
//  regulation_vitesse.cpp : organigramme A (vitesse, côté banc)
//  Toutes les 100 ms (Te = 0,1 s) :
//    1) vitesse mesurée du rouleau (D2) en km/h ;
//    2) PI : D = P + I, borné entre 0 et 0,95, avec anti-emballement ;
//    3) trame 0x55 de 10 octets, D en pour mille dans les octets 6-7 ;
//    4) échange SPI avec la voiture si la liaison est active (D49 = 1).
// =====================================================================
#include "regulation_vitesse.h"
#include "config.h"
#include "banc_etat.h"
#include "vitesse.h"
#include "proto_spi.h"

static float integrale = 0.0;   // terme I du PI
static float D         = 0.0;   // rapport cyclique demandé à la voiture

void regulation_vitesse_cycle()
{
    const float te = PERIODE_VITESSE_MS / 1000.0;     // 0,1 s
    uint8_t etatTrame;

    const float vMes = vitesse_kmh();                  // v_mes = n x 0,0294

    if (etat == RUNNING && !defautThermique && !urgenceActive
        && consigneVitesse > 0.0) {
        // --- PI ---------------------------------------------------------
        const float e    = consigneVitesse - vMes;        // erreur en km/h
        const float P    = KP_VITESSE * e;
        const float iNouv = integrale + KI_VITESSE * e * te;
        D = P + iNouv;
        if (D > D_MAX) {
            D = D_MAX;                 // saturé : I non mis à jour (anti-emballement)
        } else if (D < 0.0) {
            D = 0.0;                   // idem
        } else {
            integrale = iNouv;
        }
        etatTrame = 1;                 // la voiture a le droit de rouler
    } else {
        // Commande coupée : pas d'essai, pause, arrêt, défaut, urgence ou consigne nulle
        D = 0.0;
        integrale = 0.0;
        etatTrame = 0;                 // la voiture doit s'arrêter
    }

    // --- Trame 0x55 (banc -> voiture) ---------------------------------------
    const uint16_t vDixiemes = (uint16_t)(consigneVitesse * 10.0 + 0.5);
    const uint16_t dPourMille = (uint16_t)(D * 1000.0 + 0.5);
    uint8_t envoi[PROTO_LONGUEUR];
    envoi[0] = ENTETE_CONSIGNE;
    envoi[1] = vDixiemes >> 8;   envoi[2] = vDixiemes & 0xFF;   // vitesse, 0,1 km/h
    envoi[3] = 0;                envoi[4] = 0;                  // couple : non utilisé
    envoi[5] = etatTrame;                                        // 1 rouler, 0 arrêt
    envoi[6] = dPourMille >> 8;  envoi[7] = dPourMille & 0xFF;  // D en pour mille
    envoi[8] = 0;                                                // réservé
    envoi[9] = proto_xor(envoi, PROTO_LONGUEUR - 1);

    proto_echanger(envoi);   // ne fait rien si D49 = 0 ; mesures mises à jour si 0xAA valide
}

float regulation_d() { return D; }
