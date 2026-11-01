/* =====================================================================
   spi_esclave.c : SPI1 en esclave, copie du simulateur Uno.

   Le Mega (maître) impose l'horloge SCK à 250 kHz. Le dsPIC suit :
   à chaque front de SCK il lit un bit sur SDI et sort un bit sur SDO.
   Après 8 fronts, un octet est reçu ET un octet est envoyé.

   Deux interruptions, comme sur l'Uno :
     - _SPI1RXInterrupt : un octet vient d'être échangé
                          (équivalent de ISR(SPI_STC_vect))
     - _CNCInterrupt    : la broche SS (RC5) a changé d'état
                          (équivalent de ISR(PCINT0_vect))
   ===================================================================== */
#include <xc.h>
#include "spi_esclave.h"
#include "carte_voiture.h"

static volatile uint8_t tx[TRAME_LONGUEUR];          /* trame 0xAA en cours d'envoi */
static volatile uint8_t tx_suivante[TRAME_LONGUEUR]; /* prochaine trame 0xAA        */
static volatile bool    tx_suivante_prete = false;

static volatile uint8_t rx[TRAME_LONGUEUR];          /* octets en cours de réception */
static volatile uint8_t rx_complet[TRAME_LONGUEUR];  /* dernière trame de 10 octets  */
static volatile bool    rx_nouvelle = false;
static volatile uint8_t nb_octets = 0;               /* n de l'organigramme B        */

volatile uint16_t spi_nb_echanges_incomplets = 0;

/* ---------------------------------------------------------------------
   Lit tous les octets arrivés et prépare l'octet suivant à envoyer
   --------------------------------------------------------------------- */
static void lire_octets_recus(void)
{
    while (SPI1STATLbits.SPIRBE == 0) {          /* tampon de réception non vide */
        uint8_t octet = (uint8_t)SPI1BUFL;       /* lire SDI (broche 40)         */
        if (nb_octets < TRAME_LONGUEUR) {
            rx[nb_octets] = octet;
        }
        if (nb_octets < 255U) {
            nb_octets++;
        }
        /* octet suivant de 0xAA sur SDO (broche 41) ; 0 au-delà du 10e */
        if (SPI1STATLbits.SPITBF == 0) {
            SPI1BUFL = (nb_octets < TRAME_LONGUEUR) ? tx[nb_octets] : 0x00;
        }
    }
    if (SPI1STATLbits.SPIROV) {                  /* octet perdu : on efface l'erreur */
        SPI1STATLbits.SPIROV = 0;
    }
}

/* ---------------------------------------------------------------------
   Remet le module SPI à zéro et charge le 1er octet de la trame 0xAA.
   Couper SPIEN vide les tampons et remet le compteur de bits à 0 :
   un échange coupé au milieu ne décale pas l'échange suivant.
   --------------------------------------------------------------------- */
static void spi_redemarrer(void)
{
    SPI1CON1Lbits.SPIEN = 0;
    SPI1CON1Lbits.SPIEN = 1;
    SPI1BUFL = tx[0];                            /* 0xAA prêt avant le 1er front */
    _SPI1RXIF = 0;
}

