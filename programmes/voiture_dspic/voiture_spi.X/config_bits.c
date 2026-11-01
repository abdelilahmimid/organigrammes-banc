/* =====================================================================
   config_bits.c : bits de configuration (lus par le dsPIC au démarrage,
   avant même la première instruction du programme).
   Les bits non cités gardent leur valeur par défaut.
   ===================================================================== */

/* FOSCSEL : démarrer sur l'oscillateur interne FRC (8 MHz), sans PLL.
   Le programme passe ensuite sur la PLL (horloge.c).                  */
#pragma config FNOSC = FRC          /* oscillateur de départ : FRC      */
#pragma config IESO = OFF           /* pas de démarrage à deux vitesses */

/* FOSC : pas de quartz ; changement d'horloge autorisé par logiciel    */
#pragma config POSCMD = NONE        /* oscillateur primaire coupé       */
#pragma config OSCIOFNC = ON        /* broche 22 (OSCO) libre en E/S    */
#pragma config FCKSM = CSECMD       /* changement d'horloge autorisé    */

/* FWDT : chien de garde piloté par logiciel (laissé éteint)           */
#pragma config FWDTEN = ON_SW

/* FICD : débogage par PGD3 / PGC3 (broches 33 / 34, connecteur J16)    */
#pragma config ICS = PGD3
#pragma config JTAGEN = OFF         /* broches JTAG rendues libres      */

/* FDMT : « dead man timer » désactivé                                  */
#pragma config DMTDIS = OFF
