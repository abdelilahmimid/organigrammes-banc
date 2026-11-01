// =====================================================================
//  simulateur_voiture_uno_v3.ino
//  Arduino UNO = « voiture virtuelle » pour tester firmware_banc (Mega)
//  sans le banc et sans la vraie voiture.
//
//  Ce que fait l'Uno :
//   1) SPI esclave (comme la v2) : reçoit la trame 0x55 du banc et renvoie
//      la trame 0xAA (tension et courant simulés) dans le même échange ;
//   2) lit D (pour mille, octets 6-7) et l'état (octet 5) ;
//   3) calcule une vitesse de voiture qui monte avec D et baisse avec la
//      pente (commande PENTE:x tapée dans le moniteur série de l'Uno) ;
//   4) envoie sur D9 des impulsions comme le couplemètre :
//      60 impulsions par tour du rouleau -> vers D2 du Mega.
//
//  Câblage (Uno <-> Mega) :
//    D10 (SS)   <-> D53      D11 (MOSI) <-> D51
//    D12 (MISO) <-> D50      D13 (SCK)  <-> D52
//    D9 (impulsions) -> D2 du Mega   (+ 10 kOhm de D2 vers GND, comme R16)
//    GND        <-> GND      (NE PAS relier les 5 V)
//
//  Modèle de la voiture (très simple, premier ordre) :
//    vitesse visée = 45 km/h x D - 1,5 km/h par % de pente
//    la vitesse rejoint la vitesse visée avec une constante de temps de 1,5 s
//    Exemples (régime établi) : 30 km/h -> D = 0,667 sans pente,
//                               D = 0,767 à 3 %, D = 0,95 (saturé) vers 8,5 %.
//
//  Sécurité (comme la vraie voiture) : sans trame valide pendant 500 ms,
//  ou état = 0, D est mis à 0 : la voiture ralentit toute seule.
// =====================================================================

const uint8_t  HEADER          = 0xAA;
const uint8_t  FRAME_LEN       = 10;
const uint8_t  CONSIGNE_ENTETE = 0x55;
const uint16_t DELAI_PERTE_MS  = 500;
const uint16_t D_MAX_POUR_MILLE = 950;

// Modèle de la voiture virtuelle
const float K_KMH_PAR_D    = 45.0;    // vitesse visée pour D = 1
const float K_KMH_PAR_PENTE = 1.5;    // perte de vitesse par % de pente
const float TAU_S          = 1.5;     // constante de temps
const float KMH_PAR_TRMIN  = 0.0294;  // même valeur que config.h du Mega (rayon 0,078 m)
const float BATTERIE_V     = 48.0;

// ---------------- SPI esclave (repris de la v2) ----------------
volatile uint8_t frame[FRAME_LEN];
volatile uint8_t idx = 0;
uint8_t nextFrame[FRAME_LEN];
volatile bool nextReady = false;

volatile uint8_t recu[FRAME_LEN];
volatile uint8_t recuComplet[FRAME_LEN];
volatile bool trameRecue = false;
volatile uint8_t nbOctetsRecus = 0;

// Dernière consigne valide
float    consigneVitesse = 0.0;   // km/h (affichage seulement)
bool     essaiEnCours    = false;
uint16_t dPourMille      = 0;
unsigned long derniereConsigneMs = 0;
bool     consignePerdue  = true;
unsigned long nbTramesOK = 0, nbTramesFausses = 0;

// Voiture virtuelle
float pente   = 0.0;              // % (commande PENTE:x)
float vitesse = 0.0;              // km/h
float dApplique = 0.0;            // D réellement appliqué (0 si arrêt ou perte)

void buildFrame(uint8_t *f, uint16_t bv, int16_t bc, uint16_t mv, int16_t mc) {
  f[0] = HEADER;
  f[1] = bv >> 8;           f[2] = bv & 0xFF;
  f[3] = (uint16_t)bc >> 8; f[4] = (uint16_t)bc & 0xFF;
  f[5] = mv >> 8;           f[6] = mv & 0xFF;
  f[7] = (uint16_t)mc >> 8; f[8] = (uint16_t)mc & 0xFF;
  uint8_t x = 0;
  for (uint8_t i = 0; i < 9; i++) x ^= f[i];
  f[9] = x;
}

