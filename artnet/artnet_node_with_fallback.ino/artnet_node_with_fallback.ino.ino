#include <Artnet.h>
#include <Ethernet.h>
#include <EthernetUdp.h>
#include <SPI.h>
#include <OctoWS2811.h> // modified for this project
#include <ICMPPing.h> // ICMP

#define BETWEEN(value, min, max) (value < max && value > min)

// Controller setup
char ctrlrIDLetter = 'A'; // change ID to value A-L

int ctrlrID = ctrlrIDLetter - '0' - 16; // convert to int so A->1, B->2, etc, preferred over 12 case switch)
int networkOctet = 100 + ctrlrID;

// OctoWS2811 setup
const int ledsPerStrip = 115*4; // only using 2 data pins, but zig-zagging data over 4 rows each (configured in MADRIX)
const byte numStrips = 2;
DMAMEM int displayMemory[ledsPerStrip*6];
int drawingMemory[ledsPerStrip*6];
const int config = WS2811_GRB | WS2811_800kHz;
OctoWS2811 leds(ledsPerStrip, displayMemory, drawingMemory, config);

// Artnet setup
Artnet artnet;
const int startUniverse = (8 * ctrlrID) - 7; // A: UVSE 1-8, B: 9-16, etc
const int numberOfChannels = ledsPerStrip * numStrips * 3; 
byte channelBuffer[numberOfChannels]; 
//const int maxUniverses = 2;//numberOfChannels / 512 + ((numberOfChannels % 512) ? 1 : 0); // often 1
//bool universesReceived[maxUniverses]; // array to mark universes as rcvd
//bool sendFrame = 1;

// ICMP ping for fallback functionality if ARTNET server is unavailable
byte serverIP[] = {10,0,1,1};
SOCKET pingSocket = 0;
char buffer [256];
ICMPPing ping(pingSocket, (uint16_t)random(0, 255));
bool pingServer = 1; // one to check if ARTNET server is up, 0 to skip
bool fallback = 1; // 0 = artnet, 1 = fallback

// fallback pattern settings (geom-specific)
uint8_t brightness = 200; //0-255
const int totalLEDs = 115*8;

// Network setup 
byte ip[] = {10,0,1,networkOctet};
byte mac[] = {0x04, 0xE9, 0xEE, 0x00, 0x69, ctrlrID};

void setup() {
  
  Serial.begin(9600);
  delay(2000);
  initPrint();

  // for ICMP ping
  Ethernet.begin(mac, ip);

  // ArtNet Reciever
  artnet.begin(mac, ip);
  leds.begin();
  initTest();
  artnet.setArtDmxCallback(onDmxFrame);
  
}

void loop() {

  if (pingServer==1) {
    // check if server is reachable
    ICMPEchoReply echoReply = ping(serverIP, 4);
    if (echoReply.status == SUCCESS) {
      Serial.println("ArtNet server replied - will listen for DMX");
      fallback = 0; // artnet server is there
    }
    else {
      Serial.println("ArtNet server did not reply - falling back on preset patterns");
      fallback = 1; // no artnet server so do fallback pattern
    }
    pingServer = 0; // bypass this check from now on
  }

  if (fallback) {
    rainbowCycle(20);
  }
  else {
    // Recieve ArtNet
    artnet.read();
  }
  
}

void onDmxFrame(uint16_t universe, uint16_t length, uint8_t sequence, uint8_t* data) {

  /*
   * read dmx data from packet and write it to the pixels
   */
  
  Serial.print(millis());
  Serial.print(" Recieved Art-Net Packet len: ");
  Serial.print(length);
  Serial.print(" UVSE: ");
  Serial.println(universe);
  
  /*
  // mark which universe this packet was addresses to
  if (universe < maxUniverses)
    universesReceived[universe] = 1;

  for (int i = 0 ; i < maxUniverses ; i++) {
    if (universesReceived[i] == 0) {
      Serial.print("Universe ");
      Serial.print(universe);
      Serial.println(" is out of config range (ERR 'BROKE'). Ignoring data.");
      sendFrame = 0;
      break;
    }
  }
  */
  // read universe and put into the right part of the display buffer
  for (int i = 0 ; i < length ; i++) {
    int bufferIndex = i + ((universe - startUniverse) * length);
    if (bufferIndex < numberOfChannels) // to verify
      channelBuffer[bufferIndex] = byte(data[i]);
  }      

  // send to leds
  for (int i = 0; i < ledsPerStrip * numStrips; i++) {
    leds.setPixel(i, channelBuffer[(i) * 3], channelBuffer[(i * 3) + 1], channelBuffer[(i * 3) + 2]);
  }      
  
  //if (sendFrame) {
  leds.show();
  // Reset universeReceived to 0
  //memset(universesReceived, 0, maxUniverses);
  //}
  
}

void initPrint() {

  /*
   * print basic info to the serial console
   */

   Serial.println("----- BEGIN INIT INFO -----");
   Serial.print("Controller ID: ");
   Serial.print(ctrlrIDLetter);
   Serial.print(" / #");
   Serial.println(ctrlrID);
   Serial.print("Network octet: ");
   Serial.println(networkOctet);
   Serial.print("LEDs per strip: ");
   Serial.println(ledsPerStrip);
   Serial.print("Strip count: ");
   Serial.println(numStrips);
   Serial.print("DMX start universe: ");
   Serial.println(startUniverse);
   Serial.println("----- END INIT INFO -----");
   Serial.println();
   
}

void initTest() {

  /*
   * Show colors to indicate that strips work
   */

   Serial.println("init test - cycling colors after short sleep");

  // init strips in order in sets of 3
  delay(1000 * (ctrlrID % 4));

  int wait = 1000; // how long to show each color
  
  for (int i = 0 ; i < ledsPerStrip * numStrips ; i++)
    leds.setPixel(i, 127, 0, 0);
  leds.show();
  delay(wait);
  for (int i = 0 ; i < ledsPerStrip * numStrips  ; i++)
    leds.setPixel(i, 0, 127, 0);
  leds.show();
  delay(wait);
  for (int i = 0 ; i < ledsPerStrip * numStrips  ; i++)
    leds.setPixel(i, 0, 0, 127);
  leds.show();
  delay(wait);
  for (int i = 0 ; i < ledsPerStrip * numStrips  ; i++)
    leds.setPixel(i, 0, 0, 0);
  leds.show();
  
}

uint32_t Wheel(byte WheelPos) {
  WheelPos = 255 - WheelPos;
  if(WheelPos < 85) {
    return getRGBL(255 - WheelPos * 3, 0, WheelPos * 3);
  }
  if(WheelPos < 170) {
    WheelPos -= 85;
    return getRGBL(0, WheelPos * 3, 255 - WheelPos * 3);
  }
  WheelPos -= 170;
  return getRGBL(WheelPos * 3, 255 - WheelPos * 3, 0);
}

void rainbowCycle(uint8_t wait) {
  uint16_t i, j;

  for(j=0; j<256; j++) { 
    for(i=0; i< totalLEDs; i++) {
      leds.setPixel(i, Wheel(((i * 256 / totalLEDs) + j) & 255));
    }
    leds.show();
    delay(wait);
  }
}

uint32_t getRGBL(uint8_t r, uint8_t g, uint8_t b) {
  r = (r * brightness) >> 8;
  g = (g * brightness) >> 8;
  b = (b * brightness) >> 8;
  return (r << 16) | (g << 8) | b;
}
