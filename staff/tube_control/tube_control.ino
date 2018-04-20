/*
  exixe modules:
  https://github.com/dekuNukem/exixe

  library docs:
  https://github.com/dekuNukem/exixe/tree/master/arduino_library

  Demo 4: Loop digits on two tubes
*/


#include "exixe.h"

// change those to the cs pins you're using
int cs1 = 31;
int cs2 = 15;
unsigned char count;

//exixe my_tube1 = exixe(cs1);
//exixe my_tube2 = exixe(cs2);

int incomingByte = 0;   // for incoming serial data

void setup()
{
  Serial.begin(115200);
  Serial1.begin(115200);
  //my_tube1.clear();
  //my_tube2.clear(f);
  //my_tube1.set_led(127, 0, 127); // purple;
  //my_tube2.set_led(127, 127, 0); // yellow;
}

void loop()
{
  if (Serial1.available() > 0) {
                // read the incoming byte:
                incomingByte = Serial1.read();

                // say what you got:
                Serial.println(incomingByte, DEC);
        }
  //Serial.println("Serial1 not available");
  //delay(200);
  //my_tube1.show_digit(count, 127, 0);
  //my_tube2.show_digit(10 - count, 127, 0);

}
