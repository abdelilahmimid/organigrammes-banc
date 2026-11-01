// =====================================================================
//  frein.cpp : la pente simulée par le frein à hystérésis
//  Suit PENTE.docx (organigrammes C et D), avec les corrections décidées :
//    - en défaut : frein COUPÉ (et non alpha = 0,95) ;
//    - frein autorisé seulement si : essai en cours (RUNNING), pas de défaut
//      thermique, pas d'arrêt d'urgence, température mesurée il y a < 2,5 s ;
//    - pas de terme J x dw/dt pour l'instant ;
//    - pente max en paramètre (config.h) ;
//    - freinage de fin après STOP : le frein arrête le rouleau
//      (organigramme C, case « hors essai : FREINAGE »).
//
//  IMPORTANT : la commande est EN TENSION (rapport cyclique alpha).
//  La carte ne mesure pas le courant de la bobine : I_bobine est ESTIMÉ
//  par I = alpha x 24 V / 64 Ohm (résistance à froid). Quand la bobine
//  chauffe, le vrai courant est plus faible : c'est le PI, avec le couple
//  mesuré par le couplemètre, qui corrige.
//
//  Chaîne : Timer 4 -> D6 (PWMH) / D7 (PWMHL) -> driver MIC4104
//           -> demi-pont 24 V -> J4 -> bobine du frein.
//  Ne JAMAIS utiliser digitalWrite() ni analogWrite() sur D6 / D7.
// =====================================================================
#include "frein.h"
#include "config.h"
#include "banc_etat.h"
#include "temperature.h"
#include "vitesse.h"
#include "couple.h"
#include <util/atomic.h>

static float    alpha       = 0.0;   // rapport cyclique appliqué
static float    consigneNm  = 0.0;   // consigne au volant après la rampe
static float    integrale   = 0.0;   // terme I du PI
static uint16_t compteurSatMs = 0;
static bool     sature      = false;

// Freinage de fin
static bool     freinage      = false;   // vrai pendant le freinage de fin
static uint32_t tDebutFreinageMs = 0;
static EtatBanc etatPrecedent = IDLE;    // pour voir le passage RUNNING/PAUSED -> STOPPED

// ---------------------------------------------------------------------
// Timer 4 : PWM 20 kHz complémentaire avec temps mort
//   Mode 10 : PWM phase correcte, TOP = ICR4 = 400
//   f = 16 MHz / (2 x 400) = 20 kHz ; 1 pas = 62,5 ns
//   OC4A (D6, transistor du haut) : normal   -> à 1 quand compteur < OCR4A
//   OC4B (D7, transistor du bas)  : inversé  -> à 1 quand compteur > OCR4B
//   OCR4B = OCR4A + 8 : 8 pas x 62,5 ns = 0,5 us où les deux sont à 0,
//   avant chaque allumage (temps mort).
// ---------------------------------------------------------------------
static void ecrireTimer(uint16_t ocrA, uint16_t ocrB)
{
    ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {     // registres 16 bits : écriture d'un bloc
        OCR4A = ocrA;
        OCR4B = ocrB;
    }
}

static void couper()
{
    // OCR4A = 0 -> D6 toujours à 0 ; OCR4B = TOP -> D7 toujours à 0
    // Les deux transistors sont bloqués : plus de courant dans la bobine.
    ecrireTimer(0, TIMER4_TOP);
}

static void appliquer(float a)
{
    uint16_t ocrA = (uint16_t)(a * TIMER4_TOP + 0.5);
    if (ocrA == 0) { couper(); return; }
    ecrireTimer(ocrA, ocrA + TEMPS_MORT_TICKS);   // au max 380 + 8 = 388 < 400
}

void frein_init()
{
    // 1) Arrêter le Timer 4 (l'Arduino l'avait réglé pour analogWrite)
    TCCR4A = 0;
    TCCR4B = 0;
    TCCR4C = 0;
    TCNT4  = 0;
    // 2) Période et frein coupé AVANT de relier les sorties
    ICR4  = TIMER4_TOP;
    OCR4A = 0;
    OCR4B = TIMER4_TOP;
    // 3) Sorties : OC4A normal, OC4B inversé ; mode 10 ; horloge 16 MHz sans division
    TCCR4A = _BV(COM4A1) | _BV(COM4B1) | _BV(COM4B0) | _BV(WGM41);
    TCCR4B = _BV(WGM43) | _BV(CS40);
    // 4) D6 (PH3) et D7 (PH4) en sortie : le timer les pilote à 0
    DDRH |= _BV(PH3) | _BV(PH4);

    alpha = 0.0;
    integrale = 0.0;
}

// ---------------------------------------------------------------------
// Freinage de fin : quand commence-t-il, quand s'arrête-t-il ?
//   Début : l'opérateur envoie STOP pendant un essai (RUNNING ou PAUSED
//           -> STOPPED), sans arrêt d'urgence. La voiture reçoit état = 0
//           et ne pousse plus : le frein arrête le rouleau et le volant.
//   Fin   : rouleau arrêté (plus d'impulsion sur D2 depuis 1 s),
//           ou 30 s écoulées (sécurité), ou nouvel état (START, RESET, FAULT).
// ---------------------------------------------------------------------
static void suivreFreinage()
{
    if (etat == STOPPED && (etatPrecedent == RUNNING || etatPrecedent == PAUSED)
        && !urgenceActive) {
        freinage = true;
        tDebutFreinageMs = millis();
    }
    etatPrecedent = etat;

    if (freinage) {
        bool rouleauArrete = (vitesse_trmin() <= 0.0);
        bool tropLong      = (millis() - tDebutFreinageMs >= FREINAGE_DUREE_MAX_MS);
        if (etat != STOPPED || rouleauArrete || tropLong) freinage = false;
    }
}

