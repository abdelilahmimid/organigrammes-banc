// regulation_vitesse.h : organigramme A (PI de vitesse + trame 0x55 vers la voiture)
#ifndef REGULATION_VITESSE_H
#define REGULATION_VITESSE_H

void  regulation_vitesse_cycle();   // toutes les 100 ms
float regulation_d();               // D envoyé à la voiture (0 à 0,95), clé speed_cmd

#endif
