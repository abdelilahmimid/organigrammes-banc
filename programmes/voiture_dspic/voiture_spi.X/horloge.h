/* horloge.h : PLL (FCY = 50 MHz) et base de temps de 1 ms (Timer1) */
#ifndef HORLOGE_H
#define HORLOGE_H

#include <stdint.h>

void     horloge_init(void);   /* FRC 8 MHz -> PLL -> FOSC 100 MHz      */
void     temps_init(void);     /* Timer1 : interruption toutes les 1 ms */
uint32_t temps_ms(void);       /* millisecondes depuis le démarrage     */

#endif
