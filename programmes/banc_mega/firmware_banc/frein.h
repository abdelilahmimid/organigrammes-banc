// =====================================================================
//  frein.h : pente simulée par le frein à hystérésis
//  (organigrammes C et D de PENTE.docx, tâche du collègue)
// =====================================================================
#ifndef FREIN_H
#define FREIN_H

#include <Arduino.h>

void  frein_init();              // Timer 4 réglé, frein coupé : à appeler EN PREMIER
void  frein_cycle();             // toutes les 10 ms
float frein_alpha();             // rapport cyclique appliqué (0 à 0,95)
uint8_t frein_pwm255();          // alpha x 255 (clé brake_pwm)
float frein_couple_estime();     // N.m au volant, estimé à partir de alpha (clé brake_torque)
float frein_consigne();          // N.m au volant, consigne après la rampe (clé brake_setpoint)
bool  frein_sature();            // alerte BRAKE_SAT
bool  frein_freinage();          // vrai pendant le freinage de fin (après STOP)

#endif
