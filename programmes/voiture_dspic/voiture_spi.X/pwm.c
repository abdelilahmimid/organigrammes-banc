/* =====================================================================
   pwm.c : PWM1 en mode complémentaire
     PWM1H (broche 1) -> U6 MIC4104 entrée HI -> Q2 (MOSFET du haut)
     PWM1L (broche 2) -> U6 MIC4104 entrée LI -> Q3 (MOSFET du bas)
   Complémentaire : quand Q2 conduit, Q3 est bloqué et inversement.
   Temps mort : 200 ns où les deux sont bloqués à chaque commutation,
   pour ne jamais court-circuiter le 48 V à travers Q2 et Q3.

   Tension moyenne sur le moteur : U = D x VBUS.
   ===================================================================== */
#include <xc.h>
#include "pwm.h"
#include "carte_voiture.h"

static uint8_t sorties_bloquees = 1;

void pwm_init(void)
{
    /* Horloge du PWM : FOSC = 100 MHz -> 1 unité = 10 ns */
    PCLKCON = 0;
    PCLKCONbits.MCLKSEL = 0;        /* 00 = FOSC */

    PG1CONL = 0;                    /* générateur arrêté pendant le réglage */
    PG1CONLbits.CLKSEL = 1;         /* horloge choisie par MCLKSEL          */
    PG1CONLbits.MODSEL = 0;         /* PWM « front montant au début »       */
    PG1CONLbits.HREN   = 0;         /* résolution normale (10 ns)           */
    PG1CONH = 0;                    /* registres propres à PG1, mise à jour */
                                    /* en fin de période                    */

    PG1IOCONH = 0;
    PG1IOCONHbits.PMOD = 0;         /* 00 = complémentaire                  */
    PG1IOCONHbits.PENH = 1;         /* le PWM pilote la broche 1            */
    PG1IOCONHbits.PENL = 1;         /* le PWM pilote la broche 2            */
    PG1IOCONHbits.POLH = 0;         /* actif à 1                            */
    PG1IOCONHbits.POLL = 0;

    /* Forçage au démarrage : PWM1H = 0 et PWM1L = 0 -> Q2 et Q3 bloqués */
    PG1IOCONL = 0;
    PG1IOCONLbits.OVRDAT = 0;       /* niveau forcé : H = 0, L = 0          */
    PG1IOCONLbits.OVRENH = 1;
    PG1IOCONLbits.OVRENL = 1;
    sorties_bloquees = 1;

    PG1PHASE = 0;
    PG1PER   = (uint16_t)PWM_PERIODE;   /* 4 999 -> 50 us -> 20 kHz        */
    PG1DC    = 0;                       /* D = 0                           */
    PG1DTH   = PWM_TEMPS_MORT;          /* avant que Q2 conduise           */
    PG1DTL   = PWM_TEMPS_MORT;          /* avant que Q3 conduise           */

    PG1CONLbits.ON = 1;                 /* démarrage du générateur         */
}

void pwm_appliquer(uint16_t d_pour_mille)
{
    uint32_t dc;

    if (d_pour_mille > D_MAX_POUR_MILLE) {
        d_pour_mille = D_MAX_POUR_MILLE;    /* dernière protection : D <= 0,95 */
    }
    /* temps haut = D x période ; ex. 620 -> 620 x 5 000 / 1 000 = 3 100 */
    dc = ((uint32_t)d_pour_mille * (PWM_PERIODE + 1UL)) / 1000UL;
    PG1DC = (uint16_t)dc;
    PG1STATbits.UPDREQ = 1;                 /* prise en compte en fin de période */

    if (sorties_bloquees) {
        /* Fin du forçage : Q3 conduit d'abord (D petit au départ) et
           recharge le condensateur de bootstrap C27.                    */
        PG1IOCONLbits.OVRENH = 0;
        PG1IOCONLbits.OVRENL = 0;
        sorties_bloquees = 0;
    }
}

void pwm_arreter(void)
{
    PG1IOCONLbits.OVRDAT = 0;
    PG1IOCONLbits.OVRENH = 1;               /* Q2 bloqué */
    PG1IOCONLbits.OVRENL = 1;               /* Q3 bloqué */
    sorties_bloquees = 1;
    PG1DC = 0;
    PG1STATbits.UPDREQ = 1;
}
