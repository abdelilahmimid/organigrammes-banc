/* =====================================================================
   carte_voiture.h
   Tout ce qui dépend de la carte de la voiture (DriverShellEco) est ici :
   broches, horloge, PWM, mesures. Si une valeur change sur la carte,
   on ne modifie que ce fichier.

   dsPIC33CK32MP105, boîtier 48 broches (TQFP).
   Broches relevées dans KiCad, fonctions lues dans la table 5 de la
   fiche technique (DS70005363) :
     1  RB14 / PWM1H        2  RB15 / PWM1L        5  MCLR
     8  RA0 / OA1OUT / AN0  9  RA1 / OA1IN-       10  RA2 / OA1IN+
    17  RC6 / AN17  (THROTTLE, non lu sur le banc)
    20  RC3 / AN15  (VBUS/16)
    25  RB2 / AN1   (SHUNT_NORM+ : courant batterie, sortie de U5)
    33  RB5 / PGD3          34  RB6 / PGC3  (programmation par J16)
    38  RC4 / RP52  (SCK)   39  RC5 / RP53  (SS)
    40  RC10 / RP58 (SDI)   41  RC11 / RP59 (SDO)
    47  RB12        (SEL : choix du gain de U5)
   Pas de quartz : les broches 21 / 22 (OSCI / OSCO) servent à
   DI_Vitesse et AN_Vitesse.
   ===================================================================== */
#ifndef CARTE_VOITURE_H
#define CARTE_VOITURE_H

/* ---------------------------------------------------------------------
   Horloge : oscillateur interne FRC 8 MHz + PLL (pas de quartz utilisé)
     FVCO  = 8 MHz x 125      = 1000 MHz
     FPLLO = 1000 MHz / 5 / 1 =  200 MHz
     FOSC  = FPLLO / 2        =  100 MHz  (horloge du PWM)
     FCY   = FOSC / 2         =   50 MHz  (50 millions d'instructions / s)
   --------------------------------------------------------------------- */
#define FOSC_HZ             100000000UL
#define FCY_HZ               50000000UL

/* ---------------------------------------------------------------------
   SPI1 esclave : numéros RPn des broches 38 à 41 (toutes remappables)
   --------------------------------------------------------------------- */
#define RP_SCK              52      /* broche 38, RC4  */
#define RP_SS               53      /* broche 39, RC5  */
#define RP_SDI              58      /* broche 40, RC10 */
#define RP_SDO              59      /* broche 41, RC11 */

/* 0 : câble J2 avec les fils 3 et 4 croisés (solution actuelle)
   1 : câble droit, le dsPIC échange SDI et SDO par PPS
       (SDI sur la broche 41, SDO sur la broche 40)                     */
#define SPI_INVERSER_SDI_SDO 0

/* ---------------------------------------------------------------------
   PWM1 complémentaire (PWM1H = MOSFET du haut Q2, PWM1L = MOSFET du bas Q3)
   Une unité de compteur = 1 / FOSC = 10 ns
   --------------------------------------------------------------------- */
#define PWM_FREQ_HZ         20000UL                          /* 20 kHz  */
#define PWM_PERIODE         ((FOSC_HZ / PWM_FREQ_HZ) - 1UL)  /* 4999    */
#define PWM_TEMPS_MORT      20U          /* 20 x 10 ns = 200 ns         */
#define D_MAX_POUR_MILLE    950U         /* D <= 0,95 (bootstrap C27)   */

/* ---------------------------------------------------------------------
   Mesures (CAN 12 bits, référence AVDD = 3,3 V)
   --------------------------------------------------------------------- */
#define ADC_VOIE_MOT_CURR   0U      /* AN0  = OA1OUT (broche 8)         */
#define ADC_VOIE_BAT_CURR   1U      /* AN1  = SHUNT_NORM+ (broche 25)   */
#define ADC_VOIE_VBUS       15U     /* AN15 = VBUS/16 (broche 20)       */
#define VREF_MV             3300UL
#define ADC_PLEINE_ECHELLE  4095UL

/* VBUS : pont R7 = 36 k / R8 = 2,4 k -> 2,4 / 38,4 = 1/16             */
#define DIVISEUR_VBUS       16UL

/* Courant moteur : shunt R50 = 2 mOhm entre J3 broche 2 et GND
   -> R34 / R32 (510 Ohm) -> R33 / R31 (390 Ohm) -> OA1, contre-réaction
   R29 = 11 k, décalage par R30 = 13 k vers 3,3 V.
     gain   = (1 + 11 k / 900) x 13 k / 13,51 k = 12,72
     sortie = 1,647 V + 12,72 x 2 mOhm x I = 1,647 V + 25,45 mV/A x I
   Plage : environ -65 A à +65 A. Le zéro (1,647 V) est mesuré au
   démarrage, Q2 et Q3 bloqués, donc courant nul.                      */
#define MOT_CURR_UV_PAR_A   25446L  /* microvolts par ampère            */

/* Courant batterie : shunt R35 = 2 mOhm côté +48 V -> U5 TSC1031
   (gain 50 ou 100 choisi par SEL, broche 47) -> R39 1 k / C25 -> AN1  */
#define SHUNT_BAT_MILLIOHM  2L
#define GAIN_TSC1031        100L
#define SEL_NIVEAU          1       /* niveau donné à SEL (broche 47)  */

/* ---------------------------------------------------------------------
   Rythmes du programme (organigramme B)
   --------------------------------------------------------------------- */
#define DELAI_PERTE_MS      500UL   /* plus de trame valide -> arrêt    */
#define PERIODE_MESURES_MS  100UL   /* nouvelle trame 0xAA              */

#endif
