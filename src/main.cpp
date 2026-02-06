#include <Arduino.h>

#include <Adafruit_NeoPixel.h> 

#define PIN 48
#define NUMPIXELS 1

Adafruit_NeoPixel pixels(NUMPIXELS, PIN, NEO_GRB + NEO_KHZ800);
#define DELAYVAL 500

void setup () {  
  Serial.begin(9600);
  
  pixels.begin();

}

void loop () {

  Serial.println("Hello, world!");

  pixels.clear();

  for(int i=0; i<NUMPIXELS; i++) {
    pixels.setPixelColor(i, pixels.Color(0, 0, 150));
    pixels.show();
    delay(DELAYVAL);
  }

}