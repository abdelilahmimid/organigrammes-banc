/* mesures.h : VBUS/16 (AN15), courant moteur (R50 + OA1, AN0),
   courant batterie (R35 + U5 TSC1031, AN1) */
#ifndef MESURES_H
#define MESURES_H

#include <stdint.h>
#include "trame.h"

void mesures_init(void);             /* à appeler Q2 et Q3 bloqués : mesure */
                                     /* le zéro du courant moteur           */
void mesures_echantillonner(void);   /* appelée toutes les 1 ms : 1 lecture */
                                     /* de chaque voie, ajoutée à la somme  */
/* Toutes les 100 ms : moyenne des lectures, conversion en mV / mA.
   d_pour_mille : D appliqué (pour calculer mot_volt = D x VBUS)       */
void mesures_calculer(mesures_t *m, uint16_t d_pour_mille);

#endif
