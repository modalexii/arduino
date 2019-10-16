#include <Adafruit_GPS.h>
#include <exixe.h>
#include <SevSeg.h>

#define USBSerial Serial

/* ----- GPS Globals ----- */
  #define GPSSerial Serial1
  // Connect to the GPS on the hardware port
  Adafruit_GPS GPS(&GPSSerial);
  // Set GPSECHO to 'false' to turn off echoing the raw GPS sentences to the Serial console
  //#define GPSECHO false
  // Set to 0-3 to print different amounts of information to USBSerial
  #define GPSVerbosity 2
/* ----- / GPS Globals ----- */

/* ----- Tube Globals ----- */
  // consider crossfades if digits are too blinky?
  unsigned char tube_digits[4];
  // tube interface doesn't require continual updates, so this is a flag to
  // indicate when there is a need to send them new values
  bool tube_update_queued = false;
  // tube pins - must be CS
  #define TUBE0PIN 15
  #define TUBE1PIN 20
  #define TUBE2PIN 21
  exixe tube0 = exixe(TUBE0PIN);
  exixe tube1 = exixe(TUBE1PIN);
  exixe tube2 = exixe(TUBE2PIN);
/* ----- / Tube Globals ----- */

/* ----- Bubble Globals ----- */
  char bubble_string[5]; 
  SevSeg bubbles;
  int bubble_type = COMMON_CATHODE;
  int bubble_digit_count = 4; 
  // cathodes
  int bubble_c1 = 8; // ic pin 1
  int bubble_c2 = 5; // ic pin 10
  int bubble_c3 = 11; // ic pin 4
  int bubble_c4 = 13; // ic pin 6
  // anodes
  int bubble_aA = 7; // ic pin 12
  int bubble_aB = 6; // ic pin 11
  int bubble_aC = 10; // ic pin 3
  int bubble_aD = 3; // ic pin 8
  int bubble_aE = 9; // ic pin 2
  int bubble_aF = 4; // ic pin 9
  int bubble_aG = 2; // ic pin 7
  int segDP = 12; // ic pin 5
/* ----- / Bubble Globals ----- */

/* ----- LED Globals ----- */
unsigned char tube0_hue, tube1_hue, tube2_hue;
/* ----- / LED Globals ----- */

uint32_t timer = millis();

void setup() {

  /* ----- USB Init ----- */
  // wait until Serial is ready
  while (!USBSerial); 
  // USB serial - should not be slower than this
  USBSerial.begin(115200);
  USBSerial.println("USB Serial Up");
  /* ----- / USB Init ----- */

  /* ----- GPS Init ----- */
  // wait until Serial is ready
  while (!GPSSerial); 
  // 9600 NMEA is the default baud rate for Adafruit GPS
  GPS.begin(9600);
  // turn on RMC (recommended minimum) and GGA (fix data) including altitude
  GPS.sendCommand(PMTK_SET_NMEA_OUTPUT_RMCGGA);
  GPS.sendCommand(PMTK_SET_NMEA_UPDATE_1HZ); // 1 Hz update rate
  // Request updates on antenna status
  GPS.sendCommand(PGCMD_ANTENNA);
  GPSSerial.println(PMTK_Q_RELEASE);
  USBSerial.println("GPS Up");
  /* ----- / GPS Init ----- */

  /* ----- Bubble Init ----- */
  bubbles.Begin(bubble_type, 
                bubble_digit_count, 
                bubble_c1, bubble_c2, bubble_c3, bubble_c4, // cathodes
                bubble_aA, bubble_aB, bubble_aC, bubble_aD, // anodes
                bubble_aE, bubble_aF, bubble_aG, // anodes
                segDP);
  bubbles.SetBrightness(100); //Set the display to 100% brightness level
  /* ----- / Bubble Init ----- */
  
}

void loop() {

  pollGPS();
  updateTubeDigits();
  refreshBubbleDisplay();
  
  // if millis() or timer wraps around, we'll just reset it
  if (timer > millis()) {
    USBSerial.println("Resetting millis() timer");
    timer = millis();
  }

}

