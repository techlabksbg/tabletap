#include <Arduino.h>
#include "xtensa/core-macros.h"

// GPIO pins, on which the timings should be measured.
#define PIN0 12
#define PIN1 13
#define PIN2 14

int pins[] = {PIN0, PIN1, PIN2};
// Test pins (to test the code, directly wire pin 21 to 12, etc.)
int test[] = {21,22,23};

// Contains the number of ticks when the GPIO pin was raised
volatile unsigned long fired[3];
// Number of measured pins (each pin is only measured once)
volatile int numDone = 0;

// Interupt routine: Store time, increase number of measured pins, disable interrupt on this pin
#define itr(X) void IRAM_ATTR raisingPin##X() { detachInterrupt(PIN##X); fired[X] = XTHAL_GET_CCOUNT(); numDone++; }
// define 3 interrupt functions:
itr(0)
itr(1)
itr(2)

// function call to attch interrupts
#define attachp(X) attachInterrupt(PIN##X, raisingPin##X, RISING);
void einschalten() {
  numDone = 0;
  attachp(0);
  attachp(1);
  attachp(2);
}

// not used for now, detach all interrupts
void ausschalten() {
  for (auto pin: pins) {
    detachInterrupt(pin);
  }
  numDone = 0;
}

// Wait for every pin to be LOW for at least 1ms
void coolDown() {
  unsigned long us = micros();
  while (micros()-us<1000) {
    if (digitalRead(PIN0) || digitalRead(PIN1) || digitalRead(PIN2)) {
      us = micros();
    }
  }
}

void setup() {
  Serial.begin(115200);
  for (auto pin : pins) {
    pinMode(pin, INPUT);
  }
  for (auto pin : test) {
    pinMode(pin, OUTPUT);
    digitalWrite(pin, LOW);
  }
  // set up all interrupts
  einschalten();

}

// 240 MHz clock speed
#define COUNTSPERUSECOND 240

void auswerten() {
  int first = 0;
  for (int i=1; i<3; i++) {
    if (fired[i]<fired[first]) first = i;
  }
  for (int i=0; i<3; i++) {
    Serial.printf("Pin %d fired at %lu ticks -> %lu us\n", pins[i], fired[i]-fired[first], (fired[i]-fired[first])/COUNTSPERUSECOND);
  }
}

// Fire the test pins at some random us, and estimate the ticks
void fireTest() {
  unsigned long testus[] = {rand()%500+50, rand()%500+50, rand()%500+50};
  testus[rand()%3] = 0;
  for (int i=0; i<3; i++) {
    Serial.printf("Preparing pin %d at %lu us -> %lu ticks\n", pins[i], testus[i], testus[i]*COUNTSPERUSECOND);
  }
  unsigned long us = micros();
  int f = 0;
  while (f<3) {
    for (int i=0; i<3; i++) {
      if (micros()-us > testus[i])  {
        digitalWrite(test[i], HIGH);
        testus[i] = 10000000L;
        f++;
      }
    }
  }
  Serial.println("All pins fired, Resetting them now...");
  for (auto pin : test) digitalWrite(pin, LOW);

}

unsigned long ms = 10000L;
void loop() {
  if (numDone==3) {  // All three pins have fired?
    auswerten();
    coolDown();
    einschalten();
  }
  if (millis()>ms) {
    ms = millis()+10000L;
    fireTest();
  }
}