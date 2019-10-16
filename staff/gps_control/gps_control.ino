    
#include <Adafruit_GPS.h>

#define GPSSerial Serial1
#define DisplaySerial Serial2

// Connect to the GPS on the hardware port
Adafruit_GPS GPS(&GPSSerial);
     


uint32_t timer = millis();

typedef union
{
  int number;
  uint8_t bytes[4];
} FLOATUNION_t;

void setup()
{
  while (!GPSSerial); // wait until Serial is ready
  
  // USB serial - should not be slower than this
  Serial.begin(115200);
  
  // Display controller link
  DisplaySerial.begin(115200);
     
  // 9600 NMEA is the default baud rate for Adafruit GPS
  GPS.begin(9600);
  // turn on RMC (recommended minimum) and GGA (fix data) including altitude
  GPS.sendCommand(PMTK_SET_NMEA_OUTPUT_RMCGGA);
  GPS.sendCommand(PMTK_SET_NMEA_UPDATE_1HZ); // 1 Hz update rate
  // Request updates on antenna status
  GPS.sendCommand(PGCMD_ANTENNA);

  delay(1000);
  
  // Ask for firmware version
  GPSSerial.println(PMTK_Q_RELEASE);
}


// convert NMEA coordinate to decimal degrees
float decimalDegrees(float nmeaCoord) {
  uint16_t wholeDegrees = 0.01*nmeaCoord;
  return wholeDegrees + (nmeaCoord - 100.0*wholeDegrees)/60.0;
}

int feet(float decimalDegrees) {
  return int(decimalDegrees) * 364320;
}

void sendToDisplayController(bool fix, uint8_t satelliteCount, float latitude) {
  if (fix) {
    FLOATUNION_t latitudeFeet;
    latitudeFeet.number = feet(latitude);
    Serial.print("Latitude feet: ");
    Serial.println(latitudeFeet.number);
    
    DisplaySerial.print(0xFF); // byte to assert fix
    DisplaySerial.print(satelliteCount);
    for (int i=0; i<2; i++)
    {
      Serial.print(latitudeFeet.bytes[i], HEX);
    }
  } else {
    DisplaySerial.print(0xAA); // byte to assert no fix
  }
}

void loop() // run over and over again
{
  // read data from the GPS in the 'main loop'
  char c = GPS.read();
  // if you want to debug, this is a good time to do it!
  //if (GPSECHO)
  //  if (c) Serial.print(c);
  // if a sentence is received, we can check the checksum, parse it...
  if (GPS.newNMEAreceived()) {
    // a tricky thing here is if we print the NMEA sentence, or data
    // we end up not listening and catching other sentences!
    // so be very wary if using OUTPUT_ALLDATA and trytng to print out data
    //Serial.println(GPS.lastNMEA()); // this also sets the newNMEAreceived() flag to false
    if (!GPS.parse(GPS.lastNMEA())) // this also sets the newNMEAreceived() flag to false
      return; // we can fail to parse a sentence in which case we should just wait for another
  }
  // if millis() or timer wraps around, we'll just reset it
  if (timer > millis()) timer = millis();
     
  // approximately every 2 seconds or so, print out the current stats
  if (millis() - timer > 2000) {
    timer = millis(); // reset the timer
    Serial.print("\nTime: ");
    Serial.print(GPS.hour, DEC); Serial.print(':');
    Serial.print(GPS.minute, DEC); Serial.print(':');
    Serial.print(GPS.seconds, DEC); Serial.print('.');
    Serial.println(GPS.milliseconds);
    Serial.print("Date: ");
    Serial.print(GPS.day, DEC); Serial.print('/');
    Serial.print(GPS.month, DEC); Serial.print("/20");
    Serial.println(GPS.year, DEC);
    Serial.print("Fix: "); Serial.print((int)GPS.fix);
    Serial.print(" quality: "); Serial.println((int)GPS.fixquality);
    if (GPS.fix) {
      Serial.print("Latutide (NMEA DDM): ");
      Serial.println(GPS.latitude, 5); 
      Serial.print("Latitude (DD): ");
      float latDD = decimalDegrees(GPS.latitude);
      Serial.println(latDD);
      Serial.print("Longitude: ");
      Serial.println(GPS.longitude, 5); 
      Serial.print("Longitude (DD): ");
      float lonDD = decimalDegrees(GPS.longitude);
      Serial.println(lonDD);
      /*Serial.print("Speed (knots): "); Serial.println(GPS.speed);
      Serial.print("Angle: "); Serial.println(GPS.angle);
      Serial.print("Altitude: "); Serial.println(GPS.altitude);*/
      Serial.print("Satellites: "); Serial.println((int)GPS.satellites);
      sendToDisplayController(true,GPS.satellites,latDD);
    } else {
      sendToDisplayController(false,0,0);
    }
  }
}
