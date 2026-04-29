//Credits: Malin Lenz Litlehei, Signe Svingen Fjeldavli , Leo Korn Gjessing, Annabella Paintsil, Erik Magnus Pettersen Gustavsen

//Variabler for debounce (brukes til saltpåfyllingsknappen)
int sandKnappState = HIGH;              
static bool lastSandKnappState = HIGH;

static unsigned long lastDebounceTime = 0;
unsigned long debounceDelay = 50; 



//Tidsvariabler
const int VEDLIKEHOLD_TID = 30000; // Tid mellom hver hver gangs den strør salt
const int TID_SAND_REFIL = 10000;  // Hvor lenge luften som skyter opp sanda skal vare
unsigned long forrigeBlink = 0;   // Brukes for å holde styr på når forrige blink (saltpåfyllingsknapp)
const int blinkIntervall = 400;   // Tid mellom hvert blink ((saltpåfyllingsknapp))

const int PIN_START_SENSOR  = 2;   // Homing-sensor (limit switch innvendig)
const int PIN_STOP_SENSOR   = 3;   // Stopp-sensor slider (limit switch utvendig)
const int PIN_VENTIL_SLIDER = 4;   // Pinne koblet til relayet som styrer slider
const int PIN_VENTIL_SAND   = 5;   // Pinne koblet til relayet som styrer saltoppblåsinga

const int PIN_MER_SAND_KNAPP = 6;   // «Mer sand»-knapp
const int PIN_LED_MER_SAND = 7;     //LED-lys PÅ "mer sand"-knapp

//Pinner som brukes til kommunikasjon med oscilator-arduino
const int INFO_PIN_1 = 8;
const int INFO_PIN_2 = 9;
const int KOMMUNIKASJON_PIN = 10;

//Pinner som brukes til kommunikasjon med LED-lys-arduino
const int STATUS_PIN_1 = 12;
const int STATUS_PIN_2 = 13;


//
bool startSensorAktivert() { 
  return digitalRead(PIN_START_SENSOR) == LOW; // True når knappen trykkes 
}

bool stoppSensorAktivert() {
  return digitalRead(PIN_STOP_SENSOR) == LOW; // True når knappen trykkes 
}

bool oppstartsstroing = false;
bool lysStatus = false;



String status(){ //funksjon som holder styr på hvilket modus vi er i
   if ((digitalRead(INFO_PIN_1) == LOW) &&  (digitalRead(INFO_PIN_2) == LOW)){
    return "INISIALISERING";
  }

   if ((digitalRead(INFO_PIN_1) == HIGH) &&  (digitalRead(INFO_PIN_2) == LOW)){
    return "INAKTIV";
  }
  
  if ((digitalRead(INFO_PIN_1) == HIGH) &&  (digitalRead(INFO_PIN_2) == HIGH)){
    return "AKTIV";
  }

 if ((digitalRead(INFO_PIN_1) == LOW) &&  (digitalRead(INFO_PIN_2) == HIGH)){
    return "OPPSTART";
  }

  return "UKJENT";
}



//Hjelpefunksjoner
void sliderUT() { // Sender slidesylinder UT over platen
  digitalWrite(PIN_VENTIL_SLIDER, LOW);
}

void sliderINN() { // Sender slidesylinder INN over platen
  digitalWrite(PIN_VENTIL_SLIDER, HIGH);
}

void oppdaterBlink() { //Sørger for at lyset blinker
  if (millis() - forrigeBlink >= blinkIntervall) {
    forrigeBlink = millis();
    lysStatus = !lysStatus;
    digitalWrite(PIN_LED_MER_SAND, lysStatus);
  }
}


void kommuniserFerdig(){
  digitalWrite(KOMMUNIKASJON_PIN,HIGH);
  delay(500);
  digitalWrite(KOMMUNIKASJON_PIN,LOW);
}

void Kommuniser_inisiering(){
  digitalWrite(STATUS_PIN_1, LOW);
  digitalWrite(STATUS_PIN_2, LOW);
}

void Kommuniser_inaktiv(){
  digitalWrite(STATUS_PIN_1, HIGH);
  digitalWrite(STATUS_PIN_2, LOW);
}

void Kommuniser_aktiv(){
  digitalWrite(STATUS_PIN_1, HIGH);
  digitalWrite(STATUS_PIN_2, HIGH);
}

