// ask_transmitter.pde
// -*- mode: C++ -*-
// Simple example of how to use RadioHead to transmit messages
// with a simple ASK transmitter in a very simple way.
// Implements a simplex (one-way) transmitter with an TX-C1 module
// Tested on Arduino Mega, Duemilanova, Uno, Due, Teensy, ESP-12

#include <RH_ASK.h>
#ifdef RH_HAVE_HARDWARE_SPI
#include <SPI.h> // Not actually used but needed to compile
#endif

RH_ASK driver(2000, 4, 5, 0, true);

void setup()
{
//pinMode(15,OUTPUT);
  if (!driver.init()) {
    Serial.println("init failed");
  }
  Serial.begin(9600);	
  Serial.println("Tx: Serial up");
}

void spam() {
  uint8_t msg[4] = {byte(0x53), byte(0x52), byte(0x46), byte(0x11)};
  driver.send((uint8_t *)msg, 4);
  driver.waitPacketSent();
  Serial.println("sending SRF0x11");
}

void loop()
{
    spam();
    delay(50);
}
