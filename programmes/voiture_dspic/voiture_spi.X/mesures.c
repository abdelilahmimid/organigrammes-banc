/* =====================================================================
   mesures.c : lecture de la tension batterie et des deux courants

   Tension batterie : pont R7 / R8 (VBUS/16) -> broche 20 (AN15)
       48 V / 16 = 3,0 V sur la broche (maximum lisible 52,8 V)
   Courant moteur   : shunt R50 (2 mOhm, côté GND du moteur) -> OA1
       -> OA1OUT broche 8 (AN0) ; 1,647 V au repos, 25,45 mV par ampère
   Courant batterie : shunt R35 (2 mOhm, côté +48 V) -> U5 TSC1031
       -> R39 / C25 -> broche 25 (AN1)

   Le CAN est 12 bits : 0 V -> 0, 3,3 V -> 4 095.
   On lit chaque voie toutes les 1 ms et on fait la moyenne sur 100 ms :
   la moyenne lisse l'ondulation créée par le PWM à 20 kHz.
   ===================================================================== */
#include <xc.h>
#include "mesures.h"
#include "carte_voiture.h"

static uint32_t somme_vbus = 0;
static uint32_t somme_mot = 0;
static uint32_t somme_bat = 0;
static uint16_t nb_lectures = 0;
static uint16_t zero_mot = 2044;    /* lecture à courant nul (1,647 V), */
                                    /* remesurée au démarrage           */

/* Lit une voie du CAN (déclenchement logiciel), avec délai maximal
   pour ne jamais bloquer le programme si le CAN ne répond pas.        */
static uint16_t adc_lire(uint8_t voie)
{
    uint16_t attente = 10000;

    ADCON3Lbits.CNVCHSEL = voie;    /* voie à convertir      */
    ADCON3Lbits.CNVRTCH  = 1;       /* lancer la conversion  */

    if (voie == ADC_VOIE_MOT_CURR) {
        while (ADSTATLbits.AN0RDY == 0 && --attente) { }
        return ADCBUF0;
    } else if (voie == ADC_VOIE_BAT_CURR) {
        while (ADSTATLbits.AN1RDY == 0 && --attente) { }
        return ADCBUF1;
    } else {
        while (ADSTATLbits.AN15RDY == 0 && --attente) { }
        return ADCBUF15;
    }
}

void mesures_init(void)
{
    uint32_t somme = 0;
    uint8_t i;

    /* --- Broches analogiques (entrées) ---------------------------- */
    TRISAbits.TRISA0 = 1;  ANSELAbits.ANSELA0 = 1;   /* OA1OUT / AN0  */
    TRISAbits.TRISA1 = 1;  ANSELAbits.ANSELA1 = 1;   /* OA1IN-        */
    TRISAbits.TRISA2 = 1;  ANSELAbits.ANSELA2 = 1;   /* OA1IN+        */
    TRISBbits.TRISB2 = 1;  ANSELBbits.ANSELB2 = 1;   /* SHUNT_NORM+, AN1 */
    TRISCbits.TRISC3 = 1;  ANSELCbits.ANSELC3 = 1;   /* VBUS/16, AN15 */
    TRISCbits.TRISC6 = 1;  ANSELCbits.ANSELC6 = 1;   /* THROTTLE : entrée, non lue */

    /* --- SEL de U5 (broche 47, RB12) : choix du gain 50 ou 100 ----- */
    LATBbits.LATB12  = SEL_NIVEAU;
    TRISBbits.TRISB12 = 0;

    /* --- Ampli OA1 (OA2 reste éteint : la broche 25 est une entrée) - */
    AMPCON1Hbits.NCHDIS1 = 0;       /* entrées de 0 V à 3,3 V              */
    AMPCON1Lbits.AMPEN1  = 1;       /* OA1 activé                          */
    AMPCON1Lbits.AMPON   = 1;       /* bloc des amplis allumé              */

    /* --- CAN ------------------------------------------------------ */
    ADCON1L = 0;                    /* ADON = 0 pendant le réglage         */
    ADCON1H = 0;
    ADCON2L = 0;
    ADCON2H = 0;
    ADCON3L = 0;
    ADCON3H = 0;
    ADCON5L = 0;
    ADCON5H = 0;

    ADCON1Hbits.FORM   = 0;         /* résultat entier                     */
    ADCON1Hbits.SHRRES = 3;         /* 12 bits (cœur partagé : AN15)       */
    ADCON3Hbits.CLKSEL = 0;         /* horloge système                     */
    ADCON3Hbits.CLKDIV = 3;         /* divisée par 4                       */
    ADCON2Lbits.SHRADCS = 2;        /* puis par 4 : CAN lent mais précis   */
    ADCON2Hbits.SHRSAMC = 15;       /* temps d'échantillonnage : 17 TAD    */

    ADCORE0Hbits.RES  = 3;          /* 12 bits (cœur dédié 0 : AN0)        */
    ADCORE0Hbits.ADCS = 2;
    ADCORE0Lbits.SAMC = 15;
    ADCORE1Hbits.RES  = 3;          /* 12 bits (cœur dédié 1 : AN1)        */
    ADCORE1Hbits.ADCS = 2;
    ADCORE1Lbits.SAMC = 15;

    ADCON5Hbits.WARMTIME = 15;      /* temps de chauffe des cœurs          */
    ADCON1Lbits.ADON = 1;           /* CAN allumé                          */

    ADCON5Lbits.SHRPWR = 1;         /* alimenter le cœur partagé           */
    while (ADCON5Lbits.SHRRDY == 0) { }
    ADCON3Hbits.SHREN = 1;

    ADCON5Lbits.C0PWR = 1;          /* alimenter le cœur dédié 0           */
    while (ADCON5Lbits.C0RDY == 0) { }
    ADCON3Hbits.C0EN = 1;

    ADCON5Lbits.C1PWR = 1;          /* alimenter le cœur dédié 1           */
    while (ADCON5Lbits.C1RDY == 0) { }
    ADCON3Hbits.C1EN = 1;

    /* --- Zéro du courant moteur ------------------------------------
       Appelé quand Q2 et Q3 sont bloqués : aucun courant dans le moteur.
       La moyenne de 64 lectures donne la valeur « 0 A » (environ 2 044). */
    for (i = 0; i < 64U; i++) {
        somme += adc_lire(ADC_VOIE_MOT_CURR);
    }
    zero_mot = (uint16_t)(somme / 64UL);
}

