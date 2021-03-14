/*
Example Send DMX, transmit all received ArtNet messages out of the serial port.

@author Stephan Ruloff
@date 2016-2019
https://github.com/rstephan

*/
#include <ArtnetnodeWifi.h>
#include "DMXUSB.h"
#define DMXUSB_BAUDRATE 115200
#define EXTRELAY_1 29

byte mac[] = {
  0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED
};
IPAddress ip(192, 168, 1, 111);

uint8_t previous_cue = 0;

EthernetUDP UdpSend;
ArtnetnodeWifi artnetnode;

void onDmxFrame(uint16_t universe, uint16_t length, uint8_t sequence, uint8_t* data)
{
  uint8_t cue;
  if (universe == 0) {
    cue  = data[201];
    /*Serial.print(cue, DEC);
    Serial.println();*/
    if (cue == previous_cue) {
      // nothing
    } else if (cue == 0) {
      Serial.println("Turning off all midi notes");
      MIDIAllOff();
    } else if (cue > 0 && cue <= 60) {
      Serial.print("MIDI cue ");
      Serial.println(cue);
      usbMIDI.sendNoteOn(cue , 120, 1);
      usbMIDI.sendNoteOff(previous_cue , 0, 1);
    }
    previous_cue = cue;

    uint8_t fog = data[300];
    Serial.println(fog);
    if (fog) {
      digitalWrite(EXTRELAY_1,HIGH);
    } else {
      digitalWrite(EXTRELAY_1,LOW);
    }
  }
}

void MIDIAllOff() {
  for(uint8_t i = 0; i <= 60; i++) {
    usbMIDI.sendNoteOff(i, 0, 1);
  }
}

void setup()
{
  pinMode(9,OUTPUT);
  digitalWrite(9,LOW);
  pinMode(10,OUTPUT);
  digitalWrite(10,HIGH);
  pinMode(4,OUTPUT);
  digitalWrite(4,HIGH);
  digitalWrite(9,HIGH);

  pinMode(EXTRELAY_1,OUTPUT);
  digitalWrite(EXTRELAY_1,LOW);
  
  // set-up serial for initial info output, hopefully DMX gear will not be confused.
  Serial.begin(115200);
  Ethernet.begin(mac, ip);
  artnetnode.setName("ESP8266 - Art-Net");
  artnetnode.setNumPorts(1);
  //artnetnode.enableDMXOutput(0);
  artnetnode.begin();

  // this will be called for each packet received
  artnetnode.setArtDmxCallback(onDmxFrame);
}

void loop()
{
  // we call the read function inside the loop
  artnetnode.read();
}
