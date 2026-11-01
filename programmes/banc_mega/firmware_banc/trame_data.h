// trame_data.h : trame DATA vers le PC toutes les 50 ms (fig. 3i)
#ifndef TRAME_DATA_H
#define TRAME_DATA_H

void trame_data_preparer();     // construit la ligne DATA (toutes les 50 ms)
void trame_data_envoyer();      // à chaque tour de loop : envoie la suite sans bloquer
bool trame_data_occupee();      // vrai tant qu'une ligne DATA est en cours d'envoi

#endif
