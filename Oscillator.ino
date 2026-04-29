#include <AD9833.h>     // Henter ad9833 biblioteket
#define FNC_PIN 4       // definerer pin 4 som kontroll-pinnen
const int PIN_VIBRATION = 4;

AD9833 gen(FNC_PIN); 
int pot = 0;

// Digitale pinner brukt til kommunikajson med arduinoen som styrer sand:
const int KOMMUNIKASJON_PIN_1 = 2;
const int KOMMUNIKASJON_PIN_2 = 3;
const int INFO_PIN = 5;

const int PIN_KNAPP_START = 6;   // Startknapp (grønn) PIN_BTN_START****


// TIDSKONSTANTER
const unsigned long INAKTIV_TIMEOUT        = 60000;  // Tid før den går til inaktiv modus
const unsigned long FREKVENS_TID  = 15000;   // Tid meelom hvert mønster i inaktiv modus

// GLOBALE VARIABLER
// Tilstandsmaskin
enum State { INITIALISERING, INAKTIV, OPPSTART, AKTIV };  //**** se på
State currentState = INITIALISERING;

unsigned long sisteInputTid    = 0;       // Timer for inaktivitet (state 4 → state 2)
unsigned long sisteSandstroingTid = 0;    // Timer for vedlikeholdsstrøing (state 2)

// Variabler for frekvens og amplitude
int frekvens  = 0; 
int amplitude = 0; 

// Inaktiv modus
const int EGENFREKVENSER[]            = {64, 69, 78, 83, 88, 104, 111, 121, 128, 132, 139, 143, 148, 150, 155, 159, 169, 190, 203, 260, 379, 225, 84, 206, 265, 292}; //liste med egenfrekvenser
const int ANTALL_EGENFREKVENSER       = 26;
unsigned long attraktorTid;           //brukes for å holde styr på tiden mellom hvert nye mønster i inaktiv modus


bool oppstartsrunde = true;





// HJELPEFUNKSJONER
void settVibrasjon(int hz) {  
  gen.ApplySignal(SINE_WAVE,REG0,hz);
}

void stoppVibrasjon() {
   gen.ApplySignal(SINE_WAVE,REG0,0);
}

void ristAvSand(){
  Serial.print("rister av sand");
  gen.ApplySignal(SINE_WAVE,REG0,50);
  delay(5000);
  gen.ApplySignal(SINE_WAVE,REG0,0);
  Serial.print("risting avsluttet");
}


bool startKnappTrykket() {
  return digitalRead(PIN_KNAPP_START) == HIGH;
}

bool homingFerdig(){
  return digitalRead(INFO_PIN) == HIGH;
}

//Kommuniserer tilstand til arduinoen som stryrer sandstrøing
void Kommuniser_inisiering(){
  digitalWrite(KOMMUNIKASJON_PIN_1, LOW);
  digitalWrite(KOMMUNIKASJON_PIN_2, LOW);
}

void Kommuniser_inaktiv(){
  digitalWrite(KOMMUNIKASJON_PIN_1, HIGH);
  digitalWrite(KOMMUNIKASJON_PIN_2, LOW);
}

void Kommuniser_aktiv(){
  digitalWrite(KOMMUNIKASJON_PIN_1, HIGH);
  digitalWrite(KOMMUNIKASJON_PIN_2, HIGH);
}

void Kommuniser_oppstart(){
  digitalWrite(KOMMUNIKASJON_PIN_1, LOW);
  digitalWrite(KOMMUNIKASJON_PIN_2, HIGH);
}


// STATE 1: INITIALISERING 
void state_initialisering() {
  Serial.println("STATE_1_INITIALISERING");
  Kommuniser_inisiering();

  // Sett alle variabler til startverdi
  frekvens             = 0;
  amplitude            = 0;
  sisteInputTid        = millis();
  attraktorTid         = millis();

  stoppVibrasjon();
  Serial.println(digitalRead(INFO_PIN));

  while(!homingFerdig()){
    delay(10);
  }
  
  Serial.println(digitalRead(INFO_PIN));
  currentState = INAKTIV;
}