void spi_init(const uint8_t premiere_reponse[TRAME_LONGUEUR])
{
    uint8_t i;
    for (i = 0; i < TRAME_LONGUEUR; i++) {
        tx[i] = premiere_reponse[i];
    }

    /* --- 1) Sens des broches --------------------------------------- */
    TRISCbits.TRISC4  = 1;      /* SCK : entrée (horloge du Mega)       */
    TRISCbits.TRISC5  = 1;      /* SS  : entrée                         */
    TRISCbits.TRISC10 = 1;      /* SDI (ou SDO si inversion)            */
    TRISCbits.TRISC11 = 1;      /* SDO (ou SDI si inversion)            */
#if SPI_INVERSER_SDI_SDO
    TRISCbits.TRISC10 = 0;      /* broche 40 en sortie                  */
#else
    TRISCbits.TRISC11 = 0;      /* broche 41 en sortie                  */
#endif

    /* Pull-up interne sur SS : câble débranché -> SS = 1 -> rien lu    */
    CNPUCbits.CNPUC5 = 1;

    /* --- 2) PPS : relier SPI1 aux broches 38 à 41 ------------------- */
    __builtin_write_RPCON(0x0000);              /* déverrouiller le PPS */
    RPINR20bits.SCK1R = RP_SCK;                 /* SCK1 entrée <- RP52  */
    RPINR21bits.SS1R  = RP_SS;                  /* SS1 entrée  <- RP53  */
#if SPI_INVERSER_SDI_SDO
    RPINR20bits.SDI1R = RP_SDO;                 /* SDI1 <- RP59 (broche 41) */
    RPOR13bits.RP58R  = 5;                      /* RP58 (broche 40) -> SDO1 */
    RPOR13bits.RP59R  = 0;
#else
    RPINR20bits.SDI1R = RP_SDI;                 /* SDI1 <- RP58 (broche 40) */
    RPOR13bits.RP59R  = 5;                      /* RP59 (broche 41) -> SDO1 */
    RPOR13bits.RP58R  = 0;
#endif
    __builtin_write_RPCON(0x0800);              /* reverrouiller (IOLOCK = 1) */

    /* --- 3) Module SPI1 ------------------------------------------- */
    SPI1CON1L = 0;              /* SPIEN = 0 pendant le réglage         */
    SPI1CON1H = 0;              /* pas de mode audio ni de trame        */
    SPI1CON2L = 0;              /* mots de 8 bits                       */
    SPI1STATL = 0;
    SPI1CON1Lbits.MSTEN  = 0;   /* esclave : SCK vient du Mega          */
    SPI1CON1Lbits.MODE32 = 0;   /* MODE32 = 0, MODE16 = 0 -> 8 bits     */
    SPI1CON1Lbits.MODE16 = 0;
    SPI1CON1Lbits.CKP    = 0;   /* mode 0 : horloge au repos à 0        */
    SPI1CON1Lbits.CKE    = 1;   /* mode 0 : sortie sur le front 1 -> 0, */
                                /* lecture sur le front 0 -> 1          */
    SPI1CON1Lbits.SMP    = 0;   /* obligatoire en esclave               */
    SPI1CON1Lbits.SSEN   = 1;   /* la broche SS est utilisée            */
    SPI1CON1Lbits.ENHBUF = 0;   /* tampon simple (1 octet), comme l'Uno */

    /* --- 4) Interruption « octet reçu » ----------------------------- */
    _SPI1RXIP = 5;
    _SPI1RXIF = 0;
    _SPI1RXIE = 1;

    /* --- 5) Interruption sur changement de SS (RC5), deux fronts ---- */
    CNCONCbits.ON      = 1;
    CNCONCbits.CNSTYLE = 1;     /* détection de fronts                  */
    CNEN0Cbits.CNEN0C5 = 1;     /* un des deux fronts                   */
    CNEN1Cbits.CNEN1C5 = 1;     /* l'autre front                        */
    CNFCbits.CNFC5     = 0;
    _CNCIP = 5;                 /* même priorité que le SPI             */
    _CNCIF = 0;
    _CNCIE = 1;

    spi_redemarrer();           /* SPIEN = 1 et 1er octet 0xAA chargé   */
}

bool spi_trame_lire(uint8_t dest[TRAME_LONGUEUR])
{
    uint8_t i;
    bool ok = false;
    INTCON2bits.GIE = 0;        /* copie sans être interrompu           */
    if (rx_nouvelle) {
        for (i = 0; i < TRAME_LONGUEUR; i++) {
            dest[i] = rx_complet[i];
        }
        rx_nouvelle = false;
        ok = true;
    }
    INTCON2bits.GIE = 1;
    return ok;
}

void spi_reponse_preparer(const uint8_t src[TRAME_LONGUEUR])
{
    uint8_t i;
    INTCON2bits.GIE = 0;
    for (i = 0; i < TRAME_LONGUEUR; i++) {
        tx_suivante[i] = src[i];
    }
    tx_suivante_prete = true;
    INTCON2bits.GIE = 1;
}

/* ---------------------------------------------------------------------
   Un octet vient d'être échangé
   --------------------------------------------------------------------- */
void __attribute__((interrupt, no_auto_psv)) _SPI1RXInterrupt(void)
{
    lire_octets_recus();
    _SPI1RXIF = 0;
}

/* ---------------------------------------------------------------------
   SS a changé : 1 -> 0 début d'échange, 0 -> 1 fin d'échange
   --------------------------------------------------------------------- */
void __attribute__((interrupt, no_auto_psv)) _CNCInterrupt(void)
{
    uint8_t i;

    if (CNFCbits.CNFC5) {
        CNFCbits.CNFC5 = 0;

        if (PORTCbits.RC5 == 0) {
            /* Début d'échange : on repart de zéro */
            nb_octets = 0;
        } else {
            /* Fin d'échange */
            lire_octets_recus();                 /* dernier octet éventuel */

            /* On n'accepte la trame que si les 10 octets sont arrivés.
               Fil SCK coupé -> 0 octet -> trame ignorée -> 500 ms -> D = 0 */
            if (nb_octets == TRAME_LONGUEUR) {
                for (i = 0; i < TRAME_LONGUEUR; i++) {
                    rx_complet[i] = rx[i];
                }
                rx_nouvelle = true;
            } else if (nb_octets != 0U) {
                spi_nb_echanges_incomplets++;
            }
            nb_octets = 0;

            /* Charger la nouvelle trame de mesures entre deux échanges */
            if (tx_suivante_prete) {
                for (i = 0; i < TRAME_LONGUEUR; i++) {
                    tx[i] = tx_suivante[i];
                }
                tx_suivante_prete = false;
            }
            spi_redemarrer();
        }
    }
    _CNCIF = 0;
}
