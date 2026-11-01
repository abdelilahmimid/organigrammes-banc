/* spi_esclave.h : SPI1 esclave, mode 0, trames de 10 octets */
#ifndef SPI_ESCLAVE_H
#define SPI_ESCLAVE_H

#include <stdint.h>
#include <stdbool.h>
#include "trame.h"

void spi_init(const uint8_t premiere_reponse[TRAME_LONGUEUR]);

/* Copie la dernière trame reçue complète (10 octets) dans dest.
   Renvoie false s'il n'y a pas de nouvelle trame.                   */
bool spi_trame_lire(uint8_t dest[TRAME_LONGUEUR]);

/* Donne la prochaine trame 0xAA à envoyer ; elle est chargée à la fin
   de l'échange en cours (quand SS remonte), jamais au milieu.       */
void spi_reponse_preparer(const uint8_t src[TRAME_LONGUEUR]);

/* Compteur pour le débogage : échanges avec un nombre d'octets faux */
extern volatile uint16_t spi_nb_echanges_incomplets;

#endif
