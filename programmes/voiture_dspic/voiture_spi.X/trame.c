/* =====================================================================
   trame.c : construction / vérification des trames SPI
   Même logique que buildFrame() et traiterConsigne() de l'Uno.
   ===================================================================== */
#include "trame.h"
#include "carte_voiture.h"

uint8_t trame_xor(const uint8_t *t, uint8_t n)
{
    uint8_t x = 0;
    uint8_t i;
    for (i = 0; i < n; i++) {
        x ^= t[i];
    }
    return x;
}

bool trame_consigne_lire(const uint8_t t[TRAME_LONGUEUR], consigne_t *c)
{
    /* 1) en-tête : 0x55 (01010101), jamais 0x00 ou 0xFF d'un fil coupé */
    if (t[0] != ENTETE_CONSIGNE) {
        return false;
    }
    /* 2) XOR des octets 0 à 8 = octet 9 */
    if (trame_xor(t, TRAME_LONGUEUR - 1U) != t[TRAME_LONGUEUR - 1U]) {
        return false;
    }
    /* 3) décodage : octet haut x 256 + octet bas */
    c->v_cons_dixiemes = ((uint16_t)t[1] << 8) | t[2];
    c->couple_mNm      = ((uint16_t)t[3] << 8) | t[4];
    c->etat            = t[5];
    c->d_pour_mille    = ((uint16_t)t[6] << 8) | t[7];
    return true;
}

void trame_mesures_construire(uint8_t t[TRAME_LONGUEUR], const mesures_t *m)
{
    t[0] = ENTETE_MESURES;
    t[1] = (uint8_t)(m->bat_volt_mV >> 8);
    t[2] = (uint8_t)(m->bat_volt_mV & 0xFFU);
    t[3] = (uint8_t)((uint16_t)m->bat_curr_mA >> 8);
    t[4] = (uint8_t)((uint16_t)m->bat_curr_mA & 0xFFU);
    t[5] = (uint8_t)(m->mot_volt_mV >> 8);
    t[6] = (uint8_t)(m->mot_volt_mV & 0xFFU);
    t[7] = (uint8_t)((uint16_t)m->mot_curr_mA >> 8);
    t[8] = (uint8_t)((uint16_t)m->mot_curr_mA & 0xFFU);
    t[9] = trame_xor(t, TRAME_LONGUEUR - 1U);
}

bool commande_choisir(bool deja_recu, uint32_t age_ms,
                      const consigne_t *c, uint16_t *d)
{
    *d = 0;
    if (!deja_recu)              return false;  /* rien reçu depuis le démarrage */
    if (age_ms >= DELAI_PERTE_MS) return false;  /* sécurité 500 ms             */
    if (c->etat != 1U)           return false;  /* état = 0 : arrêt             */

    *d = c->d_pour_mille;
    if (*d > D_MAX_POUR_MILLE) {
        *d = D_MAX_POUR_MILLE;                  /* D <= 0,95                    */
    }
    return true;
}
