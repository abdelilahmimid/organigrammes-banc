// vitesse.h : vitesse du rouleau mesurée sur D2 (fig. 3a et 3b)
#ifndef VITESSE_H
#define VITESSE_H

void  vitesse_init();
void  vitesse_calculer();   // appelée toutes les 50 ms (cycle DATA)
float vitesse_trmin();      // dernière vitesse calculée, tr/min
float vitesse_kmh();        // la même, convertie en km/h

#endif