void Kommuniser_oppstart(){
  digitalWrite(STATUS_PIN_1, LOW);
  digitalWrite(STATUS_PIN_2, HIGH);
}


void sandstroing() {  //funskjon som sender ut og inn silen med salt
   
  while(true){
    Serial.println("SLIDER SENDES UT");
    sliderUT();
    while (!stoppSensorAktivert()){
      if(status() == "AKTIV"){
      oppdaterBlink();
      delay(10);
      }
    }

    Serial.println("SLIDER HAR KOMMET TIL ENDEN");
    sliderINN();
    while (!startSensorAktivert()){
      if(status() == "AKTIV"){
      oppdaterBlink();
       delay(10);
      }
    }
    Serial.println("SLIDER ER TILBAKE");

    unsigned long int refil_referansetid = millis();
    digitalWrite(PIN_VENTIL_SAND, LOW);
    while(millis()-refil_referansetid < TID_SAND_REFIL){
      if(status() == "AKTIV"){
      oppdaterBlink();
      delay(10);
      }
    }
    
    digitalWrite(PIN_VENTIL_SAND, HIGH);
    digitalWrite(PIN_LED_MER_SAND, HIGH);
    break;
  }

}


void setup() {
  Serial.begin(9600);

  pinMode(PIN_START_SENSOR, INPUT_PULLUP);
  pinMode(PIN_STOP_SENSOR, INPUT_PULLUP);
  pinMode(PIN_MER_SAND_KNAPP, INPUT_PULLUP);
  pinMode(INFO_PIN_1, INPUT);
  pinMode(INFO_PIN_2, INPUT);

  pinMode(PIN_VENTIL_SLIDER, OUTPUT);
  pinMode(PIN_VENTIL_SAND, OUTPUT);
  pinMode(PIN_LED_MER_SAND, OUTPUT);
  pinMode(KOMMUNIKASJON_PIN, OUTPUT);
  pinMode(STATUS_PIN_1, OUTPUT);
  pinMode(STATUS_PIN_2, OUTPUT);

  digitalWrite (PIN_VENTIL_SAND, HIGH);


}

void loop() {
int startTelling = millis();

//inaktivmodus
while (status()=="INAKTIV"){ 
  Serial.println(status());
  digitalWrite(PIN_LED_MER_SAND, LOW);
  Kommuniser_inaktiv();
  
  if(!oppstartsstroing){
    sliderINN(); //**
    delay(6000);
    sandstroing();
    oppstartsstroing = true;
    startTelling = millis();
    kommuniserFerdig();
  }
  else if (millis()-startTelling >= VEDLIKEHOLD_TID) {
    sandstroing();
    startTelling = millis();
  }
}

//aktiv modus
while (status()=="AKTIV") {
  Kommuniser_aktiv();
  digitalWrite(PIN_LED_MER_SAND, LOW);
  oppstartsstroing = false;

  bool reading = digitalRead(PIN_MER_SAND_KNAPP);

  // hvis endring → restart timer
  if (reading != lastSandKnappState) {
    lastDebounceTime = millis();
  }

  // hvis stabilt lenge nok
  if ((millis() - lastDebounceTime) > debounceDelay) {

    // kun på "nytt trykk"
    if (sandKnappState != reading) {
      sandKnappState = reading;

      if (sandKnappState == LOW) {
        Serial.println("TRYKK!");
        sandstroing();
      }
    }
  }

  lastSandKnappState = reading;
}


//inisialisering
while (status()=="INISIALISERING"){ // eller ønker man 0?
  digitalWrite(PIN_LED_MER_SAND, HIGH);
  Serial.println(status());
  Serial.print(digitalRead(PIN_START_SENSOR));
  Serial.print(startSensorAktivert());
  Kommuniser_inisiering();

  if (!startSensorAktivert()) {
      Serial.println("HOMING_START");
      sliderINN();// beveg innover mot beholder/startsensor

      while (!startSensorAktivert()) {
        delay(10);
      }
      Serial.println("HOMING_DONE");
    } 
  else {
      Serial.println("HOMING_ALREADY_HOME");
   }

  kommuniserFerdig();

}

//oppstart
  while (status()=="OPPSTART") {
    digitalWrite(PIN_LED_MER_SAND, HIGH);
    Kommuniser_oppstart();
    Serial.println(status());
    delay(6000);
    sandstroing();
    kommuniserFerdig();

  }

}
