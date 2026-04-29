#include <FastLED.h>

#define LED_PIN_1    6
#define LED_PIN_2    5
#define NUM_LEDS_1   100
#define NUM_LEDS_2   100
#define BRIGHTNESS   100

const int IN1 = 2;
const int IN2 = 3;

CRGB leds_1[NUM_LEDS_1]; 
CRGB leds_2[NUM_LEDS_2];

CRGB palett[] = {
  CRGB(20, 0, 180),
  CRGB(80, 0, 160),
  CRGB(120, 0, 60),
  CRGB(180, 20, 80),
  CRGB(200, 180, 220),
};
int antallFarger = 5;
int currentIndex = 0;
int steg = 0;

CRGB FARGE = CRGB(255, 160, 20);
int pos = 0;
bool ferdig = false;

void modusInaktiv() {
  fill_solid(leds_2, NUM_LEDS_2, CRGB::Black);
  CRGB fraFarge = palett[currentIndex];
  CRGB tilFarge = palett[(currentIndex + 1) % antallFarger];
  CRGB blandFarge = blend(fraFarge, tilFarge, steg);
  fill_solid(leds_1, NUM_LEDS_1, blandFarge);
  FastLED.show();
  steg = steg + 1;
  if (steg >= 255) {
    steg = 0;
    currentIndex = (currentIndex + 1) % antallFarger;
  }
  delay(20);
}

void modusAktiv() {
  if (ferdig) return;
  int midten = NUM_LEDS_2 / 2;
  if (midten + pos < NUM_LEDS_2) leds_2[midten + pos] = FARGE;
  if (midten - pos >= 0)         leds_2[midten - pos] = FARGE;
  FastLED.show();
  pos = pos + 1;
  if (pos > midten) ferdig = true;
  delay(40);
}

void setup() {
  FastLED.addLeds<WS2812B, LED_PIN_1, GRB>(leds_1, NUM_LEDS_1);
  FastLED.addLeds<WS2812B, LED_PIN_2, GRB>(leds_2, NUM_LEDS_2);
  FastLED.setBrightness(BRIGHTNESS);
  pinMode(IN1, INPUT);
  pinMode(IN2, INPUT);
  Serial.begin(9600);
}

void loop() {
  int s1 = digitalRead(IN1);
  int s2 = digitalRead(IN2);

  if (s1 == HIGH && s2 == LOW) {
    modusInaktiv();
    ferdig = false;
    pos = 0;
  }
  else if (s1 == HIGH && s2 == HIGH) {
    modusAktiv();
  }
  else {
    fill_solid(leds_1, NUM_LEDS_1, CRGB::Black);
    fill_solid(leds_2, NUM_LEDS_2, CRGB::Black);
    FastLED.show();
    ferdig = false;
    pos = 0;
  }
}