void pollGPS() {
  // read data from the GPS in the 'main loop'
  char c = GPS.read();
  // if you want to debug, this is a good time to do it!
  if (GPSVerbosity > 2)
    if (c) USBSerial.print(c);
  // if a sentence is received, we can check the checksum, parse it...
  if (GPS.newNMEAreceived()) {
    // a tricky thing here is if we print the NMEA sentence, or data
    // we end up not listening and catching other sentences!
    // so be very wary if using OUTPUT_ALLDATA and trytng to print out data
    //USBSerial.println(GPS.lastNMEA()); // this also sets the newNMEAreceived() flag to false
    if (!GPS.parse(GPS.lastNMEA())) // this also sets the newNMEAreceived() flag to false
      return; // we can fail to parse a sentence in which case we should just wait for another
  }
  // approximately every 2 seconds or so, print out the current stats
  if (millis() - timer > 2000) {
    timer = millis(); // reset the timer
    if (GPSVerbosity > 1) {
      USBSerial.print("\nTime: ");
      USBSerial.print(GPS.hour, DEC); USBSerial.print(':');
      USBSerial.print(GPS.minute, DEC); USBSerial.print(':');
      USBSerial.print(GPS.seconds, DEC); USBSerial.print('.');
      USBSerial.println(GPS.milliseconds);
      USBSerial.print("Date: ");
      USBSerial.print(GPS.day, DEC); USBSerial.print('/');
      USBSerial.print(GPS.month, DEC); USBSerial.print("/20");
      USBSerial.println(GPS.year, DEC);
      USBSerial.print("Fix: "); USBSerial.print((int)GPS.fix);
      USBSerial.print(" quality: "); USBSerial.println((int)GPS.fixquality);
    }
    if (GPS.fix) {
      // calculate the distance between the equator and the current latitude
      float lat_kilometers = HaverSine(0., GPS.longitude, GPS.latitude, GPS.longitude);
      int lat_meters = round(lat_kilometers * 1000);
      metersToDisplay(lat_meters);
      if (GPSVerbosity > 1) {
        USBSerial.print("Latutide (NMEA DDM): "); USBSerial.println(GPS.latitude, 5); 
        /*USBSerial.print("Speed (knots): "); USBSerial.println(GPS.speed);
        USBSerial.print("Angle: "); USBSerial.println(GPS.angle);
        USBSerial.print("Altitude: "); USBSerial.println(GPS.altitude);*/
        USBSerial.print("Satellites: "); USBSerial.println((int)GPS.satellites);
      }
      if (GPSVerbosity > 0) {
        USBSerial.print("Lat (meters): "); USBSerial.println(lat_meters);
      }
    } else {
      if (GPSVerbosity > 0) USBSerial.println("no fix");
    }
  }
}

float HaverSine(float lat1, float lon1, float lat2, float lon2) {
  /*
   * Returns the distance in Km between two coordinates on a sphere
   * from https://forum.arduino.cc/index.php?topic=336483.msg2320452#msg2320452
   */
  float ToRad = PI / 180.0;
  float R = 6371;   // radius earth in Km

  float dLat = (lat2-lat1) * ToRad;
  float dLon = (lon2-lon1) * ToRad;

  float a = sin(dLat/2) * sin(dLat/2) +
    cos(lat1 * ToRad) * cos(lat2 * ToRad) *
    sin(dLon/2) * sin(dLon/2);

  float c = 2 * atan2(sqrt(a), sqrt(1-a));

  float d = R * c;
  return d;
}

void metersToDisplay(int meters) {
  /*
   * given a value in meters, update the bubble and tube digits
   */
   bubble_string[0] = nthDigit(6, meters);
   bubble_string[1] = nthDigit(5, meters);
   bubble_string[2] = nthDigit(4, meters);
   bubble_string[3] = nthDigit(3, meters);
   tube_digits[0] = nthDigit(2, meters);
   tube_digits[2] = nthDigit(1, meters);
   tube_digits[3] = nthDigit(0, meters);
   tube_update_queued = true;
}

int nthDigit(int n, int k){
  /*
   * return the digit in the nth position of integer k
   */
  while(n--)
    k/=10;
  return k % 10;
}

void updateTubeDigits() {
  /*
   * Write digits to tubes using current values of the tube_digits global
   */
  if (tube_update_queued == false) return;
  USBSerial.print("Tube Digits: ");
  USBSerial.print("tube_digits[0]"); 
  USBSerial.print("tube_digits[1]"); 
  USBSerial.println("tube_digits[2]");
  USBSerial.print("Bubble String: "); // here becuase the bubbl function is called so frequently
  USBSerial.println(bubble_string);
  tube0.show_digit(tube_digits[0], 127, 0);
  tube1.show_digit(tube_digits[1], 127, 0);
  tube2.show_digit(tube_digits[2], 127, 0);
}

void refreshBubbleDisplay() {
  /*
   * Write a string to the display using current values of the bubble_string global
   */
  bubbles.DisplayString(bubble_string, 0);
}