// ---------------------------------------------------------------------
// Toutes les 10 ms : organigramme C (consigne et rampe) puis D (alpha)
// ---------------------------------------------------------------------
void frein_cycle()
{
    const float te = PERIODE_FREIN_MS / 1000.0;       // 0,01 s

    suivreFreinage();

    // Lire le couplemètre (moyenne des ~10 lectures) et le ramener au volant
    const float coupleRouleau = couple_moyenne_frein();
    const float coupleFrein   = coupleRouleau / RAPPORT_COURROIE;

    // Le frein a-t-il le droit d'être alimenté ?
    //   Essai en cours (RUNNING) ou freinage de fin, et dans les deux cas :
    //   pas de défaut thermique, pas d'arrêt d'urgence, température < 2,5 s.
    const bool modeEssai = (etat == RUNNING);
    const bool autorise = (modeEssai || freinage) && !defautThermique
                          && !urgenceActive && temperature_fraiche();
    if (!autorise) {
        consigneNm    = 0.0;           // à la reprise, la rampe repart de 0
        integrale     = 0.0;
        alpha         = 0.0;
        compteurSatMs = 0;
        sature        = false;
        freinage      = false;
        couper();
        return;
    }

    // --- Organigramme C : cible selon le mode, puis rampe de 0,5 N.m/s ------
    //   Essai    : cible = 0,22 N.m par % de pente
    //   Freinage : cible = 1,3 N.m (on part de la consigne de l'essai : pas d'à-coup)
    const float cible = modeEssai ? COUPLE_PAR_POURCENT * consignePente
                                  : FREINAGE_COUPLE_NM;
    const float pas   = RAMPE_NM_PAR_S * te;                    // 0,005 N.m par cycle
    if (consigneNm < cible - pas)      consigneNm += pas;
    else if (consigneNm > cible + pas) consigneNm -= pas;
    else                               consigneNm = cible;

    // --- Organigramme D : anticipation + PI --------------------------------
    // Anticipation (fiche du frein, bobine froide) :
    //   I_bobine estimé = consigne / 3,75 ; alpha_ff = I x 64 / 24
    const float iEstime = consigneNm / FREIN_NM_PAR_A;
    const float alphaFF = iEstime * R_BOBINE_OHM / U_FREIN_V;

    float e = 0.0;
    // PI seulement pendant l'essai. Pendant le freinage de fin, anticipation
    // seule : le rouleau ralentit, le couplemètre voit aussi le couple qui
    // freine le volant d'inertie (J x dw/dt non compensé) et pourrait sortir
    // de sa plage (1,3 N.m au volant = 5,2 N.m au rouleau). Le but est
    // seulement d'arrêter le rouleau, pas de tenir un couple précis.
    bool boucleFermee = modeEssai
                        && (vitesse_trmin() >= VITESSE_MIN_PI_TRMIN) && (consigneNm > 0.0);
    if (boucleFermee) {
        e = consigneNm - coupleFrein;                      // erreur en N.m
        float alphaBrut = alphaFF + KP_FREIN * e + integrale;
        // Anti-emballement : I n'est mis à jour que si alpha n'est pas bloqué
        bool bloqueHaut = (alphaBrut >= ALPHA_MAX) && (e > 0.0);
        bool bloqueBas  = (alphaBrut <= 0.0) && (e < 0.0);
        if (!bloqueHaut && !bloqueBas) {
            integrale += KI_FREIN * e * te;
        }
        alpha = alphaFF + KP_FREIN * e + integrale;
    } else {
        // Sous 20 tr/min (mesure de couple pas fiable) ou freinage de fin :
        // anticipation seule
        integrale = 0.0;
        alpha = alphaFF;
    }

    // alpha borné entre 0 et 0,95
    if (alpha > ALPHA_MAX) alpha = ALPHA_MAX;
    if (alpha < 0.0)       alpha = 0.0;
    appliquer(alpha);

    // --- Alerte BRAKE_SAT (pendant l'essai seulement) : frein à fond sans
    //     atteindre la consigne pendant 1 s
    bool saturationNow = modeEssai && (boucleFermee ? (alpha >= ALPHA_MAX && e > SAT_ECART_NM)
                                                    : (alphaFF > ALPHA_MAX));
    if (saturationNow) {
        if (compteurSatMs < SAT_DELAI_MS) compteurSatMs += PERIODE_FREIN_MS;
        if (compteurSatMs >= SAT_DELAI_MS) sature = true;
    } else {
        compteurSatMs = 0;
        sature = false;
    }
}

float   frein_alpha()     { return alpha; }
uint8_t frein_pwm255()    { return (uint8_t)(alpha * 255.0 + 0.5); }
float   frein_consigne()  { return consigneNm; }
bool    frein_sature()    { return sature; }
bool    frein_freinage()  { return freinage; }

// Couple estimé au volant : I estimé (24 V / 64 Ohm, à froid) x 3,75 N.m/A
float frein_couple_estime()
{
    return alpha * U_FREIN_V / R_BOBINE_OHM * FREIN_NM_PAR_A;
}
