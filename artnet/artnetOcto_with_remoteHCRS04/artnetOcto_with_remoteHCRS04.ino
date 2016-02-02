#include <Artnet.h>
#include <Ethernet.h>
#include <EthernetUdp.h>
#include <SPI.h>
#include <OctoWS2811.h>
#include <NewPing.h>

#define BETWEEN(value, min, max) (value < max && value > min)

// OctoWS2811 setup
const int ledsPerStrip = 150; // change for your setup
const byte numStrips= 2; // change for your setup
DMAMEM int displayMemory[ledsPerStrip*6];
int drawingMemory[ledsPerStrip*6];
const int config = WS2811_GRB | WS2811_800kHz;
OctoWS2811 leds(ledsPerStrip, displayMemory, drawingMemory, config);

// Artnet setup
Artnet artnet;
const int startUniverse = 0; // CHANGE FOR YOUR SETUP most software this is 1, some software send out artnet first universe as zero.
const int numberOfChannels = ledsPerStrip * numStrips * 3; // Total number of channels you want to receive (1 led = 3 channels)
byte channelBuffer[numberOfChannels]; // Combined universes into a single array
const int maxUniverses = numberOfChannels / 512 + ((numberOfChannels % 512) ? 1 : 0);
bool universesReceived[maxUniverses];
bool sendFrame = 1;

// ArtNet remote control setup
byte serverIP[] = {10,0,0,54};
unsigned int artNetPort = 6454;
const int remoteUniverse = 0;
const int numChannels=512;
char ANProtoHead[8]="Art-Net";
const int ANProtoHeaderSize=17;
short OpOutput= 0x5000;
byte DmxBuffer[numChannels]; //buffer used for Dmx data
EthernetUDP Udp;
byte  ArtDmxBuffer[(ANProtoHeaderSize+numChannels)+8+1];
elapsedMillis remoteTimer;
const int remoteFrequency = 1000;

// Network setup
//byte ip[] = {192, 168, 1, 175};
//byte mac[] = {0x22, 0x09, 0x05, 0x20, 0x69, 0x2C};
byte ip[] = {10,0,0,12};
byte mac[] = {0x04, 0xE9, 0xEE, 0x00, 0x69, 0xEC};

// HC-SR04 sensor setup
#define TRIGGER_PIN_A 3
#define ECHO_PIN_A 5
#define TRIGGER_PIN_B 6
#define ECHO_PIN_B 7
#define MAX_DISTANCE 254 // Easily fit in DMX data
NewPing sonar_a(TRIGGER_PIN_A, ECHO_PIN_A, MAX_DISTANCE);
NewPing sonar_b(TRIGGER_PIN_B, ECHO_PIN_B, MAX_DISTANCE);

// Sensor data govorning
int prevRead_a, prevRead_b = 0;
int changeBuffer = 20;

void setup() {
  
  Serial.begin(9600);

  // ArtNet Reciever
  artnet.begin(mac, ip);
  leds.begin();
  initTest();
  artnet.setArtDmxCallback(onDmxFrame);

  // ArtNet Remote Control
  initANHeader();
  Ethernet.begin(mac,ip);
  Udp.begin(artNetPort);
  
}

void loop() {
  
  // Recieve ArtNet
  artnet.read();

/*
  if (remoteTimer > remoteFrequency) {
    
    // Send sensor data (ArtNet Remote)
    bufferSensorReads();
    initANHeader(); // does this need to be done twice??
    Udp.beginPacket(serverIP, artNetPort);
    Udp.write(ArtDmxBuffer,(ANProtoHeaderSize+numChannels+1)); // was Udp.sendPacket
    Udp.endPacket();

    remoteTimer = 0;
  
  }
*/
}

void onDmxFrame(uint16_t universe, uint16_t length, uint8_t sequence, uint8_t* data) {

  Serial.print(millis());
  Serial.println(" DMX Packet recieved");
  
  sendFrame = 1;

  // Store which universe has got in
  if (universe < maxUniverses)
    universesReceived[universe] = 1;

  for (int i = 0 ; i < maxUniverses ; i++) {
    if (universesReceived[i] == 0) {
      Serial.println("Broke");
      sendFrame = 0;
      break;
    }
  }

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
  
  if (sendFrame) {
    leds.show();
    // Reset universeReceived to 0
    memset(universesReceived, 0, maxUniverses);
  }
  
}

void initTest() {
  
  for (int i = 0 ; i < ledsPerStrip * numStrips ; i++)
    leds.setPixel(i, 127, 0, 0);
  leds.show();
  delay(500);
  for (int i = 0 ; i < ledsPerStrip * numStrips  ; i++)
    leds.setPixel(i, 0, 127, 0);
  leds.show();
  delay(500);
  for (int i = 0 ; i < ledsPerStrip * numStrips  ; i++)
    leds.setPixel(i, 0, 0, 127);
  leds.show();
  delay(500);
  for (int i = 0 ; i < ledsPerStrip * numStrips  ; i++)
    leds.setPixel(i, 0, 0, 0);
  leds.show();
  
}

void initANHeader() {
  
  for (int i=0;i<7;i++) {
    ArtDmxBuffer[i]=ANProtoHead[i];
  }   
  
  //Operator code low byte first  
  ArtDmxBuffer[8]= OpOutput;
  ArtDmxBuffer[9]= OpOutput >> 8;
  ArtDmxBuffer[10]=0; // protocol
  ArtDmxBuffer[11]=14;
  ArtDmxBuffer[12]=0; // sequence
  ArtDmxBuffer[13] = 0; // physical
  ArtDmxBuffer[14]= remoteUniverse; // universe
  ArtDmxBuffer[15]= remoteUniverse>> 8;
  ArtDmxBuffer[16] = numChannels>> 8; // data length
  ArtDmxBuffer[17] = numChannels;

  for (int t= 0;t<numChannels;t++) {
    ArtDmxBuffer[t+ANProtoHeaderSize+1]=DmxBuffer[t];    
  }
     
}

void bufferSensorReads() {

  bool send = 0;
  
  // Read from sensors
  unsigned int p_a = sonar_a.ping_cm();
  unsigned int p_b = sonar_b.ping_cm();
  Serial.print("A: ");
  Serial.println(p_a);
  Serial.print("B: ");
  Serial.println(p_b);

  // Place readings in ArtNet Packet
  if (BETWEEN(p_a, prevRead_a - changeBuffer, prevRead_a + changeBuffer)) {
    DmxBuffer[0] = prevRead_a;
  }
  else {
    DmxBuffer[0] = p_a;
  }

  if (BETWEEN(p_b, prevRead_b - changeBuffer, prevRead_b + changeBuffer)) {
    DmxBuffer[1] = prevRead_b;
  }
  else {
    DmxBuffer[1] = p_b;
  }

  prevRead_a = p_a;
  prevRead_b = p_b;

}