// STATE 2: INAKTIV MODUS
void state_inaktiv() {
  Kommuniser_inaktiv();
  Serial.println("STATE_2_INAKTIV");


  if (oppstartsrunde){
    ristAvSand();
    oppstartsrunde = false;
    while(!homingFerdig()){
    delay(10);
    }
  }

  // Start attraktor-vibrasjon
  frekvens  = EGENFREKVENSER[random(0,ANTALL_EGENFREKVENSER)];
  settVibrasjon(frekvens);
  attraktorTid = millis();

  Serial.print("ATTRAKTOR_FREQ:"); // for feilsøking****
  Serial.println(frekvens);
  

  while (true) {

    // Bytt attraktor-frekvens periodisk
    if (millis() - attraktorTid > FREKVENS_TID) {
      frekvens = EGENFREKVENSER[random(0,ANTALL_EGENFREKVENSER)];
      settVibrasjon(frekvens);
      attraktorTid = millis();
      Serial.print("ATTRAKTOR_FREQ:");
      Serial.println(frekvens);
    }

    // skifter state til "oppstart" dersom startknapp trykkes
    if (startKnappTrykket()) {
      stoppVibrasjon();
      currentState = OPPSTART;
      return;
    }

    delay(20); //se om denne er nødvendig****
  }
}





// STATE 3: OPPSTART (Reset før den går over til aktiv modus) 
void state_oppstart() {
  Serial.println("STATE_3_OPPSTART");
  Kommuniser_oppstart();

  ristAvSand();   // Rist av gammel sand

  while(!homingFerdig()){
    delay(10);
  }

  // Slå på panel-lys og skjerm
  Serial.println("SKJERM_PAA"); // PC-siden håndterer oscilloskop/skjerm

  // Gå til aktiv modus
  sisteInputTid = millis();
  currentState  = AKTIV;
}





// STATE 4: AKTIV MODUS (Interaksjon)
void state_aktiv() {
  Serial.println("STATE_4_AKTIV");
  Kommuniser_aktiv();
  int gammelFrekvens = frekvens;

  while (true) {
    pot = analogRead(A1);
    frekvens  = map(pot, 0, 1023, 20, 500); 

   
    if (abs(gammelFrekvens - frekvens) > 3 ) {  // ">3" for å redusere støy
      gammelFrekvens  = frekvens;
      settVibrasjon(frekvens);
      sisteInputTid = millis(); // reset inaktivitets-timer

      
      Serial.print("FREQ:"); // Send til PC for oscilloskop-visning (kun for feilsøking)
      Serial.println(frekvens);
    }

    
    if (millis() - sisteInputTid > INAKTIV_TIMEOUT) { //går over til innaktivmodus ved lite aktivitet
      Serial.println("TIMEOUT_INAKTIV");
      stoppVibrasjon();
      currentState = INAKTIV;
      return;
    }

    delay(20); //vet ikke om denne trengs
  }
}




// SETUP 
void setup() {
  gen.Begin(); 
  gen.EnableOutput(true);  //for at AD9833 skal sende ut signaler

  Serial.begin(9600);

  // Inngangs-pinner
  pinMode(A1, INPUT);
  pinMode(PIN_KNAPP_START, INPUT);
  pinMode(INFO_PIN, INPUT);
  pinMode(KOMMUNIKASJON_PIN_1, OUTPUT);
  pinMode(KOMMUNIKASJON_PIN_2, OUTPUT);

  // Utgangs-pinner
  pinMode(PIN_VIBRATION,     OUTPUT);

  randomSeed(analogRead(A0));

  currentState = INITIALISERING;      // Starter i initialisering-modus

  


}

// LOOP 
void loop() {
  switch (currentState) { //bruker switch...case til å veksle mellom de ulike modusene
    case INITIALISERING:
      state_initialisering();
      break;
    case INAKTIV:
      state_inaktiv();
      break;
    case OPPSTART:
      state_oppstart();
      break;
    case AKTIV:
      state_aktiv();
      break;
  }
}
