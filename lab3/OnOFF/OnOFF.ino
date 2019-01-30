//Yueyang Cheng(1533989)
//Lab 3, Section B

#include "SPI.h"
#include "ILI9341_t3.h"

// For the Adafruit shield, these are the default.
#define TFT_DC  9
#define TFT_CS 10

// Use hardware SPI (on Uno, #13, #12, #11) and the above for CS/DC
ILI9341_t3 tft = ILI9341_t3(TFT_CS, TFT_DC);

void setup() {
  pinMode(1, INPUT_PULLUP);
  tft.begin();

  Serial.begin(9600);
}

volatile int sensorVal = LOW;
volatile int oldVal = LOW;
volatile boolean change = false;

void loop() {
  sensorVal = digitalRead(1);
  if (sensorVal != oldVal) {
    change = true;
  } else {
    change = false;
  }
  
  if (sensorVal == HIGH && change ) {
    off();
  } else if (sensorVal == LOW && change) {
    on();
  }

 oldVal = sensorVal;
}

unsigned long on() {
  tft.fillScreen(ILI9341_WHITE);
  tft.setCursor(96, 140);
  tft.setTextColor(ILI9341_BLACK);  tft.setTextSize(4);
  tft.println("ON");
}

unsigned long off() {
  tft.fillScreen(ILI9341_WHITE);
  tft.setCursor(88, 140);
  tft.setTextColor(ILI9341_BLACK);  tft.setTextSize(4);
  tft.println("OFF");
}

