/* =====================================================================
   trame.h : les deux trames SPI de 10 octets et la règle de sécurité.
   Ce fichier n'utilise aucun registre du dsPIC : il peut être testé
   sur un PC.
   ===================================================================== */
#ifndef TRAME_H
#define TRAME_H

#include <stdint.h>
#include <stdbool.h>

#define TRAME_LONGUEUR      10U
#define ENTETE_CONSIGNE     0x55U   /* banc -> voiture */
#define ENTETE_MESURES      0xAAU   /* voiture -> banc */

/* Contenu utile de la trame 0x55 */
typedef struct {
    uint16_t v_cons_dixiemes;   /* octets 1-2 : vitesse en 0,1 km/h      */
    uint16_t couple_mNm;        /* octets 3-4 : non utilisé              */
    uint8_t  etat;              /* octet 5    : 1 rouler, 0 arrêt        */
    uint16_t d_pour_mille;      /* octets 6-7 : D x 1000 (0 à 950)       */
} consigne_t;

/* Contenu utile de la trame 0xAA */
typedef struct {
    uint16_t bat_volt_mV;       /* octets 1-2 */
    int16_t  bat_curr_mA;       /* octets 3-4 (signé) */
    uint16_t mot_volt_mV;       /* octets 5-6 */
    int16_t  mot_curr_mA;       /* octets 7-8 (signé) */
} mesures_t;

/* XOR des n premiers octets */
uint8_t trame_xor(const uint8_t *t, uint8_t n);

/* Vérifie l'en-tête 0x55 et le XOR, puis décode.
   Renvoie false si la trame est fausse (c n'est alors pas modifié).
   Le contrôle « 10 octets reçus » est fait par spi_esclave.c.       */
bool trame_consigne_lire(const uint8_t t[TRAME_LONGUEUR], consigne_t *c);

/* Construit la trame 0xAA (octet 9 = XOR des octets 0 à 8) */
void trame_mesures_construire(uint8_t t[TRAME_LONGUEUR], const mesures_t *m);

/* Règle de sécurité de l'organigramme B.
   deja_recu : au moins une trame valide depuis le démarrage
   age_ms    : temps depuis la dernière trame valide
   Renvoie true si le moteur a le droit d'être alimenté ; *d reçoit
   alors D en pour mille, limité à 950. Renvoie false sinon (*d = 0). */
bool commande_choisir(bool deja_recu, uint32_t age_ms,
                      const consigne_t *c, uint16_t *d);

#endif