ISR(SPI_STC_vect) {
  const uint8_t octet = SPDR;
  if (nbOctetsRecus < FRAME_LEN) recu[nbOctetsRecus] = octet;
  if (nbOctetsRecus < 255) nbOctetsRecus++;
  idx++;
  if (idx >= FRAME_LEN) idx = 0;
  SPDR = frame[idx];
}

ISR(PCINT0_vect) {
  if (!(PINB & _BV(PB2))) {          // SS descend : début d'échange
    nbOctetsRecus = 0;
    return;
  }
  // SS remonte : fin d'échange, trame acceptée seulement si 10 octets
  if (nbOctetsRecus == FRAME_LEN) {
    for (uint8_t i = 0; i < FRAME_LEN; i++) recuComplet[i] = recu[i];
    trameRecue = true;
  }
  nbOctetsRecus = 0;
  if (nextReady) {
    for (uint8_t i = 0; i < FRAME_LEN; i++) frame[i] = nextFrame[i];
    nextReady = false;
  }
  idx = 0;
  SPDR = frame[0];
}

void traiterConsigne() {
  if (!trameRecue) return;
  uint8_t t[FRAME_LEN];
  noInterrupts();
  for (uint8_t i = 0; i < FRAME_LEN; i++) t[i] = recuComplet[i];
  trameRecue = false;
  interrupts();

  uint8_t x = 0;
  for (uint8_t i = 0; i < FRAME_LEN - 1; i++) x ^= t[i];
  if (t[0] != CONSIGNE_ENTETE || x != t[FRAME_LEN - 1]) {
    nbTramesFausses++;
    return;
  }
  consigneVitesse = ((uint16_t)(t[1] << 8) | t[2]) / 10.0;
  essaiEnCours    = (t[5] == 1);
  dPourMille      = ((uint16_t)t[6] << 8) | t[7];
  if (dPourMille > D_MAX_POUR_MILLE) dPourMille = D_MAX_POUR_MILLE;
  derniereConsigneMs = millis();
  consignePerdue = false;
  nbTramesOK++;
}

// ---------------- Impulsions sur D9 (Timer 1, sans interruption) ----------------
// Mode CTC, sortie OC1A (D9) inversée à chaque comparaison :
//   f = 16 MHz / (2 x 8 x (OCR1A + 1))   (prédiviseur 8)
// 60 impulsions par tour : f (Hz) = tr/min du rouleau.
void impulsions_init() {
  pinMode(9, OUTPUT);
  digitalWrite(9, LOW);
  TCCR1A = 0;
  TCCR1B = _BV(WGM12);                   // CTC, timer arrêté pour l'instant
}

void impulsions_frequence(float f) {
  if (f < 16.0) {                        // trop lent pour le Timer 1 : pas d'impulsion
    TCCR1A = 0;                          // D9 redevient une sortie normale
    TCCR1B = _BV(WGM12);                 // timer arrêté
    digitalWrite(9, LOW);
    return;
  }
  uint32_t ocr = (uint32_t)(16000000.0 / (16.0 * f)) - 1;
  if (ocr > 65535) ocr = 65535;
  OCR1A = (uint16_t)ocr;
  if (TCNT1 > OCR1A) TCNT1 = 0;          // évite un tour complet de 65 536 pas
  TCCR1A = _BV(COM1A0);                  // D9 basculée à chaque comparaison
  TCCR1B = _BV(WGM12) | _BV(CS11);       // CTC, prédiviseur 8
}

