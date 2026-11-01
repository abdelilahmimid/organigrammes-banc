// proto_spi.h : liaison SPI avec la voiture (trames 0x55 / 0xAA de 10 octets)
#ifndef PROTO_SPI_H
#define PROTO_SPI_H

#include <Arduino.h>

void proto_init();                 // SS = 1, D49 = 0 (U12 isole les cartes), SPI maître
void proto_activer(bool actif);    // D49 = 1 (START) ou 0 (démarrage, RESET)
bool proto_actif();
// Échange une trame : envoie « envoi » (0x55), lit la trame 0xAA en même temps.
// Renvoie true si la trame reçue est valide (en-tête 0xAA et XOR).
bool proto_echanger(const uint8_t envoi[]);
bool proto_frais();                // trame 0xAA valide reçue il y a moins de 1 s

// Dernières mesures valides de la voiture
extern float protoBatVolt, protoBatCurr, protoMotVolt, protoMotCurr;

uint8_t proto_xor(const uint8_t *t, uint8_t n);

#endif
