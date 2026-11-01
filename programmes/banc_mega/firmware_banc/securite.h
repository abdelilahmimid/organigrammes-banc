// securite.h : arrêt d'urgence (D3), sécurité thermique (fig. 3f), ventilation (fig. 3g)
#ifndef SECURITE_H
#define SECURITE_H

void securite_init();
void urgence_gerer();        // à chaque tour de loop
void thermique_gerer();      // à chaque tour de loop
void ventilation_gerer();    // à chaque tour de loop
bool ventilation_marche();   // état de la sortie D42

#endif
