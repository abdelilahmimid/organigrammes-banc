/* pwm.h : PWM1 complémentaire avec temps mort (pont Q2 / Q3) */
#ifndef PWM_H
#define PWM_H

#include <stdint.h>

void pwm_init(void);                       /* démarre avec les 2 MOSFET bloqués */
void pwm_appliquer(uint16_t d_pour_mille); /* 0 à 950                           */
void pwm_arreter(void);                    /* D = 0 et les 2 MOSFET bloqués     */

#endif
