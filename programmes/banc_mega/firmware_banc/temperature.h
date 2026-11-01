// temperature.h : sonde DS18B20 du frein sur D48 (fig. 3e)
#ifndef TEMPERATURE_H
#define TEMPERATURE_H

void  temperature_init();
void  temperature_gerer();         // à appeler à chaque tour de loop (non bloquant)
bool  temperature_presente();      // sonde trouvée sur le bus
bool  temperature_bloquee85();     // 85,0 °C lu 3 fois de suite
bool  temperature_fraiche();       // mesure valide de moins de 2,5 s
float temperature_valeur();        // dernière mesure valide, °C

#endif
