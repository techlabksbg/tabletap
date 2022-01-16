#include <Arduino.h>
#include "xtensa/core-macros.h"

#define PIN0 12
#define PIN1 13
#define PIN2 14

int pins[] = {PIN0, PIN1, PIN2};
int test[] = {21,22,23};
volatile unsigned long fired[3];
volatile int numDone = 0;

#define itr(X) void IRAM_ATTR raisingPin##X() { detachInterrupt(PIN##X); fired[X] = XTHAL_GET_CCOUNT(); numDone++; }
itr(0)
itr(1)
itr(2)

#define attachp(X) attachInterrupt(PIN##X, raisingPin##X, RISING);
void einschalten() {
  numDone = 0;
  attachp(0);
  attachp(1);
  attachp(2);
}

void ausschalten() {
  for (auto pin: pins) {
    detachInterrupt(pin);
  }
  numDone = 0;
}

// Wait for 1ms for every pin to be 0
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
    Serial.printf("Pin %d fired at %lu\n", pins[i], fired[i]-fired[first]);
  }
}

void fireTest() {
  unsigned long testus[] = {rand()%500+50, rand()%500+50, rand()%500+50};
  testus[rand()%3] = 0;
  for (int i=0; i<3; i++) {
    Serial.printf("Preparing pin %d at %lu us -> %lu ticks\n", pins[i], testus[i], testus[i]*240);
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
  if (numDone==3) {
    auswerten();
    coolDown();
    einschalten();
  }
  if (millis()>ms) {
    ms = millis()+10000L;
    fireTest();
  }
}