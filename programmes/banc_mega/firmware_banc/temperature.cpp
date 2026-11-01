// =====================================================================
//  temperature.cpp : sonde DS18B20 (1-Wire) sur D48
//  Repris de couplemetre_spi_temp_consigne.ino (déjà testé).
//  Bibliothèque : « DallasTemperature » de Miles Burton (+ « OneWire »).
//  Conversion non bloquante : on la lance, on revient 800 ms plus tard.
// =====================================================================
#include "temperature.h"
#include "config.h"
#include <OneWire.h>
#include <DallasTemperature.h>

static OneWire           busUnFil(PIN_TEMP);
static DallasTemperature sondes(&busUnFil);
static DeviceAddress     adresseSonde;

static bool     sondePresente     = false;
static bool     sondeBloquee85    = false;
static uint8_t  repetitions85     = 0;
static float    temperature       = 0.0;
static uint32_t tempValideMs      = 0;      // 0 = jamais de mesure valide
static uint32_t debutConversionMs = 0;

static void chercherSonde()
{
    sondes.begin();
    sondePresente = sondes.getAddress(adresseSonde, 0);
    if (sondePresente) {
        sondes.setResolution(adresseSonde, 12);   // 0,0625 °C
    }
}

static void lancerConversion()
{
    sondes.requestTemperatures();               // non bloquant (voir init)
    debutConversionMs = millis();
}

void temperature_init()
{
    sondes.setWaitForConversion(false);
    chercherSonde();
    lancerConversion();
}

void temperature_gerer()
{
    if (millis() - debutConversionMs < TEMP_CONVERSION_MS) return;   // pas fini

    if (sondePresente) {
        const float lecture = sondes.getTempC(adresseSonde);

        if (lecture < -55.0) {
            // sonde absente ou en défaut (la bibliothèque renvoie -127)
            sondePresente  = false;
            sondeBloquee85 = false;
            repetitions85  = 0;
        } else if (lecture == 85.0 &&
                   !(tempValideMs != 0 && fabs(temperature - 85.0) < 5.0)) {
            // 85,0 °C sans mesure voisine avant : valeur de démarrage de la sonde
            if (repetitions85 < 255) repetitions85++;
            if (repetitions85 >= TEMP_REPETITIONS_85) sondeBloquee85 = true;
        } else {
            temperature    = lecture;
            tempValideMs   = millis();
            repetitions85  = 0;
            sondeBloquee85 = false;
        }
    }

    if (!sondePresente) chercherSonde();        // la rechercher si elle manquait
    lancerConversion();
}

bool  temperature_presente()  { return sondePresente; }
bool  temperature_bloquee85() { return sondeBloquee85; }
float temperature_valeur()    { return temperature; }

bool temperature_fraiche()
{
    return sondePresente && !sondeBloquee85 && tempValideMs != 0
           && (millis() - tempValideMs <= TEMP_FRAICHE_MS);
}
