/* =====================================================================
   horloge.c
   1) PLL : le dsPIC démarre sur le FRC (8 MHz) et passe sur la PLL.
   2) Timer1 : compte les millisecondes (équivalent de millis()).
   ===================================================================== */
#include <xc.h>
#include "horloge.h"
#include "carte_voiture.h"

static volatile uint32_t compteur_ms = 0;

void horloge_init(void)
{
    /* N1 = 1, M = 125, N2 = 5, N3 = 1
       FVCO = 8 x 125 / 1 = 1000 MHz (plage permise 400 à 1600 MHz)
       FPLLO = 1000 / (5 x 1) = 200 MHz -> FOSC = 100 MHz, FCY = 50 MHz */
    CLKDIVbits.PLLPRE    = 1;
    PLLFBDbits.PLLFBDIV  = 125;
    PLLDIVbits.POST1DIV  = 5;
    PLLDIVbits.POST2DIV  = 1;

    /* Changement d'horloge vers « FRC + PLL » (NOSC = 0b001) */
    __builtin_write_OSCCONH(0x01);
    __builtin_write_OSCCONL(OSCCON | 0x01);
    while (OSCCONbits.OSWEN != 0) { }   /* attendre la fin du changement */
    while (OSCCONbits.LOCK != 1) { }    /* attendre que la PLL soit verrouillée */
}

void temps_init(void)
{
    /* Timer1 sur FCY = 50 MHz, prédiviseur 1:8 -> 6,25 MHz
       6 250 impulsions = 1 ms -> PR1 = 6 249                         */
    T1CON = 0;
    T1CONbits.TCKPS = 1;                /* 1:8 */
    TMR1 = 0;
    PR1 = (uint16_t)((FCY_HZ / 8UL / 1000UL) - 1UL);

    _T1IP = 3;                          /* priorité plus basse que le SPI */
    _T1IF = 0;
    _T1IE = 1;
    T1CONbits.TON = 1;
}

uint32_t temps_ms(void)
{
    /* Le dsPIC est un processeur 16 bits : lire 32 bits prend deux
       instructions. On coupe les interruptions pendant la copie pour
       ne pas lire une valeur à moitié mise à jour.                    */
    uint32_t t;
    INTCON2bits.GIE = 0;
    t = compteur_ms;
    INTCON2bits.GIE = 1;
    return t;
}

void __attribute__((interrupt, no_auto_psv)) _T1Interrupt(void)
{
    compteur_ms++;
    _T1IF = 0;
}
