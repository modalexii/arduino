#include <Artnet.h>
#include <Ethernet.h>
#include <EthernetUdp.h>
#include <SPI.h>
#include <OctoWS2811.h>
#include <ICMPPing.h>

#define BETWEEN(value, min, max) (value < max && value > min)

// OctoWS2811 setup
const int ledsPerStrip = 115*2;
const byte numStrips = 8;
DMAMEM int displayMemory[ledsPerStrip*6];
int drawingMemory[ledsPerStrip*6];
const int config = WS2811_GRB | WS2811_800kHz;
OctoWS2811 leds(ledsPerStrip, displayMemory, drawingMemory, config);

// Artnet setup
Artnet artnet;
const int startUniverse = 0;
const int numberOfChannels = ledsPerStrip * numStrips * 3; 
byte channelBuffer[numberOfChannels]; 
//const int maxUniverses = numberOfChannels / 512 + ((numberOfChannels % 512) ? 1 : 0);
//bool universesReceived[maxUniverses];
//bool = 1;

// Network setup 
byte ip[] = {172,16,0,101};
byte mac[] = {0x04, 0xE9, 0xEE, 0x00, 0x69, 0x01};

// ICMP ping for fallback functionality if ArtNet server is unavailable
byte serverIP[] = {172,16,0,1};
SOCKET pingSocket = 0;
char buffer [256];
ICMPPing ping(pingSocket, (uint16_t)random(0, 255));
bool pingServer = 1; // one to check if ARTNET server is up, 0 to skip
bool fallback = 1; // 0 = artnet, 1 = fallback

// fallback pattern settings (geom-specific)
uint8_t brightness = 30; //0-255
const int totalLEDs = 115*16;

void setup() {
  
  Serial.begin(9600);

  // for ICMP ping
  Ethernet.begin(mac, ip);

  // ArtNet Reciever
  //artnet.begin(mac, ip);
  //artnet.setArtDmxCallback(onDmxFrame);
  
  leds.begin();
  initTest();

}

void loop() {

  if (pingServer) {
    // check if server is reachable
    ICMPEchoReply echoReply = ping(serverIP, 4);
    if (echoReply.status == SUCCESS) {
      Serial.println("ArtNet server replied - will listen for DMX");
      fallback = 0;
      // init functions moved from setup()
      artnet.begin(mac, ip);
      artnet.setArtDmxCallback(onDmxFrame);
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

  //Serial.print(millis());
  //Serial.println(" DMX Packet recieved");

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
  
  leds.show();
  
}

void initTest() {
  // flash r, g, b to indicate basic function and test pixels
  
  for (int i = 0 ; i < ledsPerStrip * numStrips ; i++)
    leds.setPixel(i, 80, 0, 0);
  leds.show();
  delay(500);
  for (int i = 0 ; i < ledsPerStrip * numStrips  ; i++)
    leds.setPixel(i, 0, 80, 0);
  leds.show();
  delay(500);
  for (int i = 0 ; i < ledsPerStrip * numStrips  ; i++)
    leds.setPixel(i, 0, 0, 80);
  leds.show();
  delay(500);
  for (int i = 0 ; i < ledsPerStrip * numStrips  ; i++)
    leds.setPixel(i, 0, 0, 0);
  leds.show();
  
}

// fallback pattern stuff

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
  // given 8 bit component values, dim by value of `brightness` and return a 24 bit value
  
  r = (r * brightness) >> 8;
  g = (g * brightness) >> 8;
  b = (b * brightness) >> 8;
  
  return (r << 16) | (g << 8) | b;
  
}