void mesures_echantillonner(void)
{
    somme_vbus += adc_lire(ADC_VOIE_VBUS);
    somme_mot  += adc_lire(ADC_VOIE_MOT_CURR);
    somme_bat  += adc_lire(ADC_VOIE_BAT_CURR);
    nb_lectures++;
}

static int16_t borner_int16(int32_t x)
{
    if (x >  32767L) return  32767;
    if (x < -32768L) return -32768;
    return (int16_t)x;
}

void mesures_calculer(mesures_t *m, uint16_t d_pour_mille)
{
    uint32_t moy_vbus, moy_mot, moy_bat, vbus_mV, v_bat_mV;
    int32_t  ecart_uV;

    if (nb_lectures == 0U) {
        return;                     /* rien lu : on garde les anciennes valeurs */
    }
    moy_vbus = somme_vbus / nb_lectures;
    moy_mot  = somme_mot  / nb_lectures;
    moy_bat  = somme_bat  / nb_lectures;
    somme_vbus = 0;
    somme_mot  = 0;
    somme_bat  = 0;
    nb_lectures = 0;

    /* Tension batterie : VBUS = lecture x 3 300 mV / 4 095 x 16 */
    vbus_mV = (moy_vbus * VREF_MV * DIVISEUR_VBUS) / ADC_PLEINE_ECHELLE;
    m->bat_volt_mV = (uint16_t)vbus_mV;

    /* Tension moteur calculée : U = D x VBUS (pas de mesure sur la carte) */
    m->mot_volt_mV = (uint16_t)((vbus_mV * d_pour_mille) / 1000UL);

    /* Courant moteur : écart au zéro en microvolts, puis / 25 446 uV/A
       ex. +100 lectures -> 80 586 uV -> 3 167 mA                        */
    /* (calcul en deux temps pour rester sous 2 milliards en 32 bits) */
    ecart_uV = ((((int32_t)moy_mot - (int32_t)zero_mot) * (int32_t)VREF_MV * 100L)
               / (int32_t)ADC_PLEINE_ECHELLE) * 10L;
    m->mot_curr_mA = borner_int16((ecart_uV * 1000L) / MOT_CURR_UV_PAR_A);

    /* Courant batterie : I = V / (gain x shunt) ; en mA : V_mV x 1 000 / (G x R_mOhm)
       ex. 0,56 V avec G = 100 -> 560 x 1 000 / 200 = 2 800 mA            */
    v_bat_mV = (moy_bat * VREF_MV) / ADC_PLEINE_ECHELLE;
    m->bat_curr_mA = borner_int16(((int32_t)v_bat_mV * 1000L)
                                  / (GAIN_TSC1031 * SHUNT_BAT_MILLIOHM));
}
