/* =====================================================================
   main.c : programme du dsPIC33CK32MP105 de la voiture
   Organigramme B « appliquer la commande » (tâches 29 et 30)

   Le banc (Mega, maître SPI) envoie toutes les 100 ms la trame 0x55
   avec D (pour mille). La voiture applique D sur le pont Q2 / Q3 et
   renvoie dans le même échange la trame 0xAA (tension, courants).
   La régulation de vitesse (PI) est faite par le banc, pas ici.

   THROTTLE (broche 17) n'est pas lu : sur le banc, seul D reçu par SPI
   pilote le moteur.
   ===================================================================== */
#include <xc.h>
#include <stdint.h>
#include <stdbool.h>

#include "carte_voiture.h"
#include "trame.h"
#include "horloge.h"
#include "spi_esclave.h"
#include "pwm.h"
#include "mesures.h"

/* Variables globales : visibles dans la fenêtre « Watches » du débogueur */
consigne_t consigne = {0, 0, 0, 0};    /* dernière consigne valide        */
mesures_t  mesures  = {0, 0, 0, 0};    /* dernières mesures envoyées      */
uint16_t   d_applique = 0;             /* D réellement appliqué (pour mille) */
bool       moteur_autorise = false;
uint16_t   nb_trames_ok = 0;
uint16_t   nb_trames_fausses = 0;

int main(void)
{
    uint8_t  trame_rx[TRAME_LONGUEUR];
    uint8_t  trame_tx[TRAME_LONGUEUR];
    bool     deja_recu = false;
    uint32_t t_trame = 0;              /* instant de la dernière trame valide */
    uint32_t t_mesures = 0;
    uint32_t t_dernier_ms = 0;
    uint32_t maintenant;

    /* ---- 1) Sécurité immédiate : broches PWM en sortie, à 0 --------
       Avant tout réglage, les deux entrées du driver U6 sont à 0 :
       Q2 et Q3 restent bloqués.                                     */
    LATBbits.LATB14 = 0;
    LATBbits.LATB15 = 0;
    TRISBbits.TRISB14 = 0;
    TRISBbits.TRISB15 = 0;

    /* ---- 2) Initialisation (bloc « PWM1 ... trame 0xAA prête ») --- */
    horloge_init();                    /* FCY = 50 MHz                    */
    temps_init();                      /* base de temps 1 ms              */
    pwm_init();                        /* D = 0, Q2 et Q3 bloqués         */
    mesures_init();

    trame_mesures_construire(trame_tx, &mesures);   /* 0xAA avec des 0    */
    spi_init(trame_tx);                /* SPI1 esclave, pull-up sur SS    */

    INTCON2bits.GIE = 1;               /* interruptions autorisées        */

    /* ---- 3) Boucle principale (renvoi « A » de l'organigramme) ---- */
    while (1) {
        maintenant = temps_ms();

        /* a) Trame reçue (10 octets déjà contrôlés par spi_esclave.c) */
        if (spi_trame_lire(trame_rx)) {
            if (trame_consigne_lire(trame_rx, &consigne)) {
                t_trame = maintenant;          /* t_trame = maintenant  */
                deja_recu = true;
                nb_trames_ok++;
            } else {
                nb_trames_fausses++;           /* trame ignorée, ancienne */
            }                                  /* commande conservée      */
        }

        /* b) Sécurités : 500 ms, état, D <= 0,95 */
        moteur_autorise = commande_choisir(deja_recu, maintenant - t_trame,
                                           &consigne, &d_applique);

        /* c) PWM1H / PWM1L -> U6 -> Q2 / Q3 -> moteur : U = D x VBUS */
        if (moteur_autorise) {
            pwm_appliquer(d_applique);
        } else {
            pwm_arreter();
        }

        /* d) Une lecture des mesures par milliseconde */
        if (maintenant != t_dernier_ms) {
            t_dernier_ms = maintenant;
            mesures_echantillonner();
        }

        /* e) Toutes les 100 ms : préparer la trame 0xAA */
        if (maintenant - t_mesures >= PERIODE_MESURES_MS) {
            t_mesures = maintenant;
            mesures_calculer(&mesures, d_applique);
            trame_mesures_construire(trame_tx, &mesures);
            spi_reponse_preparer(trame_tx);
        }
    }
    return 0;
}
