// couple.h : couple mesuré par le couplemètre sur A0 (SIG_OUT)
#ifndef COUPLE_H
#define COUPLE_H

void  couple_echantillonner();     // une lecture de A0, appelée toutes les 1 ms
float couple_moyenne_frein();      // N.m au rouleau, moyenne depuis l'appel précédent (10 ms)
float couple_moyenne_data();       // N.m au rouleau, moyenne depuis l'appel précédent (50 ms)

#endif
