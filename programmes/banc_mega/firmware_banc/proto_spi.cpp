// =====================================================================
//  proto_spi.cpp : liaison SPI avec la voiture (déjà testée Mega <-> Uno)
//  Mega (maître) D50 MISO, D51 MOSI, D52 SCK, D53 SS
//  -> U12 TXS0108E (5 V <-> 3,3 V, OE = D49) -> J2 -> carte voiture (dsPIC).
//  250 kHz, mode 0, MSB en premier, pause de 20 us entre les octets.
// =====================================================================
#include "proto_spi.h"
#include "config.h"
#include <SPI.h>

float protoBatVolt = 0.0, protoBatCurr = 0.0, protoMotVolt = 0.0, protoMotCurr = 0.0;

static bool     liaisonActive = false;
static uint32_t tProtoMs      = 0;
static bool     dejaRecu      = false;

uint8_t proto_xor(const uint8_t *t, uint8_t n)
{
    uint8_t x = 0;
    for (uint8_t i = 0; i < n; i++) x ^= t[i];
    return x;
}

void proto_init()
{
    pinMode(PIN_SS_PROTO, OUTPUT);
    digitalWrite(PIN_SS_PROTO, HIGH);      // pas d'échange
    digitalWrite(PIN_ENB_PROTO, LOW);      // U12 désactivé : cartes isolées
    pinMode(PIN_ENB_PROTO, OUTPUT);
    SPI.begin();
}

void proto_activer(bool actif)
{
    liaisonActive = actif;
    digitalWrite(PIN_ENB_PROTO, actif ? HIGH : LOW);
}

bool proto_actif() { return liaisonActive; }

bool proto_echanger(const uint8_t envoi[])
{
    if (!liaisonActive) return false;

    uint8_t recu[PROTO_LONGUEUR];
    SPI.beginTransaction(SPISettings(PROTO_HORLOGE_HZ, MSBFIRST, SPI_MODE0));
    digitalWrite(PIN_SS_PROTO, LOW);
    delayMicroseconds(PROTO_PAUSE_US);
    for (uint8_t i = 0; i < PROTO_LONGUEUR; i++) {
        recu[i] = SPI.transfer(envoi[i]);   // 0x55 part, 0xAA arrive en même temps
        delayMicroseconds(PROTO_PAUSE_US);
    }
    digitalWrite(PIN_SS_PROTO, HIGH);
    SPI.endTransaction();

    // Vérifications : en-tête 0xAA et XOR des octets 0 à 8
    if (recu[0] != ENTETE_MESURES) return false;
    if (proto_xor(recu, PROTO_LONGUEUR - 1) != recu[PROTO_LONGUEUR - 1]) return false;

    protoBatVolt = (uint16_t)((recu[1] << 8) | recu[2]) / 1000.0;   // V
    protoBatCurr = (int16_t)((recu[3] << 8) | recu[4]) / 1000.0;    // A (signé)
    protoMotVolt = (uint16_t)((recu[5] << 8) | recu[6]) / 1000.0;   // V
    protoMotCurr = (int16_t)((recu[7] << 8) | recu[8]) / 1000.0;    // A (signé)
    tProtoMs = millis();
    dejaRecu = true;
    return true;
}

bool proto_frais()
{
    return dejaRecu && (millis() - tProtoMs <= PROTO_FRAICHEUR_MS);
}
