// =====================================================================
//  couple.cpp : couple au rouleau
//  SIG_OUT du couplemètre (0 à 5 V) -> filtre RC -> A0.
//  Une lecture par milliseconde ; deux moyennes séparées :
//    - une pour le frein (toutes les 10 ms, environ 10 lectures),
//    - une pour la trame DATA (toutes les 50 ms, environ 50 lectures).
// =====================================================================
#include "couple.h"
#include "config.h"

static uint32_t sommeFrein = 0, sommeData = 0;
static uint16_t nbFrein    = 0, nbData    = 0;
static float    dernierFrein = 0.0, dernierData = 0.0;

// Lecture brute (0 à 1023) -> N.m au rouleau
static float versNm(float lectureMoyenne)
{
    float tension = lectureMoyenne * VREF_ADC_V / 1023.0;
    return (tension - COUPLE_ZERO_V) * COUPLE_NM_PAR_V;
}

void couple_echantillonner()
{
    uint16_t brut = analogRead(PIN_COUPLE);
    sommeFrein += brut;  nbFrein++;
    sommeData  += brut;  nbData++;
}

float couple_moyenne_frein()
{
    if (nbFrein > 0) {
        dernierFrein = versNm((float)sommeFrein / nbFrein);
        sommeFrein = 0;
        nbFrein = 0;
    }
    return dernierFrein;           // aucune lecture : on garde la précédente
}

float couple_moyenne_data()
{
    if (nbData > 0) {
        dernierData = versNm((float)sommeData / nbData);
        sommeData = 0;
        nbData = 0;
    }
    return dernierData;
}