// ---------------- Commandes tapées dans le moniteur série de l'Uno ----------------
void lireCommandes() {
  static char ligne[24];
  static uint8_t n = 0;
  while (Serial.available() > 0) {
    char c = (char)Serial.read();
    if (c == '\r') continue;
    if (c == '\n') {
      ligne[n] = '\0';
      n = 0;
      for (char *p = ligne; *p; p++) *p = toupper(*p);
      if (strncmp(ligne, "PENTE:", 6) == 0) {
        pente = atof(ligne + 6);
        if (pente < 0.0) pente = 0.0;
        if (pente > 20.0) pente = 20.0;
        Serial.print(F("OK pente = ")); Serial.print(pente, 1); Serial.println(F(" %"));
      } else if (ligne[0] != '\0') {
        Serial.println(F("Commande inconnue. Exemple : PENTE:3"));
      }
    } else if (n < sizeof(ligne) - 1) {
      ligne[n++] = c;
    }
  }
}

void setup() {
  Serial.begin(115200);

  pinMode(MISO, OUTPUT);
  pinMode(SS, INPUT);
  buildFrame((uint8_t *)frame, 48000, 0, 0, 0);
  SPDR = frame[0];
  SPCR = _BV(SPE) | _BV(SPIE);           // SPI esclave, mode 0
  PCICR  |= _BV(PCIE0);
  PCMSK0 |= _BV(PCINT2);                 // SS = D10

  impulsions_init();

  Serial.println(F("Voiture virtuelle (Uno v3) prete. Commande : PENTE:x"));
}

void loop() {
  static unsigned long tModele = 0, tPrep = 0, tPrint = 0;

  lireCommandes();
  traiterConsigne();

  // Sécurité : plus de trame valide depuis 500 ms -> D = 0
  if (!consignePerdue && millis() - derniereConsigneMs > DELAI_PERTE_MS) {
    consignePerdue = true;
    essaiEnCours = false;
    dPourMille = 0;
    Serial.println(F("SECURITE : plus de trame du banc depuis 500 ms -> D = 0"));
  }
  dApplique = (essaiEnCours && !consignePerdue) ? dPourMille / 1000.0 : 0.0;

  // Toutes les 10 ms : modèle de la voiture + fréquence des impulsions
  if (millis() - tModele >= 10) {
    tModele += 10;
    float vVisee = K_KMH_PAR_D * dApplique - K_KMH_PAR_PENTE * pente;
    if (vVisee < 0.0) vVisee = 0.0;
    vitesse += (vVisee - vitesse) * 0.01 / TAU_S;
    if (vitesse < 0.05 && vVisee == 0.0) vitesse = 0.0;
    float trMin = vitesse / KMH_PAR_TRMIN;
    impulsions_frequence(trMin);          // 60 impulsions/tour -> f = tr/min
  }

  // Toutes les 100 ms : mesures simulées pour la trame 0xAA
  if (millis() - tPrep >= 100) {
    tPrep = millis();
    uint16_t bv = (uint16_t)(BATTERIE_V * 1000.0);
    uint16_t mv = (uint16_t)(BATTERIE_V * dApplique * 1000.0);
    int16_t  mc = (dApplique > 0.0) ? (int16_t)(500.0 + 3000.0 * dApplique + 1000.0 * pente) : 0;
    int16_t  bc = (int16_t)(mc * dApplique);
    uint8_t tmp[FRAME_LEN];
    buildFrame(tmp, bv, bc, mv, mc);
    noInterrupts();
    for (uint8_t i = 0; i < FRAME_LEN; i++) nextFrame[i] = tmp[i];
    nextReady = true;
    interrupts();
  }

  // Toutes les 500 ms : affichage
  if (millis() - tPrint >= 500) {
    tPrint = millis();
    Serial.print(F("Recu : etat="));    Serial.print(essaiEnCours ? 1 : 0);
    Serial.print(F(" D="));             Serial.print(dApplique, 3);
    Serial.print(F(" (consigne "));     Serial.print(consigneVitesse, 1);
    Serial.print(F(" km/h) | Voiture : v=")); Serial.print(vitesse, 1);
    Serial.print(F(" km/h  rouleau="));  Serial.print(vitesse / KMH_PAR_TRMIN, 0);
    Serial.print(F(" tr/min  pente="));  Serial.print(pente, 1);
    Serial.print(F(" %  (trames OK="));  Serial.print(nbTramesOK);
    Serial.print(F(", fausses="));       Serial.print(nbTramesFausses);
    Serial.println(F(")"));
  }
}
